/*
 * disks.cpp
 *
 * Copyright (c) 2002 Joseph Wenninger <jowenn@kde.org>
 * Copyright (c) 1998 Michael Kropfberger <michael.kropfberger@gmx.net>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qregexp.h>

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <qfileinfo.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "disks.h"

/****************************************************/
/********************* DiskEntry ********************/
/****************************************************/

DiskEntry::DiskEntry(QObject *parent, const char *name)
 : QObject (parent, name)
{
  realDevice="";
  device="";
  m_inode=0;
  m_inodeType=false;
  type="";
  mountedOn="";
  options="";
  size=0;
  used=0;
  avail=0;
  isMounted=FALSE;
}

QString DiskEntry::niceDescription()
{
	const QString dType(discType());
	if (dType.contains("hdd"))	return i18n("Hard Disc");
	else if (dType.contains("smb")) return i18n("Remote Share");
	else if (dType.contains("nfs")) return i18n("Remote Share");
	else if (dType.contains("cdrom")) return i18n("CD-ROM");
	else if (dType.contains("dvd")) return i18n("DVD");
	else if (dType.contains("cdwriter")) return i18n("CD Recorder");
	else if (dType.contains("floppy")) return i18n("Floppy");
	else if (dType.contains("zip")) return i18n("Zip Disk");
	else return i18n("Unknown");
}

QString DiskEntry::discType()
{
  kdDebug(7020)<<"disc/device type guessing"<<endl;
  QString typeName;
    // try to be intelligent
#ifdef Q_OS_LINUX
//  char str[21];
  QString str;
  QString tmpInfo;
    if (deviceName().startsWith("/dev/hd"))
    {
	kdDebug(7020)<<"Advanced IDE device type guessing"<<endl;
	QString tmp=deviceName();
	tmp=tmp.right(tmp.length()-5);
	tmp=tmp.left(3);
	tmp="/proc/ide/"+tmp+"/media";
	kdDebug(7020)<<"Trying to read file "<<tmp<<endl;
	QFile infoFile(tmp);
	if (infoFile.open(IO_ReadOnly))
	{
		int len;
		if (-1==(len=infoFile.readLine(tmpInfo,20))) typeName="kdedevice/TESTONLY";
		else
		{
			kdDebug(7020)<<"Type according to proc file system:"<<tmpInfo<<endl;
//			tmpInfo.fromLatin1(str,len);
			if (tmpInfo.contains("disk")) typeName="kdedevice/hdd";
			else
				if (tmpInfo.contains("cdrom")) typeName="kdedevice/cdrom";
				else
					if (tmpInfo.contains("floppy")) typeName="kdedevice/zip"; // eg IDE zip drives
					else typeName="kdedevice/hdd";
		}
		infoFile.close();
	} else typeName="kdedevice/hdd"; // this should never be reached
    }
    else
#elif defined(__FreeBSD__)
    if (-1!=deviceName().find("/acd",0,FALSE)) typeName="kdedevice/cdrom";
    else if (-1!=deviceName().find("/scd",0,FALSE)) typeName="kdedevice/cdrom";
    else if (-1!=deviceName().find("/ad",0,FALSE)) typeName="kdedevice/hdd";
    else if (-1!=deviceName().find("/da",0,FALSE)) typeName="kdedevice/hdd";
    else if (-1!=deviceName().find("/afd",0,FALSE)) typeName="kdedevice/zip";
#if 0
    else if (-1!=deviceName().find("/ast",0,FALSE)) typeName="kdedevice/tape";
#endif
    else
#endif
    /* Guessing of cdrom and cd recorder devices */
    if (-1!=mountPoint().find("cdrom",0,FALSE)) typeName="kdedevice/cdrom";
    else if (-1!=deviceName().find("cdrom",0,FALSE)) typeName="kdedevice/cdrom";
    else if (-1!=mountPoint().find("cdwriter",0,FALSE)) typeName="kdedevice/cdwriter";
    else if (-1!=deviceName().find("cdwriter",0,FALSE)) typeName="kdedevice/cdwriter";
    else if (-1!=deviceName().find("cdrw",0,FALSE)) typeName="kdedevice/cdwriter";
    else if (-1!=mountPoint().find("cdrw",0,FALSE)) typeName="kdedevice/cdwriter";
    else if (-1!=deviceName().find("cdrecorder",0,FALSE)) typeName="kdedevice/cdwriter";
    else if (-1!=mountPoint().find("dvd",0,FALSE)) typeName="kdedevice/dvd";   
    else if (-1!=deviceName().find("dvd",0,FALSE)) typeName="kdedevice/dvd";   
    else if (-1!=deviceName().find("/dev/scd",0,FALSE)) typeName="kdedevice/cdrom";
    else if (-1!=deviceName().find("/dev/sr",0,FALSE)) typeName="kdedevice/cdrom";


    /* Guessing of floppy types */
    else if (-1!=deviceName().find("fd",0,FALSE)) {
            if (-1!=deviceName().find("360",0,FALSE)) typeName="kdedevice/floppy5";
            if (-1!=deviceName().find("1200",0,FALSE)) typeName="kdedevice/floppy5";
            else typeName+="kdedevice/floppy";
         }
    else if (-1!=mountPoint().find("floppy",0,FALSE)) typeName="kdedevice/floppy";


    else if (-1!=mountPoint().find("zip",0,FALSE)) typeName+="kdedevice/zip";
    else if (-1!=fsType().find("nfs",0,FALSE)) typeName="kdedevice/nfs";
    else if (-1!=fsType().find("smb",0,FALSE)) typeName="kdedevice/smb";
    else if (-1!=deviceName().find("//",0,FALSE)) typeName="kdedevice/smb";
    else typeName="kdedevice/hdd";

  return typeName;


}

