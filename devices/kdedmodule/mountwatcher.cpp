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

#include <qglobal.h>
#include <qfile.h>
#include <qtimer.h>

#include <kdatastream.h> // DO NOT REMOVE, otherwise bool marshalling breaks
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kglobal.h>

#include "mountwatcher.h"
#include "kdirnotify_stub.h"

#ifdef _OS_SOLARIS_
#define FSTAB "/etc/vfstab"
#define MTAB "/etc/mnttab"
#else
#define FSTAB "/etc/fstab"
#define MTAB "/etc/mtab"
#endif

MountWatcherModule::MountWatcherModule(const QCString &obj)
    : KDEDModule(obj),mDiskList(this),mtabsize(0)
{
	firstTime=true;

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

	mDiskList.readFSTAB();
	mDiskList.readMNTTAB();
	mDiskList.removeOldDisks();
	reReadSpecialConfig();
	readDFDone();
}


MountWatcherModule::~MountWatcherModule()
{
}

void MountWatcherModule::reReadSpecialConfig() {
	KConfig cfg("mountwatcher.desktop");

	QStringList internat=cfg.readListEntry("catalogues");
	for ( QStringList::Iterator it = internat.begin(); it != internat.end(); ++it )
		KGlobal::locale()->insertCatalogue(*it);

	QString entryTemplate="specialEntry:%1";

	bool somethingChanged;
	do {
		somethingChanged=false;
		for (EntryMap::iterator it=mEntryMap.begin();it!=mEntryMap.end();++it) {
			if (it.data().fromConfigFile) {
				somethingChanged=true;
				mEntryMap.remove(it);
				break;
			}
		}
	} while (somethingChanged);

	for (int i=0;cfg.hasGroup(entryTemplate.arg(i));i++) {
		cfg.setGroup(entryTemplate.arg(i));
		if (cfg.readEntry("hidden","false")=="true") continue;
		QString uniqueID=cfg.readEntry("uniqueID");
		if (uniqueID.isEmpty()) continue;
		QString description=cfg.readEntry("Name");
		if (description.isEmpty()) continue;
		description=i18n(description.utf8());
		QString URL=cfg.readEntry("URL");
		if (URL.isEmpty()) continue;
		QString mimetype=cfg.readEntry("mimetype");
		if (mimetype.isEmpty()) continue;
		addSpecialDeviceInternal(uniqueID,description,URL,mimetype,true,true);
	}

}


uint MountWatcherModule::mountpointMappingCount()
{
	return mDiskList.count();
}

QStringList MountWatcherModule::basicList()
{
	return mountList;
}

QStringList MountWatcherModule::basicSystemList()
{
	return completeList;
}

QString  MountWatcherModule::mountpoint(int id)
{
	return mDiskList.at(id)->mountPoint();
}

QString MountWatcherModule::mountpoint(QString devicename)
{
	DiskEntry *ent;
	for (ent=mDiskList.first(); ent; ent=mDiskList.next()) {
		if (ent->deviceName() == devicename)
			return ent->mountPoint();
	}

	return QString::null;
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
	if (!mDiskList.at(id)) return false;
	return mDiskList.at(id)->mounted();
}

bool   MountWatcherModule::mounted(QString name)
{
	DiskEntry *ent;
	for (ent=mDiskList.first(); ent; ent=mDiskList.next()) {
		if ( (ent->deviceName() == name) || (ent->realDeviceName() == name) || (ent->mountPoint() == name) )
			return true;
	}

	return false;
}

void MountWatcherModule::reloadExclusionLists()
{
	mDiskList.loadExclusionLists();
	mDiskList.setAllOld();
	mDiskList.readFSTAB();
	mDiskList.readMNTTAB();
	mDiskList.removeOldDisks();
	readDFDone();
}

