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
#include <qregexp.h>

#include <kdatastream.h> // DO NOT REMOVE, otherwise bool marshalling breaks
#include <kicontheme.h>
#include <kimageio.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kautomount.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kglobal.h>

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
}

bool   MountWatcherModule::mounted(int id)
{
	return mDiskList.at(id)->mounted();
}

bool   MountWatcherModule::mounted(QString name)
{
	return (name=="//ide1/MP3")?true:false;
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


void MountWatcherModule::addSpecialDevice(const QString& uniqueIdentifier, const QString& description,
 const QString& URL, const QString& mimetype,bool mountState)
{
	specialEntry ent;
	ent.id=uniqueIdentifier;
	ent.description=description;
	ent.url=URL;
	ent.mimeType=mimetype;
	ent.mountState=mountState;
	mEntryMap.insert(uniqueIdentifier,ent,true);
	readDFDone();
}

void MountWatcherModule::readDFDone()
{
	mountList.clear();
	KURL::List fileList;
	for (DiskEntry *ent=mDiskList.first();ent;ent=mDiskList.next())
	{
		QString entryName="";
		entryName+=ent->deviceName().replace( QRegExp("/"), "" );
		entryName+=ent->mountPoint().replace(QRegExp("/"),"");
                QString filename = KURL(ent->deviceName()).fileName();
                if(!filename.isEmpty())
                        filename = QString::fromLatin1(" (") + filename + ")";
		else filename=" ";
       	        if (ent->mounted())
		{
			mountList<<(entryName);
                 	mountList<<i18n("%1%2 mounted at %3").arg(ent->niceDescription()).arg(filename).arg(ent->mountPoint());
			mountList<<ent->deviceName();
//			mountList<<ent->mountPoint();
			mountList<<"file:/"+(ent->mountPoint().startsWith("/")?ent->mountPoint().right(ent->mountPoint().length()-1):ent->mountPoint());
			mountList<< ent->discType()+"_mounted";
			mountList<<"true";
			mountList<<"---";
			fileList<<KURL(QString("devices:/")+entryName);
		}
               	else
		{
			mountList<<entryName;
                 	mountList<<i18n("%1%2 (not mounted)").arg(ent->niceDescription()).arg(filename);
			mountList<<ent->deviceName();
			mountList<<"file:/"+(ent->mountPoint().startsWith("/")?ent->mountPoint().right(ent->mountPoint().length()-1):ent->mountPoint());
			mountList<< ent->discType()+"_unmounted";
			mountList<<"false";
			mountList<<"---";
			fileList<<KURL(QString("devices:/")+entryName);
		}
	}

	for (EntryMap::iterator it=mEntryMap.begin();it!=mEntryMap.end();++it)
	{
		mountList<<it.data().id;
		mountList<<it.data().description;
		mountList<<" ";
		mountList<<it.data().url;
		mountList<<it.data().mimeType;
		mountList<<(it.data().mountState?"true":"false");
		mountList<<"---";

	}

        KDirNotify_stub allDirNotify("*", "KDirNotify*");
        allDirNotify.FilesAdded( "devices:/" );
}

bool MountWatcherModule::createLink(const KURL& deviceURL, const KURL& destinationURL)
{
	kdDebug(7020)<<"Should create a desktop file for a device"<<endl;
	kdDebug(7020)<<"Source:"<<deviceURL.prettyURL()<<" Destination:"<<destinationURL.prettyURL()<<endl;
	kdDebug(7020)<<"======================"<<endl;
	QStringList info;
	info=basicDeviceInfo(deviceURL.fileName());
	if (!info.isEmpty())
	{
		KURL dest(destinationURL);
		dest.setFileName(KIO::encodeFileName(*(info.at(0)))+".desktop");
		QString path=dest.path();
		QFile f(path);
		if (f.open(IO_ReadWrite))
		{
			f.close();
            KSimpleConfig config( path );
            config.setDesktopGroup();
            config.writeEntry( QString::fromLatin1("Dev"), *(info.at(1)) );
			config.writeEntry( QString::fromLatin1("Encoding"),
                               QString::fromLatin1("UTF-8"));
			config.writeEntry( QString::fromLatin1("FSType"),
                               QString::fromLatin1("Default") );
			config.writeEntry( QString::fromLatin1("Icon"), "hdd_mount");
			config.writeEntry( QString::fromLatin1("UnmountIcon"), "hdd_unmount");
			config.writeEntry( QString::fromLatin1("MountPoint"),
                               (*info.at(2)).right((*(info.at(2))).length()-5));
			config.writeEntry( QString::fromLatin1("Icon"), "hdd_mount");
			config.writeEntry( QString::fromLatin1("Type"),
                               QString::fromLatin1("FSDevice"));
            config.sync();
			return true;

		}

	}
	return false;
}



extern "C" {
    KDEDModule *create_mountwatcher(const QCString &obj)
    {
        KGlobal::locale()->insertCatalogue("kio_devices");
        return new MountWatcherModule(obj);
    }
};
