/*
	This file is part of the smb library
    Copyright (C) 1999  Nicolas Brodu
    brodu@iie.cnam.fr

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

#ifndef __IO_DESCRIPTORS_H__
#define __IO_DESCRIPTORS_H__
#include <sys/stat.h>
#include "order.h"

// We need a structure to record information about the files we
// open. And it enables us to use our own file descriptors, instead
// of those sent by SMB servers, for which we can say nothing.
// in the futur it could lead to a use of real fd through pipes
// so that smb will be more transparent, and it could be used
// too implement a kind of virtual filesystem when browsing

// At present, a list is used, fd numbers are assigned
// from 1 when the list is created, and a new cell gets number
// "last cell of the list"->fd+1
// so that no two ones are the same
class FdCell : public stat
{
public:
	int fd;  // our own file descriptor
	int fid; // file id sent by server
	char *name; // file name
	int openMode; // O_RDWR | O_CREAT ...
	char *workgroup;
	char *host;
	char *share;
	char *dir;
	char *user; // userName
	int handle; // handle for operations on remote server
	int handleExist; // handle can be 0 ! so 'if (handle)' doesn't work :-(
	uint32 pos; // current position in file for I/O
	uint8 *cache; // cache
	uint8 *cpos;  // cache current position/pointer
	int clen;    // cache length
	int cinvalid; // cache invalid boolean
	int32 cmaxRead;
	int32 cmaxWrite;
	int32 cachePositionInFile;
	FdCell *next; // next cell in list
	FdCell(int _fd, int _fid, const char* _name, const char *_workgroup, const char* _host, const char* _share, const char* _dir, const char* _user, uint32 size=0);
	~FdCell();
	int copy(FdCell *fdc); // deep copy, including cache
};


void destroyFdList(FdCell *l);
int getNewFd(void* &fdInfo, int fid=-1, const char* name=0, const char* workgroup=0,
	 const char* host=0, const char* share=0, const char* dir=0, const char* user=0, uint32 size=0);
int closeFd(void* &fdInfo, int fd);
FdCell *getFdCellFromFd(FdCell* fdInfo, int fd);
//FdCell *getFdCellFromData(FdCell* fdInfo, const char* name=0, const char* workgroup=0,
//	const char* host=0, const char* share=0, const char* dir=0, const char* user=0);



#endif //__IO_DESCRIPTORS_H__
