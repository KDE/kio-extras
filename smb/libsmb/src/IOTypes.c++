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

#include <string.h>
#include "IOTypes.h"

char *SmbAnswerCallback::getAnswer(const char *message, bool echo)
{
	// print your message here
	// ask user an answer
	// return it allocated with new, or 0
	return 0;
}


SMBShareList::SMBShareList(const char* n, int t, const char* c, SMBShareList* ne)
{
	type=t;
	if (n) {
		name=new char[strlen(n)+1];
		strcpy(name,n);
	} else name=0;
	if (c) {
		comment=new char[strlen(c)+1];
		strcpy(comment,c);
	} else comment=0;
	next=ne;
}

SMBShareList::~SMBShareList()
{
	if (name) delete name;
	if (comment) delete comment;
	if (next) delete next;	// recursion at the end
}

SMBWorkgroupList::SMBWorkgroupList(const char* n, SMBMasterList* ma, SMBMemberList *me, SMBWorkgroupList *ne)
{
	if (n) {
		name=new char[strlen(n)+1];
		strcpy(name,n);
	} else name=0;
	// WARNING : do not copy the full list...
	members=me;
	possibleMasters=ma;
	lastCheck=0; // force timeout. smb will be blown up long before year 2106 bug, if we still use 32 bit values then !
	next=ne;
}

SMBWorkgroupList::~SMBWorkgroupList()
{
	if (name) delete name;
	if (possibleMasters) delete possibleMasters;
	if (members) delete members; // destroy recursively
	if (next) delete next;	// own recursion at the end
}


SMBMemberList::SMBMemberList(const char* n, const char* c, SMBMemberList* ne)
{
	if (n) {
		name=new char[strlen(n)+1];
		strcpy(name,n);
	} else name=0;
	if (c) {
		comment=new char[strlen(c)+1];
		strcpy(comment,c);
	} else comment=0;
	next=ne;
}

SMBMemberList::~SMBMemberList()
{
	if (name) delete name;
	if (comment) delete comment;
	if (next) delete next;	// recursion at the end
}

SMBMasterList::SMBMasterList(const char* n, SMBMasterList* ne)
{
	if (n) {
		name=new char[strlen(n)+1];
		strcpy(name,n);
	} else name=0;
	next=ne;
}

SMBMasterList::~SMBMasterList()
{
	if (name) delete name;
	if (next) delete next;	// recursion at the end
}
