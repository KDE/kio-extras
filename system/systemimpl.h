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

#ifndef SYSTEMIMPL_H
#define SYSTEMIMPL_H

#include <kio/global.h>
#include <kio/job.h>
#include <kurl.h>
#include <dcopobject.h>

#include <qobject.h>
#include <qstring.h>

class SystemImpl : public QObject
{
Q_OBJECT
public:
	SystemImpl();

	void createTopLevelEntry(KIO::UDSEntry& entry) const;

	bool listRoot(QValueList<KIO::UDSEntry> &list);

	KURL findBaseURL(const QString &filename) const;

	int lastErrorCode() const { return m_lastErrorCode; }
	QString lastErrorMessage() const { return m_lastErrorMessage; }

private slots:
	void slotEntries(KIO::Job *job, const KIO::UDSEntryList &list);
	void slotResult(KIO::Job *job);

private:
	void createEntry(KIO::UDSEntry& entry, const QString &directory,
	                 const QString &file);

	bool m_lastListingEmpty;

	/// Last error code stored in class to simplify API.
	/// Note that this means almost no method can be const.
	int m_lastErrorCode;
	QString m_lastErrorMessage;
};

#endif
