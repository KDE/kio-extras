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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "removablebackend.h"

#include <klocale.h>
#include <kdirwatch.h>
#include <kurl.h>
#include <kmountpoint.h>
#include <kstandarddirs.h>

#ifdef _OS_SOLARIS_
#define MTAB "/etc/mnttab"
#else
#define MTAB "/etc/mtab"
#endif



RemovableBackend::RemovableBackend(MediaList &list)
	: QObject(), BackendBase(list)
{
	KDirWatch::self()->addFile(MTAB);

	connect( KDirWatch::self(), SIGNAL( dirty(const QString&) ),
	         this, SLOT( slotDirty(const QString&) ) );
	KDirWatch::self()->startScan();
}

RemovableBackend::~RemovableBackend()
{
	QStringList::iterator it = m_removableIds.begin();
	QStringList::iterator end = m_removableIds.end();

	for (; it!=end; ++it)
	{
		m_mediaList.removeMedium(*it);
	}
}

bool RemovableBackend::plug(const QString &devNode, const QString &label)
{
	QString name = generateName(devNode);
	QString id = generateId(devNode);

	if (!m_removableIds.contains(id))
	{
		Medium *medium = new Medium(id, name);
		medium->mountableState(devNode, QString::null,
		                       QString::null, false);

		QStringList words = QStringList::split(" ", label);
		
		QStringList::iterator it = words.begin();
		QStringList::iterator end = words.end();

		QString tmp = (*it).lower();
		tmp[0] = tmp[0].upper();
		QString new_label = tmp;
		
		++it;
		for (; it!=end; ++it)
		{
			tmp = (*it).lower();
			tmp[0] = tmp[0].upper();
			new_label+= " "+tmp;
		}
		
		medium->setLabel(new_label);
		medium->setMimeType("media/removable_unmounted");

		m_removableIds.append(id);
		return !m_mediaList.addMedium(medium).isNull();
	}
	return false;
}

bool RemovableBackend::unplug(const QString &devNode)
{
	QString id = generateId(devNode);
	if (m_removableIds.contains(id))
	{
		m_removableIds.remove(id);
		return m_mediaList.removeMedium(id);
	}
	return false;
}

bool RemovableBackend::camera(const QString &devNode)
{
	QString id = generateId(devNode);
	if (m_removableIds.contains(id))
	{
		return m_mediaList.changeMediumState(id,
			QString("camera:/"), "media/gphoto2camera");
	}
	return false;
}

void RemovableBackend::slotDirty(const QString &path)
{
	if (path==MTAB)
	{
		handleMtabChange();
	}
}


void RemovableBackend::handleMtabChange()
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

		QString id = generateId(dev);
		new_mtabIds+=id;

		if ( !m_mtabIds.contains(id)
		  && m_removableIds.contains(id) )
		{
			m_mediaList.changeMediumState(id, dev, mp, fs, true,
				"media/removable_mounted");
		}
	}

	QStringList::iterator it2 = m_mtabIds.begin();
	QStringList::iterator end2 = m_mtabIds.end();

	for (; it2!=end2; ++it2)
	{
		if ( !new_mtabIds.contains(*it2)
		  && m_removableIds.contains(*it2) )
		{
			m_mediaList.changeMediumState(*it2, false,
				"media/removable_unmounted");
		}
	}

	m_mtabIds = new_mtabIds;
}

QString RemovableBackend::generateId(const QString &devNode)
{
	QString dev = KStandardDirs::realFilePath(devNode);

	return "/org/kde/mediamanager/removable/"
	      +dev.replace("/", "");
}

QString RemovableBackend::generateName(const QString &devNode)
{
	return KURL(devNode).fileName();
}

#include "removablebackend.moc"
