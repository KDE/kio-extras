/* This file is part of the KDE Project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "fstabbackend.h"

#include <klocale.h>
#include <kdirwatch.h>
#include <kurl.h>
#include <kmountpoint.h>
#include <kstandarddirs.h>

#ifdef _OS_SOLARIS_
#define FSTAB "/etc/vfstab"
#define MTAB "/etc/mnttab"
#else
#define FSTAB "/etc/fstab"
#define MTAB "/etc/mtab"
#endif



FstabBackend::FstabBackend(MediaList &list)
	: QObject(), BackendBase(list)
{
	KDirWatch::self()->addFile(MTAB);
	KDirWatch::self()->addFile(FSTAB);

	connect( KDirWatch::self(), SIGNAL( dirty(const QString&) ),
	         this, SLOT( slotDirty(const QString&) ) );
	KDirWatch::self()->startScan();

	connect( KDirWatch::self(), SIGNAL( dirty(const QString&) ),
	         this, SLOT( slotDirty(const QString&) ) );

	handleFstabChange();
	handleMtabChange();

	KDirWatch::self()->startScan();
}

FstabBackend::~FstabBackend()
{
	QStringList::iterator it = m_mtabIds.begin();
	QStringList::iterator end = m_mtabIds.end();

	for (; it!=end; ++it)
	{
		m_mediaList.removeMedium(*it);
	}

	it = m_fstabIds.begin();
	end = m_fstabIds.end();

	for (; it!=end; ++it)
	{
		m_mediaList.removeMedium(*it);
	}
}

void FstabBackend::slotDirty(const QString &path)
{
	if (path==MTAB)
	{
		handleMtabChange();
	}
	else if (path==FSTAB)
	{
		handleFstabChange();
	}
}

bool inExclusionPattern(KMountPoint *mount)
{
	if ( mount->mountType() == "swap"
	  || mount->mountType() == "tmpfs"
	  || mount->mountType() == "sysfs"
	  || mount->mountType() == "usbfs"
	  || mount->mountType() == "proc"
	  || mount->mountType() == "unknown"
	  || mount->mountedFrom() == "none"
	  || mount->mountedFrom() == "tmpfs"
	  || mount->mountedFrom().find("shm") != -1
	  || mount->mountPoint() == "/dev/swap"
	  || mount->mountPoint() == "/dev/pts"
	  || mount->mountPoint().find("/proc") == 0
	   )
	{
		return true;
	}
	else
	{
		return false;
	}
}


void FstabBackend::handleMtabChange()
{
	QStringList new_mtabIds;
	KMountPoint::List mtab = KMountPoint::currentMountPoints();

	KMountPoint::List::iterator it = mtab.begin();
	KMountPoint::List::iterator end = mtab.end();

	for (; it!=end; ++it)
	{
		QString dev = (*it)->mountedFrom();
		QString mp = (*it)->mountPoint();
		QString fs = (*it)->mountType();

		if ( ::inExclusionPattern(*it) ) continue;

		QString id = generateId(dev, mp);
		new_mtabIds+=id;

		if ( !m_mtabIds.contains(id) && m_fstabIds.contains(id) )
		{
			QString mime, icon, label;
			guess(dev, mp, fs, true, mime, icon, label);

			m_mediaList.changeMediumState(id, true, mime,
			                              icon, label);
		}
#if 0
		else if ( !m_mtabIds.contains(id) )
		{
			QString name = generateName(dev);

			Medium *m = new Medium(id, name);

			m->mountableState(dev, mp, fs, true);

			QString mime, icon, label;
			guess(dev, mp, fs, true, mime, icon, label);

			m->setMimeType(mime);
			m->setIconName(icon);
			m->setLabel(label);

			m_mediaList.addMedium(m);
		}
#endif
	}

	QStringList::iterator it2 = m_mtabIds.begin();
	QStringList::iterator end2 = m_mtabIds.end();

	for (; it2!=end2; ++it2)
	{
		if ( !new_mtabIds.contains(*it2) && m_fstabIds.contains(*it2) )
		{
			const Medium *medium = m_mediaList.findById(*it2);

			QString dev = medium->deviceNode();
			QString mp = medium->mountPoint();
			QString fs = medium->fsType();

			QString mime, icon, label;
			guess(dev, mp, fs, false, mime, icon, label);

			m_mediaList.changeMediumState(*it2, false, mime,
			                              icon, label);
		}
#if 0
		else if ( !new_mtabIds.contains(*it2) )
		{
			m_mediaList.removeMedium(*it2);
		}
#endif
	}

	m_mtabIds = new_mtabIds;
}

void FstabBackend::handleFstabChange()
{
	QStringList new_fstabIds;
	KMountPoint::List fstab = KMountPoint::possibleMountPoints();

	KMountPoint::List::iterator it = fstab.begin();
	KMountPoint::List::iterator end = fstab.end();

	for (; it!=end; ++it)
	{
		QString dev = (*it)->mountedFrom();
		QString mp = (*it)->mountPoint();
		QString fs = (*it)->mountType();

		if ( ::inExclusionPattern(*it) ) continue;

		QString id = generateId(dev, mp);
		new_fstabIds+=id;

		if ( !m_fstabIds.contains(id) )
		{
			QString name = generateName(dev);

			Medium *m = new Medium(id, name);

			m->mountableState(dev, mp, fs, false);

			QString mime, icon, label;
			guess(dev, mp, fs, false, mime, icon, label);

			m->setMimeType(mime);
			m->setIconName(icon);
			m->setLabel(label);

			m_mediaList.addMedium(m);
		}
	}

	QStringList::iterator it2 = m_fstabIds.begin();
	QStringList::iterator end2 = m_fstabIds.end();

	for (; it2!=end2; ++it2)
	{
		if ( !new_fstabIds.contains(*it2) )
		{
			m_mediaList.removeMedium(*it2);
		}
	}

	m_fstabIds = new_fstabIds;
}

QString FstabBackend::generateId(const QString &devNode,
                                 const QString &mountPoint)
{
	QString d = KStandardDirs::realFilePath(devNode);
	QString m = KStandardDirs::realPath(mountPoint);

	return "/org/kde/mediamanager/fstab/"
	      +d.replace("/", "")
	      +m.replace("/", "");
}

QString FstabBackend::generateName(const QString &devNode)
{
	return KURL(devNode).fileName();
}

void FstabBackend::guess(const QString &devNode, const QString &mountPoint,
                         const QString &fsType, bool mounted,
                         QString &mimeType, QString &iconName, QString &label)
{
	if ( devNode.find("cdwriter")!=-1 || mountPoint.find("cdwriter")!=-1
	  || devNode.find("cdrecorder")!=-1 || mountPoint.find("cdrecorder")!=-1
	  || devNode.find("cdburner")!=-1 || mountPoint.find("cdburner")!=-1
	  || devNode.find("cdrw")!=-1 || mountPoint.find("cdrw")!=-1
	   )
	{
		mimeType = "media/cdwriter";
		label = i18n("CD Recorder");
	}
	else if ( devNode.find("cdrom")!=-1 || mountPoint.find("cdrom")!=-1
	       // LINUX SPECIFIC
	       || devNode.find("/dev/scd")!=-1 || devNode.find("/dev/sr")!=-1
	       // FREEBSD SPECIFIC
	       || devNode.find("/acd")!=-1 || devNode.find("/scd")!=-1
	        )
	{
		mimeType = "media/cdrom";
		label = i18n("CD-ROM");
	}
	else if ( devNode.find("dvd")!=-1 || mountPoint.find("dvd")!=-1 )
	{
		mimeType = "media/dvd";
		label = i18n("DVD");
	}
	else if ( devNode.find("fd")!=-1 )
	{
		if ( devNode.find("360")!=-1 || devNode.find("1200")!=-1 )
		{
			mimeType = "media/floppy5";
		}
		else
		{
			mimeType = "media/floppy";
		}
		label = i18n("Floppy");
	}
	else if ( mountPoint.find("zip")!=-1
	       // FREEBSD SPECIFIC
	       || devNode.find("/afd")!=-1
	        )
	{
		mimeType = "media/zip";
		label = i18n("Zip Disk");
	}
	else if ( mountPoint.find("removable")!=-1
	       || mountPoint.find("hotplug")!=-1
	       || mountPoint.find("usb")!=-1
	       || mountPoint.find("firewire")!=-1
	       || mountPoint.find("ieee1394")!=-1
	        )
	{
		mimeType = "media/removable";
		label = i18n("Removable Device");
	}
	else if ( fsType.find("nfs")!=-1 )
	{
		mimeType = "media/nfs";
		label = i18n("Remote Share");
	}
	else if ( fsType.find("smb")!=-1 || fsType.find("cifs")!=-1
	       || devNode.find("//")!=-1 )
	{
		mimeType = "media/smb";
		label = i18n("Remote Share");
	}
	else
	{
		mimeType = "media/hdd";
		label = i18n("Hard Disk");
	}

	if ( mimeType=="media/nfs" || mimeType=="media/smb" )
	{
		label+= " (" + devNode + ")";
	}
	else
	{
		QString tmp = devNode;
		if ( tmp.startsWith("/dev/") )
		{
			tmp = tmp.mid(5);
		}
		label+= " (" + tmp + ")";
	}
	mimeType+= (mounted ? "_mounted" : "_unmounted");
	iconName = QString::null;
}

#include "fstabbackend.moc"
