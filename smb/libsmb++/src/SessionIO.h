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

// SMBio manages the connections to a remote host

#ifndef __SESSIONIO_H__
#define __SESSIONIO_H__
#include "defines.h"
#ifndef USE_SAMBA

#include "SessionPacket.h"
#include "NMBIO.h"

class SessionIO : public NMBIO
{
protected:
	char* hostNBName;
	char* hostName;
	int sock;
//	int discardsock;
	int16 port;
	// Connects to host:port and return 0 on error, 1 otherwise
    int connect(const char *hostname, uint16 p=139);
    int connect(uint32 IP, uint16 p=139);
public:
	SessionIO(const char *ourName=0); // can specify our host name
	~SessionIO();
	// send the packet, #NbBytes sent returned, -1 on error & errorCode in errno
	int send(SessionPacket *p);
	SessionPacket *receive();   // ...
	// Returns -1 and a NetBIOS Session error code in errno on error
	int openSession(const char *hostname);
	void closeSession();
};

#endif
#endif //__SESSIONIO_H__
