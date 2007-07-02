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
#include <kcmdlineargs.h>

#include <QEventLoop>
#include <QFile>

#include "kio_system.h"


extern "C" {
	KDE_EXPORT int kdemain( int argc, char **argv )
	{
		// KApplication is necessary to use other ioslaves
		putenv(strdup("SESSION_MANAGER="));
		KCmdLineArgs::init(argc, argv, "kio_system", 0, KLocalizedString(), 0);

		KCmdLineOptions options;
		options.add("+protocol", ki18n( "Protocol name" ));
		options.add("+pool", ki18n( "Socket name" ));
		options.add("+app", ki18n( "Socket name" ));
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication app( false );

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		SystemProtocol slave( QFile::encodeName( args->arg(0) ),
		                      QFile::encodeName( args->arg(1) ),
		                      QFile::encodeName( args->arg(2) ) );
		slave.dispatchLoop();
		return 0;
	}
}


SystemProtocol::SystemProtocol(const QByteArray &protocol,
                               const QByteArray &pool, const QByteArray &app)
	: ForwardingSlaveBase(protocol, pool, app)
{
}

SystemProtocol::~SystemProtocol()
{
}

bool SystemProtocol::rewriteUrl(const KUrl &url, KUrl &newUrl)
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


void SystemProtocol::stat(const KUrl &url)
{
	kDebug() << "SystemProtocol::stat: " << url << endl;

	QString path = url.path();
	if ( path.isEmpty() || path == "/" )
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

		if ( m_impl.statByName(name, entry) )
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

void SystemProtocol::listDir(const KUrl &url)
{
	kDebug() << "SystemProtocol::listDir: " << url << endl;

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

void SystemProtocol::listRoot()
{
	KIO::UDSEntry entry;

	KIO::UDSEntryList system_entries;
	bool ok = m_impl.listRoot(system_entries);

	if (!ok)
	{
		error( m_impl.lastErrorCode(), m_impl.lastErrorMessage() );
		return;
	}

	totalSize(system_entries.count()+1);

	m_impl.createTopLevelEntry(entry);
	listEntry(entry, false);

	KIO::UDSEntryList::ConstIterator it = system_entries.begin();
	const KIO::UDSEntryList::ConstIterator end = system_entries.end();

	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}


//#include "kio_system.moc"
