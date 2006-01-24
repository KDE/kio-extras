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

#include "mediadirnotify.h"

#include <kdebug.h>

#include <kdirnotify_stub.h>

#include "medium.h"
//Added by qt3to4:
#include <Q3PtrList>

MediaDirNotify::MediaDirNotify(const MediaList &list)
	: m_mediaList(list)
{

}

KUrl::List MediaDirNotify::toMediaURL(const KUrl &url)
{
	kdDebug(1219) << "MediaDirNotify::toMediaURL(" << url << ")" << endl;

	KUrl::List result;
	
	const Q3PtrList<Medium> list = m_mediaList.list();

	Q3PtrList<Medium>::const_iterator it = list.begin();
	Q3PtrList<Medium>::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		const Medium *m = *it;
		KUrl base = m->prettyBaseURL();

		if ( base.isParentOf(url) )
		{
			QString path = KUrl::relativePath(base.path(),
			                                  url.path());

			KUrl new_url("media:/"+m->name()+"/"+path );
			new_url.cleanPath();
		
			result.append(new_url);
		}
	}

	kdDebug(1219) << result << endl;
	return result;
}

KUrl::List MediaDirNotify::toMediaURLList(const KUrl::List &list)
{
	KUrl::List new_list;

	KUrl::List::const_iterator it = list.begin();
	KUrl::List::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		KUrl::List urls = toMediaURL(*it);

		if (!urls.isEmpty())
		{
			new_list += urls;
		}
	}

	return new_list;
}

ASYNC MediaDirNotify::FilesAdded(const KUrl &directory)
{
	KUrl::List new_urls = toMediaURL(directory);

	if (!new_urls.isEmpty())
	{
		KDirNotify_stub notifier("*", "*");

		KUrl::List::const_iterator it = new_urls.begin();
		KUrl::List::const_iterator end = new_urls.end();

		for ( ; it!=end; ++it )
		{
			notifier.FilesAdded(*it);
		}
	}
}

ASYNC MediaDirNotify::FilesRemoved(const KUrl::List &fileList)
{
	KUrl::List new_list = toMediaURLList(fileList);

	if (!new_list.isEmpty())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesRemoved( new_list );
	}
}

ASYNC MediaDirNotify::FilesChanged(const KUrl::List &fileList)
{
	KUrl::List new_list = toMediaURLList(fileList);

	if (!new_list.isEmpty())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesChanged( new_list );
	}
}

