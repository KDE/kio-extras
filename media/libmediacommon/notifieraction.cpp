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

#include "notifieraction.h"

#include <qfile.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kicontheme.h>

NotifierAction::NotifierAction()
{
}

NotifierAction::~NotifierAction()
{
}

void NotifierAction::setIconName(const QString &iconName)
{
	m_iconName = iconName;
}

void NotifierAction::setLabel(const QString &label)
{
	m_label = label;
}

QString NotifierAction::iconName() const
{
	return m_iconName;
}

QPixmap NotifierAction::pixmap() const
{
	QFile f( m_iconName );
	
	if ( f.exists() )
	{
		return QPixmap( m_iconName );
	}
	else
	{
		QString path = KGlobal::iconLoader()->iconPath( m_iconName, -32 );
		return QPixmap( path );
	}
}

QString NotifierAction::label() const
{
	return m_label;
}

void NotifierAction::addAutoMimetype( const QString &mimetype )
{
	if ( !m_autoMimetypes.contains( mimetype ) )
	{
		m_autoMimetypes.append( mimetype );
	}
}

void NotifierAction::removeAutoMimetype( const QString &mimetype )
{
	m_autoMimetypes.remove( mimetype );
}

QStringList NotifierAction::autoMimetypes()
{
	return m_autoMimetypes;
}

bool NotifierAction::isWritable() const
{
	return false;
}

bool NotifierAction::supportsMimetype(const QString &/*mimetype*/) const
{
	return true;
}

