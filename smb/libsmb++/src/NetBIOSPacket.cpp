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

#include "NetBIOSPacket.h"
#include <string.h>
#ifdef DEBUG
#include <iostream.h>
#endif

NetBIOSPacket::NetBIOSPacket()
{
	modified=0;
}

// Nothing to do on this level, subclasses might use it
void NetBIOSPacket::update()
{
	modified=0;
}

// see RFC 1001/2 for this weird conversion
char* NetBIOSPacket::NBName(const char *name, bool groupFlag)
{
	uint8 len=strlen(name); // First label must be 32 bytes long
	if (len>16) len=16;   // no pointers can't be used in SMB
	int8 allocated=0;	// if we have to allocate a new string
	if (len<16)
	{
		char* name2=new char[16];
		strcpy(name2,name);
		for (int i=len; i<16; i++) name2[i]=' '; // Fill with spaces
		allocated=1;
		len=16;
		name=name2;
	}
	char *ret = new char[256]; // Max size (label size+label+scope)
	ret[0]=32;	// number of bytes in this label
	// I'd like to replace all the array access with (*array++)
	// but I don't know how CPUs with parity would react...
#if DEBUG >= 5
	cout<<"name : "<<name<<"\nlen : "<<(int)len<<"\n";
#endif
	for (int i=0; i<16; i++)
	{
		// Hope יטאמ and so won't matter...
		uint8 n=name[i];
		if ((n>='a') && (n<='z')) n-=32;
		ret[2*i+1]=((n>>4)&0xF)+0x41;
		ret[2*i+2]=(n&0xF)+0x41;
	}
	// Name is a group name (not in RFC but added for SMB...)
	if (groupFlag) {
		ret[31]=((0x1B>>4)&0xF)+0x41;
		ret[32]=(0x1B&0xF)+0x41;
	}
#if DEBUG >= 5
	cout<<"name : "<<name<<"\nlen : "<<(int)len<<"\n";
#endif
	
	char *sco=NBSCOPE; //NetBIOS scope
	uint32 posLen=33;
	uint32 posCur=34;
	uint16 len2=strlen(sco);
	uint16 lenCur=0;
	if ((lenCur<63) && (len2>0)) {
    	lenCur++;
    	ret[posCur++]='.';
    }
    for (int i=0; i<len2; i++)
    {
//    	if ((sco[i] != '.') && (lenCur<63))
    	if (lenCur<63)
    	{
    		lenCur++;
    		ret[posCur++]=sco[i];
    	}
    	else
    	{
    		ret[posLen]=lenCur;
    		lenCur=0;
    		posLen=posCur++;
    	}
    }
	ret[posLen]=lenCur;
	ret[posCur]=0;	//end
	if (allocated) delete name;
	errno=0;
	return ret;
}
#endif
