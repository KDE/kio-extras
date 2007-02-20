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

#include "remoteimpl.h"

#include <kdebug.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <kservice.h>
#include <klocale.h>

#include <QDir>
#include <QFile>

#include <sys/stat.h>

#define WIZARD_URL "remote:/x-wizard_service.desktop"
#define WIZARD_SERVICE "knetattach"

RemoteImpl::RemoteImpl()
{
	KGlobal::dirs()->addResourceType("remote_entries",
		KStandardDirs::kde_default("data") + "remoteview");

	QString path = KGlobal::dirs()->saveLocation("remote_entries");

	QDir dir = path;
	if (!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("remoteview");
	}
}

void RemoteImpl::listRoot(KIO::UDSEntryList &list) const
{
	kDebug(1220) << "RemoteImpl::listRoot" << endl;

	QStringList names_found;
	QStringList dirList = KGlobal::dirs()->resourceDirs("remote_entries");

	QStringList::ConstIterator dirpath = dirList.begin();
	const QStringList::ConstIterator end = dirList.end();
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
}

bool RemoteImpl::findDirectory(const QString &filename, QString &directory) const
{
	kDebug(1220) << "RemoteImpl::findDirectory" << endl;

	QStringList dirList = KGlobal::dirs()->resourceDirs("remote_entries");

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
				directory = *dirpath;
				return true;
			}
		}
	}

	return false;
}

QString RemoteImpl::findDesktopFile(const QString &filename) const
{
	kDebug(1220) << "RemoteImpl::findDesktopFile" << endl;

	QString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		return directory+filename+".desktop";
	}

	return QString();
}

KUrl RemoteImpl::findBaseURL(const QString &filename) const
{
	kDebug(1220) << "RemoteImpl::findBaseURL" << endl;

	QString file = findDesktopFile(filename);
	if (!file.isEmpty())
	{
		KDesktopFile desktop( file );
		return desktop.readUrl();
	}

	return KUrl();
}


void RemoteImpl::createTopLevelEntry(KIO::UDSEntry &entry) const
{
	entry.clear();
	entry.insert( KIO::UDS_NAME, QString::fromLatin1("."));
	entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR);
	entry.insert( KIO::UDS_ACCESS, 0777);
	entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));
	entry.insert( KIO::UDS_ICON_NAME, QString::fromLatin1("network"));
	entry.insert( KIO::UDS_USER, QString::fromLatin1("root"));
	entry.insert( KIO::UDS_GROUP, QString::fromLatin1("root"));
}

static KUrl findWizardRealURL()
{
	KUrl url;
	KService::Ptr service = KService::serviceByDesktopName(WIZARD_SERVICE);

	if (service && service->isValid())
	{
		url.setPath( KStandardDirs::locate("apps",
				    service->desktopEntryPath())
				);
	}

	return url;
}

bool RemoteImpl::createWizardEntry(KIO::UDSEntry &entry) const
{
	entry.clear();

	KUrl url = findWizardRealURL();

	if (!url.isValid())
	{
		return false;
	}

	entry.insert( KIO::UDS_NAME, i18n("Add a Network Folder"));
	entry.insert( KIO::UDS_FILE_TYPE, S_IFREG);
	entry.insert( KIO::UDS_URL, QString::fromLatin1(WIZARD_URL) );
	entry.insert( KIO::UDS_LOCAL_PATH, url.path());
	entry.insert( KIO::UDS_ACCESS, 0500);
	entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("application/x-desktop"));
	entry.insert( KIO::UDS_ICON_NAME, QString::fromLatin1("wizard"));

	return true;
}

bool RemoteImpl::isWizardURL(const KUrl &url) const
{
	return url==KUrl(WIZARD_URL);
}


void RemoteImpl::createEntry(KIO::UDSEntry &entry,
                             const QString &directory,
                             const QString &file) const
{
	kDebug(1220) << "RemoteImpl::createEntry" << endl;

	KDesktopFile desktop(directory+file);

	kDebug(1220) << "path = " << directory << file << endl;

	entry.clear();

	QString new_filename = file;
	new_filename.truncate( file.length()-8);

	entry.insert( KIO::UDS_NAME, desktop.readName());
	entry.insert( KIO::UDS_URL, "remote:/"+new_filename);

	entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR);
	entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));

	const QString icon = desktop.readIcon();
	entry.insert( KIO::UDS_ICON_NAME, icon);
	entry.insert( KIO::UDS_LINK_DEST, desktop.readUrl());
}

bool RemoteImpl::statNetworkFolder(KIO::UDSEntry &entry, const QString &filename) const
{
	kDebug(1220) << "RemoteImpl::statNetworkFolder: " << filename << endl;

	QString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		createEntry(entry, directory, filename+".desktop");
		return true;
	}

	return false;
}

bool RemoteImpl::deleteNetworkFolder(const QString &filename) const
{
	kDebug(1220) << "RemoteImpl::deleteNetworkFolder: " << filename << endl;

	QString directory;
	if (findDirectory(filename+".desktop", directory))
	{
		kDebug(1220) << "Removing " << directory << filename << ".desktop" << endl;
		return QFile::remove(directory+filename+".desktop");
	}

	return false;
}

bool RemoteImpl::renameFolders(const QString &src, const QString &dest,
                               bool overwrite) const
{
	kDebug(1220) << "RemoteImpl::renameFolders: "
	          << src << ", " << dest << endl;

	QString directory;
	if (findDirectory(src+".desktop", directory))
	{
		if (!overwrite && QFile::exists(directory+dest+".desktop"))
		{
			return false;
		}

		kDebug(1220) << "Renaming " << directory << src << ".desktop"<< endl;
		QDir dir(directory);
		bool res = dir.rename(src+".desktop", dest+".desktop");
		if (res)
		{
			KDesktopFile desktop(directory+dest+".desktop");
			desktop.desktopGroup().writeEntry("Name", dest);
		}
		return res;
	}

	return false;
}


