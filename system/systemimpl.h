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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef SYSTEMIMPL_H
#define SYSTEMIMPL_H

#include <kio/global.h>
#include <kio/job.h>
#include <kurl.h>
#include <dcopobject.h>

#include <qobject.h>
#include <qstring.h>
//Added by qt3to4:
#include <Q3ValueList>

class SystemImpl : public QObject
{
Q_OBJECT
public:
	SystemImpl();

	void createTopLevelEntry(KIO::UDSEntry& entry) const;
	bool statByName(const QString &filename, KIO::UDSEntry& entry);

	bool listRoot(KIO::UDSEntryList &list);

	bool parseURL(const KURL &url, QString &name, QString &path) const;
	bool realURL(const QString &name, const QString &path, KURL &url) const;

	int lastErrorCode() const { return m_lastErrorCode; }
	QString lastErrorMessage() const { return m_lastErrorMessage; }

signals:
    void leaveModality();

private slots:
	KURL findBaseURL(const QString &filename) const;
	void slotEntries(KIO::Job *job, const KIO::UDSEntryList &list);
	void slotResult(KIO::Job *job);

private:
	void createEntry(KIO::UDSEntry& entry, const QString &directory,
	                 const QString &file);
	void enterLoop();

	bool m_lastListingEmpty;

	/// Last error code stored in class to simplify API.
	/// Note that this means almost no method can be const.
	int m_lastErrorCode;
	QString m_lastErrorMessage;
};

#endif
