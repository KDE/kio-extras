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
#include "Resolve.h"
#include "strtool.h"
#include <netinet/in.h> // for struct in_addr
#include "SelectedNMB.h"

int Resolve::instances = 0;
Resolve::Cache *Resolve::Cache::cache = 0;

Resolve::Cache::Cache(const char* newname, struct in_addr* newip,
			bool newisGroup=false, int newtimeout=300)
{
	name=0; newstrcpy(name, newname);
	ip = *newip;
	isGroup = newisGroup;
	// time at which this entry becomes irrelevant
	if (newtimeout) timeout = time(0) + newtimeout;
	else timeout = 0; // never check this
	// Link ourself to the other cache entries
	beforeMe = 0;
	afterMe = cache;
	cache = this;
	if (afterMe) afterMe->beforeMe = this;
}

Resolve::Cache::~Cache()
{
	// unlink from previous element, or update list
	if (!beforeMe) cache = afterMe;
	else beforeMe->afterMe = afterMe;
	// unlink from next element
	if (afterMe) afterMe->beforeMe = beforeMe;
	// cleanup
	if (name) delete name;
}

Resolve::Cache *Resolve::Cache::next()
{
	return afterMe;
}

void Resolve::Cache::resetTimeout(int newtimeout=300)
{
	if (newtimeout) timeout = time(0) + newtimeout;
	else timeout = 0;
}

Resolve::Cache *Resolve::Cache::find(const char *name)
{
	if (!name) return 0;
	time_t now = time(0);
	struct DestroyCell {
		Cache *c;
		DestroyCell *next;
	} *destroyList = 0;
	// return value
	Cache *ret =0;
	
	for (Cache* it=cache; (it); it = it->next()) {
		if ((it->timeout) && (it->timeout < now)) {
			// stores the old entries here
			DestroyCell *cell = new DestroyCell;
			cell->c = it; cell->next = destroyList;
			destroyList = cell;
		}
		if ((it->name) && (!mystrcasecmp(it->name, name))) ret=it;
	}

	// destroy all old entries	
	while (destroyList) {
		DestroyCell *sav = destroyList;
		delete destroyList->c;
		destroyList = destroyList ->next;
		delete sav;
	}
	
	return ret;
}
	
Resolve::Resolve()
{
	rethostent.h_name = 0;
	rethostent.h_aliases = 0;
	rethostent.h_addrtype = AF_INET;
	rethostent.h_length = 4;
	rethostent.h_addr_list = new (char*)[2];
	rethostent.h_addr_list[0] = new char[4];
	rethostent.h_addr_list[1] = 0;
	
	// This should be useless: Cache::cache is static and 0 already
/*	if (!instances) {
		// Init the cache the first time
		Cache::cache = 0;
	}*/

	instances++;
}

Resolve::~Resolve()
{
	if (--instances<=0) {
		// Destructors are wonderful!
		while (Cache::cache) delete Cache::cache;
	}
	if (rethostent.h_name) delete rethostent.h_name;
	if (rethostent.h_addr_list) {
		if (rethostent.h_addr_list[0]) delete rethostent.h_addr_list[0];
		delete rethostent.h_addr_list;
	}
}

bool Resolve::isWorkgroup(const char* name)
{
	// static method, so define a temporary object here
	// Anyway, all instances share the same cache...
	Resolve r;
	
	// Try standard host first
	if (r.gethostbyname(name, false)) return false;

	return true;
	// hack
/*	// maybe it's a group
	if (r.gethostbyname(name, true)) return true;

	return false;
*/
}

// Redefine parent virtual to use the cache
hostent *Resolve::gethostbyname(const char *name, bool groupquery=false)
{
	// look in the cache first
	Cache *c = Cache::find(name);
	if (c) {
		if (c->isGroup == groupquery) {
			newstrcpy(rethostent.h_name, name);
			memcpy(rethostent.h_addr_list[0], &c->ip, 4);
			c->resetTimeout(groupquery ? 3600 : 300);
			return &rethostent;
		} else return 0;
	}

	hostent *hres = SelectedNMB::gethostbyname(name, groupquery);
	if (hres) {
		// found, add in the cache. timeout of 5 min for hosts, 1h for groups
		int timeout = groupquery ? 3600 : 300;
		new Cache(name, (struct in_addr*)(&hres->h_addr_list[0]), groupquery, timeout);
	}
	return hres;
}
