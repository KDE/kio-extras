/*
    This file is part of the smb++ library
    Copyright (C) 2000  Nicolas Brodu
    nicolas.brodu@free.fr

    Portions of this file were copied or inspired from the Samba Unix
	SMB/Netbios implementation, and modified. Those sections were
	authored initially and Copyright (C) Andrew Tridgell 1994-1998.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program, see the file COPYING; if not, write
    to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
    MA 02139, USA.
*/
#include "defines.h"
#ifdef USE_SAMBA

#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h> // for EINVAL
#include "Resolve.h"
#include "strtool.h"
#include "SambaLink.h"
#include "SambaExterns.h"

#include <iostream.h>
	
static bool sambaIsLoaded = false;
int SambaLink::instances=0;

void SambaLink::loadSamba()
{
	if (!sambaIsLoaded) {
		TimeInit();
		charset_initialise();
		// must set up variables in libsamba !!!
		if (!get_myname(myhostname,0)) return;
		get_myname(global_myname,0);
		in_client = True;   // Make sure that we tell lp_load we are
		if (!lp_load(CONFIGFILE,True,False,False)) return;
		codepage_initialise(lp_client_code_page());
		pstring term_code = "";
		interpret_coding_system(term_code);  // KANJI => non-zero
		load_interfaces();
		sambaIsLoaded = true;
	}
}

// class defined here only for storing the results of Samba callback functions
// in directory listings (including virtual directories)
// Add a comment field, because we need the master information
// when browsing workgroups.
class SMBdirentList {
public:
	SMBdirentList(SMBdirent &aDirent, const char* commentarg) {
		entry = aDirent;
		comment = 0;
		newstrcpy(comment,commentarg);
		// add us to the simple-linked list, keep it sorted
		bool c = S_ISDIR(entry.st_mode);
		bool n = dirListing ? S_ISDIR(dirListing->entry.st_mode) : false;
		bool s = dirListing ? (mystrcmp(entry.d_name, dirListing->entry.d_name)<=0) : false;
		if (!dirListing
		|| (c && !n)
		|| (s && !c && !n)
		|| (s && c && n)
		) {
			next = dirListing;
			dirListing = this;
			return;
		} else {
			for (SMBdirentList* it = dirListing; (it); it=it->next) {
				bool c = S_ISDIR(entry.st_mode);
				bool n = (it->next) ? S_ISDIR(it->next->entry.st_mode) : false;
				bool s = (it->next) ? (mystrcmp(entry.d_name, it->next->entry.d_name)<=0) : false;
				if (!(it->next)
				|| (c && !n)
				|| (s && !c && !n)
				|| (s && c && n)
				) {
					next = it->next;
					it->next = this;
					return;
				}
			}
		}
	}
	// Do recursion in the destructor because lists could be stored
	// independantly => have to be careful to reset dirListing to 0
	// when necessary.
	~SMBdirentList() {
		if (comment) delete comment;
		comment = 0;
		if (next) delete next;
		next = 0;
	}
	// Merge ourselve in the last results
	// Though it's in O(n), there are O(n) copies/deletion that could be
	// avoided. But I won't bother rewriting it all now...
	void merge() {
		if (this == dirListing) return; // merge with ourselve done!
		SMBdirentList *ori = dirListing; // take it for us
		dirListing = 0;
		// make a deep copy of ourselve first
		for (SMBdirentList *it = this; (it); it = it->next) {
			new SMBdirentList(it->entry, it->comment);
		}
		// add the original list to it
		for (SMBdirentList *it = ori; (it); it = it->next) {
			new SMBdirentList(it->entry, it->comment);
		}
		// We don't need the original anymore
		if (ori) delete ori;
		// remove the duplicates
		// dirListing is non-empty at this stage, we added 'this' to it
		for (SMBdirentList *it = dirListing; (it) && (it->next); it = it->next) {
			while ((it->next) && (!mystrcmp(it->next->entry.d_name, it->entry.d_name))) {
				SMBdirentList *tmp = it->next;
				it->next = it->next->next;
				tmp->next = 0; // stop recursion
				delete tmp;
			}
		}
	}
	// Empty the last list of result. Use the recursion in the destructor.
	static void empty() {
		if (dirListing) delete dirListing;
		dirListing=0;
	}
	// Return the comment associated with this name in the list
	static char* nameComment(const char *name, bool caseSensitive = false) {
		for (SMBdirentList *it = dirListing; (it); it = it->next) {
			if ((caseSensitive && !mystrcmp(name, it->entry.d_name))
			||  (!caseSensitive && !mystrcasecmp(name, it->entry.d_name))) {
				return it->comment;
			}
		}
		return 0;
	}
	SMBdirent entry;
	char *comment;
	SMBdirentList *next;
// static again, because of the callback.
// WARNING: THIS IS NOT RE-ENTRANT. THREADS COULD HAVE BIG PROBLEMS!!!
	static SMBdirentList *dirListing;
};
// Ok, the callback cannot access a specific Descriptor*
// It will store the information here. Same comment as above for browser()
SMBdirentList *SMBdirentList::dirListing = 0;

