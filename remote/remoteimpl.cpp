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

#include "remoteimpl.h"

#include <kdebug.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <kservice.h>
#include <klocale.h>

#include <qdir.h>
#include <qfile.h>

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

void RemoteImpl::listRoot(QValueList<KIO::UDSEntry> &list) const
{
	kdDebug() << "RemoteImpl::listRoot" << endl;

	QStringList names_found;
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
	kdDebug() << "RemoteImpl::findDirectory" << endl;

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
	kdDebug() << "RemoteImpl::findDesktopFile" << endl;
	
	QString directory;
	if (findDirectory(filename, directory))
	{
		return directory+filename;
	}
	
	return QString::null;
}

KURL RemoteImpl::findBaseURL(const QString &filename) const
{
	kdDebug() << "RemoteImpl::findBaseURL" << endl;

	QString file = findDesktopFile(filename);
	if (!file.isEmpty())
	{
		KDesktopFile desktop(file, true);
		return desktop.readURL();
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


void RemoteImpl::createTopLevelEntry(KIO::UDSEntry &entry) const
{
	entry.clear();
	addAtom(entry, KIO::UDS_NAME, 0, ".");
	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, KIO::UDS_ACCESS, 0777);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/directory");
	addAtom(entry, KIO::UDS_ICON_NAME, 0, "network");
	addAtom(entry, KIO::UDS_USER, 0, "root");
	addAtom(entry, KIO::UDS_GROUP, 0, "root");
}

static KURL findWizardRealURL()
{
	KURL url;
	KService::Ptr service = KService::serviceByDesktopName(WIZARD_SERVICE);

	if (service && service->isValid())
	{
		url.setPath( locate("apps",
				    service->desktopEntryPath())
				);
	}

	return url;
}

bool RemoteImpl::createWizardEntry(KIO::UDSEntry &entry) const
{
	entry.clear();

	KURL url = findWizardRealURL();

	if (!url.isValid())
	{
		return false;
	}

	addAtom(entry, KIO::UDS_NAME, 0, i18n("Add a Network Folder"));
	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFREG);
	addAtom(entry, KIO::UDS_URL, 0, WIZARD_URL);
	addAtom(entry, KIO::UDS_LOCAL_PATH, 0, url.path());
	addAtom(entry, KIO::UDS_ACCESS, 0500);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "application/x-desktop");
	addAtom(entry, KIO::UDS_ICON_NAME, 0, "wizard");

	return true;
}

bool RemoteImpl::isWizardURL(const KURL &url) const
{
	return url==KURL(WIZARD_URL);
}


void RemoteImpl::createEntry(KIO::UDSEntry &entry,
                             const QString &directory,
                             const QString &file) const
{
	kdDebug() << "RemoteImpl::createEntry" << endl;

	KDesktopFile desktop(directory+file, true);

	kdDebug() << "path = " << directory << file << endl;

	entry.clear();

	addAtom(entry, KIO::UDS_NAME, 0, desktop.readName());
	addAtom(entry, KIO::UDS_URL, 0, "remote:/"+file);

	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/directory");

	QString icon = desktop.readIcon();

	addAtom(entry, KIO::UDS_ICON_NAME, 0, icon);
	addAtom(entry, KIO::UDS_LINK_DEST, 0, directory+file);
}

bool RemoteImpl::statNetworkFolder(KIO::UDSEntry &entry, const QString &filename) const
{
	kdDebug() << "RemoteImpl::statNetworkFolder: " << filename << endl;

	QString directory;
	if (findDirectory(filename, directory))
	{
		createEntry(entry, directory, filename);
		return true;
	}
	
	return false;
}

bool RemoteImpl::deleteNetworkFolder(const QString &filename) const
{
	kdDebug() << "RemoteImpl::deleteNetworkFolder: " << filename << endl;

	QString directory;
	if (findDirectory(filename, directory))
	{
		kdDebug() << "Removing " << directory << filename << endl;
		return QFile::remove(directory+filename);
	}
	
	return false;
}

