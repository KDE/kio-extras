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

#include <stdlib.h>

#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kcmdlineargs.h>

#include <QEventLoop>
#include <QByteArray>

#include "kio_media.h"


static const KCmdLineOptions options[] =
{
	{ "+protocol", I18N_NOOP( "Protocol name" ), 0 },
	{ "+pool", I18N_NOOP( "Socket name" ), 0 },
	{ "+app", I18N_NOOP( "Socket name" ), 0 },
	KCmdLineLastOption
};

extern "C" {
	int KDE_EXPORT kdemain( int argc, char **argv )
	{
		// KApplication is necessary to use other ioslaves
		putenv(strdup("SESSION_MANAGER="));
		KCmdLineArgs::init(argc, argv, "kio_media", 0, 0, 0);
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication app(  false );
		// We want to be anonymous even if we use DCOP
		app.dcopClient()->attach();

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		MediaProtocol slave( args->arg(0), args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


MediaProtocol::MediaProtocol(const QByteArray &protocol, const QByteArray &pool,
				                  const QByteArray &app)
	: ForwardingSlaveBase(protocol, pool, app)
{
	connect( &m_impl, SIGNAL( warning( const QString & ) ),
	         this, SLOT( slotWarning( const QString & ) ) );
}

MediaProtocol::~MediaProtocol()
{
}

bool MediaProtocol::rewriteURL(const KUrl &url, KUrl &newUrl)
{
	QString name, path;

	if ( !m_impl.parseURL(url, name, path) )
	{
		error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
		return false;
	}


	if ( !m_impl.realURL(name, path, newUrl) )
	{
		error( m_impl.lastErrorCode(), m_impl.lastErrorMessage() );
		return false;
	}

	return true;
}

void MediaProtocol::put(const KUrl &url, int permissions,
                        bool overwrite, bool resume)
{
	kDebug(1219) << "MediaProtocol::put: " << url << endl;

	QString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( ok && path.isEmpty() )
	{
		error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, url.prettyUrl());
	}
	else
	{
		ForwardingSlaveBase::put(url, permissions, overwrite, resume);
	}
}

void MediaProtocol::rename(const KUrl &src, const KUrl &dest, bool overwrite)
{
	kDebug(1219) << "MediaProtocol::rename: " << src << ", " << dest << ", "
	          << overwrite << endl;

	QString src_name, src_path;
	bool ok = m_impl.parseURL(src, src_name, src_path);
	QString dest_name, dest_path;
	ok &= m_impl.parseURL(dest, dest_name, dest_path);

	if ( ok && src_path.isEmpty() && dest_path.isEmpty()
	  && src.protocol() == "media" && dest.protocol() == "media" )
	{
		if (!m_impl.setUserLabel(src_name, dest_name))
		{
			error(m_impl.lastErrorCode(), m_impl.lastErrorMessage());
		}
		else
		{
			finished();
		}
	}
	else
	{
		ForwardingSlaveBase::rename(src, dest, overwrite);
	}
}

void MediaProtocol::mkdir(const KUrl &url, int permissions)
{
	kDebug(1219) << "MediaProtocol::mkdir: " << url << endl;

	QString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( ok && path.isEmpty() )
	{
		error(KIO::ERR_COULD_NOT_MKDIR, url.prettyUrl());
	}
	else
	{
		ForwardingSlaveBase::mkdir(url, permissions);
	}
}

void MediaProtocol::del(const KUrl &url, bool isFile)
{
	kDebug(1219) << "MediaProtocol::del: " << url << endl;

	QString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( ok && path.isEmpty() )
	{
		error(KIO::ERR_CANNOT_DELETE, url.prettyUrl());
	}
	else
	{
		ForwardingSlaveBase::del(url, isFile);
	}
}

void MediaProtocol::stat(const KUrl &url)
{
	kDebug(1219) << "MediaProtocol::stat: " << url << endl;
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
		error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
		return;
	}

	if( path.isEmpty() )
	{
		KIO::UDSEntry entry;

		if ( m_impl.statMedium(name, entry)
		  || m_impl.statMediumByLabel(name, entry) )
		{
			statEntry(entry);
			finished();
		}
		else
		{
			error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
		}
	}
	else
	{
		ForwardingSlaveBase::stat(url);
	}
}

void MediaProtocol::listDir(const KUrl &url)
{
	kDebug(1219) << "MediaProtocol::listDir: " << url << endl;

	if ( url.path().length() <= 1 )
	{
		listRoot();
		return;
	}

	QString name, path;
	bool ok = m_impl.parseURL(url, name, path);

	if ( !ok )
	{
		error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
		return;
	}

	ForwardingSlaveBase::listDir(url);
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

	KIO::UDSEntryList::ConstIterator it = media_entries.begin();
	const KIO::UDSEntryList::ConstIterator end = media_entries.end();

	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}

void MediaProtocol::slotWarning( const QString &msg )
{
	warning( msg );
}

#include "kio_media.moc"
