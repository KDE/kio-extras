/*
    This file is part of the smb++ library
    Copyright (C) 1999  Nicolas Brodu
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

#include "defines.h"
#ifndef USE_SAMBA

#include "IODescriptors.h"
#include <string.h>
#include <unistd.h> // getuid(), getgid()

FdCell::FdCell(int _fd, int _fid, const char* _name, const char *_workgroup, const char* _host, const char* _share, const char* _dir, const char* _user, uint32 size)
{
	fd=_fd; fid=_fid;
	cache=0; cpos=0; clen=0; cinvalid=1; // do not allocate memory now
	if (_name)
	{
		name=new char[strlen(_name)+1];
		strcpy(name,_name);
	}
	else name=0;
	if (_workgroup)
	{
		workgroup=new char[strlen(_workgroup)+1];
		strcpy(workgroup,_workgroup);
	}
	else workgroup=0;
	if (_host)
	{
		host=new char[strlen(_host)+1];
		strcpy(host,_host);
	}
	else host=0;
	if (_share)
	{
		share=new char[strlen(_share)+1];
		strcpy(share,_share);
	}
	else share=0;
	if (_dir)
	{
		dir=new char[strlen(_dir)+1];
		strcpy(dir,_dir);
	}
	else dir=0;
	if (_user)
	{
		user=new char[strlen(_user)+1];
		strcpy(user,_user);
	}
	else user=0;
	pos=0; handle=0; handleExist=0;
	next=0; // most important !!!
 	st_mode=600; // default prot will indicate internally "not already checked"
	st_uid=getuid(); // ensure user exists
	st_gid=getgid(); // ensure group exists
	st_size=size;
	st_atime=st_mtime=st_ctime=0;
	openMode=0;
	cmaxRead=cmaxWrite=cachePositionInFile=0;
}

FdCell::~FdCell()
{
	if (name) delete name;
	if (workgroup) delete workgroup;
	if (host) delete host;
	if (share) delete share;
	if (dir) delete dir;
	if (user) delete user;
	if (cache) delete cache;
}

// deep copy
int FdCell::copy(FdCell *fdc)
{
	// never copy file descriptors so as to preserve integrity
	fid=fdc->fid;
 	st_mode=fdc->st_mode;
	st_uid=fdc->st_uid;
	st_gid=fdc->st_gid;
	st_size=fdc->st_size;
	st_atime=fdc->st_atime;
	st_mtime=fdc->st_mtime;
	st_ctime=fdc->st_ctime;
	handle=fdc->handle;
	handleExist=fdc->handleExist;
	pos=fdc->pos; // current position in file for I/O
	clen=fdc->clen;    // cache length
	cinvalid=fdc->cinvalid; // cache invalid boolean
	if (cache) delete cache;
	if ((fdc->cache) && (fdc->clen>0)) {
		cache=new uint8[fdc->clen];
		memcpy(cache,fdc->cache,fdc->clen);
	} else cache=0;
	if (cache) cpos=cache+(fdc->cpos-fdc->cache);
	if (name) delete name;
	if (fdc->name) {
		name=new char[strlen(fdc->name)+1];
		strcpy(name,fdc->name);
	} else name=0;
	if (workgroup) delete workgroup;
	if (fdc->workgroup) {
		workgroup=new char[strlen(fdc->workgroup)+1];
		strcpy(workgroup,fdc->workgroup);
	} else workgroup=0;
	if (host) delete host;
	if (fdc->host) {
		host=new char[strlen(fdc->host)+1];
		strcpy(host,fdc->host);
	} else host=0;
	if (share) delete share;
	if (fdc->share) {
		share=new char[strlen(fdc->share)+1];
		strcpy(share,fdc->share);
	} else share=0;
	if (dir) delete dir;
	if (fdc->dir) {
		dir=new char[strlen(fdc->dir)+1];
		strcpy(dir,fdc->dir);
	} else dir=0;
	if (user) delete user;
	if (fdc->user) {
		user=new char[strlen(fdc->user)+1];
		strcpy(user,fdc->user);
	} else user=0;
	return 0;
}


void destroyFdList(FdCell *l)
{
	if (l)
	{
		destroyFdList(l->next);
		delete l;
	}
}

int getNewFd(void* &fdInfo, int fid, const char* name, const char* workgroup,
	 const char* host, const char* share, const char* dir, const char* user, uint32 size)
{
	if (!fdInfo)
	{
		fdInfo=new FdCell(1, fid, name, workgroup, host, share, dir, user, size);
		return 1;
	}
	// list passed by address, we need to save
	FdCell *sav=(FdCell*)fdInfo;
	while (sav->next) sav=sav->next;
	FdCell *tmp=new FdCell(sav->fd+1, fid, name, workgroup, host, share, dir, user, size);
	sav->next=tmp;
	return tmp->fd;
}

int closeFd(void* &fdInfo, int fd)
{
	if (!fdInfo) return -1;
	// list passed by address, we need to save
	FdCell *sav=(FdCell*)fdInfo;
	if (sav->fd==fd)
	{
		fdInfo=sav->next;
		delete sav;
		return 0;
	}
	while ((sav->next) && (sav->next->fd!=fd)) sav=sav->next;
	if (sav->next)
	{
		FdCell *tmp=sav->next;
		sav->next=sav->next->next;
		delete tmp;
		return 0;
	}
	return -1;
}

FdCell *getFdCellFromFd(FdCell* fdInfo, int fd)
{
	// list not passed by adress, no need to save
	while ((fdInfo) && (fdInfo->fd!=fd)) fdInfo=fdInfo->next;
	return fdInfo;
}
/*
FdCell *getFdCellFromData(FdCell* fdInfo, const char* name=0, const char* workgroup=0,
	 const char* host=0, const char* share=0, const char* dir=0, const char* user=0)
{
	// list not passed by adress, no need to save
	// logic is a bit complicated here... but it works !
	while ((fdInfo) &&
	(((name) && (strcasecmp(fdInfo->name,name))) || ((!name) && (fdInfo->name)))
	|| (((workgroup) && (strcasecmp(fdInfo->workgroup,workgroup))) || ((!workgroup) && (fdInfo->workgroup)))
	|| (((host) && (strcasecmp(fdInfo->host,host))) || ((!host) && (fdInfo->host)))
	|| (((share) && (strcasecmp(fdInfo->share,share))) || ((!share) && (fdInfo->share)))
	|| (((dir) && (strcasecmp(fdInfo->dir,dir))) || ((!dir) && (fdInfo->dir)))
	|| (((user) && (strcasecmp(fdInfo->user,user))) || ((!user) && (fdInfo->user)))) {
		fdInfo=fdInfo->next;
	}
	return fdInfo;
}

*/
#endif