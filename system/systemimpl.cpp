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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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

bool SystemImpl::listRoot(KIO::UDSEntryList &list)
{
	kDebug() << "SystemImpl::listRoot" << endl;

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

		QStringList::ConstIterator filename = filenames.begin();
		const QStringList::ConstIterator endf = filenames.end();

		for(; filename!=endf; ++filename)
		{
			if (!names_found.contains(*filename))
			{
				entry.clear();
				createEntry(entry, *dirpath, *filename);
				if ( !entry.isEmpty() )
				{
					list.append(entry);
					names_found.append(*filename);
				}
			}
		}
	}

	return true;
}

bool SystemImpl::parseURL(const KUrl &url, QString &name, QString &path) const
{
	QString url_path = url.path();

	int i = url_path.find('/', 1);
	if (i > 0)
	{
		name = url_path.mid(1, i-1);
		path = url_path.mid(i+1);
	}
	else
	{
		name = url_path.mid(1);
		path.clear();
	}

	return name != QString();
}

bool SystemImpl::realURL(const QString &name, const QString &path,
                         KUrl &url) const
{
	url = findBaseURL(name);
	if (!url.isValid())
	{
		return false;
	}

	url.addPath(path);
	return true;
}

bool SystemImpl::statByName(const QString &filename, KIO::UDSEntry& entry)
{
	kDebug() << "SystemImpl::statByName" << endl;

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
			if (*name==filename+".desktop")
			{
				createEntry(entry, *dirpath, *name);
				return true;
			}
		}
	}

	return false;
}

KUrl SystemImpl::findBaseURL(const QString &filename) const
{
	kDebug() << "SystemImpl::findBaseURL" << endl;

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
			if (*name==filename+".desktop")
			{
				KDesktopFile desktop(*dirpath+filename+".desktop", true);
				if ( desktop.readURL().isEmpty() )
				{
					KUrl url;
					url.setPath( desktop.readPath() );
					return url;
				}

				return desktop.readURL();
			}
		}
	}

	return KUrl();
}


void SystemImpl::createTopLevelEntry(KIO::UDSEntry &entry) const
{
	entry.clear();
	entry.insert( KIO::UDS_NAME, QString::fromLatin1("."));
	entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR);
	entry.insert( KIO::UDS_ACCESS, 0555);
	entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/system_directory"));
	entry.insert( KIO::UDS_ICON_NAME, QString::fromLatin1("system"));
	entry.insert( KIO::UDS_USER, QString::fromLatin1("root"));
	entry.insert( KIO::UDS_GROUP, QString::fromLatin1("root"));
}

void SystemImpl::createEntry(KIO::UDSEntry &entry,
                             const QString &directory,
                             const QString &file)
{
	kDebug() << "SystemImpl::createEntry" << endl;

	KDesktopFile desktop(directory+file, true);

	kDebug() << "path = " << directory << file << endl;

	entry.clear();

	// Ensure that we really want this entry to be displayed
	if ( desktop.readURL().isEmpty() && desktop.readPath().isEmpty() )
	{
		return;
	}

	entry.insert( KIO::UDS_NAME, desktop.readName());

	QString new_filename = file;
	new_filename.truncate(file.length()-8);
	entry.insert( KIO::UDS_URL, "system:/"+new_filename);

	entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR);
	entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));

	QString icon = desktop.readIcon();
	QString empty_icon = desktop.readEntry("EmptyIcon");

	if (!empty_icon.isEmpty())
	{
		KUrl url = desktop.readURL();

		m_lastListingEmpty = true;

		KIO::ListJob *job = KIO::listDir(url, false, false);
		connect( job, SIGNAL( entries(KIO::Job *,
		                      const KIO::UDSEntryList &) ),
		         this, SLOT( slotEntries(KIO::Job *,
			             const KIO::UDSEntryList &) ) );
		connect( job, SIGNAL( result(KIO::Job *) ),
		         this, SLOT( slotResult(KIO::Job *) ) );
		enterLoop();

		if (m_lastListingEmpty) icon = empty_icon;
	}

	entry.insert( KIO::UDS_ICON_NAME, icon);
}

void SystemImpl::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),
        &eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

void SystemImpl::slotEntries(KIO::Job *job, const KIO::UDSEntryList &list)
{
	if (list.size()>0)
	{
		job->kill(true);
		m_lastListingEmpty = false;
		emit leaveModality();
	}
}

void SystemImpl::slotResult(KIO::Job *)
{
	emit leaveModality();
}


#include "systemimpl.moc"
