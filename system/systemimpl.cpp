/* This file is part of the KDE project
   Copyright (c) 2004 Kevin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "systemimpl.h"

#include <kdebug.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>

#include <qapplication.h>
#include <qeventloop.h>
#include <qdir.h>

#include <sys/stat.h>

SystemImpl::SystemImpl() : QObject()
{
	KGlobal::dirs()->addResourceType("system_entries",
		KStandardDirs::kde_default("data") + "systemview");
}

bool SystemImpl::listRoot(QValueList<KIO::UDSEntry> &list)
{
	kdDebug() << "SystemImpl::listRoot" << endl;

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


		KIO::UDSEntry entry;

		QStringList::ConstIterator name = filenames.begin();
		QStringList::ConstIterator endf = filenames.end();

		for(; name!=endf; ++name)
		{
			if (!names_found.contains(*name))
			{
				entry.clear();
				createEntry(entry, *dirpath, *name);
				list.append(entry);
				names_found.append(*name);
			}
		}
	}

	return true;
}

KURL SystemImpl::findBaseURL(const QString &filename) const
{
	kdDebug() << "SystemImpl::findBaseURL" << endl;

	QStringList dirList = KGlobal::dirs()->resourceDirs("system_entries");

	QStringList::ConstIterator dirpath = dirList.begin();
	QStringList::ConstIterator end = dirList.end();
	for(; dirpath!=end; ++dirpath)
	{
		QDir dir = *dirpath;
		if (!dir.exists()) continue;

		QStringList filenames
			= dir.entryList( QDir::Files | QDir::Readable );


		KIO::UDSEntry entry;

		QStringList::ConstIterator name = filenames.begin();
		QStringList::ConstIterator endf = filenames.end();

		for(; name!=endf; ++name)
		{
			if (*name==filename)
			{
				KDesktopFile desktop(*dirpath+filename, true);
				return desktop.readURL();
			}
		}
	}

	return KURL();
}


static void addAtom(KIO::UDSEntry &entry, unsigned int ID, long l,
                    const QString &s = QString::null)
{
	KIO::UDSAtom atom;
	atom.m_uds = ID;
	atom.m_long = l;
	atom.m_str = s;
	entry.append(atom);
}


void SystemImpl::createTopLevelEntry(KIO::UDSEntry &entry) const
{
	entry.clear();
	addAtom(entry, KIO::UDS_NAME, 0, ".");
	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, KIO::UDS_ACCESS, 0555);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/system_directory");
	addAtom(entry, KIO::UDS_ICON_NAME, 0, "kfm");
	addAtom(entry, KIO::UDS_USER, 0, "root");
	addAtom(entry, KIO::UDS_GROUP, 0, "root");
}

void SystemImpl::createEntry(KIO::UDSEntry &entry,
                             const QString &directory,
                             const QString &file)
{
	kdDebug() << "SystemImpl::createEntry" << endl;

	KDesktopFile desktop(directory+file, true);

	kdDebug() << "path = " << directory << file << endl;

	entry.clear();

	addAtom(entry, KIO::UDS_NAME, 0, desktop.readName());
	addAtom(entry, KIO::UDS_URL, 0, "system:/"+file);

	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/directory");

	QString icon = desktop.readIcon();
	QString empty_icon = desktop.readEntry("EmptyIcon");

	if (!empty_icon.isEmpty())
	{
		KURL url = desktop.readURL();

		m_lastListingEmpty = true;

		KIO::ListJob *job = KIO::listDir(url, false, false);
		connect( job, SIGNAL( entries(KIO::Job *,
		                      const KIO::UDSEntryList &) ),
		         this, SLOT( slotEntries(KIO::Job *,
			             const KIO::UDSEntryList &) ) );
		connect( job, SIGNAL( result(KIO::Job *) ),
		         this, SLOT( slotResult(KIO::Job *) ) );
		qApp->eventLoop()->enterLoop();

		if (m_lastListingEmpty) icon = empty_icon;
	}

	addAtom(entry, KIO::UDS_ICON_NAME, 0, icon);
}

void SystemImpl::slotEntries(KIO::Job *job, const KIO::UDSEntryList &list)
{
	if (list.size()>0)
	{
		job->kill(true);
		m_lastListingEmpty = false;
		qApp->eventLoop()->exitLoop();
	}
}

void SystemImpl::slotResult(KIO::Job *)
{
	qApp->eventLoop()->exitLoop();
}


#include "systemimpl.moc"
