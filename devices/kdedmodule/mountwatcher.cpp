/* This file is part of the KDE project
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <string.h>
#include <time.h>

#include <qbuffer.h>
#include <qfile.h>
#include <qimage.h>
#include <qtimer.h>

#include <kdatastream.h> // DO NOT REMOVE, otherwise bool marshalling breaks
#include <kicontheme.h>
#include <kimageio.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kautomount.h>
#include <kdirwatch.h>
#include <qregexp.h>

#include "mountwatcher.moc"
#include "mountwatcher.h"
#include "kdirnotify_stub.h"

MountWatcherModule::MountWatcherModule(const QCString &obj)
    : KDEDModule(obj),mDiskList(this)
{
	firstTime=true;
	mDiskList.readFSTAB();
	mDiskList.readDF();


#ifdef MTAB
	KDirWatch::self()->addFile(MTAB);
#endif
#ifdef FSTAB
	KDirWatch::self()->addFile(FSTAB);
#endif
#ifdef FSTAB
	connect(KDirWatch::self(),SIGNAL(dirty(const QString&)),this,SLOT(dirty(const QString&)));
	KDirWatch::self()->startScan();
#else
#ifdef MTAB
	connect(KDirWatch::self(),SIGNAL(dirty(const QString&)),this,SLOT(dirty(const QString&)));
	KDirWatch::self()->startScan();
#endif
#endif

connect(&mDiskList,SIGNAL(readDFDone()),this,SLOT(readDFDone()));
}

MountWatcherModule::~MountWatcherModule()
{
}

uint MountWatcherModule::mountpointMappingCount()
{
	return mDiskList.count();
}

QStringList MountWatcherModule::basicList()
{
	return mountList;
}

QString  MountWatcherModule::mountpoint(int id)
{
	return mDiskList.at(id)->mountPoint();
}

QString MountWatcherModule::mountpoint(QString name)
{
	return (name=="//ide1/MP3")?"/mnt2":"/mnt";
}

QString  MountWatcherModule::devicenode(int id)
{
	return mDiskList.at(id)->deviceName();

}

QString  MountWatcherModule::type(int id)
{
	return mDiskList.at(id)->discType();
//	return (id==1)?"kdedevice/floppy_unmounted":"kdedevice/floppy_mounted";
}

bool   MountWatcherModule::mounted(int id)
{
	return mDiskList.at(id)->mounted();
}

bool   MountWatcherModule::mounted(QString name)
{
	return (name=="//ide1/MP3")?true:false;
}

void MountWatcherModule::mount( bool readonly, const QString& format, const QString& device, const QString& 
mountpoint,
              const QString & desktopFile, bool show_filemanager_window )
{

	KAutoMount *m=new KAutoMount( readonly, format, device, mountpoint,
              desktopFile, show_filemanager_window);
}


void MountWatcherModule::dirty(const QString& str)
{
#ifdef MTAB
	if (str==MTAB)
	{
	        mDiskList.readFSTAB();
	        mDiskList.readDF();
		return;

	}
#endif
#ifdef FSTAB
	if (str==FSTAB)
	{
	        mDiskList.readFSTAB();
	        mDiskList.readDF();
		return;
	}
#endif
}

QStringList MountWatcherModule::basicDeviceInfo(QString name)
{
	QStringList tmp;
	for (QStringList::Iterator it=mountList.begin();it!=mountList.end();)
	{
		if ((*it)==name)
		{
			++it;
			do
			{
				tmp<<(*it);
				++it;
			}
			while ((it!=mountList.end()) && ((*it)!="---"));
			++it;
		}
		else
		{
			while ((it!=mountList.end()) && ((*it)!="---"))	++it;
			++it;
		}
	}
	return tmp;
}


void MountWatcherModule::readDFDone()
{
	mountList.clear();
	KURL::List fileList;
	for (DiskEntry *ent=mDiskList.first();ent;ent=mDiskList.next())
	{
		QString entryName="entries_";
		entryName+=ent->deviceName().replace( QRegExp("/"), "" );
		entryName+=ent->mountPoint().replace(QRegExp("/"),"");
       	        if (ent->mounted())
		{
			mountList<<(entryName);
                 	mountList<<i18n("%1 mounted at %2").arg(ent->deviceName()).arg(ent->mountPoint());
			mountList<<ent->deviceName();
			mountList<<ent->mountPoint();
			mountList<< ent->discType()+"_mounted";
			mountList<<"true";
			mountList<<"---";
			fileList<<KURL(QString("devices:/")+entryName);
		}
               	else
		{
			mountList<<entryName;
                 	mountList<<i18n("%1 (not mounted)").arg(ent->deviceName());
			mountList<<ent->deviceName();
			mountList<<ent->mountPoint();
			mountList<< ent->discType()+"_unmounted";
			mountList<<"false";
			mountList<<"---";
			fileList<<KURL(QString("devices:/")+entryName);
		}
	}

        KDirNotify_stub allDirNotify("*", "KDirNotify*");
        allDirNotify.FilesAdded( "devices:/" );
}


extern "C" {
    KDEDModule *create_mountwatcher(const QCString &obj)
    {
        return new MountWatcherModule(obj);
    }
};
