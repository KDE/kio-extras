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

#ifndef _MEDIAIMPL_H_
#define _MEDIAIMPL_H_

#include <kio/global.h>
#include <kio/job.h>
#include <kurl.h>
#include <dcopobject.h>

#include <qobject.h>
#include <qstring.h>

#include "medium.h"

class MediaImpl : public QObject, public DCOPObject
{
Q_OBJECT
K_DCOP
public:
	MediaImpl();
	bool parseURL(const KUrl &url, QString &name, QString &path) const;
	bool realURL(const QString &name, const QString &path, KUrl &url);

	bool statMedium(const QString &name, KIO::UDSEntry &entry);
	bool statMediumByLabel(const QString &label, KIO::UDSEntry &entry);
	bool listMedia(KIO::UDSEntryList &list);
	bool setUserLabel(const QString &name, const QString &label);

	void createTopLevelEntry(KIO::UDSEntry& entry) const;

	int lastErrorCode() const { return m_lastErrorCode; }
	QString lastErrorMessage() const { return m_lastErrorMessage; }

k_dcop:
	void slotMediumChanged(const QString &name);

signals:
	void warning(const QString &msg);
	void leaveModality();

private slots:
	void slotWarning(KJob *job, const QString &msg);
	void slotMountResult(KJob *job);
	void slotStatResult(KJob *job);

private:
	const Medium findMediumByName(const QString &name, bool &ok);
	bool ensureMediumMounted(Medium &medium);

	void extractUrlInfos(const KUrl &url, KIO::UDSEntry& infos);
	KIO::UDSEntry m_entryBuffer;

	void createMediumEntry(KIO::UDSEntry& entry,
	                       const Medium &medium);
	void enterLoop();

	Medium *mp_mounting;

	/// Last error code stored in class to simplify API.
	/// Note that this means almost no method can be const.
	int m_lastErrorCode;
	QString m_lastErrorMessage;
};

#endif
