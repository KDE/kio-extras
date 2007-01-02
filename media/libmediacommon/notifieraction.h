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

#ifndef _NOTIFIERACTION_H_
#define _NOTIFIERACTION_H_

#include <kfileitem.h>
#include <QString>
#include <QPixmap>

class NotifierSettings;
class KIconLoader;

class NotifierAction
{
public:
	NotifierAction();
	virtual ~NotifierAction();

	virtual QString label() const;
	virtual QString iconName() const;
	virtual void setLabel( const QString &label );
	virtual void setIconName( const QString &icon );

	QPixmap pixmap() const;
	
	QStringList autoMimetypes();

	virtual QString id() const = 0;
	virtual bool isWritable() const;
	virtual bool supportsMimetype( const QString &mimetype ) const;
	virtual void execute( KFileItem &medium ) = 0;

private:
	static KIconLoader* iconLoader();
	void addAutoMimetype( const QString &mimetype );
	void removeAutoMimetype( const QString &mimetype );

	QString m_label;
	QString m_iconName;
	QStringList m_autoMimetypes;
	static KIconLoader* s_iconLoader;

	friend class NotifierSettings;
};

#endif
