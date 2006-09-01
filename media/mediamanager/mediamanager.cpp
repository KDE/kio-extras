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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "mediamanager.h"
#include <config-media.h>
#include <config.h>
#include <QTimer>
#include <Q3PtrList>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include <kdirnotify.h>

#include "mediamanagersettings.h"
#include "mediamanageradaptor.h"

#include "fstabbackend.h"

#ifdef COMPILE_HALBACKEND
#include "halbackend.h"
#endif //COMPILE_HALBACKEND

#ifdef COMPILE_LINUXCDPOLLING
#include "linuxcdpolling.h"
#endif //COMPILE_LINUXCDPOLLING


MediaManager::MediaManager()
    : KDEDModule(), m_dirNotify(m_mediaList)
{
	new MediaManagerAdaptor( this );
	
	connect( &m_mediaList, SIGNAL(mediumAdded(const QString&, const QString&, bool)),
	         SLOT(slotMediumAdded(const QString&, const QString&, bool)) );
	connect( &m_mediaList, SIGNAL(mediumRemoved(const QString&, const QString&, bool)),
	         SLOT(slotMediumRemoved(const QString&, const QString&, bool)) );
	connect( &m_mediaList,
	         SIGNAL(mediumStateChanged(const QString&, const QString&, bool, bool)),
	         SLOT(slotMediumChanged(const QString&, const QString&, bool, bool)) );

	QTimer::singleShot( 10, this, SLOT( loadBackends() ) );
}

MediaManager::~MediaManager()
{
	while ( !m_backends.isEmpty() )
	{
		BackendBase *b = m_backends.first();
		m_backends.removeAll( b );
		delete b;
	}
}

void MediaManager::loadBackends()
{
    m_mediaList.blockSignals(true);

	while ( !m_backends.isEmpty() )
	{
		BackendBase *b = m_backends.first();
		m_backends.removeAll( b );
		delete b;
	}

	mp_removableBackend = 0L;

#ifdef COMPILE_HALBACKEND
	if ( MediaManagerSettings::self()->halBackendEnabled() )
	{
		HALBackend* hal_backend = new HALBackend(m_mediaList, this);
		if (hal_backend->InitHal())
		{
			m_backends.append( hal_backend );
			m_backends.append( new FstabBackend(m_mediaList, true) );
			// No need to load something else...
                        m_mediaList.blockSignals(false);
			return;
		}
		else
		{
			delete hal_backend;
		}
	}
#endif // COMPILE_HALBACKEND

	mp_removableBackend = new RemovableBackend(m_mediaList);
	m_backends.append( mp_removableBackend );

#ifdef COMPILE_LINUXCDPOLLING
	if ( MediaManagerSettings::self()->cdPollingEnabled() )
	{
		m_backends.append( new LinuxCDPolling(m_mediaList) );
	}
#endif //COMPILE_LINUXCDPOLLING

	m_backends.append( new FstabBackend(m_mediaList) );
        m_mediaList.blockSignals(false);
}


QStringList MediaManager::fullList()
{
	Q3PtrList<Medium> list = m_mediaList.list();

	QStringList result;

	Q3PtrList<Medium>::const_iterator it = list.begin();
	Q3PtrList<Medium>::const_iterator end = list.end();
	for (; it!=end; ++it)
	{
		result+= (*it)->properties();
		result+= Medium::SEPARATOR;
	}

	return result;
}

QStringList MediaManager::properties(const QString &name)
{
	const Medium *m = m_mediaList.findByName(name);

	if (m!=0L)
	{
		return m->properties();
	}
	else
	{
		return QStringList();
	}
}

QString MediaManager::nameForLabel(const QString &label)
{
	const Q3PtrList<Medium> media = m_mediaList.list();

	Q3PtrList<Medium>::const_iterator it = media.begin();
	Q3PtrList<Medium>::const_iterator end = media.end();
	for (; it!=end; ++it)
	{
		const Medium *m = *it;

		if (m->prettyLabel()==label)
		{
			return m->name();
		}
	}

	return QString();
}

void MediaManager::setUserLabel(const QString &name, const QString &label)
{
	m_mediaList.setUserLabel(name, label);
}

void MediaManager::reloadBackends()
{
	MediaManagerSettings::self()->readConfig();
	loadBackends();
}

bool MediaManager::removablePlug(const QString &devNode, const QString &label)
{
	if (mp_removableBackend)
	{
		return mp_removableBackend->plug(devNode, label);
	}
	return false;
}

bool MediaManager::removableUnplug(const QString &devNode)
{
	if (mp_removableBackend)
	{
		return mp_removableBackend->unplug(devNode);
	}
	return false;
}

bool MediaManager::removableCamera(const QString &devNode)
{
	if (mp_removableBackend)
	{
		return mp_removableBackend->camera(devNode);
	}
	return false;
}


void MediaManager::slotMediumAdded(const QString &/*id*/, const QString &name,
                                   bool allowNotification)
{
	kDebug(1219) << "MediaManager::slotMediumAdded: " << name << endl;

        org::kde::KDirNotify::emitFilesAdded( "media:/" );

	emit mediumAdded(name, allowNotification);
}

void MediaManager::slotMediumRemoved(const QString &/*id*/, const QString &name,
                                     bool allowNotification)
{
	kDebug(1219) << "MediaManager::slotMediumRemoved: " << name << endl;

        org::kde::KDirNotify::emitFilesRemoved( QStringList() << "media:/"+name );

	emit mediumRemoved(name, allowNotification);
}

void MediaManager::slotMediumChanged(const QString &/*id*/, const QString &name,
                                     bool mounted, bool allowNotification)
{
	kDebug(1219) << "MediaManager::slotMediumChanged: " << name << endl;

	if (!mounted)
	{
            org::kde::KDirNotify::emitFilesRemoved( QStringList() << "media:/"+name );
	}
        org::kde::KDirNotify::emitFilesChanged( QStringList() << "media:/"+name );

	emit mediumChanged(name, allowNotification);
}


extern "C" {
    KDE_EXPORT KDEDModule *create_mediamanager()
    {
        KGlobal::locale()->insertCatalog("kio_media");
        return new MediaManager();
    }
}

#include "mediamanager.moc"