// Utility class for all descriptor
class Descriptor {
private:
	int myNumber;
	Descriptor *beforeMe;
	Descriptor *afterMe;
	char *myFile;
	cli_state *myCli;
public:
	Descriptor(const char* file, cli_state* cli, int sambafd);
	~Descriptor();
	cli_state *cli();
	int fid;
	char *file();
	// directory descriptor
	bool dirdesc;
	// used for readdir, since Samba gets all the results at one time
	SMBdirentList *dirList;  // the whole list, if any
	SMBdirentList *pdirList;  // pointer to the current element of that list
	uint32 pos;
	operator const int() const;
	Descriptor *next();
	// number of the last created descriptor
	static Descriptor *find(int num);
	static int descCount;
	static Descriptor *descriptorList;
};

// Init static values to something
Descriptor *Descriptor::descriptorList=0;
int Descriptor::descCount=10000;

Descriptor::Descriptor(const char* file, cli_state* cli, int sambafd)
{
	// Link ourself to the other descriptors
	beforeMe = 0;
	afterMe = descriptorList;
	descriptorList = this;
	if (afterMe) afterMe->beforeMe = this;
	// get a number
	myNumber = descCount++;
	// record our info
	myFile=0; newstrcpy(myFile,file);
	myCli = cli;
	fid = sambafd;
	pos = 0;
	dirList = 0;
	pdirList = 0;
	dirdesc = false;
}

Descriptor::~Descriptor()
{
	// unlink from previous element, or update list
	if (!beforeMe) descriptorList = afterMe;
	else beforeMe->afterMe = afterMe;
	// unlink from next element
	if (afterMe) afterMe->beforeMe = beforeMe;
	// cleanup
	if (myCli) {
		cli_close(myCli,fid);
		cli_shutdown(myCli);
		delete myCli;
	}
	if (myFile) delete myFile;
	if (dirList) delete dirList; // recursive deletion
}

Descriptor::operator const int() const
{
	return myNumber;
}

Descriptor *Descriptor::next()
{
	return afterMe;
}

cli_state *Descriptor::cli()
{
	return myCli;
}

char* Descriptor::file()
{
	return myFile;
}

Descriptor *Descriptor::find(int num)
{
	for (Descriptor* it=descriptorList; (it); it = it->next()) {
		if ( *it == num ) return it;
	}
	return 0;
}


SambaLink::SambaLink()
{
	loadSamba();
	instances++;
	lastError = 0;
	theMaster = 0;
}

SambaLink::~SambaLink()
{
	if (--instances<=0) {
		// Destructors are wonderful!
		while (Descriptor::descriptorList) delete Descriptor::descriptorList;
	}
	if (theMaster) delete theMaster;
}

struct cli_state *SambaLink::connectUtil(const char *toparse = 0)
{
	// parse the argument if it was not done before
	if (toparse) util.parse(toparse);
	char *server = util.host();
	char *share = util.share();
	char *guest = "GUEST";  // define this here to have a valid address
	// cannot connect to nothing
	if ((!server) || (!share)) return 0;
	
	// Let's see if host really exists
	Resolve r;
	if (!r.gethostbyname(server)) return 0;
	
	struct cli_state *smb_cli;
	struct nmb_name called, calling, stupid_smbserver_called;
	struct in_addr ip;

	if ((smb_cli=cli_initialise(0)) == 0) return 0;

	make_nmb_name(&calling, global_myname, 0x0, "");
	make_nmb_name(&called, server, name_type, "");
	make_nmb_name(&stupid_smbserver_called , "*SMBSERVER", 0x20, scope);

	char *user = util.user();
	if (!user) user = userName; // No user specified: default from Options
	fstrcpy(smb_cli->usr.user_name, user);
	fstrcpy(smb_cli->usr.domain, lp_workgroup());

	char *hostip = util.ip();
	if (hostip) inet_aton(hostip, &ip);
	else inet_aton("0.0.0.0", &ip);

	if (cli_set_port(smb_cli, 139) == 0) return 0;
	
	char *pass = util.password();
	// empty pass => null pass
	if ((pass) && (pass[0]==0)) pass=0;
	
	// If a password is specified, use it now
	if (pass) {
		pwd_init(&(smb_cli->usr.pwd));
		pwd_make_lm_nt_16(&(smb_cli->usr.pwd), pass);
	}
	else pwd_set_nullpwd(&(smb_cli->usr.pwd));

