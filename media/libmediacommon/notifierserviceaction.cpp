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

#include "notifierserviceaction.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <klocale.h>

NotifierServiceAction::NotifierServiceAction()
	: NotifierAction()
{
	NotifierAction::setIconName("button_cancel");
	NotifierAction::setLabel(i18n("Unknown"));

	m_service.m_strName = "New Service";
	m_service.m_strIcon = "button_cancel";
	m_service.m_strExec = "konqueror %u";
}

QString NotifierServiceAction::id() const
{
	if (m_filePath.isEmpty() || m_service.m_strName.isEmpty())
	{
		return QString();
	}
	else
	{
		return "#Service:"+m_filePath;
	}
}

void NotifierServiceAction::setIconName( const QString &icon )
{
	m_service.m_strIcon = icon;
	NotifierAction::setIconName( icon );
}

void NotifierServiceAction::setLabel( const QString &label )
{
	m_service.m_strName = label;
	NotifierAction::setLabel( label );

	updateFilePath();
}

void NotifierServiceAction::execute(KFileItem &medium)
{
	KUrl::List urls = KUrl::List( medium.url() );
	KDEDesktopMimeType::executeService( urls, m_service );
}

void NotifierServiceAction::setService(KDEDesktopMimeType::Service service)
{
	NotifierAction::setIconName( service.m_strIcon );
	NotifierAction::setLabel( service.m_strName );

	m_service = service;

	updateFilePath();
}

KDEDesktopMimeType::Service NotifierServiceAction::service() const
{
	return m_service;
}

void NotifierServiceAction::setFilePath(const QString &filePath)
{
	m_filePath = filePath;
}

QString NotifierServiceAction::filePath() const
{
	return m_filePath;
}

void NotifierServiceAction::updateFilePath()
{
	if ( !m_filePath.isEmpty() ) return;

	QString action_name = m_service.m_strName;
	action_name.replace(   " ", "_" );

	QDir actions_dir( KStandardDirs::locateLocal( "data", "konqueror/servicemenus/", true ) );

	QString filename = actions_dir.absoluteFilePath( action_name + ".desktop" );

	int counter = 1;
	while ( QFile::exists( filename ) )
	{
		filename = actions_dir.absoluteFilePath( action_name
		                                  + QString::number( counter )
		                                  + ".desktop" );
		counter++;
	}

	m_filePath = filename;
}

void NotifierServiceAction::setMimetypes(const QStringList &mimetypes)
{
	m_mimetypes = mimetypes;
}

QStringList NotifierServiceAction::mimetypes()
{
	return m_mimetypes;
}

bool NotifierServiceAction::isWritable() const
{
	QFileInfo info( m_filePath );

	if ( info.exists() )
	{
		return info.isWritable();
	}
	else
	{
		info = QFileInfo( info.path() );
		return info.isWritable();
	}
}

bool NotifierServiceAction::supportsMimetype(const QString &mimetype) const
{
	return m_mimetypes.contains(mimetype);
}

void NotifierServiceAction::save() const
{
	QFile::remove( m_filePath );
	KDesktopFile desktopFile(m_filePath);

	desktopFile.setGroup(QString("Desktop Action ") + m_service.m_strName);
	desktopFile.writeEntry(QString("Icon"), m_service.m_strIcon);
	desktopFile.writeEntry(QString("Name"), m_service.m_strName);
	desktopFile.writeEntry(QString("Exec"), m_service.m_strExec);

	desktopFile.setDesktopGroup();

	desktopFile.writeEntry(QString("ServiceTypes"), m_mimetypes, ',');
	desktopFile.writeEntry(QString("Actions"),
	                       QStringList(m_service.m_strName),';');
}

