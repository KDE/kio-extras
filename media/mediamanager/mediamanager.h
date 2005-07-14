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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _MEDIAMANAGER_H_
#define _MEDIAMANAGER_H_

#include <kdedmodule.h>
#include <qstring.h>
#include <qstringlist.h>

#include "medialist.h"
#include "backendbase.h"
#include "removablebackend.h"
#include "mediadirnotify.h"


class MediaManager : public KDEDModule
{
Q_OBJECT
K_DCOP
public:
	MediaManager(const QCString &obj);
	~MediaManager();

k_dcop:
	QStringList fullList();
	QStringList properties(const QString &name);
	QString nameForLabel(const QString &label);
	ASYNC setUserLabel(const QString &name, const QString &label);

	ASYNC reloadBackends();

	// Removable media handling (for people not having HAL)
	bool removablePlug(const QString &devNode, const QString &label);
	bool removableUnplug(const QString &devNode);
	bool removableCamera(const QString &devNode);

k_dcop_signals:
	void mediumAdded(const QString &name);
	void mediumRemoved(const QString &name);
	void mediumChanged(const QString &name);

private slots:
	void loadBackends();
	
	void slotMediumAdded(const QString &id, const QString &name);
	void slotMediumRemoved(const QString &id, const QString &name);
	void slotMediumChanged(const QString &id, const QString &name,
	                       bool mounted);

private:
	MediaList m_mediaList;
	QValueList<BackendBase*> m_backends;
	RemovableBackend *mp_removableBackend;
	MediaDirNotify m_dirNotify;
};

#endif
