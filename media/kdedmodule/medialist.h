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

#ifndef _MEDIALIST_H_
#define _MEDIALIST_H_

#include <qobject.h>

#include "medium.h"

class MediaList : public QObject
{
Q_OBJECT

public:
	MediaList();

	// FIXME: should be <const Medium> or something similar...
	const QPtrList<Medium> list() const;
	const Medium *findById(const QString &id) const;
	const Medium *findByName(const QString &name) const;

public:
	QString addMedium(Medium *medium);
	bool removeMedium(const QString &id);

	bool changeMediumState(const Medium &medium);
	bool changeMediumState(const QString &id,
	                       const QString &baseURL,
	                       const QString &mimeType = QString::null,
	                       const QString &iconName = QString::null,
	                       const QString &label = QString::null);
	bool changeMediumState(const QString &id,
	                       const QString &deviceNode,
	                       const QString &mountPoint,
	                       const QString &fsType, bool mounted,
	                       const QString &mimeType = QString::null,
	                       const QString &iconName = QString::null,
	                       const QString &label = QString::null);
	bool changeMediumState(const QString &id, bool mounted,
	                       const QString &mimeType = QString::null,
	                       const QString &iconName = QString::null,
	                       const QString &label = QString::null);

	bool setUserLabel(const QString &name, const QString &label);

signals:
	void mediumAdded(const QString &id, const QString &name);
	void mediumRemoved(const QString &id, const QString &name);
	void mediumStateChanged(const QString &id, const QString &name);

private:
	QPtrList<Medium> m_media;
	QMap<QString,Medium*> m_nameMap;
	QMap<QString,Medium*> m_idMap;
};

#endif
