/* This file is part of the KDE Project
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

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

#include "homedirnotify.h"

#include <kdebug.h>
#include <kuser.h>

#include <kdirnotify_stub.h>
//Added by qt3to4:
#include <QList>

#define MINIMUM_UID 500

HomeDirNotify::HomeDirNotify()
: mInited( false )
{
}

void HomeDirNotify::init()
{
	if( mInited )
		return;
	mInited = true;

	KUser current_user;
	QList<KUserGroup> groups = current_user.groups();
	QList<int> uid_list;

	QList<KUserGroup>::iterator groups_it = groups.begin();
	QList<KUserGroup>::iterator groups_end = groups.end();

	for(; groups_it!=groups_end; ++groups_it)
	{
		QList<KUser> users = (*groups_it).users();

		QList<KUser>::iterator it = users.begin();
		QList<KUser>::iterator users_end = users.end();

		for(; it!=users_end; ++it)
		{
			if ((*it).uid()>=MINIMUM_UID
			 && !uid_list.contains( (*it).uid() ) )
			{
				uid_list.append( (*it).uid() );
				
				QString name = (*it).loginName();
				KURL url;
				url.setPath( (*it).homeDir() );

				m_homeFoldersMap[name] = url;
			}
		}
	}
}

KURL HomeDirNotify::toHomeURL(const KURL &url)
{
	kdDebug() << "HomeDirNotify::toHomeURL(" << url << ")" << endl;
	
	init();
	QMap<QString,KURL>::iterator it = m_homeFoldersMap.begin();
	QMap<QString,KURL>::iterator end = m_homeFoldersMap.end();
	
	for (; it!=end; ++it)
	{
		QString name = it.key();
		KURL base = it.data();
		
		if ( base.isParentOf(url) )
		{
			QString path = KURL::relativePath(base.path(),
			                                  url.path());
			KURL result("home:/"+name+"/"+path);
			result.cleanPath();
			kdDebug() << "result => " << result << endl;
			return result;
		}
	}

	kdDebug() << "result => KURL()" << endl;
	return KURL();
}

KURL::List HomeDirNotify::toHomeURLList(const KURL::List &list)
{
	init();
	KURL::List new_list;

	KURL::List::const_iterator it = list.begin();
	KURL::List::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		KURL url = toHomeURL(*it);

		if (url.isValid())
		{
			new_list.append(url);
		}
	}

	return new_list;
}

ASYNC HomeDirNotify::FilesAdded(const KURL &directory)
{
	kdDebug() << "HomeDirNotify::FilesAdded" << endl;
	
	KURL new_dir = toHomeURL(directory);

	if (new_dir.isValid())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesAdded( new_dir );
	}
}

// This hack is required because of the way we manage .desktop files with
// Forwarding Slaves, their URL is out of the ioslave (some home:/ files
// have a file:/ based UDS_URL so that they are executed correctly.
// Hence, FilesRemoved and FilesChanged does nothing... We're forced to use
// FilesAdded to re-list the modified directory.
inline void evil_hack(const KURL::List &list)
{
	KDirNotify_stub notifier("*", "*");
	
	KURL::List notified;
	
	KURL::List::const_iterator it = list.begin();
	KURL::List::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		KURL url = (*it).upURL();

		if (!notified.contains(url))
		{
			notifier.FilesAdded(url);
			notified.append(url);
		}
	}
}


ASYNC HomeDirNotify::FilesRemoved(const KURL::List &fileList)
{
	kdDebug() << "HomeDirNotify::FilesRemoved" << endl;
	
	KURL::List new_list = toHomeURLList(fileList);

	if (!new_list.isEmpty())
	{
		//KDirNotify_stub notifier("*", "*");
		//notifier.FilesRemoved( new_list );
		evil_hack(new_list);
	}
}

ASYNC HomeDirNotify::FilesChanged(const KURL::List &fileList)
{
	kdDebug() << "HomeDirNotify::FilesChanged" << endl;
	
	KURL::List new_list = toHomeURLList(fileList);

	if (!new_list.isEmpty())
	{
		//KDirNotify_stub notifier("*", "*");
		//notifier.FilesChanged( new_list );
		evil_hack(new_list);
	}
}