	smb_cli->use_ntlmv2 = lp_client_ntlmv2();
	
	struct cli_state *retval = smb_cli;
	
	if (!cli_establish_connection(smb_cli, server, &ip, &calling, &called,
	                              share, "?????", False, True) &&
	    !cli_establish_connection(smb_cli, server, &ip,
	                              &calling, &stupid_smbserver_called,
	                              share, "?????", False, True))
	{
		// Couldn't connect. Host exist but we don't know about share
		// Ask a user/pass even if it's for an inexistant share for now...
		retval = 0;

		// Save the old values
		char *savUser = 0, *savPass = 0;
		newstrcpy(savUser, user);
		newstrcpy(savPass, pass);
		
		// Now, check if we are in user level security
		if (smb_cli->sec_mode & 1) {
			// Yes, get a user for this server
			if ((!user) && (theCallback))
				user = theCallback->getAnswer(ANSWER_USER_NAME, server);
			// We _need_ a user in user security => try guest
			if ((!user) || (user[0]==0)) user = guest;
			// We suppose the account doesn't have an empty password,
			// so let's ask for a password right now and don't try an empty one
			// first (the answer might still be an empty pass anyway)
			if ((!pass) && (theCallback))
				pass = theCallback->getAnswer(ANSWER_USER_PASSWORD, user);
			// empty pass => null pass
			if ((pass) && (pass[0]==0)) pass=0;
		// Else it's a share level security => don't bother with the user
		} else {
			if ((!pass) && (theCallback))
				pass = theCallback->getAnswer(ANSWER_SERVICE_PASSWORD, share);
			// empty pass => null pass
			if ((pass) && (pass[0]==0)) pass=0;
		}
		
		// if the situation has changed, try again
		if ((!savUser && user) || (savUser && !user)
		|| (user && strcmp(savUser,user))
		|| (!savPass && pass) || (savPass && !pass)
		|| (pass && strcmp(savPass,pass))) {

			// set the new username/password
			fstrcpy(smb_cli->usr.user_name, user);
			if (pass) {
				pwd_init(&(smb_cli->usr.pwd));
				pwd_make_lm_nt_16(&(smb_cli->usr.pwd), pass);
			}
			else pwd_set_nullpwd(&(smb_cli->usr.pwd));
			
			// Try again
			if (cli_establish_connection(smb_cli, server, &ip, &calling, &called,
										share, "?????", False, True) ||
				cli_establish_connection(smb_cli, server, &ip,
										&calling, &stupid_smbserver_called,
										share, "?????", False, True))
				retval = smb_cli;
		}
		if (savPass) delete savPass;
		if (savUser) delete savUser;
	}
	return retval;
}


// find the error for this client, always returns -1
// Used in any of the functions when an error occurs
int SambaLink::findError(struct cli_state *cli = 0)
{
	if (!cli) lastError = EINVAL;
	else lastError = cli_error(cli, 0, 0);
	return -1;
}

// set the lastError to an error code, always returns -1
// Used in any of the functions when an error occurs
int SambaLink::findError(int errcode)
{
	lastError = errcode;
	return -1;
}

// set the lastError to 0, always returns its argument
// Used in any of the functions when all goes well
int SambaLink::noError(int ret=0)
{
	lastError = 0;
	return ret;
}

// Those functions work like their standard equivalent,
// but file descriptors are for internal use only.
// They accept smbURLs as parameters
int SambaLink::open(const char* file="", int flags=O_RDWR, int mode=0644)
{
	util.parse(file);
	// We want a real file, host, share,... do not qualify
	if (!util.path()) return findError(EISDIR);
	
	struct cli_state *cli = connectUtil();
	if (!cli) return findError();  // connection failed

	// Convert util.path() into DOS\PATH\FILE
	char *dospath=0;
	newstrcpy(dospath, util.path());
	for (char* p=dospath; (*p); p++) {
		if (*p == '/') *p = '\\';
	}
	int sambafd = cli_open(cli, dospath, flags, DENY_NONE);
	delete dospath;
	if (sambafd == -1) {
		findError(cli);
		cli_shutdown(cli);
		return -1;
	}
	
	Descriptor *ret = new Descriptor(file, cli, sambafd);

	// append requested, must find the size of the file
	if (flags & O_APPEND) {
		uint16 attr;
		size_t size;
		time_t ctime, atime, mtime;
		int res = cli_getattrE(cli, sambafd, &attr, &size, &ctime, &atime, &mtime);
		if (res == False) return findError(cli);
		ret->pos = size;
	}
	
	return noError(*ret);
}

int SambaLink::creat(const char* file="", int mode=0644)
{
	return open(file, O_CREAT|O_WRONLY|O_TRUNC, mode);
}

