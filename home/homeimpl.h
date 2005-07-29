/* This file is part of the KDE project
   Copyright (c) 2005 Kevin Ottens <ervin ipsquad net>

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

#ifndef HOMEIMPL_H
#define HOMEIMPL_H

#include <kio/global.h>
#include <kio/job.h>
#include <kurl.h>
#include <kuser.h>

#include <qstring.h>
//Added by qt3to4:
#include <Q3ValueList>

class HomeImpl : public QObject
{
Q_OBJECT

public:
	HomeImpl();
	bool parseURL(const KURL &url, QString &name, QString &path) const;
	bool realURL(const QString &name, const QString &path, KURL &url);
		
	bool statHome(const QString &name, KIO::UDSEntry &entry);
	bool listHomes(Q3ValueList<KIO::UDSEntry> &list);
	
	void createTopLevelEntry(KIO::UDSEntry &entry) const;
	
	int lastErrorCode() const { return m_lastErrorCode; }
	QString lastErrorMessage() const { return m_lastErrorMessage; }

private slots:
	void slotStatResult(KIO::Job *job);
       void enterLoop();

signals:
           void leaveModality();

private:
	void createHomeEntry(KIO::UDSEntry& entry, const KUser &user);

	KIO::UDSEntry extractUrlInfos(const KURL &url);
	KIO::UDSEntry m_entryBuffer;
		
	
	/// Last error code stored in class to simplify API.
	/// Note that this means almost no method can be const.
	int m_lastErrorCode;
	QString m_lastErrorMessage;

	long m_effectiveUid;
};

#endif
