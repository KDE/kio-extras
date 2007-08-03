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

#include <kdirnotify.h>

#include "medium.h"

MediaDirNotify::MediaDirNotify(const MediaList &list)
	: m_mediaList(list)
{
	QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
				    "FilesAdded", this, SLOT(filesAdded(QString))); QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
				    "FilesRemoved", this, SLOT(filesRemoved(QStringList))); QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KDirNotify",
				    "FilesChanged", this, SLOT(filesChanged(QStringList)));
}

KUrl::List MediaDirNotify::toMediaURL(const KUrl &url)
{
	kDebug(1219) << "MediaDirNotify::toMediaURL(" << url << ")";

	KUrl::List result;

	const QList<Medium *> list = m_mediaList.list();

	QList<Medium *>::const_iterator it = list.begin();
	QList<Medium *>::const_iterator end = list.end();

	for (; it!=end; ++it)
	{
		const Medium *m = *it;
		KUrl base = m->prettyBaseURL();

		if ( base.isParentOf(url) )
		{
			QString path = KUrl::relativePath(base.path(),
			                                  url.path());

			KUrl new_url("media:/"+m->name()+'/'+path );
			new_url.cleanPath();

			result.append(new_url);
		}
	}

	kDebug(1219) << result;
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

void MediaDirNotify::filesAdded(const QString &directory)
{
	KUrl::List new_urls = toMediaURL(KUrl(directory));

	if (!new_urls.isEmpty())
	{
		KUrl::List::const_iterator it = new_urls.constBegin();
		KUrl::List::const_iterator end = new_urls.constEnd();

		for ( ; it!=end; ++it )
		{
			org::kde::KDirNotify::emitFilesAdded( it->url() );
		}
	}
}

void MediaDirNotify::filesRemoved(const QStringList &fileList)
{
	KUrl::List new_list = toMediaURLList(fileList);

	if (!new_list.isEmpty())
	{
		org::kde::KDirNotify::emitFilesRemoved( new_list.toStringList() );
	}
}

void MediaDirNotify::filesChanged(const QStringList &fileList)
{
	KUrl::List new_list = toMediaURLList(fileList);

	if (!new_list.isEmpty())
	{
		org::kde::KDirNotify::emitFilesChanged( new_list.toStringList() );
	}
}

#include "mediadirnotify.moc"
