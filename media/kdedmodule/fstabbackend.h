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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _FSTABBACKEND_H_
#define _FSTABBACKEND_H_

#include "backendbase.h"

#include <qobject.h>
#include <qstringlist.h>

class FstabBackend : public QObject, public BackendBase
{
Q_OBJECT

public:
	FstabBackend(MediaList &list);

	static void guess(const QString &devNode, const QString &mountPoint,
                          const QString &fsType, bool mounted,
                          QString &mimeType, QString &iconName,
	                  QString &label);
private slots:
	void slotDirty(const QString &path);

private:
	void handleFstabChange();
	void handleMtabChange();

	static QString generateId(const QString &devNode,
	                          const QString &mountPoint);
	static QString generateName(const QString &devNode);

	QStringList m_mtabIds;
	QStringList m_fstabIds;
};

#endif
