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
#include <kdebug.h>
#include <qfileinfo.h>

#include "disks.h"

/****************************************************/
/********************* DiskEntry ********************/
/****************************************************/

/**
  * Constructor
**/
void DiskEntry::init()
{
  linkedDevice="";
  device="";
  type="";
  mountedOn="";
  options="";
  size=0;
  used=0;
  avail=0;
  isMounted=FALSE;
  mntcmd="";
  umntcmd="";
  iconSetByUser=FALSE;
  icoName="";


 // BackgroundProcesses ****************************************

 sysProc = new KShellProcess(); Q_CHECK_PTR(sysProc);
 connect( sysProc, SIGNAL(receivedStdout(KProcess *, char *, int) ),
        this, SLOT (receivedSysStdErrOut(KProcess *, char *, int)) );
 connect( sysProc, SIGNAL(receivedStderr(KProcess *, char *, int) ),
        this, SLOT (receivedSysStdErrOut(KProcess *, char *, int)) );
 readingSysStdErrOut=FALSE;


}

DiskEntry::DiskEntry(QObject *parent, const char *name)
 : QObject (parent, name)
{
  init();
}

DiskEntry::DiskEntry(const QString & deviceName, QObject *parent, const char *name)
 : QObject (parent, name)
{
  init();

  setDeviceName(deviceName);
}
DiskEntry::~DiskEntry()
{
  disconnect(this);
  delete sysProc;
};

int DiskEntry::toggleMount()
{
  if (!mounted())
      return mount();
  else
      return umount();
};

int DiskEntry::mount()
{
  QString cmdS=mntcmd;
  if (cmdS.isEmpty()) // generate default mount cmd
    if (getuid()!=0 ) // user mountable
      cmdS="mount %d";
	else  // root mounts with all params/options
      cmdS="mount -t%t %d %m -o %o";

  cmdS.replace(QRegExp("%d"),this->deviceName());
  cmdS.replace(QRegExp("%m"),this->mountPoint());
  cmdS.replace(QRegExp("%t"),this->fsType());
  cmdS.replace(QRegExp("%o"),this->mountOptions());

  kdDebug() << "mount-cmd: [" << cmdS << "]" << endl;
  int e=sysCall(cmdS);
  if (!e) setMounted(TRUE);
  kdDebug() << "mount-cmd: e=" << e << endl;
  return e;
};

int DiskEntry::umount()
{
  kdDebug() << "umounting" << endl;
  QString cmdS=umntcmd;
  if (cmdS.isEmpty()) // generate default umount cmd
      cmdS="umount %d";

  cmdS.replace(QRegExp("%d"),this->deviceName());
  cmdS.replace(QRegExp("%m"),this->mountPoint());

  kdDebug() << "umount-cmd: [" << cmdS << "]" << endl;
  int e=sysCall(cmdS);
  if (!e) setMounted(FALSE);
  kdDebug() << "umount-cmd: e=" << e << endl;
 return e;
};

int DiskEntry::remount()
{
  if (mntcmd.isEmpty() && umntcmd.isEmpty() // default mount/umount commands
      && (getuid()==0)) // you are root
    {
    QString oldOpt=options;
    if (options.isEmpty())
       options="remount";
    else
       options+=",remount";
    int e=this->mount();
    options=oldOpt;
    return e;
   } else {
    if (int e=this->umount())
      return this->mount();
   else return e;
  }
};

void DiskEntry::setMountCommand(const QString & mnt)
{
  mntcmd=mnt;
};

void DiskEntry::setUmountCommand(const QString & umnt)
{
  umntcmd=umnt;
};

void DiskEntry::setIconName(const QString & iconName)
{
  iconSetByUser=TRUE;
  icoName=iconName;
  if (icoName.right(6) == "_mount")
     icoName.truncate(icoName.length()-6);
  else if (icoName.right(8) == "_unmount")
     icoName.truncate(icoName.length()-8);

  emit iconNameChanged();
};

