/*
    This file is part of the smb++ library
    Copyright (C) 2000  Nicolas Brodu
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

/*
	Implements NameQueryInterface using the Samba code
*/
#include "defines.h"
#ifdef USE_SAMBA

#include <sys/types.h> // needed on FreeBSD
#include <netdb.h>
#include <sys/socket.h>  // for AF_INET
#include "SambaNMB.h"
#include "SambaLink.h"
#include "strtool.h"

extern "C" {
BOOL resolve_name(const char *name, struct in_addr *return_ip, int name_type);
}

hostent *SambaNMB::gethostbyname(const char *name, bool groupquery=false)
{
	struct in_addr ip;
	BOOL ret = True;
	if (!groupquery) ret = resolve_name(name, &ip, 0x20);
	else {
		// try master browser
		ret = resolve_name(name, &ip, 0x1D);
		// no => try for domain
		if (ret != True) ret = resolve_name(name, &ip, 0x1B);
	}
	if (ret != True) return 0;
	newstrcpy(returnValue.h_name, name);
	memcpy(returnValue.h_addr_list[0], &ip, 4);
	return &returnValue;
}
/*
hostent *SambaNMB::gethostbyaddr(const char *addr, int len = 4, int type = AF_INET)
{
	if (type != AF_INET) return 0; // according to man, the only one supported
	return gethostbyname(addr, false); // use Samba resolve_name facility
}
*/

SambaNMB::SambaNMB()
{
	returnValue.h_name = 0;
	returnValue.h_aliases = 0;
	returnValue.h_addrtype = AF_INET;
	returnValue.h_length = 4;
	returnValue.h_addr_list = new (char*)[2];
	returnValue.h_addr_list[0] = new char[4];
	returnValue.h_addr_list[1] = 0;
	// load Samba code if not already done
	SambaLink::loadSamba();
}

SambaNMB::~SambaNMB()
{
	if (returnValue.h_name) delete returnValue.h_name;
	if (returnValue.h_addr_list) {
		if (returnValue.h_addr_list[0]) delete returnValue.h_addr_list[0];
		delete returnValue.h_addr_list;
	}
}



#endif