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
#ifndef SAMBA_NMB_H
#define SAMBA_NMB_H
#include "defines.h"
#ifdef USE_SAMBA

#include "NameQueryInterface.h"
#include <netdb.h>

class SambaNMB : public NameQueryInterface
{
private:
	hostent returnValue;

public:
	SambaNMB();
	~SambaNMB();
	virtual hostent *gethostbyname(const char *name, bool groupquery=false);
//	virtual hostent *gethostbyaddr(const char *addr, int len = 4, int type = AF_INET);
};

#endif
#endif
