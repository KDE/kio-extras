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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "defines.h"
#ifndef USE_SAMBA

#include <errno.h>
#undef errno    // need the error codes but not the variable
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>     // for qsort
#include <string.h>
#include <unistd.h> // getuid(), getgid()
#include <time.h>       // needed for mktime() and tm, time_t structs
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE // needed for st_Xtime members of struct stat on freeBSD
#endif
#include "SMBIO.h" // include <sys/stat.h> in here
#include "SMBPacket.h"
#include "SessionCodes.h"
#include "SMBCodes.h"
#include "IOTypes.h"
#include "types.h"
#include "strtool.h"
#include "IODescriptors.h"
#include "DES.h"
#ifdef DEBUG
#include <iostream.h>
#include <stdio.h>
#else
#define DEBUG 0
#endif

// Reversed comparison between strings
// used for reversed qsort
int rstrcmp(char* s1, char* s2)
{
	return strcmp(s2,s1);
}

char *SMBIO::dummyGet(const char*, bool)
{
	return 0;
/*	char *ret=new char[1];
	ret[0]=0;
	return ret;*/
}

SMBIO::SMBIO(char* (*getFunc)(const char*, bool), const char *hostname) : SessionIO(hostname)
{
	if (getFunc) getFuncCallback=getFunc;
	else getFuncCallback=dummyGet;
	getObjectCallback=0;
	dialect=0;
	security=0;
	readRawAvailable=0;
	challenge=0;
	challengeLength=0;
	maxMessageSize=0;
	sessionKey=0;
	TID=0;
	fdInfo=0;
	currentShare=0;
	workgroupList=0;
	memberList=0;
	defaultBrowser=0; // use samba
	defaultUser=0;    // anonymous
	hasBeenDeconnected=1;
	char_cnv=new CharCnv;
}

SMBIO::SMBIO(SmbAnswerCallback *getObject, const char *hostname) : SessionIO(hostname)
{
	getObjectCallback=getObject;
	getFuncCallback=dummyGet;
	dialect=0;
	security=0;
	readRawAvailable=0;
	challenge=0;
	challengeLength=0;
	maxMessageSize=0;
	sessionKey=0;
	TID=0;
	fdInfo=0;
	currentShare=0;
	workgroupList=0;
	memberList=0;
	defaultBrowser=0; // use samba
	defaultUser=0;    // anonymous
	hasBeenDeconnected=1;
	char_cnv=new CharCnv;
}

SMBIO::~SMBIO()
{
	if (currentShare) closeService();
	if (hostName) closeSession();
	if (currentShare) delete currentShare;
	if (workgroupList) delete workgroupList;
	// do not delete, it is just a pointer to a member of a workgroupList
	// so it has already been deleted above !!!
//	if (memberList) delete memberList;
	// fdInfo cleanup
	if (fdInfo) destroyFdList((FdCell*)fdInfo);
	if (char_cnv) delete char_cnv;
}

void SMBIO::setPasswordCallback(char* (*getFunc)(const char*, bool))
{
	if (getFunc) getFuncCallback=getFunc;
	else getFuncCallback=dummyGet;
}

void SMBIO::setPasswordCallback(SmbAnswerCallback *getObject)
{
	getObjectCallback=getObject;
}

char *SMBIO::getString(int type, const char *optmessage)
{
	if (getObjectCallback)
		// got rid of the old convention for this one!
		return getObjectCallback->getAnswer(type, optmessage);
	else if (getFuncCallback) {
		char *message=0; bool echo = false;
		switch (type) {
			case ANSWER_USER_NAME:
				newstrcpy(message, "User for host ");
				newstrappend(message, optmessage);
				echo = true;
				break;
			case ANSWER_USER_PASSWORD:
				newstrcpy(message, "Password for user ");
				newstrappend(message, optmessage);
				break;
			case ANSWER_SERVICE_PASSWORD:
				newstrcpy(message, "Password for service ");
				newstrappend(message, optmessage);
				break;
		}
		char *ret = getFuncCallback(message, echo);
		if (message) delete message;
		return ret;
	}
	return 0;
}


int SMBIO::openSession(const char *hostname)
{
	hasBeenDeconnected=1;
	if (SessionIO::openSession(hostname)==-1)
		return -1;
	SMBnegprotPacket *smbp=new SMBnegprotPacket;
	if (send(smbp)==-1) {delete smbp; return -1;}
	delete smbp;
	SessionPacket *sesp = receive();
	if (sesp==0)
	{
		errno=ERRerror;
		return -1;
	}
	if (sesp->getType()!=SESSION_MESSAGE)
	{
		delete sesp;
		errno=ERRerror;
		return -1;
	}
	smbp=new SMBnegprotPacket;
	if (smbp->parse(sesp)==-1)
	{
#if DEBUG >= 1
		cout<<"openSession : Answer is not a valid SMB packet\n";
#endif
		errno=smbp->errno;
		return -1;
	}
	delete sesp;
	if (smbp->getSMBType()!=SMBnegprot)
	{
#if DEBUG >= 1
		cout<<"openSession : Answer is not a Negotation Protocol SMB packet\n";
#endif
		errno=smbp->errno;
		return -1;
	}
	dialect=smbp->getAcceptedDialect();
	security=smbp->getSecurityMode();
	sessionKey=smbp->getSessionKey();
	maxMessageSize=smbp->getMaxMessageSize();
	readRawAvailable=smbp->isReadRawPossible();
	if (challenge) delete challenge;
	challenge=smbp->getCryptKey(challengeLength);
	return 1;
}

// Crypts password if necessary, using DES (see also doLogin)
// Length will contain the length of the returned buffer
uint8* SMBIO::crypt(const char* password, int& length)
{
	if (!password) {
		length=0;
		return 0;
	}
	if (security & 2) { // bit 1==encrypt passwords
#if DEBUG >= 6
		cout<<"SMBIO::crypt: using encrypted password\n";
#endif
		// first pad password to 14 bytes with 0
		unsigned char pass[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		if (password) {
			int l=strlen(password);
			for (int i=0; i<((l<14)?l:14); i++)
				pass[i]=toupper(password[i]);
//			memcpy((char*)pass,password,((l<14)?l:14)); // doesn't copy 0 at the end
		}
		// and prepare fixed initial byte sequence
		unsigned char buf[21]={0x4b, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25, 0x4b, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25, 0, 0, 0, 0, 0};
		// challenge should be 8 character long at this point, but...
		unsigned char chal[8]={0,0,0,0,0,0,0,0};
		if (challenge) memcpy((char*)chal,(char*)challenge,challengeLength>8?8:challengeLength);
		// and prepare a buffer to get the final data
		uint8* fin = new uint8[24];
		for (int i=0; i<8; i++) {fin[i]=chal[i]; fin[i+8]=chal[i]; fin[i+16]=chal[i];}
		// now do the weird crypt
		DES des;
		des.setKey(pass);
		des.encrypt(buf,8);
		des.setKey(pass+7);
		des.encrypt(buf+8,8);
		des.setKey(buf);
		des.encrypt(fin,8);
		des.setKey(buf+7);
		des.encrypt(fin+8,8);
		des.setKey(buf+14);
		des.encrypt(fin+16,8);
		// and return this value
#if DEBUG >= 5
		printf("crypt : Challenge :\n");
		for (int i=0; i<8; i++) printf("%X ",chal[i]);
		printf("\ncrypt : Pass created :\n");
		for (int i=0; i<24; i++) printf("%X ",fin[i]);
		printf("\n");
#endif
		length=24;
		return fin;
	} else { // send plain text
		length=(password)? strlen(password)+1 : 0;
		uint8* ret=0;
		if (length)
		{
			ret=new uint8[length];
			memcpy(ret,(uint8*)password,length);
		}
		return ret;
	}
}

int SMBIO::doLogin(const char* user, const char* password, int16 UID)
{
	int length=0;
//	uint8* pass=((password) && (password[0] != '\0'))?crypt(password,length):0;
	uint8* pass=((password) && (password[0] != '\0'))?crypt(password,length):crypt("",length);
#if DEBUG >= 6
		cout<<"doLogin : user="<<user<<", password="<<password<<", passlen="<<length<<"\n";
		for (int i=0; i<length; i++) printf("%X ",pass[i]);
		printf("\n");
#endif
	SMBsesssetupPacket *setupp=new SMBsesssetupPacket(sessionKey, user, pass, length,UID);
	if (pass) delete pass;
	if (send(setupp)==-1) {delete setupp; return -1;}
	SessionPacket *sesp;
	if ((sesp=receive())==0)
	{
		errno=ERRerror;
		return -1;
	}
	if (setupp->parse(sesp)==-1)
	{
#if DEBUG >= 1
		cout<<"doLogin : Answer is not a valid SMB packet\n";
#endif
		errno=setupp->errno;
		return -1;
	}
#if DEBUG >= 6
	cout<<"doLogin : server response :\n";
	setupp->print();
#endif
	delete sesp;
	errno=setupp->getError();
	delete setupp;
	if (errno)	// user/password invalid
		return -1;
	else return 1;
}

int SMBIO::login(const char* u, const char* p)
{
	if ((security&1) == 0) { // share level, don't care about user
		if (doLogin(u, p, 0)!=-1) return 1;
		// else try with user anyway !
	}
	int UID=0;
	char *user=0, *password=0;
	if (u) {user=new char[strlen(u)+1]; strcpy(user,u);}
	if (p) {password=new char[strlen(p)+1]; strcpy(password,u);}
	// now check for default user if needs be
	if ((!user) && (defaultUser)) {user=new char[strlen(defaultUser)+1]; strcpy(user,defaultUser);}
	int guestTried=((!user) && (!password));
	// Try given arguments
//	if (doLogin(user, password, UID)!=-1) {delete user; delete password; return 1;}
//	if (errno!=ERRbadpw) {delete user; delete password; return -1;}
	// We wanted anonymous access, so try another way right now
	if (guestTried)
	{
#if DEBUG >= 6
		cout<<"Trying anonymous login with null user and passwords\n";
#endif
		// try guest with null user and password
		if (doLogin(0, 0, UID)!=-1) {delete user; delete password; return 1;}
#if DEBUG >= 6
		cout<<"Trying anonymous login with empty user and passwords\n";
#endif
		// try guest with empty user and password
		if (doLogin("", "", UID)!=-1) {delete user; delete password; return 1;}
	}
	// Try with complete login if not already done
	if ((!user) || (!password))
	{
		if (!user) {
			char *resp = getString(ANSWER_USER_NAME,hostName);
			user = new char[strlen(resp)+1];
			strcpy(user,resp);
		}
		if (!password) {
			char *resp = getString(ANSWER_USER_PASSWORD,user);
			password = new char[strlen(resp)+1];
			strcpy(password,resp);
		}
		if (doLogin(user, password, UID)!=-1) {delete user; delete password; return 1;}
		if (errno!=ERRbadpw) {delete user; delete password; return -1;}
	}
	int l1=strlen(user);
	int i;
	// try both in maj
	for (i=0; i<l1; i++) if ((user[i]>='a') && (user[i]<='z')) user[i]-='a'-'A';
	for (i=0; i<l1; i++) if ((password[i]>='a') && (password[i]<='z')) user[i]-='a'-'A';
	if (doLogin(user, password, UID)!=-1) {delete user; delete password; return 1;}
	if (errno!=ERRbadpw) {delete user; delete password; return -1;}
	// try both in min
	for (i=0; i<l1; i++) if ((user[i]>='A') && (user[i]<='Z')) user[i]+='a'-'A';
	for (i=0; i<l1; i++) if ((password[i]>='A') && (password[i]<='Z')) user[i]+='a'-'A';
	if (doLogin(user, password, UID)!=-1) {delete user; delete password; return 1;}
	if (errno!=ERRbadpw) {delete user; delete password; return -1;}
	// try guest if not tried before
	if (!guestTried)
	{
		if (doLogin(0, 0, UID)!=-1) {delete user; delete password; return 1;}
		if (errno!=ERRbadpw) {delete user; delete password; return -1;}
		// try guest with empty user and password
		if (doLogin("", "", UID)!=-1) {delete user; delete password; return 1;}
	}
	// errno contains ERRbadpw or anything else
	delete user; delete password; return -1;
}


void SMBIO::closeSession()
{
	hasBeenDeconnected=1;
	dialect=0;  // for a given session
	security=0; // for a given session
	readRawAvailable=0;
	if (challenge) delete challenge;
	challenge=0;
	challengeLength=0;
	SessionIO::closeSession();
}

int SMBIO::openService(const char* service, const char* password, uint8 type)
{
	if (!hostName)
	{
		errno=ERRerror;
		return -1;
	}
	hasBeenDeconnected=1;
	int l1=strlen(hostName);
	int l2=strlen(service);
	char *path=new char[2+l1+1+l2+1]; // for \\host\service string
	path[0]='\\'; path[1]='\\';
	memcpy(path+2,hostName,l1);
	path[l1+2]='\\';
	strcpy(path+l1+3,service);	// copy \0 at the end as well
	char device[5];
	switch (type) {
		case SMB_DISK: strcpy(device,"A:"); break;
		case SMB_PRINT_QUEUE: strcpy(device,"LPT1:"); break;
		case SMB_DEVICE: strcpy(device,"COMM"); break;
		case SMB_IPC: strcpy(device,"IPC"); break;
		default: device[0]=0; break;
	}
	int passLen=0;
#if DEBUG >= 6
	cout<<"openService : passlen="<<passLen<<"\n";
#endif
	uint8* pass=(password)?crypt(password, passLen):0;
#if DEBUG >= 5
	cout<<"openService : passlen="<<passLen<<"\n";
	for (int i=0; i<passLen; i++) printf("%X ",pass[i]);
	printf("\n");
#endif
/*	char* msg=new char[strlen(service)+21+1];
	strcpy(msg,"Password for service ");
	strcpy(msg+21,service);
	char *pass1=getString(msg);
	delete msg;
	uint8* pass=(pass1)?crypt(pass1, passLen):0;*/
	SMBtconXPacket *t=new SMBtconXPacket(path, pass, passLen, device);
	if (pass) delete pass;
	if (send(t)==-1) {delete t; return -1;}
	delete t;
	SessionPacket *sesp = receive();
	if (sesp==0)
	{
#if DEBUG >= 1
		cout<<"openService : No answer\n";
#endif
		errno=ERRerror;
		return -1;
	}
#if DEBUG >= 6
	cout<<"received packet :\n";
	sesp->print();
#endif
	
	if (sesp->getType()!=SESSION_MESSAGE)
	{
#if DEBUG >= 1
		cout<<"openService : wrong packet type\n";
#endif
		delete sesp;
		errno=ERRerror;
		return -1;
	}
	t=new SMBtconXPacket;
	if (t->parse(sesp)==-1)
	{
#if DEBUG >= 1
		cout<<"openService : Answer is not a valid SMB packet\n";
#endif
		errno=t->errno;
		delete t;
		return -1;
	}
	delete sesp;
	if (t->getSMBType()!=SMBtconX)
	{
#if DEBUG >= 1
		cout<<"openService : Answer is not a Tree Connect SMB packet\n";
#endif
		errno=t->errno;
		delete t;
		return -1;
	}
//	maxMessageSize=t->getMaxMessageSize();
	TID=t->getTID();
	
#if DEBUG >= 4
	cout<<"openService : Tree connect : max msg size : "<<maxMessageSize<<", TID : "<<TID<<"\n";
#endif
//	if ((maxMessageSize==0) || ((int16)TID==-1))
	if (TID==0)
	{
		errno=t->getError();
		delete t;
		// Was a password provided ?
		if ((password==0) || (strcasecmp(password,"")==0)) {
			// try with a dummy password first, no need to ask the user
			// if it's just the server fault
/*			int retour=openService(service,"foo",type);
			if (retour==1)	// Ah !
				return retour;
*/
			int retour;
			char *password2=getString(ANSWER_SERVICE_PASSWORD, service);
			if ((password2) && (strcmp(password2,""))) { // else user didn't know !
				retour=openService(service,password2,type); // should not recurse
				// Try password in maj if error
/*				if (retour!=1) {
					char *majPassword=new char[strlen(password2)+1];
					for (uint32 i=0; i<strlen(password2); i++) majPassword[i]=toupper(password2[i]);
					retour=openService(service,majPassword,type);
					delete majPassword;
				}*/
				// getString doesn't allocate anymore //delete password2;
				return retour;
			}
			// getString doesn't allocate anymore //delete password2;
		}
		// Yes (or we might be in a recursion)
		// Lets try to convert service to MAJ
		uint32 len=strlen(service);
		char *majService=new char[len+1];
		for (uint32 i=0; i<len; i++) majService[i]=toupper(service[i]);
		majService[len]=0;
		if (strcmp(majService,service))
		{
			int retour=openService(majService,password,type);
			delete majService;
			return retour;
		}
		// Service already in MAJ
		return -1;
	}
#if DEBUG >= 2
	cout<<"Service "<<service<<" opened\n";
#endif
	delete t;
	if (currentShare) delete currentShare;
	currentShare=new char[strlen(service)+1];
	strcpy(currentShare,service);
	return 1;
}


void SMBIO::closeService()
{
	hasBeenDeconnected=1;
	SMBtdisPacket *t=new SMBtdisPacket(TID);
	send(t);
	delete t;
	SessionPacket *sesp = receive();
	if (sesp) delete sesp;
#if DEBUG >= 2
	cout<<"Service closed\n";
#endif
	if (currentShare) delete currentShare;
	currentShare=0;
}


// open a file and returns a file descriptor, or -1 on error
// the mode argument is ignored
int SMBIO::open(const char* file, int flags, int)
{
	char *workgroup=0; // those values have to be parsed
	char *host=0;
	char *share=0;
	char *dir=0;
	char *user=0;

	parse(file, workgroup, host, share, dir, user);
	
	if (!dir) { // browse, or sharelist
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		errno=EISDIR;
		return -1;
	}

	// now that we're sure there is a dir/file, we can safely assume
	// stat won't browse or open unecessary connections.
	// => check if dir/file is a file, and not a directory
	struct stat statbuf;
	if (stat(file,&statbuf)==-1) { // inexistant file, or without session
		// should we create it ?
		if (!(flags&O_CREAT)) {  // no => end with error
			if (workgroup) delete workgroup;
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT;
			return -1;
		}
		int newfd=createRemoteFile(file);
		if (newfd==-1) return -1; // error, errno already set !
		// SMB files are created with RDWR !
		if ( flags & O_RDWR ) return newfd; // great
		// So we'll reopen it with whatever perm user wish below.
		close(newfd);
		// and clear the TRUNC flag if any
		// otherwise it will lead to another call to create below !
		flags &= ~O_TRUNC;
		stat(file,&statbuf);    // Get status of recently created file.
	// now check if we didn't wan't the file to exist !
	} else if ( (flags & O_EXCL) && (flags & O_CREAT)) {
		errno=EEXIST;
		return -1;
	}
	if (statbuf.st_mode&040000) {  // S_ISDIR standard macro...
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EISDIR;
		return -1;
	}
	// Ah, now it's a file and it exists
	// do we want to trunc it ?
	if ( flags & O_TRUNC ) {
		// yes, we HAVE TO use the smb for creat instead of open
		int newfd=createRemoteFile(file);
		if (newfd==-1) return -1; // error, errno already set !
		// SMB files are created with RDWR !
		if ( flags & O_RDWR ) return newfd; // great
		// So we'll reopen it with whatever perm user wish below.
		close(newfd);
	}
	
	// Now it's a file, it exists, and we have dealt with CREAT/TRUNC cases
	SMBopenPacket *p=new SMBopenPacket(TID, dir, flags);
	if (send(p)==-1) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		delete p;
		errno=ERRerror; // no mapping acceptable in standard open !
		return -1;
	}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ERRerror;
		return -1;
	}
	if (sesp->getType()!=SESSION_MESSAGE) {
		delete sesp;
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ERRerror;
		return -1;
	}
	p=new SMBopenPacket;
	if (p->parse(sesp)==-1) {
#if DEBUG >= 1
		cout<<"open : Answer is not a valid SMB packet\n";
#endif
		errno=p->errno;
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		delete sesp;
		delete p;
		return -1;
	}
	delete sesp;
	if (p->getSMBType()!=SMBopen) {
#if DEBUG >= 1
		cout<<"open : Answer is not an open SMB packet\n";
#endif
		errno=p->errno;
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		delete p;
		return -1;
	}
	int fid=p->getFID();
	if (((int16)fid==-1) && (p->errno!=SUCCESS)) {
		errno=p->getError();
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		delete p;
		return -1;
	}
	delete p;
	// remove directory part of the name now...
	char *name=strrchr(dir,'/');  // last occurence
	if (!name) name=dir; // 'dir' was already a file
	else name++; // do not keep '/' in name
	int fd=getNewFd(fdInfo, fid, name, workgroup, host, share, dir, user, statbuf.st_size);
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	info->openMode=flags;
	if (flags & O_APPEND) info->pos=statbuf.st_size;