int SambaLink::stat(const char *filename, struct stat *buf)
{
	util.parse(filename);
	// If it's real file, connect and send a getatr SMB command
	if (util.path()) {
		struct cli_state *cli = connectUtil();
		uint16 attr;
		size_t size;
		time_t theTime;
		
		// Convert util.path() into DOS\PATH\FILE
		char *dospath=0;
		newstrcpy(dospath, util.path());
		for (char* p=dospath; (*p); p++) {
			if (*p == '/') *p = '\\';
		}
		int res = cli_getatr(cli, dospath, &attr, &size, &theTime);
		delete dospath;
		if (res == False) {
			findError(cli);
			cli_shutdown(cli);
			return -1;
		}
		// archive attribute mapped to 0644
		buf->st_mode=0644;
		if (attr&0x20) buf->st_mode|=0644;
		// read-only attribute removes write permission
		if (attr&0x01) buf->st_mode&=077666;
		// directory attribute mapped to 040755
		if (attr&0x10) buf->st_mode|=040755;
		// Anyone has any idea about what volume, hidden and
		// system should be mapped to ???
		buf->st_ctime = buf->st_ctime = buf->st_ctime = theTime;
		buf->st_uid = getuid();
		buf->st_gid = getgid();
		buf->st_rdev=0100000; // regular file
		buf->st_size = size;
		cli_shutdown(cli);
	
	// Else it's a virtual directory (or the root directory)
	} else {
		buf->st_mode=040755; // directory
		buf->st_uid=getuid();
		buf->st_gid=getgid();
		buf->st_size=0; // Not important for a directory anyway
		buf->st_ctime=buf->st_mtime=buf->st_atime=0;
		buf->st_rdev=0100000; // regular file
	}	
	return noError();
}

int SambaLink::fstat(int fd, struct stat *buf)
{
	Descriptor *d = Descriptor::find(fd);
	if ((!d) || (d->dirdesc)) return findError(EBADF);
	
	uint16 attr;
	size_t size;
	int res = cli_getattrE(d->cli(), d->fid, &attr, &size,
		 &buf->st_ctime, &buf->st_atime, &buf->st_mtime);
	if (res == False) return findError(d->cli());
	buf->st_size = size;
	// archive attribute mapped to 0644
	buf->st_mode=0644;
	if (attr&0x20) buf->st_mode|=0644;
	// read-only attribute removes write permission
	if (attr&0x01) buf->st_mode&=077666;
	// directory attribute mapped to 040755
	if (attr&0x10) buf->st_mode|=040755;
	// Anyone has any idea about what volume, hidden and
	// system should be mapped to ???
	buf->st_uid = getuid();
	buf->st_gid = getgid();
	buf->st_rdev=0100000; // regular file
	
	return noError();
}

// read and write do caching !
int SambaLink::read(int fd, void *buf, uint32 count)
{
	Descriptor *d = Descriptor::find(fd);
	if ((!d) || (d->dirdesc)) return findError(EBADF);
	
	// caching already done in Samba :-)
	int ret = cli_read(d->cli(), d->fid, (char*)buf, d->pos, count, True);
	
	// update position in file
	if (ret>0) d->pos += ret;
	else findError(d->cli());
	
	return noError(ret);
}

int SambaLink::write(int fd, void *buf, uint32 count)
{
	Descriptor *d = Descriptor::find(fd);
	if ((!d) || (d->dirdesc)) return findError(EBADF);
	
	// caching already done in Samba :-)
	int ret = cli_write(d->cli(), d->fid, 0, (char*)buf, d->pos, count, 0);
	
	// update position in file
	if (ret>0) d->pos += ret;
	
	return noError(ret);
}

// flush forces a write of all buffered data for the given fd
int SambaLink::flush(int fd)
{
	Descriptor *d = Descriptor::find(fd);
	if ((!d) || (d->dirdesc)) return findError(EBADF);
	
	// Use Samba "disallow write caching" function and write 0 bytes!
	char buf; // To be on the safe side... make a pointer in our address space!
	int ret = cli_write(d->cli(), d->fid, 1, &buf, d->pos, 0, 0);
	if (ret==-1) return findError(d->cli());
	else return noError();
}

int32 SambaLink::lseek(int fd, int32 offset, int from)
{
	Descriptor *d = Descriptor::find(fd);
	if ((!d) || (d->dirdesc)) return findError(EBADF);
	
	int res = True;  // must declare it here because of the case label
	switch (from) {
		case SEEK_SET:
			d->pos = offset;
			break;
		case SEEK_CUR:
			d->pos += offset;
			break;
		case SEEK_END:
			uint16 attr;
			size_t size;
			time_t c_time, a_time, m_time;
			res = cli_getattrE(d->cli(), d->fid, &attr, &size, &c_time, &a_time, &m_time);
			if (res == False) return findError(d->cli());
			d->pos = size + offset;
			break;
		default:
			return findError(EINVAL);
	}
		
	return noError(d->pos);
}

