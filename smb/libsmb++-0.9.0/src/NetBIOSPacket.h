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

// Interface for basic packet opérations (get data length, etc)
#ifndef __NETBIOS_PACKET_H__
#define __NETBIOS_PACKET_H__

#include "defines.h"
#ifndef USE_SAMBA

class NetBIOSPacket
{
protected:
	// Converts a name into its NetBIOS representation
	// returned pointer is allocated with new
	char* NBName(const char *, bool groupFlag=false);
	// modified is set when a modification has been made in
	// the packet causes some internal data to be
	// irrelevant. update check modified and then do the
	// necessary changes. update should be called in all the
	// public virtual functions below before returning anything
	// if this system is used.
	int modified;
	// Nothing to do on this level, subclasses might use it
	virtual void update();
public:
	NetBIOSPacket();
	virtual ~NetBIOSPacket() {}
	// indicator modified by member functions if an error occurs.
	// Functions able to do so should stick to the convention of
	// returning -1 when an error occurs.
	int errno;
	// Returns a pointer to a packet ready to be sent
	virtual uint8* packet() = 0;
	virtual int8 getType() = 0;
	// Get the real length of the packet in byte
	virtual uint16 getLength() = 0;
	// Get the real length of the data in byte
	virtual uint16 getDataLength() = 0;
	virtual uint8 *getData() = 0;
};

#endif
#endif //__NETBIOS_PACKET_H__
