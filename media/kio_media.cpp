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

#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kcmdlineargs.h>

#include <qeventloop.h>

#include "kio_media.h"


static const KCmdLineOptions options[] =
{
	{ "+protocol", I18N_NOOP( "Protocol name" ), 0 },
	{ "+pool", I18N_NOOP( "Socket name" ), 0 },
	{ "+app", I18N_NOOP( "Socket name" ), 0 },
	KCmdLineLastOption
};

extern "C" {
	int kdemain( int argc, char **argv )
	{
		// KApplication is necessary to use other ioslaves
		KCmdLineArgs::init(argc, argv, "kio_media", 0, 0, 0, 0);
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication app( false, false );
		// We want to be anonymous even if we use DCOP
		app.dcopClient()->attach();
		app.disableSessionManagement();

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		MediaProtocol slave( args->arg(0), args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


MediaProtocol::MediaProtocol(const QCString &protocol,
                             const QCString &pool, const QCString &app)
	: QObject(), SlaveBase(protocol, pool, app)
{
}

MediaProtocol::~MediaProtocol()
{
}

void MediaProtocol::get(const KURL &url)
{
	kdDebug() << "MediaProtocol::get: " << url << endl;

	error(KIO::ERR_UNSUPPORTED_ACTION, "Not implemented yet");
}

void MediaProtocol::put(const KURL &url, int mode, bool overwrite, bool resume)
{
	kdDebug() << "MediaProtocol::put: " << url << ", " << overwrite << ", "
	          << resume << endl;

	error(KIO::ERR_UNSUPPORTED_ACTION, "Not implemented yet");
}

void MediaProtocol::copy(const KURL &src, const KURL &dest,
                         int mode, bool overwrite )
{
	kdDebug() << "MediaProtocol::copy: " << src << ", " << dest << ", "
	          << overwrite << endl;

	error(KIO::ERR_UNSUPPORTED_ACTION, "Not implemented yet");
}

void MediaProtocol::rename(const KURL &src, const KURL &dest, bool overwrite)
{
	kdDebug() << "MediaProtocol::rename: " << src << ", " << dest << ", "
	          << overwrite << endl;

	error(KIO::ERR_UNSUPPORTED_ACTION, "Not implemented yet");
}

void MediaProtocol::symlink(const QString &target, const KURL &dest,
                            bool overwrite)
{
	kdDebug() << "MediaProtocol::symlink: " << target << ", "
	          << dest << ", " << overwrite << endl;

	error(KIO::ERR_UNSUPPORTED_ACTION, "Not implemented yet");
}

void MediaProtocol::stat(const KURL &url)
{
	kdDebug() << "MediaProtocol::stat: " << url << endl;
	QString path = url.path();
	if( path.isEmpty() || path == "/" )
	{
		// The root is "virtual" - it's not a single physical directory
		KIO::UDSEntry entry;
		m_impl.createTopLevelEntry( entry );
		statEntry( entry );
		finished();
		return;
	}

	QString name;
	bool ok = m_impl.parseURL(url, name, path);

	if ( !ok )
	{
		error(KIO::ERR_MALFORMED_URL, url.prettyURL());
		return;
	}

	if( path.isEmpty() )
	{
		KIO::UDSEntry entry;
		ok = m_impl.statMedium(name, entry);
		statEntry(entry);
		finished();
	}
	else
	{
		KIO::StatJob *job = m_impl.stat(name, path);
		if ( job == 0L )
		{
			error( m_impl.lastErrorCode(), m_impl.lastErrorMessage() );
		}
		else
		{
			connect( job, SIGNAL( result(KIO::Job *) ),
			         this, SLOT( slotStatResult(KIO::Job *) ) );

			qApp->eventLoop()->enterLoop();
		}
	}
}

void MediaProtocol::slotStatResult(KIO::Job *job)
{
	if ( job->error() == 0)
	{
		statEntry( static_cast<KIO::StatJob *>(job)->statResult() );
	}

	slotResult(job);
}

void MediaProtocol::listDir(const KURL &url)
{
	kdDebug() << "MediaProtocol::listDir: " << url << endl;

	if ( url.path().length() <= 1 )
	{
		listRoot();
		return;
	}

	QString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( !ok )
	{
		error(KIO::ERR_MALFORMED_URL, url.prettyURL());
		return;
	}

	KIO::ListJob *job = m_impl.list(name, path);
	if ( job == 0L )
	{
		error( m_impl.lastErrorCode(), m_impl.lastErrorMessage() );
	}
	else
	{
		connect( job, SIGNAL( result(KIO::Job *) ),
			 this, SLOT( slotResult(KIO::Job *) ) );
		connect( job, SIGNAL( entries(KIO::Job *, const KIO::UDSEntryList &) ),
			 this, SLOT( slotEntries(KIO::Job *, const KIO::UDSEntryList &) ) );
		qApp->eventLoop()->enterLoop();
	}
}

void MediaProtocol::slotEntries(KIO::Job */*job*/, const KIO::UDSEntryList &entries)
{
	listEntries( entries );
}

void MediaProtocol::listRoot()
{
	KIO::UDSEntry entry;

	KIO::UDSEntryList media_entries;
	bool ok = m_impl.listMedia(media_entries);

	if (!ok)
	{
		error( m_impl.lastErrorCode(), m_impl.lastErrorMessage() );
		return;
	}

	totalSize(media_entries.count()+1);

	m_impl.createTopLevelEntry(entry);
	listEntry(entry, false);

	KIO::UDSEntryListIterator it = media_entries.begin();
	KIO::UDSEntryListIterator end = media_entries.end();

	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}

void MediaProtocol::mkdir(const KURL &url, int permissions)
{
	kdDebug() << "MediaProtocol::mkdir: " << url << endl;

	error(KIO::ERR_UNSUPPORTED_ACTION, "Not implemented yet");
}

void MediaProtocol::chmod(const KURL &url, int permissions)
{
	kdDebug() << "MediaProtocol::chmod: " << url << endl;

	error(KIO::ERR_UNSUPPORTED_ACTION, "Not implemented yet");
}

void MediaProtocol::del(const KURL &url, bool isFile)
{
	kdDebug() << "MediaProtocol::del: " << url << ", " << isFile << endl;

	error(KIO::ERR_UNSUPPORTED_ACTION, "Not implemented yet");
}


void MediaProtocol::slotResult(KIO::Job *job)
{
	if ( job->error() != 0)
	{
		error( job->error(), job->errorString() );
	}
	else
	{
		finished();
	}

	qApp->eventLoop()->exitLoop();
}

#include "kio_media.moc"