int SambaLink::close(int fd)
{
	Descriptor *d = Descriptor::find(fd);
	if ((!d) || (d->dirdesc)) return findError(EBADF);
	delete d;  // also close and shut the client down
	return noError();
}

// unlink sends a unlink SMB. Will delete or unlink depending on target OS
int SambaLink::unlink(const char *file)
{
	util.parse(file);
	// We want a real file, host, share,... do not qualify
	if (!util.path()) return findError(EPERM);
	
	struct cli_state *cli = connectUtil();
	if (!cli) return findError();  // connection failed

	// Convert util.path() into DOS\PATH\FILE
	char *dospath=0;
	newstrcpy(dospath, util.path());
	for (char* p=dospath; (*p); p++) {
		if (*p == '/') *p = '\\';
	}
	BOOL ret = cli_unlink(cli, dospath);
	delete dospath;
	if (ret == False) {
		findError(cli);
		cli_shutdown(cli);
		return -1;
	}
	return noError();
}

// Renames a file, but doesn't move it between directories
int SambaLink::rename(const char *fileURL, const char *newname)
{
	util.parse(fileURL);
	// We want a real file, host, share,... do not qualify
	if (!util.path()) return findError(EPERM);
	
	struct cli_state *cli = connectUtil();
	if (!cli) return findError();  // connection failed
	
	// Convert util.path() into DOS\PATH\FILE. Look for the last token (file name)
	char *dospath=0;
	newstrcpy(dospath, util.path());
	char *lastToken = dospath;
	for (char* p=dospath; (*p); p++) {
		if (*p == '/') {*p = '\\'; lastToken = p;}
	}
	int len = lastToken-dospath;
	char *destpath = new char[len+1];
	if (len) memcpy(destpath,dospath,len);
	destpath[len]=0;
/*	char* upname = 0;
	newstrcpy(upname, newname);
	for (char* p=upname; (*p); p++) *p = toupper(*p);
	*/
	if (len) newstrappend(destpath,"\\",newname);
	else newstrcpy(destpath,newname);
//	delete upname;

	BOOL ret = cli_rename(cli, dospath, destpath);
	delete destpath;
	if (ret == False) {
		findError(cli);
		cli_shutdown(cli);
		return -1;
	}
	return noError();
}

// deletes a directory
int SambaLink::rmdir(const char *pathname)
{
	util.parse(pathname);
	// We want a real file, host, share,... do not qualify
	if (!util.path()) return findError(EPERM);
	
	struct cli_state *cli = connectUtil();
	if (!cli) return findError();  // connection failed
	
	// Convert util.path() into DOS\PATH\FILE
	char *dospath=0;
	newstrcpy(dospath, util.path());
	for (char* p=dospath; (*p); p++) {
		if (*p == '/') *p = '\\';
	}
	BOOL ret = cli_rmdir(cli, dospath);
	delete dospath;
	if (ret == False) {
		findError(cli);
		cli_shutdown(cli);
		return -1;
	}
	return noError();
}

// creates a directory
int SambaLink::mkdir(const char *pathname)
{
	util.parse(pathname);
	// We want a real file, host, share,... do not qualify
	if (!util.path()) return findError(EACCES);
	
	struct cli_state *cli = connectUtil();
	if (!cli) return findError();  // connection failed
	
	// Convert util.path() into DOS\PATH\FILE
	char *dospath=0;
	newstrcpy(dospath, util.path());
	for (char* p=dospath; (*p); p++) {
		if (*p == '/') *p = '\\';
	}
	BOOL ret = cli_mkdir(cli, dospath);
	delete dospath;
	if (ret == False) {
		findError(cli);
		cli_shutdown(cli);
		return -1;
	}
	return noError();
}


// AH-AH!!! If you read everything this far, you probably deserve a coffe break
// before reading further on.

// Samba needed callback for listing directories. Use mask as comment!
static void dirLister(file_info *info, const char *mask)
{
	SMBdirent entry;
	newstrcpy(entry.d_name, dos_to_unix(info->name, False));
	entry.st_rdev=0100000; // regular file
	// archive attribute mapped to 0644
	entry.st_mode=0644;
	if (info->mode&0x20) entry.st_mode|=0644;
	// read-only attribute removes write permission
	if (info->mode&0x01) entry.st_mode&=077666;
	// directory attribute mapped to 040755
	if (info->mode&0x10) entry.st_mode|=040755;
	// Anyone has any idea about what volume, hidden and
	// system should be mapped to ???
	entry.st_ctime = info->ctime;
	entry.st_atime = info->atime;
	entry.st_mtime = info->mtime;
	entry.st_uid = info->uid;
	entry.st_gid = info->gid;
	entry.st_size = info->size;
	// Create new SMBdirent entry that adds itself to dirListing
	// Cannot manage memory from this callback => BE CAUTIOUS
	new SMBdirentList(entry, mask);
}

