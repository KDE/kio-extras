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

#include "remotedirnotify.h"

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>

#include <kdirnotify_stub.h>

#include <qdir.h>

RemoteDirNotify::RemoteDirNotify()
{
	KGlobal::dirs()->addResourceType("remote_entries",
		KStandardDirs::kde_default("data") + "remoteview");

	QString path = KGlobal::dirs()->saveLocation("remote_entries");
	m_baseURL.setPath(path);
}

KURL RemoteDirNotify::toRemoteURL(const KURL &url)
{
	if ( m_baseURL.isParentOf(url) )
	{
		QString path = KURL::relativePath(m_baseURL.path(),
						  url.path());
		KURL result("remote:/"+path );
		result.cleanPath();
		return result;
	}

	return KURL();
}

KURL::List RemoteDirNotify::toRemoteURLList(const KURL::List &list)
{
	KURL::List new_list;

	KURL::List::const_iterator it = list.begin();
	KURL::List::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		KURL url = toRemoteURL(*it);

		if (url.isValid())
		{
			new_list.append(url);
		}
	}

	return new_list;
}

ASYNC RemoteDirNotify::FilesAdded(const KURL &directory)
{
	KURL new_dir = toRemoteURL(directory);

	if (new_dir.isValid())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesAdded( new_dir );
	}
}

ASYNC RemoteDirNotify::FilesRemoved(const KURL::List &fileList)
{
	KURL::List new_list = toRemoteURLList(fileList);

	if (!new_list.isEmpty())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesRemoved( new_list );
	}
}

ASYNC RemoteDirNotify::FilesChanged(const KURL::List &fileList)
{
	KURL::List new_list = toRemoteURLList(fileList);

	if (!new_list.isEmpty())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesChanged( new_list );
	}
}
