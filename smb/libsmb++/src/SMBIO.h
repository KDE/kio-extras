/*
    This file is part of the smb++ library
    Copyright (C) 1999-2000  Nicolas Brodu
    nicolas.brodu@free.fr

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

// SMBio manages the connections to a remote host

#ifndef __SMBIO_H__
#define __SMBIO_H__
#include "defines.h"
#ifndef USE_SAMBA

#include <fcntl.h>	// need open() flags
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE // needed for st_Xtime members of struct stat on freeBSD
#endif
#include <sys/stat.h>
#include "SessionIO.h"
#include "IOTypes.h"
#include "CharCnv.h"
#include "Util.h"
#include "FileManagementInterface.h"
#include "Options.h"

class SMBIO : public SessionIO, public FileManagementInterface, public Options
{
protected:
	// Use this class for the Transact "protocol" over SMB
	// It's a class rather than a struct for the pointers
	// All pointer fields must be allocated with 'new'
	class TransactInfo
	{
	public:
		char* transactName;
		uint16 tid;
		uint16 *setup; uint16 setupLength; // # of uint16
		uint8* param; uint16 paramLength; // in bytes
		uint8* data; uint16 dataLength; // in bytes
		TransactInfo();
		~TransactInfo();
	};
	// Does a transaction with the remote host
	// Used to retreive the share list
	TransactInfo *transact(TransactInfo *tinfo);
	// One per instance, which adress is returned by getShareList
	SMBShareList theShareList;
	SMBdirent theDirEnt; // same thing
	SMBWorkgroupList *workgroupList;  // checked and allocated by getWorkgroupList
	SMBMemberList *memberList;  // checked and allocated by getWorkgroupMembers
	// thoses 2 functions ask a specific host. Use getWorkgroupList if you
	// want a correct list, a specific host has its own vision of the network.
	SMBWorkgroupList *askWorkgroupList(const char *browser, const char *user=0);
	SMBMemberList *askMemberList(const char *master, const char *workgroup, const char *user=0);
	// Workgroup list is not returned directly, will be allocated if necessary
	int getWorkgroupList(const char *user=0); // can be call in various places
	int getWorkgroupMembers(const char *workgroup, const char *user=0);
	char *currentShare;		// check this
	char *currentDirectory;	// and this for internal caching
	char *defaultBrowser;// Make browsing work without samba installed locally
	char *defaultUser;
	// Variables for a given session
	uint8 dialect;
	uint8 security;
	uint16 maxMessageSize;
	uint16 TID;
	int challengeLength;
	uint8* challenge;
	uint32 sessionKey;
	uint8 readRawAvailable;
	// file id invalid if deconnected between open and read/write
	uint8 hasBeenDeconnected;
	// unlike unix creat, this is equivalent to open with
	// O_CREAT|O_RDWR|O_TRUNC. This only sends a SMBcreatePacket
	int createRemoteFile(const char* file);
	// Pointer to a list of information for each file descriptor.
	// The actual implementation is internal, may change in the
	// future, and is confined in SMBIO.c++
	// This pointer is here to avoid a static list in SMBIO.c++,
	// end therefore should avoid a nasty problem with multiple instances
	void *fdInfo;
	// user interaction variables
	// Warning: function callback barely supported. (NB 20000201)
	char* (*getFuncCallback)(const char*, bool);
	SmbAnswerCallback *getObjectCallback;
	// To simplify internal code, both methods are gathered in a single
	// function that calls in prefered order the object method or the callback
	char *getString(int type, const char *optmessage);
	// Crypts a password if necessary
	// length is returned as an argument
	uint8* crypt(const char* password, int& length);
	// Try to log and returns -1 on failure
	// used by login below to try lower/UPPER cases an password/no_pass.
	int doLogin(const char* user, const char* password, int16 UID);
	// read/write, but without cache. Does the i/o job.
	int readRaw(int fd, void *buf, uint32 count);
	int writeRaw(int fd, void *buf, uint32 count);
	// write, using core protocol packet
	int writeSimple(int fd, void *buf, uint32 count);
	// Doesn't get anything at all, but return ""
	// allocated with new. It's static so that no 'this'
	// hidden argument makes type incompatible.
	static char *dummyGet(const char* message, bool echo);
	CharCnv *char_cnv;;
	Util util;
public:
	// Constructor can take a function as argument
	// It is used when a password is required
	// Typically it should display the message and return
	// an answer from the user
	// The second argument can specify our host name
	// It will be passed to parent constructor
	SMBIO(char* (*getFunc)(const char*, bool) = 0, const char *hostname=0);
	// Overload with the new class that makes user interaction more C++ like
	SMBIO(SmbAnswerCallback *getObject, const char *hostname=0);
	~SMBIO();
	void setPasswordCallback(char* (*getFunc)(const char*, bool) = 0);
	// Overload with the new class that makes user interaction more C++ like
	void setPasswordCallback(SmbAnswerCallback *getObject);
// Optional functions
	// Default user is used instead of anonymous access
	void setDefaultUser(const char *userName);
	// Make browsing work without samba installed locally
	void setDefaultBrowseServer(const char *serverName);
	// Simple parser for smb URLs
	// Parameters are passed by value and will be modified (=>also returned values)
	// On return, those not null must be deallocated with delete after use.
	int parse(const char *name, char* &workgroup, char* &host, char* &share, char* &dir, char* &user);
	// Does the opposite of the parser
	// Returned string must be deallocated with delete after use
	char *buildURL(const char* workgroup=0, const char* host=0, const char* share=0, const char* file=0, const char* user=0);
	// getName returns the name of the file for this fd, stripping URL
	// returns 0 on error, or the name allocated with new
	char *getName(int fd);
	// getName returns the full URL of thefile or directory of the descriptor
	// returns 0 on error, or the URL allocated with new
	char *getURL(int desc);
	// appends a string to an URL, making it another URL
	// note : subdir ".." is recognized and interpreted in all cases !
	// note2 : subdir "." will return a different dd for the same dir
	// returns 0 on error, or the new URL allocated with new
	char *append(const char* URL, const char *string);
	// opens subdir "name" of dir dd and returns a directory descriptor
	// same notes as above
	// returns -1 on error
	int cd(int dd, const char *subdir);
// Session management
	int openSession(const char *hostname);
	// encyption not supported yet
	int login(const char* user=0, const char* password=0);
	// see #defs above for valid types
	int openService(const char* service, const char* password=0, uint8 type=SMB_DISK);
	void closeService();
	void closeSession();
	// user can be specified for non anonymous access (login name of an account)
	// see IOTypes.h for SMBShareList description
	SMBShareList *getShareList(const char *hostname, const char *user=0, const char *password=0);
// File management
	// Those functions work like their standard equivalent,
	// but file descriptors are for internal use only.
	// Maybe they'll be replaced by pipe descriptors later
	// so they could work with standard functions.
	// They accept smbURLs as parameters as well
	// mode for open and creat is ignored at present
	int open(const char* file="", int flags=O_RDWR, int mode=0644);
	int creat(const char* file="", int mode=0644);
	int stat(const char *filename, struct stat *buf);
	int fstat(int fd, struct stat *buf);
	// read and write do caching !
	int read(int fd, void *buf, uint32 count);
	int write(int fd, void *buf, uint32 count);
	// flush forces a write of all buffered data for the given fd
	int flush(int fd);
	int32 lseek(int fd, int32 offset, int from);
	int close(int fd);
	// unlink sends a unlink SMB. Will delete or unlink depending on target OS
	int unlink(const char *file);
	// Renames a file, but doesn't move it between directories
	int rename(const char *fileURL, const char *newname);
	// deletes a directory
	int rmdir(const char *pathname);
	// creates a directory
	int mkdir(const char *pathname);
	// opendir returns a descriptor as well.
	int opendir(const char *name);
	// see IOTypes.h for SMBdirent description
	SMBdirent *readdir(int dirdesc);
	int closedir(int dirdesc);
	// OK, the standard version returns nothing. Here -1 = error (0 otherwise)
	int rewinddir(int dirdesc);
	// Set the charset to use. If not set, no conversion.
	void setCharSet(const char *char_set);
	// return the last error encountered when a function returns -1	
	int error();
};

#endif //USE_SAMBA
#endif //__SMBIO_H__
