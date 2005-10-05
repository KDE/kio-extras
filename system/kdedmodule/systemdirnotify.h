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
#include <qmap.h>

class SystemDirNotify : public KDirNotify
{
K_DCOP

public:
	SystemDirNotify();

k_dcop:
	virtual ASYNC FilesAdded (const KURL &directory);
	virtual ASYNC FilesRemoved (const KURL::List &fileList);
	virtual ASYNC FilesChanged (const KURL::List &fileList);

private:
	KURL toSystemURL(const KURL &url);
	KURL::List toSystemURLList(const KURL::List &list);

	QMap<KURL,KURL> m_urlMap;
};

#endif
