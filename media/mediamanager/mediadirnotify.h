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

#ifndef _MEDIADIRNOTIFY_H_
#define _MEDIADIRNOTIFY_H_

#include <kurl.h>

#include "medialist.h"


class MediaDirNotify : public QObject
{
Q_OBJECT
public:
	MediaDirNotify(const MediaList &list);

private slots:
	void filesAdded(const QString &directory);
	void filesRemoved(const QStringList &fileList);
	void filesChanged(const QStringList &fileList);

private:
	KUrl::List toMediaURL(const KUrl &url);
	KUrl::List toMediaURLList(const KUrl::List &list);

	const MediaList &m_mediaList;
};

#endif