// shareBrowser is used for shares, it filters out printers, IPC, ...
static void shareBrowser(const char *name, uint32 m, const char *comment)
{
	if (m != STYPE_DISKTREE) return; // filter out printers, IPC, ...
	SMBdirent entry;
	newstrcpy(entry.d_name, name);
	entry.st_mode=040755; // directory
	entry.st_uid=getuid();
	entry.st_gid=getgid();
	entry.st_size=0; // Not important for a directory anyway
	entry.st_ctime=entry.st_mtime=entry.st_atime=0;
	entry.st_rdev=0100000; // regular file
	// Create new SMBdirent entry that adds itself to dirListing
	// Cannot manage memory from this callback => BE CAUTIOUS
	new SMBdirentList(entry, comment); // get rid of the 2nd 'uint32' argument
}

// netBrowser is used for members & workgroups and don't filter anything
static void netBrowser(const char *name, uint32 m, const char *comment)
{
	SMBdirent entry;
	newstrcpy(entry.d_name, name);
	entry.st_mode=040755; // directory
	entry.st_uid=getuid();
	entry.st_gid=getgid();
	entry.st_size=0; // Not important for a directory anyway
	entry.st_ctime=entry.st_mtime=entry.st_atime=0;
	entry.st_rdev=0100000; // regular file
	// Create new SMBdirent entry that adds itself to dirListing
	// Cannot manage memory from this callback => BE CAUTIOUS
	new SMBdirentList(entry, comment); // get rid of the 2nd 'uint32' argument
}


// Get the share list of the host in the url.
// Use the full url because a user/pass might be in it
int SambaLink::getShareList(const char* toparse = 0)
{
	// parse the argument if it was not done before
	if (toparse) util.parse(toparse);
	// overwrite the possible share
	util.share("IPC$");
	// Empty the result list
	SMBdirentList::empty();
	// connect and do the job
	struct cli_state *cli = connectUtil();
	if (!cli) return findError();
	if (!cli_RNetShareEnum(cli, shareBrowser)) return findError(cli);
	cli_shutdown(cli);
	return noError();
}

// Get the workgroup list according to the host in the url.
// Use the full url because a user/pass might be in it
int SambaLink::getWorkgroupList(const char* toparse = 0)
{
	// parse the argument if it was not done before
	if (toparse) util.parse(toparse);
	// overwrite the possible share
	util.share("IPC$");
	// Empty the result list
	SMBdirentList::empty();
	// connect and do the job
	struct cli_state *cli = connectUtil(toparse);
	if (!cli) return findError();
	if (!cli_NetServerEnum(cli, util.workgroup(), SV_TYPE_DOMAIN_ENUM, netBrowser))
		return findError(cli);
	cli_shutdown(cli);
	return noError();
}

// Get the member of workgroup list according to the host in the url.
// Use the full url because a user/pass might be in it
int SambaLink::getMemberList(const char* toparse = 0)
{
	// parse the argument if it was not done before
	if (toparse) util.parse(toparse);
	// overwrite the possible share
	util.share("IPC$");
	// Empty the result list
	SMBdirentList::empty();
	// connect and do the job
	struct cli_state *cli = connectUtil(toparse);
	if (!cli) return findError();
	if (!cli_NetServerEnum(cli, util.workgroup(), SV_TYPE_ALL, netBrowser))
		return findError(cli);
	cli_shutdown(cli);
	return noError();
}


// opendir returns a descriptor as well.
int SambaLink::opendir(const char *name)
{
	util.parse(name);
	// Easy first case: virtual directory
	if (!util.share()) {
		Descriptor *ret = new Descriptor(name, 0, 0);
		ret->dirdesc = true;
		return noError(*ret);
	}

	// Is it really a directory?
	struct stat buf;
	if (stat(name, &buf)==-1) return -1; // error already set
	if (!S_ISDIR(buf.st_mode)) return findError(ENOTDIR);
	
	// So now we open the connection to the host
	struct cli_state *cli = connectUtil();
	if (!cli) return findError();  // connection failed
	
	// And store the client state structure in a descriptor for later use
	Descriptor *ret = new Descriptor(name, cli, 0);
	ret->dirdesc = true;
	return noError(*ret);
}