#if DEBUG >= 2
	cout<<"file "<<name<<" opened\n";
#endif
	if (workgroup) delete workgroup;
	if (host) delete host;
	if (share) delete share;
	if (user) delete user;
	if (dir) delete dir;
	// deconnection between open and read/write invalidates fid
	hasBeenDeconnected=0;
	return fd;
}


// As its unix equivalent, creat should do the same as open with
// O_CREAT|O_WRONLY|O_TRUNC
// NB20000828: but sometimes there is a bug, change it to RDWR is OK
int SMBIO::creat(const char* file, int mode)
{
	return open(file,O_CREAT|O_RDWR|O_TRUNC, mode);
}


// send a SMB for creating a file
// returns a file descriptor, or -1 on error
// unlike unix creat, this is equivalent to open with
// O_CREAT|O_RDWR|O_TRUNC
int SMBIO::createRemoteFile(const char* file)
{
	char *workgroup=0; // those values have to be parsed
	char *host=0;
	char *share=0;
	char *dir=0;
	char *user=0;

	parse(file, workgroup, host, share, dir, user);
	
	if (!dir) { // browse, or sharelist
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		errno=EISDIR;
		return -1;
	}

	// now that we're sure there is a dir/file, we can safely assume
	// stat won't browse or open unecessary connections.
	// => check if dir/file is a file, and not a directory, if it exists
	struct stat statbuf;
 	if ( (stat(file,&statbuf)!=-1) && (statbuf.st_mode&040000) ) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EISDIR;
		return -1;
	}
	
#ifdef OVERKILL
	closeSession();
	if ((openSession(host)==-1) || (login(user)==-1)) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ENOENT; return -1;
	}
#else
	if (!hostName) {
		if (openSession(host)==-1) {
			if (workgroup) delete workgroup;
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
   			errno=ENOENT; return -1;
		}
	} else if (strcasecmp(hostName,host)) {
		closeSession();
		if (openSession(host)==-1) {
			if (workgroup) delete workgroup;
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	}
	if (login(user)==-1) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EACCES; return -1;
	}
#endif
	if (openService(share)==-1) {
		errno=ENOENT; return -1;
	}
	
	// Now if it exists it is a file
	SMBcreatePacket *p=new SMBcreatePacket(TID, dir);
	if (send(p)==-1) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		delete p;
		errno=ERRerror; // no mapping acceptable in standard open !
		return -1;
	}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ERRerror;
		return -1;
	}
	if (sesp->getType()!=SESSION_MESSAGE) {
		delete sesp;
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ERRerror;
		return -1;
	}
	p=new SMBcreatePacket;
	if (p->parse(sesp)==-1) {
#if DEBUG >= 1
		cout<<"createRemoteFile : Answer is not a valid SMB packet\n";
#endif
		errno=p->errno;
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		delete sesp;
		delete p;
		return -1;
	}
	delete sesp;
	if (p->getSMBType()!=SMBcreate) {
#if DEBUG >= 1
		cout<<"createRemoteFile : Answer is not a create SMB packet\n";
#endif
		errno=p->errno;
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		delete p;
		return -1;
	}
	int fid=p->getFID();
	if (((int16)fid==-1) && (p->errno!=SUCCESS)) {
		errno=p->getError();
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		delete p;
		return -1;
	}
	delete p;
	// remove directory part of the name now...
	char *name=strrchr(dir,'/');  // last occurence
	if (!name) name=dir; // 'dir' was already a file
	else name++; // do not keep '/' in name
	// new file with 0ed size
	int fd=getNewFd(fdInfo, fid, name, workgroup, host, share, dir, user, 0);
#if DEBUG >= 2
	cout<<"file "<<name<<" created\n";
#endif
	if (workgroup) delete workgroup;
	if (host) delete host;
	if (share) delete share;
	if (user) delete user;
	if (dir) delete dir;
	// deconnection between open and read/write invalidates fid
	hasBeenDeconnected=0;
	return fd;
}

int SMBIO::stat(const char *filename, struct stat *buf)
{
	char *workgroup=0; // those values have to be parsed
	char *host=0;
	char *share=0;
	char *dir=0;
	char *user=0;

	parse(filename, workgroup, host, share, dir, user);

	if ((!host) && (!share) && (!dir)) { // "smb:" exists, workgroup not important
		buf->st_mode=040755; // directory
		buf->st_uid=getuid();
		buf->st_gid=getgid();
		buf->st_size=0; // Not important for a directory anyway
		buf->st_ctime=buf->st_mtime=buf->st_atime=0;
		buf->st_rdev=0100000; // regular file
		if (user) delete user;
		errno=0;
		return 0;  // OK !
	}
	
	// directory field empty => we're sure it's a dir :-) (browsing or sharelist)
	// but we should check if it exists !
	if (!dir) {
	    char *url=buildURL(workgroup,host,share,0,user);
		int fd=opendir(url); // doesn't matter if 0, but should not be
		delete url;
		if (fd==-1) {
			if (workgroup) delete workgroup;
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			errno=ENOENT; return -1;
		}
		// fd should be kept in memory, with a timeout, in case caller does
		// an opendir just after stat !
		closedir(fd);
		buf->st_mode=040755; // directory
		buf->st_uid=getuid();
		buf->st_gid=getgid();
		buf->st_size=0; // Not important for a directory anyway
		buf->st_ctime=buf->st_mtime=buf->st_atime=0;
		buf->st_rdev=0100000; // regular file
		errno=0;
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		return 0;  // OK !
	}

	// Now we have to check on the server if dir is really a directory !
	if (host) {
		if (!hostName) {
			if ((openSession(host)==-1) || (login(user)==-1)) {
				if (workgroup) delete workgroup;
				if (host) delete host;
				if (share) delete share;
				if (dir) delete dir;
				if (user) delete user;
				errno=ENOENT; return -1;
			}
		} else if (strcasecmp(host,hostName)) {
			closeSession();
			if ((openSession(host)==-1) || (login(user)==-1)) {
				if (workgroup) delete workgroup;
				if (host) delete host;
				if (share) delete share;
				if (dir) delete dir;
				if (user) delete user;
				errno=ENOENT; return -1;
			}
		}
	}
	if (!hostName) {  // still no connection !
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (dir) delete dir;
		if (user) delete user;
		errno=ENOENT; return -1;
	}
	if (share) {
		if (!currentShare) {
			if (openService(share)==-1) {
				if (workgroup) delete workgroup;
				if (host) delete host;
				if (share) delete share;
				if (dir) delete dir;
				if (user) delete user;
				errno=ENOENT; return -1;
			}
		} else if (strcasecmp(share,currentShare)) {
			closeService();
			if (openService(share)==-1) {
				if (workgroup) delete workgroup;
				if (host) delete host;
				if (share) delete share;
				if (dir) delete dir;
				if (user) delete user;
				errno=ENOENT; return -1;
			}
		}
	}
	if (!currentShare) {  // still no connection !
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (dir) delete dir;
		if (user) delete user;
		errno=ENOENT; return -1;
	}
	SMBgetatrPacket *p=new SMBgetatrPacket(TID, dir);
	if (workgroup) delete workgroup;  // parse values not used anymore
	if (host) delete host;
	if (share) delete share;
	if (dir) delete dir;
	if (user) delete user;
	if (send(p)==-1) {delete p; errno=ENOENT; return -1;}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {errno=ENOENT; return -1; }
	if (sesp->getType()!=SESSION_MESSAGE) {
		delete sesp;
		errno=ENOENT;
		return -1;
	}
	p=new SMBgetatrPacket;
	if (p->parse(sesp)==-1) {
#if DEBUG >= 1
		cout<<"stat : Answer is not a valid SMB packet\n";
#endif
		errno=ENOENT;
		delete sesp;
		delete p;
		return -1;
	}
	if (p->getSMBType()!=SMBgetatr) {
#if DEBUG >= 1
		cout<<"stat : Answer is not an getatr SMB packet\n";
#endif
		errno=ENOENT;
		delete sesp;
		delete p;
		return -1;
	}
	struct stat *result=p->getStat();
	if (!result) {
		errno=ENOENT;
		delete sesp;
		delete p;
		return -1;
	}
	delete sesp;
	memcpy(buf, result, sizeof(struct stat));
	delete result;	// allocation subsystem convention
#if DEBUG >= 3
	cout<<"stat : file size : "<<(uint32)(buf->st_size)<<"\n";
#endif
#if DEBUG >= 4
	printf("       mode : %o\n",buf->st_mode);
#endif
	delete p;
	return 0;
}

