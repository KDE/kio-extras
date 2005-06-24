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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "homeimpl.h"

#include <kdebug.h>
#include <qapplication.h>
#include <qeventloop.h>

#include <sys/stat.h>

#define MINIMUM_UID 500

HomeImpl::HomeImpl()
{
	KUser user;
	m_effectiveUid = user.uid();
}

bool HomeImpl::parseURL(const KURL &url, QString &name, QString &path) const
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
		path = QString::null;
	}

	return name != QString::null;
}

bool HomeImpl::realURL(const QString &name, const QString &path, KURL &url)
{
	KUser user(name);
	
	if ( user.isValid() )
	{
		KURL res;
		res.setPath( user.homeDir() );
		res.addPath(path);
		url = res;
		return true;
	}
	
	return false;
}


bool HomeImpl::listHomes(QValueList<KIO::UDSEntry> &list)
{
	kdDebug() << "HomeImpl::listHomes" << endl;

	KUser current_user;
	QValueList<KUserGroup> groups = current_user.groups();
	QValueList<int> uid_list;
	
	QValueList<KUserGroup>::iterator groups_it = groups.begin();
	QValueList<KUserGroup>::iterator groups_end = groups.end();

	for(; groups_it!=groups_end; ++groups_it)
	{
		QValueList<KUser> users = (*groups_it).users();

		QValueList<KUser>::iterator it = users.begin();
		QValueList<KUser>::iterator users_end = users.end();
		
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

static void addAtom(KIO::UDSEntry &entry, unsigned int ID, long l,
                    const QString &s = QString::null)
{
	KIO::UDSAtom atom;
	atom.m_uds = ID;
	atom.m_long = l;
	atom.m_str = s;
	entry.append(atom);
}


void HomeImpl::createTopLevelEntry(KIO::UDSEntry &entry) const
{
	entry.clear();
	addAtom(entry, KIO::UDS_NAME, 0, ".");
	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, KIO::UDS_ACCESS, 0555);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/directory");
	addAtom(entry, KIO::UDS_ICON_NAME, 0, "kfm_home");
	addAtom(entry, KIO::UDS_USER, 0, "root");
	addAtom(entry, KIO::UDS_GROUP, 0, "root");
}

void HomeImpl::createHomeEntry(KIO::UDSEntry &entry,
                               const KUser &user)
{
	kdDebug() << "HomeImpl::createHomeEntry" << endl;
	
	entry.clear();
	
	QString full_name = user.loginName();
	
	if (!user.fullName().isEmpty())
	{
		full_name = user.fullName()+" ("+user.loginName()+")";
	}
	
	full_name = KIO::encodeFileName( full_name );
	
	addAtom(entry, KIO::UDS_NAME, 0, full_name);
	addAtom(entry, KIO::UDS_URL, 0, "home:/"+user.loginName());
	
	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/directory");

	QString icon_name = "folder_home2";

	if (user.uid()==m_effectiveUid)
	{
		icon_name = "folder_home";
	}
	
	addAtom(entry, KIO::UDS_ICON_NAME, 0, icon_name);

	KURL url;
	url.setPath(user.homeDir());
	entry += extractUrlInfos(url);
}

bool HomeImpl::statHome(const QString &name, KIO::UDSEntry &entry)
{
	kdDebug() << "HomeImpl::statHome: " << name << endl;

	KUser user(name);

	if (user.isValid())
	{
		createHomeEntry(entry, user);
		return true;	
	}
	
	return false;
}

void HomeImpl::slotStatResult(KIO::Job *job)
{
	if ( job->error() == 0)
	{
		KIO::StatJob *stat_job = static_cast<KIO::StatJob *>(job);
		m_entryBuffer = stat_job->statResult();
	}

	qApp->eventLoop()->exitLoop();
}

KIO::UDSEntry HomeImpl::extractUrlInfos(const KURL &url)
{
	m_entryBuffer.clear();

	KIO::StatJob *job = KIO::stat(url, false);
	connect( job, SIGNAL( result(KIO::Job *) ),
	         this, SLOT( slotStatResult(KIO::Job *) ) );
	qApp->eventLoop()->enterLoop();

	KIO::UDSEntry::iterator it = m_entryBuffer.begin();
	KIO::UDSEntry::iterator end = m_entryBuffer.end();

	KIO::UDSEntry infos;

	for(; it!=end; ++it)
	{
		switch( (*it).m_uds )
		{
		case KIO::UDS_ACCESS:
		case KIO::UDS_USER:
		case KIO::UDS_GROUP:
		case KIO::UDS_CREATION_TIME:
		case KIO::UDS_MODIFICATION_TIME:
		case KIO::UDS_ACCESS_TIME:
			infos.append(*it);
			break;
		default:
			break;
		}
	}
	
	addAtom(infos, KIO::UDS_LOCAL_PATH, 0, url.path());

	return infos;
}

#include "homeimpl.moc"