float DiskEntry::percentFull() const
{
   if (size != 0) {
      return 100 - ( ((float)avail / (float)size) * 100 );
   } else {
      return -1;
   }
}

void DiskEntry::setDeviceName(const QString & deviceName)
{
 device=deviceName;
 realDevice=deviceName;
 m_inodeType=false;
 if (deviceName.startsWith("/dev/"))
 {
	realDevice=KStandardDirs::realPath(deviceName);

	kdDebug(7020)<<"Device "<<deviceName<<" is a actually "<<realDevice<<endl;
 }

 struct stat st;
 if (stat(deviceName.latin1(),&st)!=-1)
 {
   m_inodeType=true;
   m_inode=st.st_ino;
 }

 emit deviceNameChanged();
};

void DiskEntry::setMountPoint(const QString & mountPoint)
{
  mountedOn=mountPoint;
 emit mountPointChanged();
};

void DiskEntry::setMountOptions(const QString & mountOptions)
{
 options=mountOptions;
 emit mountOptionsChanged();
};

void DiskEntry::setFsType(const QString & fsType)
{
  type=fsType;
  emit fsTypeChanged();
};

void DiskEntry::setMounted(bool nowMounted)
{
  isMounted=nowMounted;
  emit mountedChanged();
};

void DiskEntry::setKBSize(int kb_size)
{
  size=kb_size;
  emit kBSizeChanged();
};

void DiskEntry::setKBUsed(int kb_used)
{
  used=kb_used;
  if ( size < (used+avail) ) {  //adjust kBAvail
     kdWarning() << "device " << device << ": kBAvail(" << avail << ")+*kBUsed(" << used << ") exceeds kBSize(" << size << ")" << endl;
     setKBAvail(size-used);
  }
  emit kBUsedChanged();
};

void DiskEntry::setKBAvail(int kb_avail)
{
  avail=kb_avail;
  if ( size < (used+avail) ) {  //adjust kBUsed
     kdWarning() << "device " << device << ": *kBAvail(" << avail << ")+kBUsed(" << used << ") exceeds kBSize(" << size << ")" << endl;
     setKBUsed(size-avail);
  }
  emit kBAvailChanged();
};


#include "disks.moc"
