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

#include "systemdirnotify.h"

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <Q3CString>
#include <kdirnotify_stub.h>

#include <qdir.h>

SystemDirNotify::SystemDirNotify()
: mInited( false )
{
}

void SystemDirNotify::init()
{
	if( mInited )
		return;
	mInited = true;
	KGlobal::dirs()->addResourceType("system_entries",
		KStandardDirs::kde_default("data") + "systemview");

	QStringList names_found;
	QStringList dirList = KGlobal::dirs()->resourceDirs("system_entries");

	QStringList::ConstIterator dirpath = dirList.begin();
	QStringList::ConstIterator end = dirList.end();
	for(; dirpath!=end; ++dirpath)
	{
		QDir dir = *dirpath;
		if (!dir.exists()) continue;

		QStringList filenames
			= dir.entryList( QDir::Files | QDir::Readable );

		QStringList::ConstIterator name = filenames.begin();
		QStringList::ConstIterator endf = filenames.end();

		for(; name!=endf; ++name)
		{
			if (!names_found.contains(*name))
			{
				KDesktopFile desktop(*dirpath+*name, true);

				QString system_name = *name;
				system_name.truncate(system_name.length()-8);

				KURL system_url("system:/"+system_name);
				
				if ( !desktop.readURL().isEmpty() )
				{
					m_urlMap[desktop.readURL()] = system_url;
					names_found.append( *name );
				}
				else if ( !desktop.readPath().isEmpty() )
				{
					KURL url;
					url.setPath( desktop.readPath() );
					m_urlMap[url] = system_url;
					names_found.append( *name );
				}
			}
		}
	}
}

KURL SystemDirNotify::toSystemURL(const KURL &url)
{
	kdDebug() << "SystemDirNotify::toSystemURL(" << url << ")" << endl;

	init();
	QMap<KURL,KURL>::const_iterator it = m_urlMap.begin();
	QMap<KURL,KURL>::const_iterator end = m_urlMap.end();

	for (; it!=end; ++it)
	{
		KURL base = it.key();

		if ( base.isParentOf(url) )
		{
			QString path = KURL::relativePath(base.path(),
			                                  url.path());
			KURL result = it.data();
			result.addPath(path);
			result.cleanPath();
			kdDebug() << result << endl;
			return result;
		}
	}

	kdDebug() << "KURL()" << endl;
	return KURL();
}

KURL::List SystemDirNotify::toSystemURLList(const KURL::List &list)
{
	init();
	KURL::List new_list;

	KURL::List::const_iterator it = list.begin();
	KURL::List::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		KURL url = toSystemURL(*it);

		if (url.isValid())
		{
			new_list.append(url);
		}
	}

	return new_list;
}

ASYNC SystemDirNotify::FilesAdded(const KURL &directory)
{
	KURL new_dir = toSystemURL(directory);

	if (new_dir.isValid())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesAdded( new_dir );
		if (new_dir.upURL().upURL()==KURL("system:/"))
		{
			notifier.FilesChanged( new_dir.upURL() );
		}
	}
}

ASYNC SystemDirNotify::FilesRemoved(const KURL::List &fileList)
{
	KURL::List new_list = toSystemURLList(fileList);

	if (!new_list.isEmpty())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesRemoved( new_list );
		
		KURL::List::const_iterator it = new_list.begin();
		KURL::List::const_iterator end = new_list.end();

		for (; it!=end; ++it)
		{
			if ((*it).upURL().upURL()==KURL("system:/"))
			{
				notifier.FilesChanged( (*it).upURL() );
			}
		}
	}
}

ASYNC SystemDirNotify::FilesChanged(const KURL::List &fileList)
{
	KURL::List new_list = toSystemURLList(fileList);

	if (!new_list.isEmpty())
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesChanged( new_list );
	}
}

