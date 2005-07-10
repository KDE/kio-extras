/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

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

#include "medianotifier.h"

#include <kdebug.h>

#include "notificationdialog.h"
#include "notifiersettings.h"
#include "notifieraction.h"

MediaNotifier::MediaNotifier(const QCString &name) : KDEDModule(name)
{
	kdDebug() << "Starting new service... " << endl;
	
	m_firstNewItems = true;
	m_mediaWatcher = new KDirLister();
	m_mediaWatcher->openURL(KURL("media:/"));
	
	connect( m_mediaWatcher, SIGNAL( newItems( const KFileItemList & ) ),
	         this, SLOT( slotMediaAdded( const KFileItemList& ) ) );
	connect( m_mediaWatcher, SIGNAL( completed() ),
	         this, SLOT( slotFirstListingDone() ) );
}

MediaNotifier::~MediaNotifier()
{
	kdDebug() << "Going away... " << endl;
	
	delete m_mediaWatcher;
}

void MediaNotifier::slotMediaAdded(const KFileItemList& medias)
{
	kdDebug() << "Media directory has changed." << endl;
	
	if ( m_firstNewItems == true )
	{
		return;
	}
	
	for ( KFileItemListIterator it(medias); it.current(); ++it )
	{
		kdDebug() << "Detected: " << it.current()->url() << endl;
		mediumDetected( **it );
	}
}

void MediaNotifier::slotFirstListingDone()
{
	m_firstNewItems = false;
}

void MediaNotifier::mediumDetected(KFileItem &medium)
{
	kdDebug() << "Notification received." << endl;

	NotifierSettings settings;
	
	if ( settings.autoActionForMimetype( medium.mimetype() )==0L )
	{
		NotificationDialog dialog( medium, settings );
		dialog.exec();
	}
	else
	{
		NotifierAction *action = settings.autoActionForMimetype( medium.mimetype() );
		action->execute( medium );
	}
}

extern "C"
{
	KDEDModule *create_medianotifier(const QCString &name)
	{
		return new MediaNotifier(name);
	}
}

#include "medianotifier.moc"
