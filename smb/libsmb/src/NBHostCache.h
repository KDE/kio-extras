/*
	This file is part of the smb library
    Copyright (C) 1999 Erik Forsberg
    forsberg@lysator.liu.se

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

#ifndef __NBHOSTCACHE_H__
#define __NBHOSTCACHE_H__

// Define our own hostent class, mainly for a clean destuctor
// (if a <netdb.h> struct hostent was used, it would be up to
// the caller to destroy each member separately)
class NBHostEnt
{
public:
	char *NBName; // NetBIOS name
	char *name;   // clear asciiz name
	uint32 ip;    // IP of workstation or master of group
	bool isGroup;
	NBHostEnt *next; // for group names, could be the members (not implemented)
	NBHostEnt(const char *nbn=0, const char *n=0, uint32 i=0, NBHostEnt *ne=0, bool groupFlag=0);
	~NBHostEnt();
};


class NBHostCache
{
private:
  static NBHostCache *base;
  static long nrof_cache_entries;
  uint32 lastCheck, timeout;
  NBHostEnt *host;
  NBHostCache *next;
  void purgeOldEntries();
  NBHostCache(const char *nbn, const char *n, uint32 ip,
      NBHostEnt *ne, bool groupFlag=false, uint32 time_out=0);
public:
  NBHostCache();
  ~NBHostCache();
  void add (const char *nbn=0, const char *n=0, uint32 ip=0,
      NBHostEnt *ne=0, bool groupFlag=false, uint32 time_out=0);
  //remove(const char *nbn=0, const char *n=0, uint32 ip=0);
  void remove(const char *n); // Do we need anything else?
  NBHostEnt *find(const char *n=0, bool groupFlag=false);
  NBHostEnt *find(uint32 ip=0, bool groupFlag=false);

};
#endif //__NBHOSTCACHE_H__
