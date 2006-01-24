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

bool MediaImpl::parseURL(const KUrl &url, QString &name, QString &path) const
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

bool MediaImpl::realURL(const QString &name, const QString &path, KUrl &url)
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


bool MediaImpl::listMedia(KIO::UDSEntryList& list)
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
		job->setAutoWarningHandlingEnabled(false);
		connect( job, SIGNAL( result( KIO::Job * ) ),
		         this, SLOT( slotMountResult( KIO::Job * ) ) );
		connect( job, SIGNAL( warning( KIO::Job *, const QString & ) ),
		         this, SLOT( slotWarning( KIO::Job *, const QString & ) ) );
		kapp->dcopClient()
		->connectDCOPSignal("kded", "mediamanager",
		                    "mediumChanged(QString, bool)",
		                    "mediaimpl",
		                    "slotMediumChanged(QString)",
		                    false);

		enterLoop();

		mp_mounting = 0L;

		kapp->dcopClient()
		->disconnectDCOPSignal("kded", "mediamanager",
		                       "mediumChanged(QString, bool)",
		                       "mediaimpl",
		                       "slotMediumChanged(QString)");

		return m_lastErrorCode==0;
	}

	return true;
}

void MediaImpl::slotWarning( KIO::Job * /*job*/, const QString &msg )
{
	emit warning( msg );
}

void MediaImpl::slotMountResult(KIO::Job *job)
{
	kdDebug(1219) << "MediaImpl::slotMountResult" << endl;

	if ( job->error() != 0)
	{
		m_lastErrorCode = job->error();
		m_lastErrorMessage = job->errorText();
		emit leaveModality();
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
		emit leaveModality();
	}
}

void MediaImpl::createTopLevelEntry(KIO::UDSEntry& entry) const
{
	entry.clear();
	entry.insert(KIO::UDS_URL, QString::fromLatin1("media:/"));
	entry.insert(KIO::UDS_NAME, QString::fromLatin1("."));
	entry.insert(KIO::UDS_FILE_TYPE, S_IFDIR);
	entry.insert(KIO::UDS_ACCESS, 0555);
	entry.insert(KIO::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));
	entry.insert(KIO::UDS_ICON_NAME, QString::fromLatin1("blockdevice"));
	entry.insert(KIO::UDS_USER, QString::fromLatin1("root"));
	entry.insert(KIO::UDS_GROUP, QString::fromLatin1("root"));
}

void MediaImpl::slotStatResult(KIO::Job *job)
{
	if ( job->error() == 0)
	{
		KIO::StatJob *stat_job = static_cast<KIO::StatJob *>(job);
		m_entryBuffer = stat_job->statResult();
	}

	emit leaveModality();
}

void MediaImpl::extractUrlInfos(const KUrl &url, KIO::UDSEntry& infos)
{
	m_entryBuffer.clear();

	KIO::StatJob *job = KIO::stat(url, false);
	job->setAutoWarningHandlingEnabled( false );
	connect( job, SIGNAL( result(KIO::Job *) ),
	         this, SLOT( slotStatResult(KIO::Job *) ) );
	connect( job, SIGNAL( warning( KIO::Job *, const QString & ) ),
	         this, SLOT( slotWarning( KIO::Job *, const QString & ) ) );
	enterLoop();

        infos.insert( KIO::UDS_ACCESS, m_entryBuffer.value( KIO::UDS_ACCESS ) );
        infos.insert( KIO::UDS_USER, m_entryBuffer.value( KIO::UDS_USER ) );
        infos.insert( KIO::UDS_GROUP, m_entryBuffer.value( KIO::UDS_GROUP ) );
        infos.insert( KIO::UDS_CREATION_TIME, m_entryBuffer.value( KIO::UDS_CREATION_TIME ) );
        infos.insert( KIO::UDS_MODIFICATION_TIME, m_entryBuffer.value( KIO::UDS_MODIFICATION_TIME ) );
        infos.insert( KIO::UDS_ACCESS_TIME, m_entryBuffer.value( KIO::UDS_ACCESS_TIME ) );

        if (url.isLocalFile())
	{
		infos.insert( KIO::UDS_LOCAL_PATH, url.path() );
	}
}


void MediaImpl::createMediumEntry(KIO::UDSEntry& entry,
                                  const Medium &medium)
{
	kdDebug(1219) << "MediaProtocol::createMedium" << endl;

	QString url = "media:/"+medium.name();

	kdDebug(1219) << "url = " << url << ", mime = " << medium.mimeType() << endl;

	entry.clear();

	entry.insert( KIO::UDS_URL, url );

	QString label = KIO::encodeFileName( medium.prettyLabel() );
	entry.insert( KIO::UDS_NAME, label );

	entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR);

	entry.insert( KIO::UDS_MIME_TYPE, medium.mimeType()  );
	entry.insert( KIO::UDS_GUESSED_MIME_TYPE, QString::fromLatin1("inode/directory") );

	if (!medium.iconName().isEmpty())
	{
		entry.insert( KIO::UDS_ICON_NAME, medium.iconName() );
	}
	else
	{
		QString mime = medium.mimeType();
		QString icon = KMimeType::mimeType(mime)->icon(mime, false);
		entry.insert( KIO::UDS_ICON_NAME, icon );
	}

	if (medium.needMounting())
	{
		entry.insert( KIO::UDS_ACCESS, 0400 );
	}
	else
	{
		KUrl url = medium.prettyBaseURL();
		extractUrlInfos(url, entry);
	}
}

void MediaImpl::enterLoop()
{
    QEventLoop eventLoop;
    connect(this, SIGNAL(leaveModality()),
        &eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

#include "mediaimpl.moc"
