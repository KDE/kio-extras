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

#ifndef __IO_TYPES_H__
#define __IO_TYPES_H__

#include "defines.h"
#ifndef USE_SAMBA

// For external & internal use
// The list destroys itself if you delete the first cell
// Those defines are also used in openService
#define SMB_DISK 0
#define SMB_PRINT_QUEUE 1
#define SMB_DEVICE 2
#define SMB_IPC 3
class SMBShareList
{
public:
	char *name;
	int type;	// one of the defines above
	char *comment;
	SMBShareList *next;
	SMBShareList(const char* n=0, int t=SMB_DISK, const char* c=0, SMBShareList *ne=0);
	~SMBShareList();
};

class SMBMasterList
{
public:
	char *name;
	SMBMasterList *next;
	SMBMasterList(const char* n=0, SMBMasterList *ne=0);
	~SMBMasterList(); // destroy recursively
};

class SMBMemberList
{
public:
	char *name; // member name
	char *comment; // comment on this member if available
	SMBMemberList *next;
	SMBMemberList(const char* n=0, const char* c=0, SMBMemberList *ne=0);
	~SMBMemberList(); // destroy recursively
};

class SMBWorkgroupList
{
public:
	char *name; // workgroup name
	SMBMasterList *possibleMasters; // workgroup master for this workgroup
	SMBMemberList *members; // members of this workgroup
	uint32 lastCheck;  // date of last check. enable timeouts
	SMBWorkgroupList *next;
	// WARNING : do not copy the full member & master list...
	SMBWorkgroupList(const char* n=0, SMBMasterList* ma=0, SMBMemberList *me=0, SMBWorkgroupList *ne=0);
	~SMBWorkgroupList(); // destroy recursively
};

#endif
#endif // __IO_TYPES_H__