void MountWatcherModule::dirty(const QString& str)
{
#ifdef MTAB
	if (str==MTAB)
	{
		QFile f(MTAB);
		f.open(IO_ReadOnly);
		uint newsize=f.readAll().size();
		f.close();
		if (newsize!=mtabsize) {
			mtabsize=newsize;
			kdDebug()<<"MTAB FILESIZE:"<<f.size()<<endl;
			mDiskList.setAllOld();
			mDiskList.readFSTAB();
			mDiskList.readMNTTAB();
			mDiskList.removeOldDisks();
			readDFDone();
			return;
		}
	}
#endif
#ifdef FSTAB
	if (str==FSTAB)
	{
		mDiskList.setAllOld(); 
		mDiskList.readFSTAB();
		mDiskList.readMNTTAB();
		mDiskList.removeOldDisks();
		readDFDone();
		return;
	}
#endif
}

QStringList MountWatcherModule::basicDeviceInfo(QString name)
{
	QStringList tmp;
	for (QStringList::Iterator it=completeList.begin();it!=completeList.end();)
	{
		++it;
		if ((*it)==name)
		{
			--it;
			do
			{
				tmp<<(*it);
				++it;
			}
			while ((it!=completeList.end()) && ((*it)!="---"));
			++it;
		}
		else
		{
			while ((it!=completeList.end()) && ((*it)!="---"))	++it;
			++it;
		}
	}
	return tmp;
}

QStringList MountWatcherModule::basicDeviceInfoForMountPoint(QString mountpoint)
{
	QStringList tmp;
	for (QStringList::Iterator it=mountList.begin();it!=mountList.end();)
	{	
		QString name=(*it);++it;
		QString description=(*it); ++it;
		QString device=(*it); ++it;
		if ((*it)==mountpoint)
		{
			tmp<<description<<device;
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
			kdDebug()<<(*it)<<"!="<<mountpoint<<endl;
			while ((it!=mountList.end()) && ((*it)!="---"))	++it;
			++it;
		}
	}
	return tmp;
}


void MountWatcherModule::addSpecialDevice(const QString& uniqueIdentifier, const QString& description,
 const QString& URL, const QString& mimetype,bool mountState)
{
	addSpecialDeviceInternal(uniqueIdentifier,description,URL,mimetype,mountState,false);
}

void MountWatcherModule::addSpecialDeviceInternal(const QString& uniqueIdentifier, const QString& description,
 const QString& URL, const QString& mimetype,bool mountState,bool fromConfigFile)
{
	specialEntry ent;
	ent.id=uniqueIdentifier;
	ent.description=description;
	ent.url=URL;
	ent.mimeType=mimetype;
	ent.mountState=mountState;
	ent.fromConfigFile=fromConfigFile;
	mEntryMap.insert(uniqueIdentifier,ent,true);
	readDFDone();
}

void MountWatcherModule::removeSpecialDevice(const QString& uniqueIdentifier)
{
	mEntryMap.remove(uniqueIdentifier);
	readDFDone();
}

