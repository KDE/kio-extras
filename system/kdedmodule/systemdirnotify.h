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

#ifndef _SYSTEMDIRNOTIFY_H_
#define _SYSTEMDIRNOTIFY_H_

#include <kurl.h>
#include <kdirnotify.h>
#include <QMap>

class SystemDirNotify : public KDirNotify
{
K_DCOP

public:
	SystemDirNotify();

k_dcop:
	virtual ASYNC FilesAdded (const KUrl &directory);
	virtual ASYNC FilesRemoved (const KUrl::List &fileList);
	virtual ASYNC FilesChanged (const KUrl::List &fileList);

private:
	void init();
	KUrl toSystemURL(const KUrl &url);
	KUrl::List toSystemURLList(const KUrl::List &list);

	QMap<KUrl,KUrl> m_urlMap;
	bool mInited;
};

#endif