// see types.h for SMBdirent description
SMBdirent *SambaLink::readdir(int dirdesc)
{
	Descriptor *d = Descriptor::find(dirdesc);
	if ((!d) || (!d->dirdesc)) {
		findError(EBADF);
		return 0;
	}

	// fid is used to indicate there already was a connection
	if (d->fid) {
		// The only way to know it's an end-of-dir for the caller
		noError(); // and not an error is to look at the error code
		// list might still be empty (empty dirs)
		if (!(d->dirList)) return 0;
		if (d->pdirList) {
			SMBdirent *ret = &(d->pdirList->entry);
			d->pdirList = d->pdirList->next;  // increment counter
			return ret;
		}
		return 0; // end of directory
	}
	
	// cli exist => real directory
	if (d->cli()) {
		util.parse(d->file());
		// Convert util.path() into DOS\PATH
		char *dosmask=0;
		newstrcpy(dosmask, util.path());
		for (char* p=dosmask; (p && (*p)); p++) {
			if (*p == '/') *p = '\\';
		}
		// Append the * mask => want all files
		newstrappend(dosmask,"\\*");
		// Empty the result list
		SMBdirentList::empty();
		// 0x02|0x04|0x10: include hidden, system, directory
		cli_list(d->cli(), dosmask, 0x02|0x04|0x10, dirLister);
		// The descriptor becomes the owner of the list
		d->dirList = SMBdirentList::dirListing;
		SMBdirentList::dirListing = 0;
		// Set the pointer at the begining
		d->pdirList = d->dirList;
		d->fid = 1; // set this to a non-zero value for next readdir();
		noError();
		// list might still be empty (empty dirs)
		if (!(d->dirList)) return 0;
		if (d->pdirList) {
			SMBdirent *ret = &(d->pdirList->entry);
			d->pdirList = d->pdirList->next;  // increment counter
			return ret;
		}
		return 0; // end of directory
	}

// Now the most difficult part of all this: correct browsing.
// Because the information is distributed over the network, and the election
// scheme takes time, it's possible (I saw it happening more than once) a
// workgroup master have a wrong idea about who is the real master of the
// other workgroups, and would in that case return an incorrect list in
// domain enumeration. This is also true for the local samba...
// In the native code, there is a good algorithm to do this (perhaps not
// writen clean enough). Here, we use some functions from Samba instead.

	// So now it's a virtual dir => must do browsing
	util.parse(d->file());
	// If there is an host => list the shares
	if (util.host()) {
		int res = getShareList();
		// The descriptor becomes the owner of the list
		d->dirList = SMBdirentList::dirListing;
		SMBdirentList::dirListing = 0;
		// Set the pointer at the begining
		d->pdirList = d->dirList;
		d->fid = 1; // set this to a non-zero value for next readdir();
		if (res==-1) return 0; // error already set in getShareList
		// The only way to know it's an end-of-dir for the caller
		noError(); // and not an error is to look at the error code
		// list might still be empty (empty dirs)
		if (!(d->dirList)) return 0;
		if (d->pdirList) {
			SMBdirent *ret = &(d->pdirList->entry);
			d->pdirList = d->pdirList->next;  // increment counter
			return ret;
		}
		return 0; // end of directory
	}
	

	// If there is a workgroup => list the members
	if (util.workgroup()) {
		// We must find the real master first
		if (findMaster() == -1) {
			findError();
			return 0;
		}
		// Now we have it. Ask it the members for its workgroup
		util.host(theMaster);
		int res = getMemberList();
		// The descriptor becomes the owner of the list
		d->dirList = SMBdirentList::dirListing;
		SMBdirentList::dirListing = 0;
		// Set the pointer at the begining
		d->pdirList = d->dirList;
		d->fid = 1; // set this to a non-zero value for next readdir();
		if (res==-1) return 0; // error already set in getShareList
		// The only way to know it's an end-of-dir for the caller
		noError(); // and not an error is to look at the error code
		// list might still be empty (empty dirs)
		if (!(d->dirList)) return 0;
		if (d->pdirList) {
			SMBdirent *ret = &(d->pdirList->entry);
			d->pdirList = d->pdirList->next;  // increment counter
			return ret;
		}
		return 0; // end of directory
	}
	
	// No workgroup, we'll try to find them all!
	// first find a master for our workgroup
	char *ourGroup = 0;
	newstrcpy(ourGroup, lp_workgroup());
	util.workgroup(ourGroup);
	if (findMaster() == -1) {
		findError();
		if (ourGroup) delete ourGroup;
		return 0;
	}
	// do a domain enum on it
	util.host(theMaster);
	int res = getWorkgroupList();
	if (res==-1) {
		if (ourGroup) delete ourGroup;
		return 0; // error already set in getWorkgroupList
	}
	// for each other workgroup than ours, find the master and ask it the same
	// thing. This way we should find the whole domain controller tree.
	SMBdirentList* first = SMBdirentList::dirListing;
	SMBdirentList::dirListing = 0;  // we own it
	SMBdirentList* final = first;       // the final resulting list
	for (SMBdirentList *it = first; (it); it = it->next) {
		if (mystrcasecmp(ourGroup, it->entry.d_name)) {
			util.workgroup(it->entry.d_name);
			util.host(it->comment);
			if (getWorkgroupList() == -1) continue; // skip the bad guys
			final->merge(); // final non 0
			if (final!=first) delete final; // keep the first list for iterating
			final = SMBdirentList::dirListing;
			SMBdirentList::dirListing = 0;  // got it now
		}
	}
	// cleanup
	if (ourGroup) delete ourGroup;
	if ((first) && (first!=final)) delete first;
	// The descriptor becomes the owner of the final list
	d->dirList = final;
	// Set the pointer at the begining
	d->pdirList = d->dirList;
	d->fid = 1; // set this to a non-zero value for next readdir();
	// The only way to know it's an end-of-dir for the caller
	noError(); // and not an error is to look at the error code
	// list might still be empty (empty dirs)
	if (!(d->dirList)) return 0;
	if (d->pdirList) {
		SMBdirent *ret = &(d->pdirList->entry);
		d->pdirList = d->pdirList->next;  // increment counter
		return ret;
	}
	return 0; // end of directory
}

