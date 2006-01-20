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

#ifndef _MEDIALIST_H_
#define _MEDIALIST_H_

#include <qobject.h>
#include <qmap.h>
//Added by qt3to4:
#include <Q3PtrList>

#include "medium.h"

class MediaList : public QObject
{
Q_OBJECT

public:
	MediaList();

	// FIXME: should be <const Medium> or something similar...
	const Q3PtrList<Medium> list() const;
	const Medium *findById(const QString &id) const;
	const Medium *findByName(const QString &name) const;

public:
	QString addMedium(Medium *medium, bool allowNotification = true);
	bool removeMedium(const QString &id, bool allowNotification = true);

	bool changeMediumState(const Medium &medium, bool allowNotification);
	bool changeMediumState(const QString &id,
	                       const QString &baseURL,
	                       bool allowNotification = true,
	                       const QString &mimeType = QString(),
	                       const QString &iconName = QString(),
	                       const QString &label = QString());
	bool changeMediumState(const QString &id,
	                       const QString &deviceNode,
	                       const QString &mountPoint,
	                       const QString &fsType, bool mounted,
	                       bool allowNotification = true,
	                       const QString &mimeType = QString(),
	                       const QString &iconName = QString(),
	                       const QString &label = QString());
	bool changeMediumState(const QString &id, bool mounted,
	                       bool allowNotification = true,
	                       const QString &mimeType = QString(),
	                       const QString &iconName = QString(),
	                       const QString &label = QString());

	bool setUserLabel(const QString &name, const QString &label);

Q_SIGNALS:
	void mediumAdded(const QString &id, const QString &name,
	                 bool allowNotification);
	void mediumRemoved(const QString &id, const QString &name,
	                   bool allowNotification);
	void mediumStateChanged(const QString &id, const QString &name,
	                        bool mounted, bool allowNotification);

private:
	Q3PtrList<Medium> m_media;
	QMap<QString,Medium*> m_nameMap;
	QMap<QString,Medium*> m_idMap;
};

#endif
