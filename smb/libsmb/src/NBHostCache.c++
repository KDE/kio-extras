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

#include "order.h"
#include "NBHostCache.h"
#include <sys/time.h>
#include <string.h>

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
			 uint32 ip, NBHostEnt *ne, uint32 time_out)
{
  host = new NBHostEnt(nbn, n, ip, ne);
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
		 uint32 ip, NBHostEnt *ne, uint32 time_out)
{
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
  new NBHostCache(nbn, n, ip, ne, time_out);
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

NBHostEnt *NBHostCache::find(const char *n)
{
  purgeOldEntries();
  if(base!=0) {
    NBHostCache *cur=base;
    while(cur!=0) {
      if(cur->host != 0 && !strcasecmp(cur->host->name, n)) {
	//	if(temp_ent == 0)
	  NBHostEnt *temp_ent = 
	    new NBHostEnt(cur->host->NBName, cur->host->name,
				   cur->host->ip, cur->host->next);
	  //	else
	  //	  *temp_ent=*cur->host;
	return temp_ent;
      }
      cur=cur->next;
    }
  }
  return 0;
}
    
    
NBHostEnt *NBHostCache::find(uint32 ip)
{
  purgeOldEntries();
  if(base!=0) {
    NBHostCache *cur=base;
    while(cur!=0) {
      if(cur->host->ip == ip) {
	//	if(temp_ent == 0)
	  NBHostEnt *temp_ent = 
	    new NBHostEnt(cur->host->NBName, cur->host->name,
				   cur->host->ip, cur->host->next);
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

  

  










