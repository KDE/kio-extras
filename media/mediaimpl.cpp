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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "mediaimpl.h"

#include <klocale.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <kio/netaccess.h>

#include <kmimetype.h>

#include <kapplication.h>
#include <qeventloop.h>

#include <sys/stat.h>

#include "medium.h"

MediaImpl::MediaImpl() : QObject(), DCOPObject("mediaimpl"), mp_mounting(0L)
{

}

bool MediaImpl::parseURL(const KURL &url, QString &name, QString &path) const
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

bool MediaImpl::realURL(const QString &name, const QString &path, KURL &url)
{
	bool ok;
	Medium m = findMediumByName(name, ok);
	if ( !ok ) return false;

	ok = ensureMediumMounted(m);
	if ( !ok ) return false;

	url = m.prettyBaseURL();
	url.addPath(path);
	return true;
}


bool MediaImpl::statMedium(const QString &name, KIO::UDSEntry &entry)
{
	kdDebug(1219) << "MediaImpl::statMedium: " << name << endl;

	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "properties", name );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The KDE mediamanager is not running.");
		return false;
	}

	Medium m = Medium::create(reply);

	if (m.id().isEmpty())
	{
		entry.clear();
		return false;
	}

	createMediumEntry(entry, m);

	return true;
}

bool MediaImpl::statMediumByLabel(const QString &label, KIO::UDSEntry &entry)
{
	kdDebug(1219) << "MediaImpl::statMediumByLabel: " << label << endl;

	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "nameForLabel", label );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The KDE mediamanager is not running.");
		return false;
	}

	QString name = reply;

	if (name.isEmpty())
	{
		entry.clear();
		return false;
	}

	return statMedium(name, entry);
}


bool MediaImpl::listMedia(QValueList<KIO::UDSEntry> &list)
{
	kdDebug(1219) << "MediaImpl::listMedia" << endl;

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

bool MediaImpl::setUserLabel(const QString &name, const QString &label)
{
	kdDebug(1219) << "MediaImpl::setUserLabel: " << name << ", " << label << endl;


	DCOPRef mediamanager("kded", "mediamanager");
	DCOPReply reply = mediamanager.call( "nameForLabel", label );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The KDE mediamanager is not running.");
		return false;
	}
	else
	{
		QString returned_name = reply;
		if (!returned_name.isEmpty()
		 && returned_name!=name)
		{
			m_lastErrorCode = KIO::ERR_DIR_ALREADY_EXIST;
			m_lastErrorMessage = i18n("This media name already exists.");
			return false;
		}
	}

	reply = mediamanager.call( "setUserLabel", name, label );

	if ( !reply.isValid() )
	{
		m_lastErrorCode = KIO::ERR_SLAVE_DEFINED;
		m_lastErrorMessage = i18n("The KDE mediamanager is not running.");
		return false;
	}
	else
	{
		return true;
	}
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

bool MediaImpl::ensureMediumMounted(Medium &medium)
{
	if ( medium.needMounting() )
	{
		m_lastErrorCode = 0;

		mp_mounting = &medium;
		
		KIO::Job* job = KIO::mount(false, 0,
		                           medium.deviceNode(),
		                           medium.mountPoint());
		job->setInteractive(false);
		connect( job, SIGNAL( result( KIO::Job * ) ),
		         this, SLOT( slotMountResult( KIO::Job * ) ) );
		kapp->dcopClient()
		->connectDCOPSignal("kded", "mediamanager",
		                    "mediumChanged(QString)",
		                    "mediaimpl",
		                    "slotMediumChanged(QString)",
		                    false);

		qApp->eventLoop()->enterLoop();

		mp_mounting = 0L;
		
		kapp->dcopClient()
		->disconnectDCOPSignal("kded", "mediamanager",
		                       "mediumChanged(QString)",
		                       "mediaimpl",
		                       "slotMediumChanged(QString)");
		
		return m_lastErrorCode==0;
	}

	return true;
}

void MediaImpl::slotMountResult(KIO::Job *job)
{
	kdDebug(1219) << "MediaImpl::slotMountResult" << endl;
	
	if ( job->error() != 0)
	{
		m_lastErrorCode = job->error();
		m_lastErrorMessage = job->errorText();
		qApp->eventLoop()->exitLoop();
	}
}

void MediaImpl::slotMediumChanged(const QString &name)
{
	kdDebug(1219) << "MediaImpl::slotMediumChanged:" << name << endl;

	if (mp_mounting->name()==name)
	{
		kdDebug(1219) << "MediaImpl::slotMediumChanged: updating mp_mounting" << endl;
		bool ok;
		*mp_mounting = findMediumByName(name, ok);
		qApp->eventLoop()->exitLoop();
	}
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
	addAtom(entry, KIO::UDS_URL, 0, "media:/");
	addAtom(entry, KIO::UDS_NAME, 0, ".");
	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
	addAtom(entry, KIO::UDS_ACCESS, 0555);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/directory");
	addAtom(entry, KIO::UDS_ICON_NAME, 0, "blockdevice");
	addAtom(entry, KIO::UDS_USER, 0, "root");
	addAtom(entry, KIO::UDS_GROUP, 0, "root");
}

void MediaImpl::slotStatResult(KIO::Job *job)
{
	if ( job->error() == 0)
	{
		KIO::StatJob *stat_job = static_cast<KIO::StatJob *>(job);
		m_entryBuffer = stat_job->statResult();
	}

	qApp->eventLoop()->exitLoop();
}

KIO::UDSEntry MediaImpl::extractUrlInfos(const KURL &url)
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

	if (url.isLocalFile())
	{
		addAtom(infos, KIO::UDS_LOCAL_PATH, 0, url.path());
	}
	
	return infos;
}


void MediaImpl::createMediumEntry(KIO::UDSEntry& entry,
                                  const Medium &medium)
{
	kdDebug(1219) << "MediaProtocol::createMedium" << endl;

	QString url = "media:/"+medium.name();

	kdDebug(1219) << "url = " << url << ", mime = " << medium.mimeType() << endl;

	entry.clear();

	addAtom(entry, KIO::UDS_URL, 0, url);

	QString label = KIO::encodeFileName( medium.prettyLabel() );
	addAtom(entry, KIO::UDS_NAME, 0, label);

	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);

	addAtom(entry, KIO::UDS_MIME_TYPE, 0, medium.mimeType());
	addAtom(entry, KIO::UDS_GUESSED_MIME_TYPE, 0, "inode/directory");

	if (!medium.iconName().isEmpty())
	{
		addAtom(entry, KIO::UDS_ICON_NAME, 0, medium.iconName());
	}
	else
	{
		QString mime = medium.mimeType();
		QString icon = KMimeType::mimeType(mime)->icon(mime, false);
		addAtom(entry, KIO::UDS_ICON_NAME, 0, icon);
	}

	if (medium.needMounting())
	{
		addAtom(entry, KIO::UDS_ACCESS, 0400);
	}
	else
	{
		KURL url = medium.prettyBaseURL();
		entry+= extractUrlInfos(url);
	}
}

#include "mediaimpl.moc"
