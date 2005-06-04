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

#include "systemdirnotify.h"

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>

#include <kdirnotify_stub.h>

#include <qdir.h>

SystemDirNotify::SystemDirNotify()
{
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
				
				if (!desktop.readEntry("EmptyIcon").isNull())
				{
					m_urlList.append(desktop.readURL());
				}
				
				names_found.append(*name);
			}
		}
	}
}

bool SystemDirNotify::isInsideList(const KURL &url)
{
	KURL::List::ConstIterator it = m_urlList.begin();
	KURL::List::ConstIterator end = m_urlList.end();

	for(; it!=end; ++it)
	{
		if ((*it).isParentOf(url))
		{
			return true;
		}
	}

	return false;
}

ASYNC SystemDirNotify::FilesAdded(const KURL &directory)
{
	if (isInsideList(directory))
	{
		KDirNotify_stub notifier("*", "*");
		notifier.FilesAdded( "system:/" );
	}
}

ASYNC SystemDirNotify::FilesRemoved(const KURL::List &fileList)
{
	KURL::List::ConstIterator it = fileList.begin();
	KURL::List::ConstIterator end = fileList.end();

	for(; it!=end; ++it)
	{
		if (isInsideList(*it))
		{
			KDirNotify_stub notifier("*", "*");
			notifier.FilesAdded( "system:/" );
			return;
		}
	}
}

ASYNC SystemDirNotify::FilesChanged(const KURL::List &fileList)
{
	KURL::List::ConstIterator it = fileList.begin();
	KURL::List::ConstIterator end = fileList.end();

	for(; it!=end; ++it)
	{
		if (isInsideList(*it))
		{
			KDirNotify_stub notifier("*", "*");
			notifier.FilesAdded( "system:/" );
			return;
		}
	}
}