QString DiskEntry::iconName()
{
  QString iconName=icoName;
  if (iconSetByUser) {
    mounted() ? iconName+="_mount" : iconName+="_unmount";
   return iconName;
  } else
   return guessIconName();
};

QString DiskEntry::guessIconName()
{
    QString iconName;
    // try to be intelligent
    if (-1!=mountPoint().find("cdrom",0,FALSE)) iconName+="cdrom";
    else if (-1!=deviceName().find("cdrom",0,FALSE)) iconName+="cdrom";
    else if (-1!=mountPoint().find("writer",0,FALSE)) iconName+="cdwriter";
    else if (-1!=deviceName().find("writer",0,FALSE)) iconName+="cdwriter";
    else if (-1!=mountPoint().find("mo",0,FALSE)) iconName+="mo";
    else if (-1!=deviceName().find("mo",0,FALSE)) iconName+="mo";
    else if (-1!=deviceName().find("fd",0,FALSE)) {
        if (-1!=deviceName().find("360",0,FALSE)) iconName+="5floppy";
        if (-1!=deviceName().find("1200",0,FALSE)) iconName+="5floppy";
        else iconName+="3floppy";
	 }
    else if (-1!=mountPoint().find("floppy",0,FALSE)) iconName+="3floppy";
    else if (-1!=mountPoint().find("zip",0,FALSE)) iconName+="zip";
    else if (-1!=fsType().find("nfs",0,FALSE)) iconName+="nfs";
    else if (-1!=fsType().find("fat",0,FALSE)) iconName+="hdd_windows";
    else if (-1!=fsType().find("msdos",0,FALSE)) iconName+="hdd_windows";
    else iconName+="hdd";
    mounted() ? iconName+="_mount" : iconName+="_unmount";
//    if ( -1==mountOptions().find("user",0,FALSE) )
//      iconName.prepend("root_"); // special root icon, normal user can´t mount

    //debug("device %s is %s",deviceName().latin1(),iconName.latin1());

    //emit iconNameChanged();
    return iconName;
};


QString DiskEntry::niceDescription()
{
	const QString dType(discType());
	if (dType.contains("hdd"))	return i18n("Hard Disc");
	else if (dType.contains("smb")) return i18n("Remote Share");
	else if (dType.contains("nfs")) return i18n("Remote Share");
	else if (dType.contains("cdrom")) return i18n("CD-ROM");
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
    else if (-1!=deviceName().find("scd",0,FALSE)) typeName="kdedevice/cdrom";
    else if (-1!=deviceName().find("/sr",0,FALSE)) typeName="kdedevice/cdrom";


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

/***************************************************************************
  * starts a command on the underlying system via /bin/sh
**/
int DiskEntry::sysCall(const QString & command)
{
  if (readingSysStdErrOut || sysProc->isRunning() )  return -1;

  sysStringErrOut=i18n("Called: %1\n\n").arg(command); // put the called command on ErrOut
  sysProc->clearArguments();
  (*sysProc) << command;
    if (!sysProc->start( KProcess::Block, KProcess::AllOutput ))
    {
//WARNING FIXME
     kdWarning(7020)<<i18n("could not execute [%1]").arg(command)<<endl;
     return -1;
    }

  if (sysProc->exitStatus()!=0) emit sysCallError(this, sysProc->exitStatus());

  return (sysProc->exitStatus());
};


/***************************************************************************
  * is called, when the Sys-command writes on StdOut or StdErr
**/
void DiskEntry::receivedSysStdErrOut(KProcess *, char *data, int len)
{
  QString tmp = QString::fromLocal8Bit(data, len);
  sysStringErrOut.append(tmp);
};

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
 if (deviceName.startsWith("/dev/"))
 {
	QFileInfo finfo=QFileInfo(deviceName);
	if (finfo.isSymLink())
	{
		linkedDevice=finfo.readLink();
		kdDebug(7020)<<"Device "<<deviceName<<" is a symlink to "<<linkedDevice<<endl;
	}
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