void MountWatcherModule::readDFDone()
{
	QStringList oldmountList(mountList);
	QStringList oldcompleteList(completeList);
	mountList.clear();
	KURL::List fileList;
	QMap <QString,QString> descriptionToDeviceMap;
	QMap <QString,QString> descriptionToMountMap;
	for (DiskEntry *ent=mDiskList.first();ent;ent=mDiskList.next())
	{
	   if (descriptionToDeviceMap.contains(ent->niceDescription()))
	   {
	      if (descriptionToDeviceMap[ent->niceDescription()] != ent->deviceName())
	         descriptionToDeviceMap[ent->niceDescription()] = QString::null;
	   }
	   else
	   {
	      descriptionToDeviceMap[ent->niceDescription()] = ent->deviceName();
	   }

	   if (descriptionToMountMap.contains(ent->niceDescription()))
	   {
	      if (descriptionToMountMap[ent->niceDescription()] != ent->mountPoint())
	         descriptionToMountMap[ent->niceDescription()] = QString::null;
	   }
	   else
	   {
	      descriptionToMountMap[ent->niceDescription()] = ent->mountPoint();
	   }
	}

	KConfig cfg("mountwatcherrc");
	cfg.setGroup("Labels");
			
	
	for (DiskEntry *ent=mDiskList.first();ent;ent=mDiskList.next())
	{
		QString entryName="";
		entryName+=ent->deviceName().replace("/", "");
		entryName+=ent->mountPoint().replace("/","");

		ent->setUniqueIdentifier(entryName);
		if(cfg.hasKey(entryName))
		{
			ent->setUserDescription(cfg.readEntry(entryName));
		}
		
		QString filename = KURL(ent->deviceName()).fileName();
		if(!filename.isEmpty())
			filename = QString::fromLatin1(" (") + filename + ")";
		else
			filename=" ";
		if (ent->mounted())
		{
			mountList<<(entryName);
			QString name = ent->niceDescription();

			if(!ent->userDescription().isEmpty())
			{
				name = ent->userDescription();
			}
			else
			{
				if (descriptionToDeviceMap[ent->niceDescription()] != ent->deviceName())
					name += filename;
				if (descriptionToMountMap[ent->niceDescription()] != ent->mountPoint())
					name = i18n("%1 [%2]").arg(name).arg(ent->mountPoint());
			}
			mountList<<name;
			mountList<<ent->deviceName();
			mountList<<"file:/"+(ent->mountPoint().startsWith("/")?ent->mountPoint().right(ent->mountPoint().length()-1):ent->mountPoint());
			mountList<< ent->discType()+"_mounted";
			mountList<<"true";
			mountList<<"---";
			fileList<<KURL(QString("devices:/")+entryName);
		}
		else
		{
			mountList<<entryName;
			QString name = ent->niceDescription();

			if(!ent->userDescription().isEmpty())
			{
				name = ent->userDescription();
			}
			else
			{
				if (descriptionToDeviceMap[ent->niceDescription()] != ent->deviceName())
					name += filename;
				if (descriptionToMountMap[ent->niceDescription()] != ent->mountPoint())
					name = i18n("%1 [%2]").arg(name).arg(ent->mountPoint());
			}
			mountList<<name;
			mountList<<ent->deviceName();
			mountList<<"file:/"+(ent->mountPoint().startsWith("/")?ent->mountPoint().right(ent->mountPoint().length()-1):ent->mountPoint());
			mountList<< ent->discType()+"_unmounted";
			mountList<<"false";
			mountList<<"---";
			fileList<<KURL(QString("devices:/")+entryName);
		}
	}
	completeList=mountList;
	for (EntryMap::iterator it=mEntryMap.begin();it!=mEntryMap.end();++it)
	{
		completeList<<it.data().id;
		completeList<<it.data().description;
		completeList<<" ";
		completeList<<it.data().url;
		completeList<<it.data().mimeType;
		completeList<<(it.data().mountState?"true":"false");
		completeList<<"---";

	}
	bool triggerUpdate=false;
	if (completeList.count()!=oldcompleteList.count())
		triggerUpdate=true;
	else
	{
		QStringList::iterator it1=completeList.begin();
		QStringList::iterator it2=oldcompleteList.begin();
		while (it1!=completeList.end())
		{
			if ((*it1)!=(*it2)) {
				triggerUpdate=true;
				break;
			}
			++it1;
			++it2;
		}
	}

	if (triggerUpdate)
	{
	        KDirNotify_stub allDirNotify("*", "KDirNotify*");
	        allDirNotify.FilesAdded( KURL( "devices:/" ) );
	        allDirNotify.FilesAdded( KURL( "system:/" ) );
	} else
		kdDebug()<<" kiodevices No Update needed"<<endl;
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

bool MountWatcherModule::setDisplayName(const QString& oldName, const QString& newName)
{
	DiskEntry *to_rename = 0L;
	for (DiskEntry *ent=mDiskList.first();ent;ent=mDiskList.next())
	{
		if (ent->niceDescription()==oldName)
		{
			to_rename = ent;
		}
		else if (ent->niceDescription()==newName)
		{
			return false;
		}
	}
	
	if (to_rename!=0L)
	{
		to_rename->setUserDescription(newName);

		KConfig cfg("mountwatcherrc");

		cfg.setGroup("Labels");
		cfg.writeEntry(to_rename->uniqueIdentifier(), newName);
		
		cfg.sync();
		
		readDFDone();
		return true;
	}
	
	return false;
}

extern "C" {
    KDEDModule *create_mountwatcher(const QCString &obj)
    {
        KGlobal::locale()->insertCatalogue("kio_devices");
        return new MountWatcherModule(obj);
    }
}

#include "mountwatcher.moc"
