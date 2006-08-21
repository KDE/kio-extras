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

#include "remotedirnotify.h"

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <kdirnotify.h>

#include <QtDBus/QtDBus>
#include <QDir>

RemoteDirNotify::RemoteDirNotify()
{
	KGlobal::dirs()->addResourceType("remote_entries",
		KStandardDirs::kde_default("data") + "remoteview");

	QString path = KGlobal::dirs()->saveLocation("remote_entries");
	m_baseURL.setPath(path);

	QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
				    "FilesAdded", this, SLOT(FilesAdded(QString))); QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
				    "FilesRemoved", this, SLOT(FilesRemoved(QStringList))); QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
				    "FilesChanged", this, SLOT(FilesChanged(QStringList)));
}

KUrl RemoteDirNotify::toRemoteURL(const KUrl &url)
{
	kDebug(1220) << "RemoteDirNotify::toRemoteURL(" << url << ")" << endl;
	if ( m_baseURL.isParentOf(url) )
	{
		QString path = KUrl::relativePath(m_baseURL.path(),
						  url.path());
		KUrl result("remote:/"+path);
		result.cleanPath();
		kDebug(1220) << "result => " << result << endl;
		return result;
	}

	kDebug(1220) << "result => KUrl()" << endl;
	return KUrl();
}

KUrl::List RemoteDirNotify::toRemoteURLList(const KUrl::List &list)
{
	KUrl::List new_list;

	KUrl::List::const_iterator it = list.begin();
	KUrl::List::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		KUrl url = toRemoteURL(*it);

		if (url.isValid())
		{
			new_list.append(url);
		}
	}

	return new_list;
}

void RemoteDirNotify::FilesAdded(const QString &directory)
{
	kDebug(1220) << "RemoteDirNotify::FilesAdded" << endl;

	KUrl new_dir = toRemoteURL(directory);

	if (new_dir.isValid())
	{
		org::kde::KDirNotify::emitFilesAdded( new_dir.url() );
	}
}

// This hack is required because of the way we manage .desktop files with
// Forwarding Slaves, their URL is out of the ioslave (most remote:/ files
// have a file:/ based UDS_URL so that they are executed correctly.
// Hence, FilesRemoved and FilesChanged does nothing... We're forced to use
// FilesAdded to re-list the modified directory.
inline void evil_hack(const KUrl::List &list)
{
	KUrl::List notified;

	KUrl::List::const_iterator it = list.begin();
	KUrl::List::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		KUrl url = (*it).upUrl();

		if (!notified.contains(url))
		{
			org::kde::KDirNotify::emitFilesAdded(url.toString());
			notified.append(url);
		}
	}
}


void RemoteDirNotify::FilesRemoved(const QStringList &fileList)
{
	kDebug(1220) << "RemoteDirNotify::FilesRemoved" << endl;

	KUrl::List new_list = toRemoteURLList(fileList);

	if (!new_list.isEmpty())
	{
		//KDirNotify_stub notifier("*", "*");
		//notifier.FilesRemoved( new_list );
		evil_hack(new_list);
	}
}

void RemoteDirNotify::FilesChanged(const QStringList &fileList)
{
	kDebug(1220) << "RemoteDirNotify::FilesChanged" << endl;

	KUrl::List new_list = toRemoteURLList(fileList);

	if (!new_list.isEmpty())
	{
		//KDirNotify_stub notifier("*", "*");
		//notifier.FilesChanged( new_list );
		evil_hack(new_list);
	}
}

#include "remotedirnotify.moc"
