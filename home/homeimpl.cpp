/* This file is part of the KDE project
   Copyright (c) 2005 Kevin Ottens <ervin ipsquad net>

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

#include "homeimpl.h"

#include <kdebug.h>
#include <QApplication>
#include <QEventLoop>
//Added by qt3to4:
#include <QList>

#include <sys/stat.h>

#define MINIMUM_UID 500

HomeImpl::HomeImpl()
{
	KUser user;
	m_effectiveUid = user.uid();
}

bool HomeImpl::parseURL(const KUrl &url, QString &name, QString &path) const
{
	QString url_path = url.path();

	int i = url_path.indexOf('/', 1);
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

bool HomeImpl::realURL(const QString &name, const QString &path, KUrl &url)
{
	KUser user(name);

	if ( user.isValid() )
	{
		KUrl res;
		res.setPath( user.homeDir() );
		res.addPath(path);
		url = res;
		return true;
	}

	return false;
}


bool HomeImpl::listHomes(KIO::UDSEntryList &list)
{
	kDebug() << "HomeImpl::listHomes" << endl;

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
				KIO::UDSEntry entry;
				createHomeEntry(entry, *it);
				list.append(entry);
			}
		}
	}

	return true;
}

void HomeImpl::createTopLevelEntry(KIO::UDSEntry &entry) const
{
	entry.clear();
	entry.insert( KIO::UDS_NAME, QString::fromLatin1(".") );
	entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR);
	entry.insert( KIO::UDS_ACCESS, 0555);
	entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/directory") );
	entry.insert( KIO::UDS_ICON_NAME, QString::fromLatin1("kfm_home") );
	entry.insert( KIO::UDS_USER, QString::fromLatin1("root") );
	entry.insert( KIO::UDS_GROUP, QString::fromLatin1("root") );
}

void HomeImpl::createHomeEntry(KIO::UDSEntry &entry,
                               const KUser &user)
{
	kDebug() << "HomeImpl::createHomeEntry" << endl;

	entry.clear();

	QString full_name = user.loginName();

	if (!user.fullName().isEmpty())
	{
		full_name = user.fullName()+" ("+user.loginName()+")";
	}

	full_name = KIO::encodeFileName( full_name );

	entry.insert( KIO::UDS_NAME, full_name );
	entry.insert( KIO::UDS_URL, QString::fromLatin1( "home:/" ) + user.loginName() );

	entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR );
	entry.insert( KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/directory") );

	QString icon_name = "folder_home2";

	if (user.uid()==m_effectiveUid)
	{
		icon_name = "folder_home";
	}

	entry.insert( KIO::UDS_ICON_NAME, icon_name);

	KUrl url;
	url.setPath(user.homeDir());
	extractUrlInfos(url, entry);
}

bool HomeImpl::statHome(const QString &name, KIO::UDSEntry &entry)
{
	kDebug() << "HomeImpl::statHome: " << name << endl;

	KUser user(name);

	if (user.isValid())
	{
		createHomeEntry(entry, user);
		return true;
	}

	return false;
}

void HomeImpl::slotStatResult(KJob *job)
{
	if ( job->error() == 0)
	{
		KIO::StatJob *stat_job = static_cast<KIO::StatJob *>(job);
		m_entryBuffer = stat_job->statResult();
	}
	emit leaveModality();
}

void HomeImpl::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),
        &eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

void HomeImpl::extractUrlInfos(const KUrl &url, KIO::UDSEntry& infos)
{
	m_entryBuffer.clear();

	KIO::StatJob *job = KIO::stat(url, false);
	connect( job, SIGNAL( result(KJob*) ),
	         this, SLOT( slotStatResult(KJob*) ) );
	enterLoop();

	KIO::UDSEntry::iterator it = m_entryBuffer.begin();
	KIO::UDSEntry::iterator end = m_entryBuffer.end();

        infos.insert( KIO::UDS_ACCESS, m_entryBuffer.value( KIO::UDS_ACCESS ) );
        infos.insert( KIO::UDS_USER, m_entryBuffer.value( KIO::UDS_USER ) );
        infos.insert( KIO::UDS_GROUP, m_entryBuffer.value( KIO::UDS_GROUP ) );
        infos.insert( KIO::UDS_CREATION_TIME, m_entryBuffer.value( KIO::UDS_CREATION_TIME ) );
        infos.insert( KIO::UDS_MODIFICATION_TIME, m_entryBuffer.value( KIO::UDS_MODIFICATION_TIME ) );
        infos.insert( KIO::UDS_ACCESS_TIME, m_entryBuffer.value( KIO::UDS_ACCESS_TIME ) );

	infos.insert( KIO::UDS_LOCAL_PATH, url.path() );
}

#include "homeimpl.moc"

