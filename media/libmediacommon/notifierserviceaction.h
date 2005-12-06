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

#ifndef _NOTIFIERSERVICEACTION_H_
#define _NOTIFIERSERVICEACTION_H_

#include <kmimetype.h>
#include <qstring.h>

#include "notifieraction.h"

class NotifierServiceAction : public NotifierAction
{
public:
	NotifierServiceAction();
	virtual QString id() const;
	virtual void execute(KFileItem &item);

	virtual void setIconName( const QString &icon );
	virtual void setLabel( const QString &label );
	
	void setService(KDEDesktopMimeType::Service service);
	KDEDesktopMimeType::Service service() const;
	
	void setFilePath(const QString &filePath);
	QString filePath() const;
	
	void setMimetypes(const QStringList &mimetypes);
	QStringList mimetypes();
	
	virtual bool isWritable() const;
	virtual bool supportsMimetype(const QString &mimetype) const;

	void save() const;

private:
	void updateFilePath();

	KDEDesktopMimeType::Service m_service;
	QString m_filePath;
	QStringList m_mimetypes;
};

#endif

