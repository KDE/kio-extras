/*
 * disklist.cpp
 *
 * $Id$
 *
 * Copyright (c) 1999 Michael Kropfberger <michael.kropfberger@gmx.net>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config.h>

#include <stdlib.h>

#include <kapplication.h>
#include <kmountpoint.h>

#include "disklist.h"



/***************************************************************************
  * constructor
**/
DiskList::DiskList(QObject *parent, const char *name)
    : QObject(parent,name)
{
   mountPointExclusionList.setAutoDelete(true);
   loadExclusionLists();

   disks = new Disks;
   disks->setAutoDelete(TRUE);
};


/***************************************************************************
  * destructor
**/
DiskList::~DiskList()
{
};


void DiskList::loadExclusionLists()
{
   QString val;
   KConfig cfg("mountwatcher");
   cfg.setGroup("mountpoints");
   for (int i=0;(!(val=cfg.readEntry(QString("exclude%1").arg(i),"")).isEmpty());i++)
      mountPointExclusionList.append(new QRegExp(val));
}



bool DiskList::ignoreDisk(DiskEntry *disk)
{
	bool ignore;
	if ( (disk->deviceName() != "none")
	      && (disk->fsType() != "swap")
	      && (disk->fsType() != "tmpfs")
	      && (disk->deviceName() != "tmpfs")
	      && (disk->mountPoint() != "/dev/swap")
	      && (disk->mountPoint() != "/dev/pts")
	      && (disk->mountPoint().find("/proc") != 0)
	      && (disk->deviceName().find("shm") == -1  ))
		ignore=false;
	else
		ignore=true;

	if (!ignore) {
		for (QRegExp *exp=mountPointExclusionList.getFirst();exp;exp=mountPointExclusionList.next())
		{
			kdDebug()<<"TRYING TO DO A REGEXP SEARCH"<<endl;
			if (exp->search(disk->mountPoint())!=-1)
			{
				kdDebug()<<"IGNORING BECAUSE OF REGEXP SEARCH"<<endl;
				return true;
			}
		}
	}

	return ignore;
}

void DiskList::replaceDeviceEntryMounted(DiskEntry *disk)
{
	//I'm assuming that df always returns the real device name, not a symlink
	int pos = -1;
	for( u_int i=0; i<disks->count(); i++ ) {
		DiskEntry *item=disks->at(i);
		if ( ((
			(item->realDeviceName()==disk->deviceName()) ) ||
			((item->inodeType()==true) &&
			(disk->inodeType()==true) &&
			(disk->inode()==item->inode()))) &&
			(item->mountPoint()==disk->mountPoint()) ) {
			item->setMounted(TRUE);
			pos=i;
			break;
		}
	}
	if (pos==-1)
		disks->append(disk);
	else
		delete disk;

}

/***************************************************************************
  * updates or creates a new DiskEntry in the KDFList and TabListBox
**/
void DiskList::replaceDeviceEntry(DiskEntry *disk)
{
  //
  // The 'disks' may already already contain the 'disk'. If it do
  // we will replace some data. Otherwise 'disk' will be added to the list
  //

  //
  // 1999-27-11 Espen Sand:
  // I can't get find() to work. The Disks::compareItems(..) is
  // never called.
  //
  //int pos=disks->find(disk);

  kdDebug()<<"Trying to find an item referencing: "<<disk->deviceName()<<endl;
  int pos = -1;
  for( u_int i=0; i<disks->count(); i++ )
  {
    DiskEntry *item = disks->at(i);
    int res = disk->deviceName().compare( item->deviceName() );
    if( res == 0 )
    {
      res = disk->mountPoint().compare( item->mountPoint() );
    }
    if( res == 0 )
    {
      pos = i;
      break;
    }
  }

  if ((pos == -1) && (disk->mounted()) )
    // no matching entry found for mounted disk
    if ((disk->fsType() == "?") || (disk->fsType() == "cachefs")) {
      //search for fitting cachefs-entry in static /etc/vfstab-data
      DiskEntry* olddisk = disks->first();
      QString odiskName;
      while (olddisk != 0) {
        int p;
        // cachefs deviceNames have no / behind the host-column
	// eg. /cache/cache/.cfs_mnt_points/srv:_home_jesus
	//                                      ^    ^
        odiskName = olddisk->deviceName().copy();
        int ci=odiskName.find(':'); // goto host-column
        while ((ci =odiskName.find('/',ci)) > 0) {
           odiskName.replace(ci,1,"_");
        }//while
        // check if there is something that is exactly the tail
	// eg. [srv:/tmp3] is exact tail of [/cache/.cfs_mnt_points/srv:_tmp3]
        if ( ( (p=disk->deviceName().findRev(odiskName
	            ,disk->deviceName().length()) )
                != -1)
	      && (p + odiskName.length()
	          == disk->deviceName().length()) )
        {
             pos = disks->at(); //store the actual position
             disk->setDeviceName(olddisk->deviceName());
             olddisk=0;
	} else
          olddisk=disks->next();
      }// while
    }// if fsType == "?" or "cachefs"

  if (pos != -1) {  // replace
      disks->remove(pos); // really deletes old one
      disks->insert(pos,disk);
  } else {
    disks->append(disk);
  }//if

}

/***************************************************************************
  * tries to figure out the possibly mounted fs
**/
void DiskList::readFSTAB()
{
   KMountPoint::List mountPoints = KMountPoint::possibleMountPoints(0);

   for(KMountPoint::List::ConstIterator it = mountPoints.begin(); 
       it != mountPoints.end(); ++it)
   {
      KMountPoint *mp = *it;
      DiskEntry *disk = new DiskEntry();
      disk->setDeviceName(mp->mountedFrom());
      disk->setMountPoint(mp->mountPoint());
      disk->setFsType(mp->mountType());
      if (!ignoreDisk(disk))
         replaceDeviceEntry(disk);
      else
         delete disk;
   }
}

/***************************************************************************
  * tries to figure out the possibly mounted fs
**/
void DiskList::readMNTTAB()
{
   KMountPoint::List mountPoints = KMountPoint::currentMountPoints(0);

   for(KMountPoint::List::ConstIterator it = mountPoints.begin(); 
       it != mountPoints.end(); ++it)
   {
      KMountPoint *mp = *it;
      DiskEntry *disk = new DiskEntry();
      disk->setMounted(true);    // its now mounted
      disk->setDeviceName(mp->mountedFrom());
      disk->setMountPoint(mp->mountPoint());
      disk->setFsType(mp->mountType());
      if (!ignoreDisk(disk))
         replaceDeviceEntryMounted(disk);
      else
         delete disk;
   }
}

#include "disklist.moc"

