/*
    This file is part of the smb++ library
    Copyright (C) 1999 Erik Forsberg
    forsberg@lysator.liu.se
    Copyright (C) 1999-2000 Nicolas Brodu
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

#include "NBHostCache.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>
#if DEBUG >=6
#include <stdio.h>
#endif

NBHostEnt::NBHostEnt(const char *nbn, const char *n, uint32 i, NBHostEnt *ne, bool groupFlag)
{
	if (nbn) {
		NBName=new char[strlen(nbn)+1];
		strcpy(NBName,nbn);
	} else NBName=0;
	if (n) {
		name=new char[strlen(n)+1];
		strcpy(name,n);
	} else name=0;
	ip=i;
	isGroup=groupFlag;
	next=ne;
}

NBHostEnt::~NBHostEnt()
{
	if (name) delete (name); // not recursive of course !
	if (NBName) delete (NBName); // not recursive of course !
	if (next) delete (next); // recursively
}


NBHostCache *NBHostCache::base = 0;
long NBHostCache::nrof_cache_entries = 0;

NBHostCache::NBHostCache()
{
  lastCheck=0;
  timeout=0;
  host=0;
  next=0;
  base=0;
}

NBHostCache::~NBHostCache()
{
    delete host;
    delete next; // Recursive
    nrof_cache_entries--;
    if (nrof_cache_entries == 0) {
      delete base;
      base=0;
    }
}

NBHostCache::NBHostCache(const char *nbn, const char *n, 
			 uint32 ip, NBHostEnt *ne, bool groupFlag, uint32 time_out)
{
  host = new NBHostEnt(nbn, n, ip, ne, groupFlag);
  lastCheck = (uint32)time(0);
  timeout=time_out;

  if (nrof_cache_entries == 0) {
    next=0;
    base = this;
  }
  else {
    this->next = base;
    base = this;
  }
  nrof_cache_entries++;
}

    

void NBHostCache::add(const char *nbn, const char *n, 
		 uint32 ip, NBHostEnt *ne, bool groupFlag, uint32 time_out)
{
#if DEBUG >=6
printf("NBHostCache::add: name=%s, groupflag=%d\n",n,groupFlag);
#endif
  if(base!=0) {
    NBHostCache *cur = base;
    while(cur!=0) {
      if (cur->host != 0) {
	if(cur->host->ip == ip             &&
	   !strcasecmp(cur->host->NBName, nbn) &&
	   !strcasecmp(cur->host->name, n)) {  // Host is already in cache
	  cur->lastCheck=(uint32)time(0);
	  return;
	}
      }
      cur=cur->next;
    }
  }
  new NBHostCache(nbn, n, ip, ne, groupFlag, time_out);
  purgeOldEntries();
}

void NBHostCache::remove(const char *n)
{
  if (base!=0) {
    NBHostCache *cur=base;
    NBHostCache *prev=0;
    while(cur!=0) {
      if(!strcasecmp(cur->host->name, n)) {
	if(prev == 0) 
	  base = cur->next;
	else
	  prev->next = cur->next;
	cur->next = 0; // Prevent recursive deletion
	delete cur;
	nrof_cache_entries--;
	if (prev == 0) {
	  cur = base;
	  continue;
	}
      }
      prev = cur;
      cur = prev->next;
    }
  }
  purgeOldEntries();

}

NBHostEnt *NBHostCache::find(const char *n, bool groupFlag)
{
#if DEBUG >=6
printf("NBHostCache::find: name=%s, groupflag=%d\n",n,groupFlag);
#endif
  purgeOldEntries();
  if(base!=0) {
    NBHostCache *cur=base;
    while(cur!=0) {
#if DEBUG >=6
printf("found %s and %d\n",cur->host->name,cur->host->isGroup);
#endif
      if(cur->host != 0 && !strcasecmp(cur->host->name, n)
	      && (cur->host->isGroup==groupFlag)) {
	//	if(temp_ent == 0)
	  NBHostEnt *temp_ent = 
	    new NBHostEnt(cur->host->NBName, cur->host->name,
				   cur->host->ip, cur->host->next, cur->host->isGroup);
	  //	else
	  //	  *temp_ent=*cur->host;
	return temp_ent;
      }
      cur=cur->next;
    }
  }
  return 0;
}
    
    
NBHostEnt *NBHostCache::find(uint32 ip, bool groupFlag)
{
  purgeOldEntries();
  if(base!=0) {
    NBHostCache *cur=base;
    while(cur!=0) {
      if ((cur->host->ip == ip)
	      && (cur->host->isGroup==groupFlag)) {
	//	if(temp_ent == 0)
	  NBHostEnt *temp_ent = 
	    new NBHostEnt(cur->host->NBName, cur->host->name,
				   cur->host->ip, cur->host->next, cur->host->isGroup);
	  //	else 
	  //	  *temp_ent = *cur->host;
	return temp_ent;
      }
      cur=cur->next;
    }
  }
  return 0;
}
  

void NBHostCache::purgeOldEntries()
{
  if(base!=0) {
    NBHostCache *cur=base;
    NBHostCache *prev=0;
    uint32 now=(uint32)time(0);
    while(cur!=0) {
      if((cur->lastCheck+cur->timeout) < now) {
	if(prev==0) 
	  base=cur->next;
	else
	  prev->next = cur->next;
	cur->next = 0; // Prevent recursive deletion
	delete cur;
	nrof_cache_entries--;
	if (prev == 0) {
	  cur = base;
	  continue;
	}
      }
      prev = cur;
      cur = cur->next;
    }
  }
}

#endif


