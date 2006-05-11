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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _NOTIFIERSETTINGS_H_
#define _NOTIFIERSETTINGS_H_


#include <QMap>

#include "notifieraction.h"
#include "notifierserviceaction.h"


class NotifierSettings
{
public:
	NotifierSettings();
	~NotifierSettings();

	QList<NotifierAction*> actions();
	QList<NotifierAction*> actionsForMimetype( const QString &mimetype );
	
	bool addAction( NotifierServiceAction *action );
	bool deleteAction( NotifierServiceAction *action );

	void setAutoAction( const QString &mimetype, NotifierAction *action );
	void resetAutoAction( const QString &mimetype );
	void clearAutoActions();
	NotifierAction *autoActionForMimetype( const QString &mimetype );

	const QStringList &supportedMimetypes();
	
	void reload();
	void save();
	
private:
	QList<NotifierServiceAction*> listServices( const QString &mimetype = QString() ) const;
	bool shouldLoadActions( KDesktopFile &desktop, const QString &mimetype ) const;
	QList<NotifierServiceAction*> loadActions( KDesktopFile &desktop ) const;

	QStringList m_supportedMimetypes;
	QList<NotifierAction*> m_actions;
	QList<NotifierServiceAction*> m_deletedActions;
	QMap<QString,NotifierAction*> m_idMap;
	QMap<QString,NotifierAction*> m_autoMimetypesMap;
};
#endif