int SMBIO::readRaw(int fd, void *buf, uint32 count)
{
#if DEBUG >= 5
	cout<<"readRaw : count param="<<count<<"\n";
#endif
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	if ((!info) || (info->fid==-1) || (!(info->dir))) {
		errno=EBADF; return -1;
	}
	if (info->pos>=(unsigned int)(info->st_size)) {
#if DEBUG >= 5
	cout<<"readRaw : cannot read after eof !\n";
#endif
#if DEBUG >= 6
	cout<<"    pos="<<info->pos<<", size="<<info->st_size<<"\n";
#endif
		errno=0; return 0; // end of file
	}
	if (hasBeenDeconnected) { // must reopen, fid invalid
	    char *url=buildURL(info->workgroup,info->host,info->share,info->dir,info->user);
		int fd2=open(url, info->openMode); // set hasBeenDeconnected to 0
		delete url;
		if (fd2==-1) {errno=EBADF; return -1;}
		FdCell *info2 = getFdCellFromFd((FdCell*)fdInfo, fd2);
		info->copy(info2); // deep copy
		closeFd(fdInfo, fd2); // free this fd
	}

	uint32 queue=0, cpt=0;
	int ret=0;
	uint8 *rawdata=new uint8[65539]; //accounts for NetBIOS 4 byte header
	uint32 maxTransfert;
	// Just a hack really, should be in a different method
	if (readRawAvailable) do {
		queue=0; cpt=0;
		// how much we have to ask to the server
		maxTransfert=count>65535?65535:count;
		// limit it to number of bytes remaining in the file
		if (maxTransfert>info->pos-info->st_size) maxTransfert=info->pos-info->st_size;
#if DEBUG >= 4
		cout<<"readRaw : file position : "<<info->pos<<"\n";
#endif
		SMBreadBrawPacket *p1=new SMBreadBrawPacket(TID, info->fid, info->pos, maxTransfert);
		if (send(p1)==-1) {delete p1; delete rawdata; errno=EIO; return -1;}
		queue=recvfrom(sock, rawdata, 65535, 0, 0, 0);//MSG_PEEK, 0, 0);
		if (queue<4) {break; delete rawdata; errno=EIO; return -1;}
		queue-=4; // size of NetBIOS header
		uint8 type=rawdata[0];
		if (type!=0) {delete rawdata; errno=EIO; return -1;} //Not NetBIOS session
		uint32 length=((uint32)rawdata[1])&1
			| ((uint32)rawdata[2])<<8
			| ((uint32)rawdata[3]);
#if DEBUG >= 4
		cout<<"readRaw : received "<<queue<<" bytes, length of NetBIOS data : "<<length<<"\n";
#endif
		if (length==0) break; //gasp {delete rawdata; errno=SUCCESS; return cpt+ret;}
		// copy queue and not length bytes, packet might not be a
		// valid NetBIOS packet, or not complete yet
		memcpy((uint8*)buf+cpt+ret,rawdata+4,queue);
		cpt+=queue;   // total received in this loop
		count-=queue; // how many left to receive before buffer is full
		length-=queue;  // how many left before packet is complete
		info->pos+=queue; // update internal file position pointer
#if DEBUG >= 4
		cout<<"       file position : "<<info->pos<<"\n";
#endif
		while (length>0) // wait for packet completion
		{
			queue=recvfrom(sock, rawdata, length, 0, 0, 0);//MSG_PEEK, 0, 0);
// TODO : ckeck here for I/O errors
			length-=queue; // #bytes still to receive
			memcpy((uint8*)buf+cpt+ret,rawdata,queue);
			cpt+=queue; count-=queue;
			info->pos+=queue;
#if DEBUG >= 4
			cout<<"readRaw : received "<<queue<<" bytes, remaining : "<<length<<"\n";
			cout<<"       file position : "<<info->pos<<"\n";
#endif
		}
#if DEBUG >= 3
		cout<<"readRaw complete, file position : "<<info->pos<<"\n";
#endif
		if (info->pos>=(unsigned int)(info->st_size)) { // end of file
			delete rawdata;
			errno=0;
			return cpt+ret;
		}
		if ((int32)count<=0) // buffer is full
		{
#if DEBUG >= 5
			cout<<"readRaw : buffer full, returned value : "<<cpt+ret<<"\n";
#endif
			delete rawdata;
			return cpt+ret;
		}
#if DEBUG >= 5
		cout<<"readRaw : buffer not full\n";
#endif
		ret+=maxTransfert;
	// maximum reached, buffer not full  => do it again
	} while (cpt==maxTransfert);
	
	// now we won't use cpt
	ret+=cpt;
	delete rawdata;
	
#if DEBUG >= 5
	if (readRawAvailable)
		cout<<"readRaw : max size reached, but buffer not full\n";
#endif
#if DEBUG >= 4
	cout<<"readRaw : trying simple read transaction\n";
#endif
	

    // Hmmm, we have to ckeck the server
    // doc says : might be EOF, error, or "server temporarily
    // out of large buffers", whatever this means
    // Solution is to send another type of read request and see.
        	
	// transfert only what we can, minus 100 bytes to account for
	// the remaining of the SMB packet.
	if ((int32)count>(int32)maxMessageSize-100) count=maxMessageSize-100;
	SMBreadPacket *p=new SMBreadPacket(TID, info->fid, info->pos, count);
	if (send(p)==-1) {delete p; errno=EIO; return -1;}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {errno=EIO; return -1;}
	if (sesp->getType()!=SESSION_MESSAGE)
	{
		delete sesp;
		errno=EIO;
		return -1;
	}
	p=new SMBreadPacket;
	if (p->parse(sesp)==-1)
	{
#if DEBUG >= 1
		cout<<"readRaw : Answer is not a valid SMB packet\n";
#endif
		errno=EIO;
		delete sesp;
		delete p;
		return -1;
	}
	if (p->getSMBType()!=SMBread)
	{
#if DEBUG >= 1
		cout<<"readRaw : Answer is not an read SMB packet\n";
#endif
		errno=EIO;
		delete sesp;
		delete p;
		return -1;
	}
	delete sesp;
	count=p->getReadCount();
	if ((int)count==-1) {errno=EIO; delete p; return -1;}
	uint8 *data=p->getReadData();
	if (data==0) {errno=EIO; delete p; return -1;}
	memcpy((uint8*)buf+ret, data, count);
	info->pos+=count;
	delete data;
#if DEBUG >= 3
	cout<<"readRaw : "<<ret+count<<" bytes read\n";
	cout<<"       file position : "<<info->pos<<"\n";
#endif
	delete p;
	return ret+count;
}

// read, with caching
int SMBIO::read(int fd, void *buf, uint32 count)
{
#if DEBUG >= 2
cout<<"read\n";
#endif
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	if ((!info) || (info->fid==-1) || (!(info->dir))) {
		errno=EBADF; return -1;
	}
	if (hasBeenDeconnected) { // must reopen, fid invalid
	    char *url=buildURL(info->workgroup,info->host,info->share,info->dir,info->user);
		int fd2=open(url, info->openMode); // set hasBeenDeconnected to 0
		delete url;
		if (fd2==-1) {errno=EBADF; return -1;}
		FdCell *info2 = getFdCellFromFd((FdCell*)fdInfo, fd2);
		info->copy(info2); // deep copy
		closeFd(fdInfo, fd2); // free this fd
	}

	// must allocate a cache the first time, or when connection broken
	if (!(info->cache)) {
		// how much yet to do ?
		uint32 maxSize=info->st_size-info->pos; // st_size set by open
		// now we can adapt cache to the demand !
		uint32 countK=(count>>9);  // twice the size of demand, in K
		// limit the cache to 63 K (still a good cache !) : see readRaw
		if (countK>63) countK=63;
		if (countK<4) countK=4; // and use a standard cache for small demands
		countK<<=10;  // now put it in bytes
		// no need to ask more than file size !
		if (countK>maxSize) countK=maxSize;
		info->clen=countK;
		info->cache=new uint8[countK]; // adaptative cache created !
		info->cpos=info->cache;
		// force filling the first time
		info->cmaxRead=info->cmaxWrite=0;
		info->cachePositionInFile=info->pos;
	}
#if DEBUG >= 5
cout<<"read : cache size : "<<info->clen<<", info->pos="<<info->pos<<"\n";
cout<<"read : info->cpos-info->cache : "<<info->cpos-info->cache<<", info->cmaxRead="<<info->cmaxRead<<"\n";
cout<<"read : info->cachePositionInFile : "<<info->cachePositionInFile<<"\n";
#endif

	uint8 *current=(uint8*)buf, *end=(uint8*)buf+count;
	while (current<end) {
		if (info->cpos-info->cache>=info->cmaxRead) { // must fill the cache ?
			 // end of file ?
			if ((info->pos >= (uint32)info->st_size)
			|| ((uint32)(info->cachePositionInFile+info->cmaxRead) >= (uint32)info->st_size)) {
#if DEBUG >= 5
cout<<"read : end 1 : info->pos="<<info->pos<<"\n";
cout<<"read : info->cpos-info->cache : "<<info->cpos-info->cache<<", info->cmaxRead="<<info->cmaxRead<<"\n";
cout<<"read : info->cachePositionInFile : "<<info->cachePositionInFile<<"\n";
#endif
				errno=0; return current-(uint8*)buf;
			}
			int num=0;
			uint32 posSav=info->pos;
			// chech wether we'll erase data if we refill the cache
			if ((info->cmaxRead<info->cmaxWrite) || (info->cmaxRead>=info->clen)) {
				if (flush(fd)==-1) return -1;
				info->pos=info->cachePositionInFile;
			} else {
				info->pos=info->cachePositionInFile+info->cmaxRead;
			}
			// we can fill the cache without disturbing write !
			num=readRaw(fd, info->cache+info->cmaxRead, info->clen-info->cmaxRead);
			info->pos=posSav;
			if ((num==-1) || (num==0)) {
#if DEBUG >= 5
cout<<"read : end 2 : info->pos="<<info->pos<<"\n";
cout<<"read : info->cpos-info->cache : "<<info->cpos-info->cache<<", info->cmaxRead="<<info->cmaxRead<<"\n";
cout<<"read : info->cachePositionInFile : "<<info->cachePositionInFile<<"\n";
#endif
				errno=0;
				return current-(uint8*)buf;
			}
			info->cmaxRead+=num;
		}
		*(current++)=*(info->cpos++);  // fill buffer
		info->pos++;
//		info->pos=info->cachePositionInFile+(info->cpos-info->cache);
	}
#if DEBUG >= 5
cout<<"read : after loop : info->pos="<<info->pos<<"\n";
#endif
	errno=0;
	return current-(uint8*)buf;
}

// doesn't work !
int SMBIO::writeRaw(int fd, void *buf, uint32 count)
{
#if DEBUG >= 5
	cout<<"writeRaw : count param="<<count<<"\n";
#endif
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	if ((!info) || (info->fid==-1) || (!(info->dir))) {
		errno=EBADF; return -1;
	}
	if (hasBeenDeconnected) { // must reopen, fid invalid
	    char *url=buildURL(info->workgroup,info->host,info->share,info->dir,info->user);
		int fd2=open(url, info->openMode); // set hasBeenDeconnected to 0
		delete url;
		if (fd2==-1) {errno=EBADF; return -1;}
		FdCell *info2 = getFdCellFromFd((FdCell*)fdInfo, fd2);
		info->copy(info2); // deep copy
		closeFd(fdInfo, fd2); // free this fd
	}

	uint32 cpt=0;
	int32 maxTransfert;
	uint8 *rawdata=0;
	SessionPacket *sesp=0;
	do
	{
		// how much we have to send to the server
		maxTransfert=count-cpt>65535?65535:count-cpt;
#if DEBUG >= 4
		cout<<"writeRaw : sending request to server, file position : "<<info->pos<<"\n";
#endif
		SMBwriteBrawPacket *p=new SMBwriteBrawPacket(TID, info->fid, info->pos, maxTransfert);
		if (send(p)==-1) {delete p; errno=EIO; return -1;}
#if DEBUG >= 4
		cout<<"writeRaw : waiting server response\n";
#endif
		delete p;
		sesp = receive();
		if (sesp==0) {errno=EIO; return -1;}
#if DEBUG >= 6
		sesp->print();
#endif
		if (sesp->getType()!=SESSION_MESSAGE) {delete sesp;	errno=EIO; return -1;}
		p=new SMBwriteBrawPacket;
		if (p->parse(sesp)==-1) {
#if DEBUG >= 1
			cout<<"writeRaw : Answer is not a valid SMB packet\n";
#endif
			errno=EIO; delete sesp; delete p;return -1;
		}
		if ((p->getSMBType()!=SMBwriteBraw) && (p->getSMBType()!=SMBwriteBs))
		{
#if DEBUG >= 1
			cout<<"writeRaw : Answer is not an writeBraw SMB packet\n";
#endif
			errno=EIO; delete sesp; delete p;return -1;
		}
		delete sesp;
		if (p->getError()) { // try with another write request
			delete p;
#if DEBUG >= 4
			cout<<"writeRaw : trying simple write transaction\n";
#endif
			// Hmmm, we have to ckeck the server
			// doc says : "server might be out of ressources" or busy
			// Solution is to send another type of write request, or try again
			// after waiting a little

			// transfert only what we can, minus 100 bytes to account for
			// the remaining of the SMB packet.
			maxTransfert=(int32)(count-cpt)>(int32)maxMessageSize-100?(int32)maxMessageSize-100:count-cpt;
			int simple=writeSimple(fd, (uint8*)buf+cpt, maxTransfert);
			if (simple==-1) {errno=EIO; return -1;} // even simple write failed
			cpt+=simple;   // total data sent
			// file position pointer already updated by writeSimple //info->pos+=written;
			continue; // loop and try writeRaw again
		}
		delete p;

		rawdata=new uint8[65539]; //accounts for NetBIOS 4 byte header
		// prepare a NetBIOS header
		rawdata[0]=rawdata[1]=0;
		rawdata[2]=(uint8)(maxTransfert&0xFF);
		rawdata[3]=(uint8)(maxTransfert>>8);
		// Now send data to server
		int nbLoop2=0;
		do {
			// try to send all data in one go !
			memcpy(rawdata+4, (uint8*)buf+cpt, maxTransfert);
			int sent=::write(sock, rawdata, 4+maxTransfert)-4; // remove NetBIOS header
			if (sent<0) {errno=EIO; delete rawdata; return -1;} // error !
#if DEBUG >= 4
			cout<<"writeRaw : sent "<<sent<<" bytes\n";
#endif
			cpt+=sent;   // total data sent
			info->pos+=sent; // update internal file position pointer
			maxTransfert-=sent;
#if DEBUG >= 4
			cout<<"       file position : "<<info->pos<<"\n";
#endif
		} while ((maxTransfert>0) && (nbLoop2++<3));
#if DEBUG >= 3
		cout<<"writeRaw complete, waiting for server ack.\n";
#endif
		delete rawdata;
		sesp = receive();
		if (sesp==0) {errno=EIO; return -1;}
#if DEBUG >= 6
		sesp->print();
#endif
		if (sesp->getType()!=SESSION_MESSAGE) {delete sesp;	errno=EIO; return -1;}
		SMBwriteCPacket *pC=new SMBwriteCPacket;
		if (pC->parse(sesp)==-1) {
#if DEBUG >= 1
			cout<<"writeRaw : Answer is not a valid SMB packet\n";
#endif
			errno=EIO; delete sesp; delete pC;return -1;
		}
		if (pC->getSMBType()!=SMBwriteC)
		{
#if DEBUG >= 1
			cout<<"writeRaw : Answer is not an write complete SMB packet\n";
#endif
			errno=EIO; delete sesp; delete pC; return -1;
		}
		delete sesp;
		if (pC->getError()) {errno=EIO; delete pC; return -1;}
		delete pC;
	
	} while (cpt<count); // end of main loop
	
	if (rawdata) delete rawdata;
	errno=0;
	return cpt;
}

