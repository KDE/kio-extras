/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _MEDIAIMPL_H_
#define _MEDIAIMPL_H_

#include <kio/global.h>
#include <kio/job.h>
#include <kurl.h>

#include <qobject.h>
#include <qstring.h>

#include "medium.h"

class MediaImpl : public QObject
{
Q_OBJECT
public:

	static bool parseURL(const KURL &url, QString &name, QString &path);

	KIO::StatJob *stat(const QString &name, const QString &path);
	bool statMedium(const QString &name, KIO::UDSEntry &entry);

	KIO::ListJob *list(const QString &name, const QString &path);
	bool listMedia(QValueList<KIO::UDSEntry> &list);



	void createTopLevelEntry(KIO::UDSEntry& entry) const;

	int lastErrorCode() const { return m_lastErrorCode; }
	QString lastErrorMessage() const { return m_lastErrorMessage; }

private slots:
	void slotMountResult(KIO::Job *job);

private:
	const Medium findMediumByName(const QString &name, bool &ok);
	bool ensureMediumMounted(const Medium &medium);

	void createMediumEntry(KIO::UDSEntry& entry,
	                       const Medium &medium) const;

	/// Last error code stored in class to simplify API.
	/// Note that this means almost no method can be const.
	int m_lastErrorCode;
	QString m_lastErrorMessage;
};

#endif
