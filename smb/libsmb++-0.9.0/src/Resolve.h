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
#ifndef RESOLVE_H
#define RESOLVE_H
#include <time.h>
#include <sys/types.h> // needed on FreeBSD
#include <sys/socket.h> // AF_INET
#include <netinet/in.h> // for in_addr
#include "SelectedNMB.h"

// All static so that it's common to everything (a unique cache makes sense)
// but not very threadsafe...

class Resolve : public SelectedNMB
{
private:
	static int instances;
	// For parent virtual return value when using cache
	hostent rethostent;

public:
	Resolve();
	~Resolve();
	
	class Cache {
	private:
		Cache *beforeMe;
		Cache *afterMe;
		time_t timeout;
	public:
		Cache(const char* newname, struct in_addr* newip,
			bool newisGroup=false, int newtimeout=300);
		~Cache();
		Cache* next();
		void resetTimeout(int newtimeout=300);
		char *name;
		struct in_addr ip;
		bool isGroup;
		static Cache *find(const char* name);
		static Cache *cache;
	};
	
	static bool isWorkgroup(const char* name);
	virtual hostent *gethostbyname(const char *name, bool groupquery=false);
//	virtual hostent *gethostbyaddr(const char *addr, int len = 4, int type = AF_INET);
};

#endif