// write, using core protocol packets
int SMBIO::writeSimple(int fd, void *buf, uint32 count)
{
	// transfert only what we can, minus 100 bytes to account for
	// the remaining of the SMB packet.
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	int32 cpt=0;
	int32 written=0;
	uint32 maxTransfert=0;
	do {
		maxTransfert=(int32)(count-cpt)>(int32)maxMessageSize-100?(int32)maxMessageSize-100:count-cpt;
		SMBwritePacket *p2=new SMBwritePacket(TID, info->fid, info->pos, (uint8*)buf+cpt, maxTransfert);
		if (send(p2)==-1) {delete p2; errno=EIO; return cpt?cpt:-1;}
		delete p2;
		SessionPacket *sesp = receive();
		if (sesp==0) {errno=EIO; return -1;}
		if (sesp->getType()!=SESSION_MESSAGE) {
			delete sesp; errno=EIO; return cpt?cpt:-1;
		}
		p2=new SMBwritePacket;
		if (p2->parse(sesp)==-1) {
#if DEBUG >= 1
			cout<<"write : simple write : Answer is not a valid SMB packet\n";
#endif
			errno=EIO; delete sesp; delete p2;
			return cpt?cpt:-1;
		}
		if (p2->getSMBType()!=SMBwrite) {
#if DEBUG >= 1
			cout<<"write : simple write :  Answer is not an read SMB packet\n";
#endif
			errno=EIO; delete sesp;	delete p2; return cpt?cpt:-1;
		}
		delete sesp;
		written=p2->getWrittenCount();
		if (written==-1) {errno=EIO; delete p2; return cpt?cpt:-1;}
		cpt+=written;
		info->pos+=written; // update internal file position pointer
#if DEBUG >= 3
		cout<<"write : simple write :  "<<written<<" bytes written\n";
		cout<<"       file position : "<<info->pos<<"\n";
#endif
		delete p2;
	} while (((int32)cpt<(int32)count) && (written!=0)); // stop when not possible to write
	errno=0;
	return cpt;
}


// write, with caching
int SMBIO::write(int fd, void *buf, uint32 count)
{
#if DEBUG >= 2
cout<<"write\n";
#endif
	
//	return writeSimple(fd, buf, count);

	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	if ((!info) || (info->fid==-1) || (!(info->dir))) {
		errno=EBADF; return -1;
	}
	if (!(info->openMode & O_RDWR) && !(info->openMode & O_WRONLY)) {
		errno=EACCES; return -1;
	}
	if (hasBeenDeconnected) { // must reopen, fid invalid
	    char *url=buildURL(info->workgroup,info->host,info->share,info->dir,info->user);
		int fd2=open(url, info->openMode); // set hasBeenDeconnected to 0
		delete url;
		if (fd2==-1) {errno=EBADF; return -1;}
		FdCell *info2 = getFdCellFromFd((FdCell*)fdInfo, fd2);
		info->copy(info2); // deep copy
		closeFd(fdInfo, fd2); // free this fd
	}

	// must allocate a cache the first time, or when connection broken
	if (!(info->cache)) {
		// now we can adapt cache to the demand !
		uint32 countK=(count>>9);  // twice the size of demand, in K
		// limit the cache to 63 K (still a good cache !) : see readRaw
		if (countK>63) countK=63;
		if (countK<4) countK=4; // and use a standard cache for small demands
		countK<<=10;  // now put it in bytes
		info->clen=countK;
		info->cache=new uint8[countK]; // adaptative cache created !
		info->cpos=info->cache; // empty cache for the first time
		// force filling the first time
		info->cmaxRead=info->cmaxWrite=0;
		info->cachePositionInFile=info->pos;
	}
#if DEBUG >= 5
cout<<"write : cache size : "<<info->clen<<"\n";
#endif

	uint8 *current=(uint8*)buf, *end=(uint8*)buf+count;
	uint32 nbWritten=0;
	while (current<end) {
		if (info->cpos-info->cache>=info->clen) { // must write the cache ?
			if (flush(fd)==-1) return -1;
		}
		*(info->cpos++)=*(current++);  // fill cache
		nbWritten++;
		if (info->cmaxWrite<info->cpos-info->cache)
			info->cmaxWrite=info->cpos-info->cache;
		if (info->cmaxRead<info->cmaxWrite) info->cmaxRead=info->cmaxWrite;
		info->pos++;
		if ((unsigned int)(info->st_size)<info->pos) info->st_size=info->pos;
//		info->pos=info->cachePositionInFile+(info->cpos-info->cache);
	}
	errno=0;
	return nbWritten;
}


// flush forces a write of all buffered data for the given fd
int SMBIO::flush(int fd)
{
#if DEBUG >= 2
cout<<"flush\n";
#endif
	
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	if ((!info) || (info->fid==-1) || (!(info->dir)) || (!(info->cache))) {
		errno=EBADF; return -1;
	}
	if (info->cmaxWrite==0) { // nothing to do
#if DEBUG >= 3
cout<<"flush : nothing to flush !\n";
#endif
		if (info->cpos-info->cache>=info->clen) { // empty this cache
			info->cachePositionInFile=info->pos;
			info->cpos=info->cache;
			info->cmaxRead=info->cmaxWrite=0;
			errno=0;
		} // else wait for the cache to be full
		return 0;
	}
	if (hasBeenDeconnected) { // must reopen, fid invalid
	    char *url=buildURL(info->workgroup,info->host,info->share,info->dir,info->user);
		int fd2=open(url, info->openMode); // set hasBeenDeconnected to 0
		delete url;
		if (fd2==-1) {errno=EBADF; return -1;}
		FdCell *info2 = getFdCellFromFd((FdCell*)fdInfo, fd2);
		info->copy(info2); // deep copy
		closeFd(fdInfo, fd2); // free this fd
	}
	uint32 posSav=info->pos;
	info->pos=info->cachePositionInFile;
#if DEBUG >= 4
	cout<<"flush: info->cachePositionInFile="<<info->cachePositionInFile<<"\n";
#endif
	int num=writeSimple(fd, info->cache, info->cmaxWrite);
#if DEBUG >= 4
	cout<<"flush: num="<<num<<", info->cmaxWrite="<<info->cmaxWrite<<"\n";
#endif
	// Error or no write : errno set in writeSimple (might be 0)
	if ((num<=0) || (num>info->clen)) {
		info->pos=posSav;
		return -1;
	}
	// couldn't write everything
	if (num<info->cmaxWrite) {
		int leftover=(info->cmaxWrite-num);
		for (uint8* p=info->cache; p<info->cache+leftover; p++) *p=*(p+num);
		info->cachePositionInFile+=num;
		info->cpos=info->cache+leftover; // this has been sent OK
		info->cmaxWrite=leftover;
		info->cmaxRead=leftover;
		info->pos=posSav;
		return -1;
	}
	info->pos=posSav;
	// set up an empty cache
	info->cachePositionInFile=info->pos;
	info->cpos=info->cache;
	info->cmaxRead=info->cmaxWrite=0;
#if DEBUG >= 4
	cout<<"flush: info->cachePositionInFile="<<info->cachePositionInFile<<"\n";
#endif
	errno=0;
	return 0;
}

// flush forces a write of all buffered data for the given fd
int32 SMBIO::lseek(int fd, int32 offset, int from)
{
#if DEBUG >= 2
	cout<<"lseek\n";
#endif
	FdCell *info=getFdCellFromFd((FdCell*)fdInfo, fd);
	if (!info) {errno=EBADF; return -1;} // not opened, or invalid fd
	int32 newPos=0;
	switch (from) {
		case SEEK_SET:
			newPos=offset;
			break;
		case SEEK_CUR:
/*			if (info->cache)
				newPos=info->cachePositionInFile+(info->cpos-info->cache)+offset;
			else*/
				newPos=info->pos+offset;
			break;
		case SEEK_END:
			newPos=info->st_size+offset;
			break;
		default: {errno=EINVAL; return -1;}
	}
	if (newPos<0) newPos=0;
#if DEBUG >= 4
	cout<<"lseek : info->pos="<<info->pos<<", offset="<<offset<<", newPos="<<newPos<<"\n";
#endif
	if (info->cache) {
		if ((newPos>=info->cachePositionInFile)
		&& (newPos<info->cachePositionInFile+info->clen)) {
			info->cpos=info->cache+(newPos-info->cachePositionInFile);
		} else {
			if (flush(fd)==-1) return -1;
			info->cachePositionInFile=newPos;
			info->cpos=info->cache;
			info->cmaxRead=info->cmaxWrite=0;
		}
	}
	info->pos=newPos;
	return newPos;
}

int SMBIO::close(int fd)
{
	FdCell *info=getFdCellFromFd((FdCell*)fdInfo, fd);
	if (!info) {errno=EBADF; return -1;} // already closed, or invalid fd
	// write last changes eventually
	flush(fd);
	SMBclosePacket *p=new SMBclosePacket(info->fid);
	if (send(p)==-1) {delete p; errno=EIO; return -1;}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {errno=EIO; return -1;}
	if (sesp->getType()!=SESSION_MESSAGE) {
		delete sesp; errno=EIO; return -1;
	}
	p=new SMBclosePacket;
	if (p->parse(sesp)==-1) {
#if DEBUG >= 1
		cout<<"close : Answer is not a valid SMB packet\n";
#endif
		errno=EIO; delete sesp; delete p;
		return -1;
	}
	if (p->getSMBType()!=SMBclose) {
#if DEBUG >= 1
		cout<<"close : Answer is not a close SMB packet\n";
#endif
		errno=EIO; delete sesp;	delete p; return -1;
	}
	delete sesp;
	delete p;
#if DEBUG >= 2
	cout<<"file "<<info->name<<" closed.\n";
#endif
	return closeFd(fdInfo, fd);
}
	
// unlink sends a unlink SMB. Will delete or unlink depending on target OS
int SMBIO::unlink(const char *file)
{
	char *workgroup=0; // those values have to be parsed
	char *host=0;
	char *share=0;
	char *dir=0;
	char *user=0;
	parse(file, workgroup, host, share, dir, user);
	
	if (!dir) { // browse, or sharelist
		errno=EISDIR;
		return -1;
	}

	// now that we're sure there is a dir/file, we can safely assume
	// stat won't browse or open unecessary connections.
	// => check if dir/file is a file, and not a directory, if it exists
	struct stat statbuf;
 	if ( (stat(file,&statbuf)!=-1) && (statbuf.st_mode&040000) ) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EISDIR;
		return -1;
	}
	
	if (workgroup) delete workgroup;
#ifdef OVERKILL
	closeSession();
	if ((openSession(host)==-1) || (login(user)==-1)) {
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ENOENT; return -1;
	}
