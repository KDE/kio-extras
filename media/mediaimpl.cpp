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

#include "mediaimpl.h"

#include <klocale.h>
#include <kdebug.h>
#include <dcopref.h>

#include <qapplication.h>
#include <qeventloop.h>

#include <sys/stat.h>

#include "medium.h"

bool MediaImpl::parseURL(const KURL &url, QString &name, QString &path)
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

KIO::StatJob *MediaImpl::stat(const QString &name, const QString &path)
{
	kdDebug() << "MediaImpl::list: " << name << ", " << path << endl;

	bool ok;
	Medium m = findMediumByName(name, ok);
	if ( !ok ) return 0L;

	ok = ensureMediumMounted(m);
	if ( !ok ) return 0L;

	KURL file = m.prettyBaseURL();
	file.addPath(path);

	return KIO::stat(file, false);
}

bool MediaImpl::statMedium(const QString &name, KIO::UDSEntry &entry)
{
	kdDebug() << "MediaImpl::statMedium: " << name << endl;

	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "properties", name );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The KDE mediamanager is not running.");
		return false;
	}

	Medium m = Medium::create(reply);

	createMediumEntry(entry, m);

	return true;
}

KIO::ListJob *MediaImpl::list(const QString &name, const QString &path)
{
	kdDebug() << "MediaImpl::list: " << name << ", " << path << endl;

	bool ok;
	Medium m = findMediumByName(name, ok);
	if ( !ok ) return 0L;

	ok = ensureMediumMounted(m);
	if ( !ok ) return 0L;

	KURL directory = m.prettyBaseURL();
	directory.addPath(path);

	return KIO::listDir(directory, false);
}

bool MediaImpl::listMedia(QValueList<KIO::UDSEntry> &list)
{
	kdDebug() << "MediaImpl::listMedia" << endl;

	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "fullList" );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The KDE mediamanager is not running.");
		return false;
	}

	Medium::List media = Medium::createList(reply);

	KIO::UDSEntry entry;

	Medium::List::iterator it = media.begin();
	Medium::List::iterator end = media.end();

	for(; it!=end; ++it)
	{
		entry.clear();

		createMediumEntry(entry, *it);

		list.append(entry);
	}

	return true;
}


const Medium MediaImpl::findMediumByName(const QString &name, bool &ok)
{
	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "properties", name );

	if ( reply.isValid() )
	{
		ok = true;
	}
	else
	{
		m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The KDE mediamanager is not running.");
		ok = false;
	}

	return Medium::create(reply);
}

bool MediaImpl::ensureMediumMounted(const Medium &medium)
{
	if ( medium.needMounting() )
	{
		m_lastErrorCode = 0;

		KIO::Job* job = KIO::mount(false, 0,
		                           medium.deviceNode(),
		                           medium.mountPoint());
		connect( job, SIGNAL( result( KIO::Job * ) ),
		         this, SLOT( slotMountResult( KIO::Job * ) ) );

		qApp->eventLoop()->enterLoop();

		return m_lastErrorCode==0;
	}

	return true;
}

void MediaImpl::slotMountResult(KIO::Job *job)
{
	if ( job->error() != 0)
	{
		m_lastErrorCode = job->error();
		m_lastErrorMessage = job->errorString();
	}

	qApp->eventLoop()->exitLoop();
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


void MediaImpl::createTopLevelEntry(KIO::UDSEntry& entry) const
{
	entry.clear();
	addAtom(entry, KIO::UDS_NAME, 0, ".");
	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, KIO::UDS_ACCESS, 0500);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/directory");
}

void MediaImpl::createMediumEntry(KIO::UDSEntry& entry,
                                  const Medium &medium) const
{
	kdDebug() << "MediaProtocol::createMedium" << endl;

	QString url = "media:/"+medium.name();

	kdDebug() << "url = " << url << ", mime = " << medium.mimeType() << endl;

	entry.clear();

	addAtom(entry, KIO::UDS_URL, 0, url);
	addAtom(entry, KIO::UDS_NAME, 0, medium.label());

	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	// Use the mountpoint or base url access
	addAtom(entry, KIO::UDS_ACCESS, 0500);

	if (!medium.iconName().isEmpty())
	{
		addAtom(entry, KIO::UDS_ICON_NAME, 0, medium.iconName());
	}

	addAtom(entry, KIO::UDS_MIME_TYPE, 0, medium.mimeType());
	addAtom(entry, KIO::UDS_GUESSED_MIME_TYPE, 0, "inode/directory");
}

#include "mediaimpl.moc"
