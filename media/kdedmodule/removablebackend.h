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

#ifndef _REMOVABLEBACKEND_H_
#define _REMOVABLEBACKEND_H_

#include "backendbase.h"

#include <qobject.h>
#include <qstringlist.h>

class RemovableBackend : public QObject, public BackendBase
{
Q_OBJECT

public:
	RemovableBackend(MediaList &list);
	virtual ~RemovableBackend();

	void plug(const QString &devNode, const QString &label);
	void unplug(const QString &devNode);

private slots:
	void slotDirty(const QString &path);

private:
	void handleMtabChange();

	static QString generateId(const QString &devNode);
	static QString generateName(const QString &devNode);

	QStringList m_removableIds;
	QStringList m_mtabIds;
};

#endif