// This function supposes util contains correct values
int SambaLink::findMaster()
{
	// temporary result
	char *m = 0;
	
	// Try Samba trusted server list
	fstring aname;
	BOOL res = get_any_dc_name(util.workgroup(), aname);
	if (res) {
		// skip Samba double-backslash garbage and convert "." to our name
		if (mystrcmp(aname+2,"\\\\.")) newstrcpy(theMaster, global_myname);
		else newstrcpy(theMaster, aname+2);
		return 0;
	}
	
	// Use user's kindness first: this might very well be the PDC!
	if (browseServer) {
		util.host(browseServer);
		getWorkgroupList();
		// find the master
		newstrcpy(m, SMBdirentList::nameComment(util.workgroup()));
		// found one, is it the correct one?
		if (m) {
			// According to itself, yes => suppose it's the real one
			if (!mystrcasecmp(m, browseServer)) {
				if (theMaster) delete theMaster;
				theMaster = m;  // correctly allocated
				return 0;
			}
			util.host(m);
			getWorkgroupList();
			char *m2 = SMBdirentList::nameComment(util.workgroup());
			// According to itself, yes => suppose it's the real one
			if (!mystrcasecmp(m, m2)) {
				if (theMaster) delete theMaster;
				theMaster = m;  // correctly allocated
				return 0;
			}
			// so, there is an inconsistency. We'll find it differently...
		}
	}

	// The local Samba tried itself if the trusted list was empty above
	// So try it now, but we can expect some inconsistency
	util.host(global_myname);
	getWorkgroupList();
	// find the master
	newstrcpy(m, SMBdirentList::nameComment(util.workgroup()));
	// found one, is it the correct one?
	if (m) {
		if (!mystrcasecmp(m, global_myname)) { // Hey, weird, it's us!
			if (theMaster) delete theMaster;
			theMaster = m;  // correctly allocated
			return 0;
		}
		util.host(m);
		getWorkgroupList();
		char *m2 = SMBdirentList::nameComment(util.workgroup());
		// According to itself, yes => suppose it's the real one
		if (!mystrcasecmp(m, m2)) {
			if (theMaster) delete theMaster;
			theMaster = m;  // correctly allocated
			return 0;
		}
		// so, there is an inconsistency. We'll find it differently...
	}
	
	// hmm, try an IP trick
	struct in_addr ip;
	res = find_master_ip(util.workgroup(), &ip);
	if (res) {
		util.host(0);
		util.ip(inet_ntoa(ip));
		getWorkgroupList();
		// if found one, OK
		newstrcpy(theMaster, SMBdirentList::nameComment(util.workgroup()));
		if (theMaster) return 0;
	}

	// Never mind...
	return -1;
}

int SambaLink::closedir(int dirdesc)
{
	Descriptor *d = Descriptor::find(dirdesc);
	if ((!d) || (!d->dirdesc)) return findError(EBADF);
	delete d;  // also close and shut the client down
	return noError();
}

// OK, the standard version returns nothing. Here -1 = error (0 otherwise)
int SambaLink::rewinddir(int dirdesc)
{
	Descriptor *d = Descriptor::find(dirdesc);
	if ((!d) || (!d->dirdesc)) return findError(EBADF);
	
	// reset 'stream' pointer whether it's null or not
	d->pdirList = d->dirList;
	
	// That's all folks!
	return noError();
}

// return the last error encountered when a function returns -1	
int SambaLink::error()
{
	return lastError;
}

// AAAhh, it's 22:41, 01 Feb 2000, and I finished implementing all the functions
// in a crazy week!
// Enjoy :-)
//   Nicolas Brodu
#endif
