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

// SMBio manages the connections to a remote host
#ifndef __NMBIO_H__
#define __NMBIO_H__

#include "order.h"
#include "NBHostCache.h"
// NBHostEnt definition moved to NBHostCache.h


class NMBIO
{
protected:
	char *decodeNBName(const char* NBName);
	char *ourName;
	char *ourNBName;
	NBHostCache *cache;
	uint32 netaddr; // guess ?
	struct sockaddr_in *socknetaddr; // parameters of the connection
	uint32 NBNS; // IP of a NetBIOS name server
public:
	NMBIO(const char *us=0); // can specify our host name
	~NMBIO();
	int errno;
	int getError();
	// return a string containing ourName, which will be deallocated by caller
	char *getOurName();
	// return a string containing ourNBName, which will be deallocated by caller
	char *getOurNBName();
	// set our name (ASCIIZ)
	void setOurName(const char *name);
	// sets the broadcast address !
	int setNetworkBroadcastAddress(uint32 addr);
	int setNetworkBroadcastAddress(const char *addr);
	// sets the NBNS address !
	int setNBNSAddress(uint32 addr);
	int setNBNSAddress(const char *addr);
	// Works with DNS or NetBIOS names. In case of a conflict
	// between NetBIOS and DNS, NetBIOS is used.
	// At present, IP dot notation is not supported, and
	// In case of a group name, only the members IP are valid
	// In the future, the list of the member names will be
	// returned as well (see NBHostEnt class)
	struct NBHostEnt *gethostbyname(const char *name); //,int flag=0);
	// doesn't work !
	struct NBHostEnt *gethostbyaddr(uint32 IP);
        void addNameIpToCache(const char *name, uint32 ip, uint32 timeout);
};


#endif //__NMBIO_H__









