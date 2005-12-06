/* This file is part of the KDE Project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

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

#ifndef _FSTABBACKEND_H_
#define _FSTABBACKEND_H_

#include "backendbase.h"

#include <qobject.h>
#include <qstringlist.h>

#ifdef Q_OS_FREEBSD
#include <qtimer.h>
#endif

class FstabBackend : public QObject, public BackendBase
{
Q_OBJECT

public:
	FstabBackend(MediaList &list, bool networkSharesOnly = false);
	virtual ~FstabBackend();

	static void guess(const QString &devNode, const QString &mountPoint,
                          const QString &fsType, bool mounted,
                          QString &mimeType, QString &iconName,
	                  QString &label);
private slots:
	void slotDirty(const QString &path);
	void handleFstabChange(bool allowNotification = true);
	void handleMtabChange(bool allowNotification = true);

private:
	static QString generateId(const QString &devNode,
	                          const QString &mountPoint);
	static QString generateName(const QString &devNode,
	                            const QString &fsType);

	bool m_networkSharesOnly;
	QStringList m_mtabIds;
	QStringList m_fstabIds;
#ifdef Q_OS_FREEBSD
	QTimer m_mtabTimer;
#endif
};

#endif