#else
	if (!hostName) {
		if (openSession(host)==-1) {
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	} else if (strcasecmp(hostName,host)) {
		closeSession();
		if (openSession(host)==-1) {
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	}
	if (login(user)==-1) {
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EACCES; return -1;
	}
#endif
	if (openService(share)==-1) {
		errno=ENOENT; return -1;
	}
	if (host) delete host;
	if (share) delete share;
	if (user) delete user;

	char *dospath=new char[strlen(dir)+3];
	dospath[0]=dospath[1]='\\';
	for (int i=0; i<(int)strlen(dir); i++)
		dospath[i+2]=(dir[i]=='/')?'\\':dir[i];
	dospath[strlen(dir)+2]=0;
	if (dir) delete dir;
	
	// Destroy the file
	SMBunlinkPacket *p=new SMBunlinkPacket(TID, dospath);
	delete dospath;
	if (send(p)==-1) {
		delete p;
		errno=ERRerror;
		return -1;
	}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {
		errno=ERRerror;
		return -1;
	}
	if (sesp->getType()!=SESSION_MESSAGE) {
		delete sesp;
		errno=ERRerror;
		return -1;
	}
	p=new SMBunlinkPacket;
	if (p->parse(sesp)==-1) {
#if DEBUG >= 1
		cout<<"unlink : Answer is not a valid SMB packet\n";
#endif
		errno=p->errno;
		delete sesp;
		delete p;
		return -1;
	}
	delete sesp;
	if (p->getSMBType()!=SMBunlink) {
#if DEBUG >= 1
		cout<<"unlink : Answer is not an unlink SMB packet\n";
#endif
		errno=p->errno;
		delete p;
		return -1;
	}
	if ((errno=p->getError())!=0) {
		errno=EACCES;
		return -1;
	}
	delete p;
	return 0;
}
	
// deletes a directory
int SMBIO::rmdir(const char *pathname)
{
	char *workgroup=0; // those values have to be parsed
	char *host=0;
	char *share=0;
	char *dir=0;
	char *user=0;
	parse(pathname, workgroup, host, share, dir, user);
	
	if (!dir) { // browse, or sharelist
		errno=EACCES;
		return -1;
	}

	// now that we're sure there is a dir/file, we can safely assume
	// stat won't browse or open unecessary connections.
	// => check if dir/file is a file, and not a directory, if it exists
	struct stat statbuf;
 	if ( (stat(pathname,&statbuf)!=-1) && (!(statbuf.st_mode&040000)) ) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ENOTDIR;
		return -1;
	}
	
	if (workgroup) delete workgroup;
#ifdef OVERKILL
	closeSession();
	if ((openSession(host)==-1) || (login(user)==-1)) {
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ENOENT; return -1;
	}
#else
	if (!hostName) {
		if (openSession(host)==-1) {
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	} else if (strcasecmp(hostName,host)) {
		closeSession();
		if (openSession(host)==-1) {
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	}
	if (login(user)==-1) {
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EACCES; return -1;
	}
#endif
	if (openService(share)==-1) {
		errno=ENOENT; return -1;
	}
	if (host) delete host;
	if (share) delete share;
	if (user) delete user;

	char *dospath=new char[strlen(dir)+3];
	dospath[0]=dospath[1]='\\';
	for (int i=0; i<(int)strlen(dir); i++)
		dospath[i+2]=(dir[i]=='/')?'\\':dir[i];
	dospath[strlen(dir)+2]=0;
	if (dir) delete dir;
	
	// Now delete the directory
	SMBrmdirPacket *p=new SMBrmdirPacket(TID, dospath);
	delete dospath;
	if (send(p)==-1) {
		delete p;
		errno=ERRerror;
		return -1;
	}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {
		errno=ERRerror;
		return -1;
	}
	if (sesp->getType()!=SESSION_MESSAGE) {
		delete sesp;
		errno=ERRerror;
		return -1;
	}
	p=new SMBrmdirPacket;
	if (p->parse(sesp)==-1) {
#if DEBUG >= 1
		cout<<"rmdir : Answer is not a valid SMB packet\n";
#endif
		errno=p->errno;
		delete sesp;
		delete p;
		return -1;
	}
	delete sesp;
	if (p->getSMBType()!=SMBrmdir) {
#if DEBUG >= 1
		cout<<"rmdir : Answer is not a rmdir SMB packet\n";
#endif
		errno=p->errno;
		delete p;
		return -1;
	}
	if ((errno=p->getError())!=0) {
		errno=EACCES;
		return -1;
	}
	delete p;
	return 0;
}

// creates a directory
int SMBIO::mkdir(const char *pathname)
{
	// Must append ".." and check the parent
	char *urlParent=append(pathname, "..");
	
	if (!urlParent) {
		errno=ENOENT;
		return -1;
	}
	
	char *workgroup=0; // those values have to be parsed
	char *host=0;
	char *share=0;
	char *dir=0;
	char *user=0;
	parse(urlParent, workgroup, host, share, dir, user);
	
	if (!share) { // browse
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (user) delete user;
		if (dir) delete dir;
		errno=EACCES;
		delete urlParent;
		return -1;
	}

	// => check if parent is a directory, and not a file, if it exists
	struct stat statbuf;
 	if ( (stat(urlParent,&statbuf)!=-1) && (!(statbuf.st_mode&040000)) ) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ENOTDIR;
		delete urlParent;
		return -1;
	}
	delete urlParent;
		
	// now parse the pathname
	if (workgroup) delete workgroup;
	if (host) delete host;
	if (share) delete share;
	if (user) delete user;
	if (dir) delete dir;
	parse(pathname, workgroup, host, share, dir, user);
	
	if (workgroup) delete workgroup;
#ifdef OVERKILL
	closeSession();
	if ((openSession(host)==-1) || (login(user)==-1)) {
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ENOENT; return -1;
	}
#else
	if (!hostName) {
		if (openSession(host)==-1) {
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	} else if (strcasecmp(hostName,host)) {
		closeSession();
		if (openSession(host)==-1) {
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	}
	if (login(user)==-1) {
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EACCES; return -1;
	}
#endif
	if (openService(share)==-1) {
		errno=ENOENT; return -1;
	}
	if (host) delete host;
	if (share) delete share;
	if (user) delete user;

	char *dospath=new char[strlen(dir)+3];
	dospath[0]=dospath[1]='\\';
	for (int i=0; i<(int)strlen(dir); i++)
		dospath[i+2]=(dir[i]=='/')?'\\':dir[i];
	dospath[strlen(dir)+2]=0;
	if (dir) delete dir;
	
	// Now create the directory
	SMBmkdirPacket *p=new SMBmkdirPacket(TID, dospath);
	delete dospath;
	if (send(p)==-1) {
		delete p;
		errno=ERRerror;
		return -1;
	}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {
		errno=ERRerror;
		return -1;
	}
	if (sesp->getType()!=SESSION_MESSAGE) {
		delete sesp;
		errno=ERRerror;
		return -1;
	}
	p=new SMBmkdirPacket;
	if (p->parse(sesp)==-1) {
#if DEBUG >= 1
		cout<<"mkdir : Answer is not a valid SMB packet\n";
#endif
		errno=p->errno;
		delete sesp;
		delete p;
		return -1;
	}
	delete sesp;
	if (p->getSMBType()!=SMBmkdir) {
#if DEBUG >= 1
		cout<<"mkdir : Answer is not a mkdir SMB packet\n";
#endif
		errno=p->errno;
		delete p;
		return -1;
	}

	if ((errno=p->getError())!=0) {
		errno=EACCES;
		return -1;
	}

	delete p;
	return 0;
}

// Renames a file, but doesn't move it between directories
int SMBIO::rename(const char *fileURL, const char *newname)
{
	if ((!fileURL) || (!newname)) {
		errno=ENOENT;
		return -1;
	}
	char *workgroup=0; // those values have to be parsed
	char *host=0;
	char *share=0;
	char *dir=0;
	char *user=0;
	parse(fileURL, workgroup, host, share, dir, user);
	
	if (!dir) { // browse, or sharelist
		errno=EACCES;
		return -1;
	}

	// now that we're sure there is a dir/file, we can safely assume
	// stat won't browse or open unecessary connections.
	// => check if dir/file is a file, and not a directory, if it exists
	struct stat statbuf;
 	if ( (stat(fileURL,&statbuf)==-1) || (statbuf.st_mode&040000) ) {
		if (workgroup) delete workgroup;
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EISDIR;
		return -1;
	}
	
	if (workgroup) delete workgroup;
#ifdef OVERKILL
	closeSession();
	if ((openSession(host)==-1) || (login(user)==-1)) {
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=ENOENT; return -1;
	}
#else
	if (!hostName) {
		if (openSession(host)==-1) {
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	} else if (strcasecmp(hostName,host)) {
		closeSession();
		if (openSession(host)==-1) {
			if (host) delete host;
			if (share) delete share;
			if (user) delete user;
			if (dir) delete dir;
			errno=ENOENT; return -1;
		}
	}
	if (login(user)==-1) {
		if (host) delete host;
		if (share) delete share;
		if (user) delete user;
		if (dir) delete dir;
		errno=EACCES; return -1;
	}
#endif
	if (openService(share)==-1) {
		errno=ENOENT; return -1;
	}
	if (host) delete host;
	if (share) delete share;
	if (user) delete user;

	// Must give the full path relative to the share for newname
	char *slash=strrchr(dir,'/');
	// Worse case allocation
	char *destfile=new char[strlen(dir)+3+strlen(newname)+1];
	if (slash) {
		strcpy(destfile,"\\\\");
		strncpy(destfile+2,dir,slash-dir);
		strcpy(destfile+2+(slash-dir),"\\");
		strcpy(destfile+3+(slash-dir),newname);
	} else {
		strcpy(destfile,"\\\\");
		strcpy(destfile+2,newname);
	}
	
	char *dospath=new char[strlen(dir)+3];
	dospath[0]=dospath[1]='\\';
	for (int i=0; i<(int)strlen(dir); i++)
		dospath[i+2]=(dir[i]=='/')?'\\':dir[i];
	dospath[strlen(dir)+2]=0;
	if (dir) delete dir;

	// Build and send the packet
	SMBmvPacket *p=new SMBmvPacket(TID, dospath, destfile);
	delete dospath;
	delete destfile;
	
	if (send(p)==-1) {
		delete p;
		errno=ERRerror;
		return -1;
	}
	delete p;
	SessionPacket *sesp = receive();
	if (sesp==0) {
		errno=ERRerror;
		return -1;
	}
	if (sesp->getType()!=SESSION_MESSAGE) {
		delete sesp;
		errno=ERRerror;
		return -1;
	}
	p=new SMBmvPacket;
	if (p->parse(sesp)==-1) {
#if DEBUG >= 1
		cout<<"rename : Answer is not a valid SMB packet\n";
#endif
		errno=p->errno;
		delete sesp;
		delete p;
		return -1;
	}
	delete sesp;
	if (p->getSMBType()!=SMBmv) {
#if DEBUG >= 1
		cout<<"rename : Answer is not a mv SMB packet\n";
#endif
		errno=p->errno;
		delete p;
		return -1;
	}
	delete p;
	if ((errno=p->getError())!=0) {
		errno=EACCES;
		return -1;
	}
	return 0;
}


SMBIO::TransactInfo::TransactInfo()
{
	tid=0;
	transactName=0;
	setup=0; setupLength=0; // # of uint16
	param=0; paramLength=0; // in bytes
	data=0; dataLength=0; // in bytes
}

SMBIO::TransactInfo::~TransactInfo()
{
	if (transactName) delete transactName;
	if (setup) delete setup;
	if (param) delete param;
	if (data) delete data;
}

SMBIO::TransactInfo *SMBIO::transact(SMBIO::TransactInfo *tinfo)
{
#if DEBUG >= 6
	cout<<"TransactInfo : pointeur :"<<tinfo<<"\n";
	if (tinfo)
	{
		cout<<"  dataLength :"<<tinfo->dataLength<<"\n  Data :\n";
		for (int i=0; i<tinfo->dataLength; i++) printf("%X ",tinfo->data[i]);
		cout<<"\n";
		cout<<"  paramLength :"<<tinfo->paramLength<<"\n  Param :\n";
		for (int i=0; i<tinfo->paramLength; i++) printf("%X ",tinfo->param[i]);
		cout<<"\n";
		cout<<"  tid :"<<tinfo->tid<<"\n";
	}
#endif

	// we can expect at maximum the following size for the first packet
	uint16 nameLength=(tinfo->transactName)?strlen(tinfo->transactName)+1:0;
	uint16 size=35+14*2+tinfo->setupLength*2+nameLength+1+tinfo->paramLength+1+tinfo->dataLength;
	uint16 maxDataLength=tinfo->dataLength;
	uint16 maxParamLength=tinfo->paramLength;
	if (size>maxMessageSize)
	{
		// doc says : send param first when max packet too short
		if ((size-maxMessageSize)<=tinfo->dataLength)
		{
			maxDataLength=tinfo->dataLength-(size-maxMessageSize);
			maxParamLength=tinfo->paramLength;
		}
		else
		{
			maxDataLength=0;
			// if name+setup are too long, return with error
			if (tinfo->paramLength<((size-maxMessageSize)-tinfo->dataLength)) {errno=ERRerror; return 0;}
			maxParamLength=tinfo->paramLength-((size-maxMessageSize)-tinfo->dataLength);
		}
	}
	// Send first packet with maximum param/data in
	SMBtransPacket *p=new SMBtransPacket(tinfo->tid,tinfo->transactName,tinfo->setup,
		tinfo->setupLength, tinfo->paramLength, tinfo->dataLength, // total count of each
		tinfo->param, maxParamLength, tinfo->data, maxDataLength);
#if DEBUG >=6
	cout<<"1st Trans Packet sent :\n";
	p->print();
#endif
	if (send(p)==-1) {delete p; errno=ERRerror; return 0;}
	delete p;
	// if we could not send everything the first time
	if ((maxDataLength<tinfo->dataLength) || (maxParamLength<tinfo->paramLength))
	{
		// wait for server interim response
		SessionPacket *sesp = receive();
		if (sesp==0) {errno=ERRerror; return 0;}
		if (sesp->getType()!=SESSION_MESSAGE) {delete sesp; errno=ERRerror; return 0;}
		delete sesp;
		// We should really check here if response is correct...
		// now send all the param first
		uint16 paramDisplacement=maxParamLength;
		uint16 dataDisplacement=maxDataLength; // see above
		maxParamLength=maxMessageSize-53; // without data, secondary request packet
		if (maxParamLength<=0) {errno=ERRerror; return 0;}
		while (paramDisplacement<tinfo->paramLength)
		{
			uint16 n=tinfo->paramLength-paramDisplacement;
			if (n>maxParamLength) n=maxParamLength;
			// use secondary transact request
			SMBtranssPacket *p=new SMBtranssPacket(tinfo->tid,
				tinfo->paramLength, tinfo->dataLength, // total count of each
				paramDisplacement, dataDisplacement,
				tinfo->param+paramDisplacement, n, tinfo->data, 0);
			if (send(p)==-1) {delete p; errno=ERRerror; return 0;}
			delete p;
			paramDisplacement+=n;
		}
		// send the data now
		// rem : param displacement = paramLength here
		maxDataLength=maxMessageSize-53; // without data, secondary request packet
		if (maxDataLength<=0) {errno=ERRerror; return 0;}
		while (dataDisplacement<tinfo->dataLength)
		{
			uint16 n=tinfo->dataLength-dataDisplacement;
			if (n>maxDataLength) n=maxDataLength;
			// use secondary transact request
			SMBtranssPacket *p=new SMBtranssPacket(tinfo->tid,
				tinfo->paramLength, tinfo->dataLength, // total count of each
				tinfo->paramLength, dataDisplacement, // displacements
				tinfo->param, 0, tinfo->data, n);
			if (send(p)==-1) {delete p; errno=ERRerror; return 0;}
			delete p;
			dataDisplacement+=n;
		}
	}
#if DEBUG >= 5
	cout<<"transact : waiting for server response...\n";
#endif
	// Everything has been sent, wait for server response
	SessionPacket *sesp = receive();
	if (sesp==0) {errno=ERRerror; return 0;}
	if (sesp->getType()!=SESSION_MESSAGE) {delete sesp; errno=EIO; return 0;}
	SMBtranssPacket *rep=new SMBtranssPacket;
	if (rep->parse(sesp)==-1) {errno=EIO; delete sesp; delete rep; return 0;}
	delete sesp;
#if DEBUG >= 6
	cout<<"transact : server response #1:\n"; rep->print(); cout<<"\n";
#endif
	SMBIO::TransactInfo *ret=new SMBIO::TransactInfo;
	uint16 l=rep->getSetupCount();
	if (l)
	{
		ret->setup=rep->getSetup();
		ret->setupLength=l;
	}
	else
	{
		ret->setup=0;
		ret->setupLength=0;
	}

	// Since we receive data chunk by chunk, make a list of each
	// chunk so that we don't have too many copy-and-resize
	// memory operations
    struct LocalList
    {
    	uint8 *data;
    	uint32 dc;
    	uint8* param;
    	uint32 pc;
    	LocalList *next;
    } *resultList=new LocalList;
    resultList->next=0;
    resultList->dc=0;
    resultList->pc=0;
    // Well, I won't use the 1st cell so as to simplify
    struct LocalList *saveList=resultList;

    errno=0; // so as to manage broken loops
	while ((rep->getParamCount()+rep->getParamDisplacement()<rep->getTotalParamCount())
		|| (rep->getDataCount()+rep->getDataDisplacement()<rep->getTotalDataCount()))
	{
		
		resultList->next=new LocalList;
		resultList=resultList->next;
		resultList->next=0;
		resultList->dc=rep->getDataCount();
		resultList->pc=rep->getParamCount();
		if ((resultList->pc==0) && (resultList->dc==0))
			{errno=ERRerror; resultList->data=0; resultList->param=0; break;}
		resultList->data=rep->getData();
		resultList->param=rep->getParam();
		// check for timeout in receive()
		sesp = receive();
		if (sesp==0) {errno=EIO; break;}
		if (sesp->getType()!=SESSION_MESSAGE) {delete sesp; errno=ERRerror; break;}
		delete rep; rep=new SMBtranssPacket;
		if (rep->parse(sesp)==-1) {errno=EIO; delete sesp; break;}
		delete sesp;
	}
	if (!errno)
	{
		// save data in last packet
		resultList->next=new LocalList;
		resultList=resultList->next;
		resultList->next=0;
		resultList->dc=rep->getDataCount();
		resultList->pc=rep->getParamCount();
		if ((resultList->pc==0) && (resultList->dc==0))
			{errno=ERRerror; resultList->data=0; resultList->param=0;}
		else {
			resultList->data=rep->getData();
			resultList->param=rep->getParam();
		}
	}
	delete rep;
	if (errno!=0)
	{
		// error : clean and exit
		while (saveList)
		{
			resultList=saveList->next;
			if (saveList->dc) delete saveList->data;
			if (saveList->pc) delete saveList->param;
			saveList=resultList;
		}
		errno=ERRerror;
		return ret;
	}
	// OK : concatenate list into a memory block
	// Note : this block should be < 65536 octets,
	// and that's what microsoft calls "big" buffers...
	resultList=saveList->next;
	ret->dataLength=0;
	ret->paramLength=0;
	// First sum length. I won't check here if this is the
	// total announced in the TotalCount fields. There is
	// something unclear in the doc, which seems to say that
	// those values can be modified and one should check
	// them in each packet as I do in the loop. This is
	// plain stupid, but doesn't surprise me !
	while (resultList)
	{
		ret->dataLength+=resultList->dc;
		ret->paramLength+=resultList->pc;
		resultList=resultList->next;
	}
	ret->data=new uint8[ret->dataLength];
	ret->param=new uint8[ret->paramLength];
	uint16 posd=0, posp=0;
	// copy into result structure and clean memory in the same time
	while (saveList)
	{
		resultList=saveList->next;
		if (saveList->dc)
		{
			memcpy(ret->data+posd,saveList->data,saveList->dc);
			posd+=saveList->dc;
			delete saveList->data;
		}
		if (saveList->pc)
		{
			memcpy(ret->param+posp,saveList->param,saveList->pc);
			posp+=saveList->pc;
			delete saveList->param;
		}
		delete saveList;
		saveList=resultList;
	}
	return ret;
}

SMBShareList* SMBIO::getShareList(const char *hostname, const char *user, const char *password)
{
#ifndef OVERKILL
	if (!hostName) {
		if (openSession(hostname)==-1) {errno=ENOENT; return 0;}
	} else if (strcasecmp(hostname,hostName)) {
		closeSession();
		if (openSession(hostname)==-1) {errno=ENOENT; return 0;}
	}
	if (login(user, password)==-1) {errno=ENOENT; return 0;}
#else
	closeSession();
	if (openSession(hostname)==-1) {errno=ENOENT; return 0;}
	if (login(user, password)==-1) {errno=ENOENT; return 0;}
//	login(user, password);
#endif
	if (openService("IPC$",0,SMB_IPC)==-1) {errno=ENOENT; return 0;}
	
	SMBIO::TransactInfo *info=new SMBIO::TransactInfo;
	info->tid=TID;
	info->transactName=new char[13];
	strcpy((char*)(info->transactName),"\\PIPE\\LANMAN");
	info->paramLength=19;
	info->param=new uint8[info->paramLength];
	info->param[0]=info->param[1]=0;
	strcpy((char*)(info->param+2),"WrLeh");  // "data descriptors", don't
	strcpy((char*)(info->param+8),"B13BWz"); // bother understand them
	info->param[15]=1; // 1 in little endian,
	info->param[16]=0; // as said in doc
	info->param[17]=0xE8; // 65000 in little endian
	info->param[18]=0xFD; // will do for our max buffer size
	// No data section
	SMBIO::TransactInfo *result=transact(info);
	delete info;
	if (!result) return 0;	// error code in errno
	if (result->paramLength<8) {delete result; errno=ERRerror; return 0;}
	errno=(uint16)(result->param[0])|((uint16)(result->param[1])<<8);
	if (errno) {delete result; return 0;} // Server error, acces denied, etc...
	uint16 converter=(uint16)(result->param[2])|((uint16)(result->param[3])<<8);
	uint16 numEntries=(uint16)(result->param[4])|((uint16)(result->param[5])<<8);
	if (numEntries*20>result->dataLength) {delete result; errno=ERRerror; return 0;}

	// Set a guard for string functions at the end of data
	// There should already be a 0 at the end, if the packet is correct...
	result->data[result->dataLength-1]=0;

	// reverse sort data, so they will end-up in correct order
	// when the list is built from tail to head
	// We already checked qsort won't access memory >= data+datalength
	qsort(result->data,numEntries,20,(int (*)(const void *, const void *))rstrcmp);
	
	// Ok, now get the data and build the return list
	SMBShareList *ret=0;
	uint8 *p=result->data, *pmax=result->data+result->dataLength;
	while ((numEntries) && (p<pmax-20))
	{
		// only the 16 low bits of this 32 bit "pointer" should be used
		uint16 commentOffset=(uint16)(*(p+16))|((uint16)(*(p+17))<<8);
		if (commentOffset) commentOffset-=converter;
		char* comment=(commentOffset)?(char*)(result->data+commentOffset):0;
		if (comment>=(char*)pmax) comment=0;
		ret=new SMBShareList((char*)p,	// share name
			(int)((uint16)(*(p+14))|((uint16)(*(p+15))<<8)), // type
			(char*)comment,ret);	// insert to head of list
		numEntries--; p+=20;
	}
	delete result;

	closeService(); // close IPC$
//	closeSession(); // not necessary, it will be checked later on
	return ret;
}


// ask a specific browser its vision of the network groups
// do not ask for or check the workgroup members (see getMemberList for that)
SMBWorkgroupList *SMBIO::askWorkgroupList(const char *browser, const char *user)
{
	if (!browser) {errno=ENOENT; return 0;}
#ifdef OVERKILL
	closeSession();
	if (openSession(browser)==-1) {errno=ENOENT; return 0;}
	if (login(user)==-1) {errno=EACCES; return 0;}
//	login(user);
#else
	if (!hostName) {
		if (openSession(browser)==-1) {errno=ENOENT; return 0;}
	} else if (strcasecmp(hostName,browser)) {
		closeSession();
		if (openSession(browser)==-1) {errno=ENOENT; return 0;}
	}
	if (login(user)==-1) {errno=EACCES; return 0;}
#endif
	if (openService("IPC$",0,SMB_IPC)==-1) {errno=ENOENT; return 0;}
	
	SMBIO::TransactInfo *trans=new SMBIO::TransactInfo;
	trans->tid=TID;
	trans->transactName=new char[13];
	strcpy((char*)(trans->transactName),"\\PIPE\\LANMAN");
	trans->paramLength=26;
	trans->param=new uint8[trans->paramLength];
	trans->param[0]=104; // NetServerEnum2 function number
	trans->param[1]=0;   // 16 bits little endian
	strcpy((char*)(trans->param+2),"WrLehDz");  // "data descriptors", don't
	strcpy((char*)(trans->param+10),"B16BBDz"); // bother understand them
	trans->param[18]=1; // 1 in little endian,
	trans->param[19]=0; // stands for detail level
	trans->param[20]=0xE8; // 65000 in little endian
	trans->param[21]=0xFD; // will do for our max buffer size
	trans->param[22]=0;    // what to list
	trans->param[23]=0;    // in little endian
	trans->param[24]=0;    // 0x80000000 stands for
	trans->param[25]=0x80; // domain enumeration
	// No data section
	SMBIO::TransactInfo *result=transact(trans);
	delete trans;
	if (!result) {errno=ENOENT; return 0;}
	if (result->paramLength<8) {delete result; errno=ENOENT; return 0;}
	errno=(uint16)(result->param[0])|((uint16)(result->param[1])<<8);
	if (errno) {delete result; errno=ENOENT; return 0;}
	uint16 converter=(uint16)(result->param[2])|((uint16)(result->param[3])<<8);
	uint16 numEntries=(uint16)(result->param[4])|((uint16)(result->param[5])<<8);
	if (((int16)numEntries)<=0) {delete result; errno=ENOENT; return 0;}
	if (numEntries*26>result->dataLength) {delete result; errno=ENOENT; return 0;}
         	
	// Set a guard for string functions at the end of data
	// There should already be a 0 at the end, if the packet is correct...
	result->data[result->dataLength-1]=0;
            	
	// reverse sort data, so they will end-up in correct order
	// when the list is built from tail to head
	// We already checked qsort won't access memory >= data+datalength
	qsort(result->data,numEntries,26,(int (*)(const void *, const void *))rstrcmp);
				
	// Ok, now get the data and build the workgroup list
	SMBWorkgroupList *theList=0;
	uint8 *p=result->data, *pmax=result->data+result->dataLength;
	while ((numEntries) && (p<pmax-26))
	{
		// only the 16 low bits of this 32 bit "pointer" should be used
		uint16 commentOffset=(uint16)(*(p+22))|((uint16)(*(p+23))<<8);
		if (commentOffset) commentOffset-=converter;
		char* comment=(commentOffset)?(char*)(result->data+commentOffset):0;
		if (comment>=(char*)pmax) comment=0;
		if ((comment) && (comment[0]==0)) comment=0;
		if ((p[0]!=0) && (comment)) {
			theList=new SMBWorkgroupList(
			(char*)p, //name
			new SMBMasterList((char*)comment,0), // master name
			0, // members
			theList);	// insert to head of list
		}
#ifdef OVERKILL // Argl ! some servers can't follow when we send packets too
		else { // fast ! Their response is scrambled or incomplete (buffer overflow on server ?)
			if (theList) delete theList;
			theList=0; // do not suppose the beginning was correct, if any
			break;
		}
#endif
#if DEBUG>=3
cout<<"askWorkgroupList : workgroup : "<<(char*)p<<", master : "<<(char*)comment<<"\n";
#endif
		numEntries--; p+=26;
	}
	delete result;
	closeService(); // close IPC$
	if (theList) {errno=0; return theList;}
	else {errno=ENOENT; return 0;}
}


// same think for group members
SMBMemberList *SMBIO::askMemberList(const char *master, const char *workgroup, const char *user)
{
	if (!master) {errno=ENOENT; return 0;}
#ifdef OVERKILL
	closeSession();
	if (openSession(master)==-1) {errno=ENOENT; return 0;}
	if (login(user)==-1) {errno=EACCES; return 0;}
//	login(user);
#else
	if (!hostName) {
		if (openSession(master)==-1) {errno=ENOENT; return 0;}
	} else if (strcasecmp(hostName,master)) {
		closeSession();
		if (openSession(master)==-1) {errno=ENOENT; return 0;}
	}
	if (login(user)==-1) {errno=EACCES; return 0;}
#endif
	if (openService("IPC$",0,SMB_IPC)==-1) {errno=ENOENT; return 0;}
			
	SMBIO::TransactInfo *trans=new SMBIO::TransactInfo;
	trans->tid=TID;
	trans->transactName=new char[13];
	strcpy((char*)(trans->transactName),"\\PIPE\\LANMAN");
	trans->paramLength=26+strlen(workgroup)+1;
	trans->param=new uint8[trans->paramLength];
	trans->param[0]=104; // NetServerEnum2 function number
	trans->param[1]=0;   // 16 bits little endian
	strcpy((char*)(trans->param+2),"WrLehDz");  // "data descriptors", don't
	strcpy((char*)(trans->param+10),"B16BBDz"); // bother understand them
	trans->param[18]=1; // 1 in little endian,
	trans->param[19]=0; // stands for detail level
	trans->param[20]=0xE8; // 65000 in little endian
	trans->param[21]=0xFD; // will do for our max buffer size
	trans->param[22]=0xFB; // what to list
	trans->param[23]=0xBF; // in little endian
	trans->param[24]=0x0F; // 0x000FBFFB stands for
	trans->param[25]=0;    // quite everything :-)
	strcpy((char*)(trans->param+26),workgroup);
	// No data section
	SMBIO::TransactInfo *result=transact(trans);
	delete trans;
	closeService(); // close IPC$
	if (!result) {errno=ENOENT; return 0;}
	if (result->paramLength<8) {delete result; errno=ENOENT; return 0;}
	errno=(uint16)(result->param[0])|((uint16)(result->param[1])<<8);
	if (errno) {delete result; errno=ENOENT; return 0;}
	uint16 converter=(uint16)(result->param[2])|((uint16)(result->param[3])<<8);
	uint16 numEntries=(uint16)(result->param[4])|((uint16)(result->param[5])<<8);
	if ((int16)(numEntries)<=0) {delete result; errno=ENOENT; return 0;}
	if (numEntries*26>result->dataLength) {delete result; errno=ENOENT; return 0;}
	// Set a guard for string functions at the end of data
	// There should already be a 0 at the end, if the packet is correct...
	result->data[result->dataLength-1]=0;
	// reverse sort data, so they will end-up in correct order
	// when the list is built from tail to head
	// We already checked qsort won't access memory >= data+datalength
	qsort(result->data,numEntries,26,(int (*)(const void *, const void *))rstrcmp);
	// Ok, now get the data and build the workgroup member list
	SMBMemberList *theList=0;
	uint8 *p=result->data, *pmax=result->data+result->dataLength;
	uint8 masterFound=0;
	while ((numEntries) && (p<pmax-26))
	{
		// only the 16 low bits of this 32 bit "pointer" should be used
		uint16 commentOffset=(uint16)(*(p+22))|((uint16)(*(p+23))<<8);
		if (commentOffset) commentOffset-=converter;
		char* comment=(commentOffset)?(char*)(result->data+commentOffset):0;
		if (comment>=(char*)pmax) comment=0;
		if (p[0]!=0) theList=new SMBMemberList(
			(char*)p,	// name
			(char*)comment,theList);	
#if DEBUG>=3
cout<<"askWorkgroupMemberList : member : "<<(char*)p;
if (comment) cout<<", comment : "<<(char*)comment;
cout<<"\n";
#endif
		if (!strcasecmp((char*)p,master)) masterFound=1;
		numEntries--; p+=26;
	}
	delete result;
	if (!theList) {errno=ENOENT; return 0;} // error !
#ifdef OVERKILL
	if (masterFound) {errno=0; return theList;}
	// else error, master not in his workgroup. Suppose list is invalid.
	delete theList; errno=ENOENT; return 0;
#else
	errno=0; return theList; // stange, but...
#endif
}

// retreive a correct (supposedly) workgroup list.
// Many checks are done, including timeouts, and we do not rely on only one
// browse server if possible. see doc for more info (or am I too lazy to write it ?)
// Any comment on my algorithm is welcome.
int SMBIO::getWorkgroupList(const char *user)
{
//	uint32 now=(uint32)time(0);  // time in seconds
	if (!workgroupList) {
		char *browser=0;
		if (!defaultBrowser) {
			browser=new char[strlen(ourName)+1];
			strcpy(browser,ourName);
		} else { // make a copy we can safely delete (don't delete defaultBrowser !)
			browser=new char[strlen(defaultBrowser)+1];
			strcpy(browser,defaultBrowser);
		}
		// add default browser list to the list
		workgroupList=askWorkgroupList(browser,user);
#ifdef OVERKILL
		// default browser not reliable => retry ?
		for (int i=0; ((!workgroupList) && i<3); i++) {
			workgroupList=askWorkgroupList(browser,user);
		}
#endif
		delete browser;
	}

	errno=0;
	return 0; // ok

}

// guess what it does ?
int SMBIO::getWorkgroupMembers(const char *workgroup, const char *user)
{
	SMBWorkgroupList *wkList=0;
	int flag=0;
	if (!workgroupList) getWorkgroupList(user);
	for (wkList=workgroupList; (wkList); wkList=wkList->next)
		if (!strcasecmp(workgroup,wkList->name)) break;  // match
	if (!wkList) flag=1;
	else {
		if (!(wkList->members)) {
			if (wkList->possibleMasters) flag=2;
			else flag=1;
		} else { // check timeout
//			if ((wkList->lastCheck+180)<(uint32)time(0)) flag=2;
		}
	}
    if (!flag) {
    	memberList=wkList->members;
    	return 0;
    }
	
	// flag==2 => ask possible masters for a list, take the best
	if (flag==2) {
		SMBMemberList *members=0;
		SMBMasterList *master=0;
		for (SMBMasterList *maList=wkList->possibleMasters; (maList); maList=maList->next) {
			SMBMemberList *mbList=askMemberList(maList->name,wkList->name,user);
			if (!mbList) continue;
			if ((!mbList->next) || (!mbList->next->next)) {
				// "master" only or this host + the true master
				// check if there is a better list with more members...
				if (!members) {
					members=mbList;
					master=maList;
				}
			} else {
				if (members) delete members;
				members=mbList;
				master=maList;
				break; // found a good list
			}
		}
		// now reorder master list so that the good one is found first next time
		if (master) { // so 'members' is not null as well
			SMBMasterList *maList=wkList->possibleMasters;
			while ((maList) && (maList->next!=master)) maList=maList->next;
			if (maList) { // else it's already at the beginning
				if (maList->next) { // should always be true
					maList->next=maList->next->next;
					master->next=wkList->possibleMasters;
					wkList->possibleMasters=master;
				}
			}
			memberList=members;
			if (wkList->members) delete wkList->members;
			wkList->members=members;
			return 0; // ok
		} else flag=1; // try to solve this problem
	} // endif flag==2
	
	if (flag==1) {
		// for each other workgroup prefered master
		for (SMBWorkgroupList *wk2=workgroupList; (wk2); wk2=wk2->next)
		if (wk2!=wkList) {
			// ask if know a master not in possible master list
			SMBWorkgroupList *tmpWkList=0, *tmpWkList2=0;
			if ((wk2->possibleMasters) && (wk2->possibleMasters->name)) {
				tmpWkList=askWorkgroupList(wk2->possibleMasters->name,user);
			}
			for (tmpWkList2=tmpWkList; (tmpWkList2); tmpWkList2=tmpWkList2->next) {
				if (!strcasecmp(tmpWkList2->name,workgroup)) break;  // match
			}
			if (tmpWkList2) { // match, is there a different master ?
				if (wkList) {
					SMBMasterList *ma=0;
					for (ma=wkList->possibleMasters; (ma); ma=ma->next) {
						if (!strcasecmp(ma->name,tmpWkList2->possibleMasters->name)) break; // match
					}
					if (!ma) { // no master match, it's a new one
						SMBMemberList *mbList=askMemberList(tmpWkList2->possibleMasters->name, workgroup, user);
						if (mbList) { // and it has a member list
							if (wkList->members) delete wkList->members;
							wkList->members=mbList;
							// add new master at the beginning
							wkList->possibleMasters=new SMBMasterList(
								tmpWkList2->possibleMasters->name,
								wkList->possibleMasters);
							// can return happily
							if (tmpWkList) delete tmpWkList;
							memberList=mbList;
							return 0; // ok
						}
					}
					// we found a bad master, continue without paying attention
				} else { // !wkList => not in initial workgroupList
					SMBMemberList *mbList=askMemberList(tmpWkList2->possibleMasters->name, workgroup, user);
					if (mbList) { // found master has a member list
						// add the new workgroup entry (at the beginning)
						workgroupList=new SMBWorkgroupList(workgroup,
							new SMBMasterList(tmpWkList2->possibleMasters->name,0),
							mbList,workgroupList);
						// can return happily
						if (tmpWkList) delete tmpWkList;
						memberList=mbList;
						return 0; // ok
					}
				}
			} // endif different master found
			if (tmpWkList) delete tmpWkList;
		} // end of "search for other master" loop
	} // end of flag==1;
	errno=ENOENT;
	return -1;
}


int SMBIO::parse(const char *n, char* &workgroup, char* &host, char* &share,
	char* &dir, char* &user)
{
	// Port this to the old crappy convention "caller manages pointers"
	// Here, no password or IP
    char *name = 0;
    newstrcpy( name, n );
    if( char_cnv ) char_cnv->unix2win( name );

	util.parse(name);
	workgroup=0;
	host=0;
	share=0;
	dir=0;
	user=0;
	newstrcpy(workgroup, util.workgroup());
	newstrcpy(host, util.host());
	newstrcpy(share, util.share());
	newstrcpy(dir, util.path());
	newstrcpy(user, util.user());

	if (name) delete name;
	return 0;
}


// Does the opposite of parse, build and return an smbURL from parameters
char *SMBIO::buildURL(const char* workgroup, const char* host, const char* share, const char* file, const char* user)
{
	// Port this to the old crappy convention "caller manages pointers"
	// Here, no password or IP
	char *tmp = util.buildURL(user, 0, workgroup, host, share, file, 0);
	char *ret=new char[strlen(tmp)+1];
	strcpy(ret,tmp);
	return ret;
}


// returns an dir descriptor, or -1 on error
int SMBIO::opendir(const char *name)
{
	char *workgroup=0; // those values have to be parsed
	char *host=0;
	char *share=0;
	char *dir=0;
	char *user=0;

	parse(name, workgroup, host, share, dir, user);

// now that we have values in vars, check consistency
	int ret=-1;	// error by default
	int doItAnyway=0;
	// first fill in the blanks
	if (((!share) || (share[0]==0)) && (dir)) {
		if (!currentShare) {
			errno=ENOENT;	// MUST open a share before specifying a directory
			goto CLEANUP;
		}
		share=new char[strlen(currentShare)+1];
		strcpy(share,currentShare);
	}
	if (((!host) || (host[0]==0)) && (share)) {
		if ((!hostName) || (host[0]==0)) {	// fine, but where is the share then ?
			errno=ENOENT;
			goto CLEANUP;
		}
		host=new char[strlen(hostName)+1];
		strcpy(host,hostName);
	}
	if ((!host) || (host[0]==0)) {	// the 3 vars are 0 !!!
		ret=getNewFd(fdInfo, -1, "browse", workgroup, 0, 0, 0, user);
		goto CLEANUP;
	}
	if ((!hostName) || (!host) || (strcasecmp(host,hostName))) { // request on a new host
		closeSession();	// if there was one...
		if (openSession(host)==-1) {	// not nice...
			errno=ENOENT;
			goto CLEANUP;
		}
		doItAnyway=1; // for share/dir with same name on different hosts !
	}
	if ((!share) || (share[0]==0)) {	// OK, it was just a connection request
		ret=getNewFd(fdInfo, -1, "sharelist", workgroup, hostName, 0, 0, user);
		goto CLEANUP;
	}
	if ((doItAnyway) || (!currentShare) || (strcasecmp(share,currentShare))) {
		closeService();
		if (openService(share)==-1) {
			errno=ENOENT;
			goto CLEANUP;
		}
		doItAnyway=1; // for directory with same name on different shares
	}
	if (!dir) {	// OK, root directory from share
		ret=getNewFd(fdInfo, -1, "dir", workgroup, hostName, currentShare, "", user);
		goto CLEANUP;
	}
	ret=getNewFd(fdInfo, -1, "dir", workgroup, hostName, currentShare, dir, user);
//	goto CLEANUP;

CLEANUP:
	if (workgroup) delete workgroup;
	if (host) delete host;
	if (share) delete share;
	if (dir) delete dir;
	if (user) delete user;
	return ret;
}

SMBdirent *SMBIO::readdir(int dirdesc)
{
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, dirdesc);
	if (!info) {errno=EBADF; return 0;}
	if (info->fid!=-1) {errno=EBADF; return 0;}	// file descriptor
#if DEBUG>=2
cout<<"readdir\n";
#if DEBUG >= 5
cout<<"  host : "; if (info->host) cout<<info->host; else cout<<"0"; cout<<"\n";
cout<<"  share : "; if (info->share) cout<<info->share; else cout<<"0"; cout<<"\n";
cout<<"  dir : "; if (info->dir) cout<<info->dir; else cout<<"0"; cout<<"\n";
#endif
#endif
	if ((!(info->host)) || (!strcmp(info->name,"browse"))) {
//		cout<<"Un canard et un canard ça fait 2 canards !\n";
		if ((!(info->cache)) || (info->cinvalid)) { // then fill it up
			if ((!(info->workgroup)) || (!workgroupList)) {
				// if no workgroup build it anyway, since it can change frequently
				if (getWorkgroupList(info->user)==-1) {errno=EBADF; info->cinvalid=1; return 0;}
    		} // endif "build workgroupList"

			if (info->workgroup) {
				if (getWorkgroupMembers(info->workgroup, info->user)==-1)
					{errno=EBADF; info->cinvalid=1; return 0;}
				// paranoid check...
				if (!memberList) {errno=EBADF; info->cinvalid=1; return 0;}
				int total=0; // calculate cache length with a tmp list
				SMBMemberList* theList=memberList;
				while (theList) {
					if (theList->name) {
						total+=strlen(theList->name);
						if (theList->comment) total+=strlen(theList->comment);
						total+=2; // 0 => "" in any case
					}
					theList=theList->next;
				}
				if (info->cache) delete info->cache; // destroy invalid cache
				info->clen=total; // put everything in cache, won't be too large
				info->cache=new uint8[total];
				info->cpos=info->cache;
				for (theList=memberList; (theList); theList=theList->next) {
					if (theList->name) {
						strcpy((char*)(info->cpos),theList->name);
						info->cpos+=strlen(theList->name)+1;
						if (theList->comment) {
							strcpy((char*)(info->cpos),theList->comment);
							info->cpos+=strlen(theList->comment)+1;
						} else *(info->cpos++)=0; // store empty string
					}
				}
				info->cpos=info->cache;
				info->cinvalid=0;
			} else { // We just build the cache with the list
				// paranoid check...
				if (!workgroupList) {errno=EBADF; info->cinvalid=1; return 0;}
				int total=0; // calculate cache length with a tmp list
				SMBWorkgroupList* theList=workgroupList;
				while (theList) {
					if (theList->name) {
						total+=strlen(theList->name);
						if (theList->possibleMasters->name) total+=strlen(theList->possibleMasters->name);
						total+=2; // 0 => "" in any case
					}
					theList=theList->next;
				}
				if (info->cache) delete info->cache; // destroy invalid cache
				info->clen=total; // put everything in cache, won't be too large
				info->cache=new uint8[total];
				info->cpos=info->cache;
				for (theList=workgroupList; (theList); theList=theList->next) {
					if (theList->name) {
						strcpy((char*)(info->cpos),theList->name);
						info->cpos+=strlen(theList->name)+1;
						if (theList->possibleMasters->name) {
							strcpy((char*)(info->cpos),theList->possibleMasters->name);
							info->cpos+=strlen(theList->possibleMasters->name)+1;
						} else *(info->cpos++)=0; // store empty string
					}
				}
				info->cpos=info->cache;
				info->cinvalid=0;
			} // end "put workgroup list in cache" part
		} // endif no cache
		if (info->cpos-info->cache>=info->clen) {
#if DEBUG >= 4
	cout<<"readdir : browsing : no more entries !\n";
#endif
			// do not invalidate cache, otherwise it will start over !
			errno=0; // not really an error (see man 3 readdir)
			return 0; // eof reached
		}
		if (theDirEnt.d_name) delete (theDirEnt.d_name);
		theDirEnt.d_name=new char[strlen((char*)(info->cpos))+1];
		strcpy(theDirEnt.d_name,(char*)(info->cpos)); // copy name
		if (char_cnv) char_cnv->win2unix(theDirEnt.d_name);  // convert it
		while (*(++info->cpos)) ; // skip name
		while (*(++info->cpos)) ; // skip comment if any
		info->cpos=info->cpos+1; // next name if any
		theDirEnt.st_mode=040755; // directory
		theDirEnt.st_uid=info->st_uid; // don't call getuid() again
		theDirEnt.st_gid=info->st_gid; // don't call getgid() again
		theDirEnt.st_size=0; // Not important for a directory anyway
		theDirEnt.st_atime=theDirEnt.st_mtime=theDirEnt.st_ctime=0;
		errno=0;                        // no time info available for servers
		return &theDirEnt;
	}
	if ((!(info->share)) || (!strcmp(info->name,"sharelist"))) {
		if ((!(info->cache)) || (info->cinvalid)) { // then fill it up
			SMBShareList* shareList=getShareList(info->host,info->user);
			SMBShareList* savList=shareList;
			// don't use internal "theShareList" directly, so as to detect errors
			if (!shareList) {errno=EBADF; info->cinvalid=1; return 0;}
			// 2 pass algo for storing names. Does anyone know a better solution ?
			// in the case of a share list, store data in the cache. Only DISK
			// entries are processed, as others are too weird to be included
			// in this "virtual filesystem". OK, I could maybe do something for
			// print queues, but not at present...
			// save comments as well (for extended stat function ?)
			// do not save bad entries. (no name ?)
			int total=0;
			while (shareList) {
				if ((shareList->type==SMB_DISK) && (shareList->name)) {
					total+=strlen(shareList->name);
					if (shareList->comment) total+=strlen(shareList->comment);
					total+=2; // 0 => "" in any case
				}
				shareList=shareList->next;
			}
			if (info->cache) delete info->cache; // destroy invalid cache
			info->clen=total; // put everything in cache, won't be too large
			info->cache=new uint8[total];
			info->cpos=info->cache;
			shareList=savList;
			while (shareList) {
				if ((shareList->type==SMB_DISK) && (shareList->name)) {
					strcpy((char*)(info->cpos),shareList->name);
					info->cpos+=strlen(shareList->name)+1;
					if (shareList->comment) {
						strcpy((char*)(info->cpos),shareList->comment);
						info->cpos+=strlen(shareList->comment)+1;
					} else *(info->cpos++)=0; // store empty string
				}
				shareList=shareList->next;
			}
			info->cpos=info->cache;
			delete savList;
			info->cinvalid=0;
		} // endif no cache => now we're sure there is a cache
		if (info->cpos-info->cache>=info->clen) {
#if DEBUG >= 4
	cout<<"readdir : sharelist : no more entries !\n";
#endif
			// do not invalidate cache, otherwise it will start over !
			errno=0; // not really an error (see man 3 readdir)
			return 0; // eof reached
		}
		if (theDirEnt.d_name) delete (theDirEnt.d_name);
		theDirEnt.d_name=new char[strlen((char*)(info->cpos))+1];
		strcpy(theDirEnt.d_name,(char*)(info->cpos)); // copy name
		if (char_cnv) char_cnv->win2unix(theDirEnt.d_name);  // convert it
		while (*(++info->cpos)) ; // skip name
		while (*(++info->cpos)) ; // skip comment if any
		info->cpos=info->cpos+1; // next name if any
		theDirEnt.st_mode=040755; // directory
		theDirEnt.st_uid=info->st_uid; // don't call getuid() again
		theDirEnt.st_gid=info->st_gid; // don't call getgid() again
		theDirEnt.st_size=0; // Not important for a directory anyway
		theDirEnt.st_atime=theDirEnt.st_mtime=theDirEnt.st_ctime=0;
		errno=0;                          // no time info available for shares
		return &theDirEnt;
	} // endif sharelist

	if ((!strcmp(info->name,"dir"))) {	// should be a dir now
		if (!(info->dir)) {
			info->dir=new char[1];
			info->dir[0]=0; // empty string
		}
		// MUST keep connection with host, otherwise TID invalid
		if ((!hostName) || (strcasecmp(info->host,hostName))) {
			closeSession();	// if there was one...
			if (openSession(info->host)==-1) {	// ? host down since opendir !
				errno=EBADF; return 0;
			}
			if (login(info->user)==-1) {errno=EBADF; return 0;}
			info->cinvalid=1; if (info->cache) delete info->cache;
			info->cache=0;
			info->handleExist=0; // start over
		}
		if (strcasecmp(info->share,currentShare)) {
			closeService();
			if (openService(info->share)==-1) {errno=EBADF; return 0;}
			info->cinvalid=1; if (info->cache) delete info->cache;
			info->cache=0;
			info->handleExist=0; // start over
		}
		if ((!(info->cache)) || (info->cinvalid)) { // then fill it up
			SMBIO::TransactInfo *trans=new SMBIO::TransactInfo;
			SMBIO::TransactInfo *result=0;
			uint8 endReached=0; // boolean
			if ((info->handleExist) && (info->cache)) { // find next
#ifdef OVERKILL
				int flag=0;  // sometimes, server not ready
				while (flag<5) { // we just have to retry...
				delete trans;
				trans=new SMBIO::TransactInfo;
#endif
				trans->tid=TID;
				trans->setupLength=1;
				trans->setup=new uint16[1];
				trans->setup[0]=2; // 2 stands for trans2_find_next2 function
				trans->transactName=0;	// transact 2
				trans->paramLength=13;
				trans->param=new uint8[trans->paramLength];
				trans->param[0]=(uint8)(info->handle&0xFF); // little endian
				trans->param[1]=(uint8)((info->handle&0xFF00)>>8);
				trans->param[2]=16; // max entries to return. Not too much, watch
				trans->param[3]=0; // the cache size... (little endian)
				trans->param[4]=1; // "standard" info level
				trans->param[5]=0;	// was on 16 bits little endian
				trans->param[6]=info->cache[0]; // should be
				trans->param[7]=info->cache[1]; // the resume key
				trans->param[8]=info->cache[2]; // on 32 bits
				trans->param[9]=info->cache[3]; // little endian
				trans->param[10]=2|4|8; // close if end, resume keys, continue from last ending place
				trans->param[11]=0;	// was on 16 bits little endian
				trans->param[12]=0;	// empty filename (no need with those flags)
				// No data section
				result=transact(trans);
				delete trans;
#ifndef OVERKILL
				if (!result) return 0;	// error code in errno
				if (result->paramLength<8) {delete result; errno=EBADF; return 0;}
#else
				if (!result) {flag++; trans=new SMBIO::TransactInfo;}
				else if (result->paramLength<8) {delete result; flag++; trans=new SMBIO::TransactInfo;}
				else flag=100;
				if ((flag>3) & (flag<100)) sleep(1); // wait a little, just in case...
				} // end of loop
#endif
				if ((result->param[2]) | (result->param[3])) endReached=1;
			} else { // first demand
				trans->tid=TID;
				trans->setupLength=1;
				trans->setup=new uint16[1];
				trans->setup[0]=1;	// 1 stands for trans2_find_first2 function
				trans->transactName=0;	// transact 2
				uint16 tmplen=strlen(info->dir);
				char *pattern=new char[tmplen+3]; // search pattern
				strcpy(pattern,info->dir);
				strcpy(pattern+tmplen,"\\*"); // append \* to dir
				
//				if( char_cnv ) char_cnv->unix2win( pattern );

				trans->paramLength=12+tmplen+3;
				trans->param=new uint8[trans->paramLength];
				trans->param[0]=0x02|0x04|0x10; // include hidden, system, directory
				trans->param[1]=0;	// was on 16 bits little endian
				trans->param[2]=1; // max entries to return. Not too much, watch
				trans->param[3]=0; // the cache size... (little endian)
				trans->param[4]=2|4|8; // close if end, resume keys, continue from last ending place
				trans->param[5]=0;	// was on 16 bits little endian
				trans->param[6]=1; // "standard" info level
				trans->param[7]=0;	// was on 16 bits little endian
				trans->param[8]=trans->param[9]=0; // search storage type : not
				trans->param[10]=trans->param[11]=0; // documented
				strcpy((char*)(trans->param+12),pattern);
				delete pattern;
				// No data section
				result=transact(trans);
				delete trans;
				if (!result) return 0;	// error code in errno
				if (result->paramLength<10) {delete result; errno=EBADF; return 0;}
				info->handle=(uint16)(result->param[0])|((uint16)(result->param[1])<<8);
				info->handleExist=1;
				if ((result->param[4]) | (result->param[5])) endReached=1;
			}
			
			if (info->cache) delete info->cache; // destroy invalid cache
			info->clen=4; // allocate space for search key
			for (int i=0; i<result->dataLength; ) {
				uint8 l=result->data[i+26];
#if DEBUG >= 6
				cout<<"readdir : len="<<(int)l<<" : ";
				for (int j=0; j<l; j++)
					if (result->data[i+27+j]>31) printf("%c",result->data[i+27+j]);
				cout<<"\n";
#endif
				info->clen+=l+1+4+4+4+4+4; // space for name+attributes
				i+=28+l;
			}
			info->cache=new uint8[info->clen]; // new cache shouldn't be too
			info->cpos=info->cache+4;          // large
			
			for (int i=0; i<result->dataLength; ) {
				info->cache[0]=result->data[i+0]; // search key : will be
				info->cache[1]=result->data[i+1]; // erased except for last
				info->cache[2]=result->data[i+2]; // entry => booo, I'm too lazy
				info->cache[3]=result->data[i+3]; // to calculate last offset
				// attributes : data[24 & 25] in little endian => use 24
				uint32 attr=0644; // default
				// archive attribute mapped to 0644
				if (result->data[i+24]&0x20) attr|=0644;
				// read-only attribute removes write permission
				if (result->data[i+24]&0x01) attr&=077666;
				// directory attribute mapped to 040755
				if (result->data[i+24]&0x10) attr|=040755;
				// Anyone has any idea about what volume, hidden and
				// system should be mapped to ???
				attr=LITEND32(attr);		// avoid unaligned access
				info->cpos[0]=attr&0xFF;	// on machines where it matters
				info->cpos[1]=(attr&0xFF00)>>8;   // ex : alpha
				info->cpos[2]=(attr&0xFF0000)>>16;
				info->cpos[3]=(attr&0xFF000000)>>24;
				uint32 size=(uint32)(result->data[i+16])     // little endian
				           |((uint32)(result->data[i+17])<<8)
				           |((uint32)(result->data[i+18])<<16)
				           |((uint32)(result->data[i+19])<<24);
				size=LITEND32(size);		// avoid unaligned access
				info->cpos[4]=size&0xFF;	// on machines where it matters
				info->cpos[5]=(size&0xFF00)>>8;   // ex : alpha
				info->cpos[6]=(size&0xFF0000)>>16;
				info->cpos[7]=(size&0xFF000000)>>24;
				struct tm footm;
				footm.tm_sec=(result->data[i+10]&0x1F)<<1; // 2 sec increments
				footm.tm_min=((result->data[i+10]&0xE0)>>5)
				            |((result->data[i+11]&0x7)<<3); // minutes
				footm.tm_hour=(result->data[i+11]&0xF8)>>3; // hours
				footm.tm_mday=result->data[i+8]&0x1F; // day of month
				footm.tm_mon=(((result->data[i+8]&0xE0)>>5)    // month 1..12
				             |((result->data[i+9]&0x1)<<3))-1; // => 0..11
				// And make us ready for year 2100 bug...
				footm.tm_year=((result->data[i+9]&0xFE)>>1)+80; // weird 0..119 interval from 1980
				footm.tm_isdst=-1; // daylight info not available
				uint32 theTime=mktime(&footm); // computes wday and yday and converts to time_t type
				theTime=LITEND32(theTime);
				info->cpos[8]=theTime&0xFF;
				info->cpos[9]=(theTime&0xFF00)>>8;
				info->cpos[10]=(theTime&0xFF0000)>>16;
				info->cpos[11]=(theTime&0xFF000000)>>24;
				footm.tm_sec=(result->data[i+6]&0x1F)<<1; // 2 sec increments
				footm.tm_min=((result->data[i+6]&0xE0)>>5)
				            |((result->data[i+7]&0x7)<<3); // minutes
				footm.tm_hour=(result->data[i+7]&0xF8)>>3; // hours
				footm.tm_mday=result->data[i+4]&0x1F; // day of month
				footm.tm_mon=(((result->data[i+4]&0xE0)>>5)    // month 1..12
				             |((result->data[i+5]&0x1)<<3))-1; // => 0..11
				// And make us ready for year 2100 bug...
				footm.tm_year=((result->data[i+5]&0xFE)>>1)+80; // weird 0..119 interval from 1980
				footm.tm_isdst=-1; // daylight info not available
				theTime=mktime(&footm); // computes wday and yday and converts to time_t type
				theTime=LITEND32(theTime);
				info->cpos[12]=theTime&0xFF;
				info->cpos[13]=(theTime&0xFF00)>>8;
				info->cpos[14]=(theTime&0xFF0000)>>16;
				info->cpos[15]=(theTime&0xFF000000)>>24;
				footm.tm_sec=(result->data[i+14]&0x1F)<<1; // 2 sec increments
				footm.tm_min=((result->data[i+14]&0xE0)>>5)
				            |((result->data[i+15]&0x7)<<3); // minutes
				footm.tm_hour=(result->data[i+15]&0xF8)>>3; // hours
				footm.tm_mday=result->data[i+12]&0x1F; // day of month
				footm.tm_mon=(((result->data[i+12]&0xE0)>>5)    // month 1..12
				             |((result->data[i+13]&0x1)<<3))-1; // => 0..11
				// And make us ready for year 2100 bug...
				footm.tm_year=((result->data[i+13]&0xFE)>>1)+80; // weird 0..119 interval from 1980
				footm.tm_isdst=-1; // daylight info not available
				theTime=mktime(&footm); // computes wday and yday and converts to time_t type
				theTime=LITEND32(theTime);
				info->cpos[16]=theTime&0xFF;
				info->cpos[17]=(theTime&0xFF00)>>8;
				info->cpos[18]=(theTime&0xFF0000)>>16;
				info->cpos[19]=(theTime&0xFF000000)>>24;
				uint8 l=result->data[i+26];
				memcpy(info->cpos+20,result->data+i+27,l); // copy name
				*(info->cpos+20+l)=0;
				info->cpos+=20+l+1;
				i+=28+l;
			}
			info->cpos=info->cache+4; // reset cache
			info->cinvalid=0;
			delete result;
			if (endReached) info->handleExist=0; // handle no longer valid
#if DEBUG >= 4
			if (endReached) cout<<"readdir : cache contains last data\n";
#endif
		} // endif no cache => now we're sure there is a cache
		if (info->cpos-info->cache>=info->clen) {
#if DEBUG >= 4
			cout<<"readdir : dir : no more entries !\n";
#endif
			// do not invalidate cache, otherwise it will start over !
			errno=0; // not really an error (see man 3 readdir)
			return 0; // eof reached
		}
		if (theDirEnt.d_name) delete (theDirEnt.d_name);
		int nameLen=strlen((char*)(info->cpos+20));
		theDirEnt.d_name=new char[nameLen+1];
		strcpy(theDirEnt.d_name,(char*)(info->cpos+20)); // copy name
		if (char_cnv) char_cnv->win2unix(theDirEnt.d_name);  // convert it
		memcpy(&theDirEnt.st_mode,info->cpos,4);
		theDirEnt.st_uid=info->st_uid; // don't call getuid() again
		theDirEnt.st_gid=info->st_gid; // don't call getgid() again
		memcpy(&theDirEnt.st_size,info->cpos+4,4);
		memcpy(&theDirEnt.st_atime,info->cpos+8,4);
		memcpy(&theDirEnt.st_mtime,info->cpos+12,4);
		memcpy(&theDirEnt.st_ctime,info->cpos+16,4);
		info->cpos=info->cpos+20+nameLen+1; // next name if any
		if ((info->cpos-info->cache>=info->clen) && (info->handleExist)) {
			info->cinvalid=1; // there is a handle => end not reached
#if DEBUG >= 4
			cout<<"readdir : dir : end of cache, more entries next time\n";
#endif
		}
		errno=0;
		return &theDirEnt;
	} // endif dir
#if DEBUG >= 1
	cout<<"readdir : not a directory !\n";
#endif
	errno=-1;
	return 0;
}

// must introduce caching, late writing, implement flush


int SMBIO::closedir(int dirdesc)
{
#if DEBUG >= 2
	if( getFdCellFromFd((FdCell*)fdInfo, dirdesc) )
	  cout<<"directory "<<getFdCellFromFd((FdCell*)fdInfo, dirdesc)->name<<" closed.\n";
	else
	  cout<<"SMBIO::closedir( " << dirdesc << " ): getFdCellFromFd((FdCell*)fdInfo, dirdesc) == NULL" << endl;
#endif
	return closeFd(fdInfo, dirdesc);
}

int SMBIO::rewinddir(int dirdesc)
{
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, dirdesc);
	if (!info) {errno=EBADF; return -1;}
	if (info->fid!=-1) {errno=EBADF; return -1;}	// file descriptor

	// if there is no cache, no problem
	if (!(info->cache)) {
		info->cpos=0;
		info->handleExist=0; errno=0; return 0;
	}
	// if there is an invalid one, erase it
	if (info->cinvalid) {
		delete info->cache; // already checked above
		info->cpos=info->cache=0; // should be really useful in this case
		info->handleExist=0; errno=0; return 0;
	}
	
	// in these cases cache contains everything => reset cpos
	if ((!(info->host)) || (!strcmp(info->name,"browse"))
	 || (!(info->share)) || (!strcmp(info->name,"sharelist"))) {
		info->cpos=info->cache; errno=0; return 0; // reset cpos
	}

	// cache might not correspond to the first demand, check it
	if ((!strcmp(info->name,"dir"))) {	// should be a dir now
		if (info->handleExist) {	// not the first demand
			delete info->cache;
			info->handleExist=0;    // we will ask again
			info->cpos=info->cache=0; errno=0; return 0; // reset cache
		}
		info->cpos=info->cache+4; errno=0; return 0; // reset cpos
	}
	errno=ENOENT;
	return -1;
}
	
// Default user is used instead of anonymous access
void SMBIO::setDefaultUser(const char *userName)
{
	if (defaultUser) delete defaultUser; // change it
	if ((!userName) || (!strcmp(userName,""))) {defaultUser=0; return;}
	defaultUser=new char[strlen(userName)+1];
	strcpy(defaultUser,userName);
}

// Make browsing work without samba installed locally
void SMBIO::setDefaultBrowseServer(const char *serverName)
{
	if (defaultBrowser) delete defaultBrowser; // change it
	if ((!serverName) || (!strcmp(serverName,""))) {defaultBrowser=0;}
	else {
		defaultBrowser=new char[strlen(serverName)+1];
		strcpy(defaultBrowser,serverName);
	}
	if (workgroupList) delete workgroupList;
	workgroupList=0;
}

// Added here for Option <=> NMBIO link
// Set the network broadcast address
void SMBIO::setBroadcastAddress(const char *addr)
{
	NMBIO::setNetworkBroadcastAddress(addr);
}

// Set the WINS address
void SMBIO::setWINSAddress(const char *addr)
{
	NMBIO::setNBNSAddress(addr);
}

// getName returns the name of the file for this fd, stripping URL
char *SMBIO::getName(int fd)
{
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	if ((!info) || (!(info->name))) {
		errno=EBADF;
		return 0;
	}
	char *ret=new char[strlen(info->name)+1];
	strcpy(ret,info->name);
	return ret;
}

int SMBIO::fstat(int fd, struct stat *buf)
{
	if (!buf) {
		errno=ENOENT; // well, not exactly...
		return -1;		
	}
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, fd);
	if(!info)    // This would produce a segfault
	  return -1; // Most probably because of file not found.
 	buf->st_mode=info->st_mode;
	buf->st_uid=info->st_uid;
	buf->st_gid=info->st_gid;
	buf->st_size=info->st_size;
	buf->st_atime=info->st_atime;
	buf->st_mtime=info->st_mtime;
	buf->st_ctime=info->st_ctime;
	errno=0;
	return 0;
}

// getName returns the URL associated to this descriptor
char *SMBIO::getURL(int desc)
{
	FdCell *info = getFdCellFromFd((FdCell*)fdInfo, desc);
	if (!info) {
		errno=EBADF;
		return 0;
	}
	return buildURL(info->workgroup,info->host,info->share,info->dir,info->user);
}

// opens subdir "name" of dir dd and returns a directory descriptor
// interpret "." and ".." subdirectories
int SMBIO::cd(int dd, const char *subdir)
{
	char *url=getURL(dd);
	if (!url) {
		errno=EBADF;
		return -1;
	}
	char *url2=append(url,subdir);
	delete url;
	if (!url2) {
		errno=EBADF;
		return -1;
	}
	int ret=opendir(url2);
	delete url2;
	return ret;
}


// opens subdir "name" of dir dd and returns a directory descriptor
// interpret "." and ".." subdirectories
char *SMBIO::append(const char *URL, const char *string)
{
	// Port this to the old crappy convention "caller manages pointers"
	char *tmp = util.append(URL, string, true);
	char *ret=new char[strlen(tmp)+1];
	strcpy(ret,tmp);
	return ret;
}


void SMBIO::setCharSet(const char *char_set)
{
  if (char_cnv) delete char_cnv;
  if (char_set) char_cnv = new CharCnv(char_set);
  else char_cnv=0;
}

// return the last error encountered when a function returns -1	
int SMBIO::error()
{
	return errno;
}

#endif

