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

#ifndef _MEDIAMANAGER_H_
#define _MEDIAMANAGER_H_

#include <kdedmodule.h>


#include "medialist.h"
#include "backendbase.h"
#include "removablebackend.h"
#include "mediadirnotify.h"

class MediaManager : public KDEDModule
{
Q_OBJECT
public:
	MediaManager();
	~MediaManager();

public Q_SLOTS:
	QStringList fullList();
	QStringList properties(const QString &name);
	QString nameForLabel(const QString &label);
	void setUserLabel(const QString &name, const QString &label);

	void reloadBackends();

	// Removable media handling (for people not having HAL)
	bool removablePlug(const QString &devNode, const QString &label);
	bool removableUnplug(const QString &devNode);
	bool removableCamera(const QString &devNode);

Q_SIGNALS:
	void mediumAdded(const QString &name, bool allowNotification);
	void mediumRemoved(const QString &name, bool allowNotification);
	void mediumChanged(const QString &name, bool allowNotification);

private Q_SLOTS:
	void loadBackends();

	void slotMediumAdded(const QString &id, const QString &name,
	                     bool allowNotification);
	void slotMediumRemoved(const QString &id, const QString &name,
	                       bool allowNotification);
	void slotMediumChanged(const QString &id, const QString &name,
	                       bool mounted, bool allowNotification);

private:
	MediaList m_mediaList;
	QList<BackendBase*> m_backends;
	RemovableBackend *mp_removableBackend;
	MediaDirNotify m_dirNotify;
};

#endif
