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
#include <kio/netaccess.h>
#include <kfileitem.h>

#include "notificationdialog.h"
#include "notifiersettings.h"
#include "notifieraction.h"

MediaNotifier::MediaNotifier(const QCString &name) : KDEDModule(name)
{
	connectDCOPSignal( "kded", "mediamanager", "mediumAdded(QString, bool)",
	                   "onMediumAdded(QString, bool)", true );
	
	connectDCOPSignal( "kded", "mediamanager", "mediumChanged(QString, bool)",
	                   "onMediumChanged(QString, bool)", true );
}

MediaNotifier::~MediaNotifier()
{
}

void MediaNotifier::onMediumAdded( const QString &name, bool allowNotification )
{
	kdDebug() << "MediaNotifier::onMediumAdded( " << name << ", "
	          << allowNotification << ")" << endl;
	
	if (  allowNotification ) notify(  name );
}

void MediaNotifier::onMediumChanged( const QString &name, bool allowNotification )
{
	kdDebug() << "MediaNotifier::onMediumChanged( " << name << ", "
	          << allowNotification << ")" << endl;

	if ( allowNotification ) notify( name );
}

void MediaNotifier::notify( const QString &name )
{
	kdDebug() << "Notification triggered." << endl;

	KURL url( "system:/media/"+name );
	
	KIO::UDSEntry entry;
	bool res = KIO::NetAccess::stat( url, entry, 0L );
	if ( !res ) return;
	
	KFileItem medium( entry, url );
	
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
	KDE_EXPORT KDEDModule *create_medianotifier(const QCString &name)
	{
		return new MediaNotifier(name);
	}
}

#include "medianotifier.moc"
