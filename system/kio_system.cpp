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
#include <dcopclient.h>
#include <dcopref.h>

#include <qeventloop.h>
//Added by qt3to4:
#include <Q3CString>

#include "kio_system.h"


static const KCmdLineOptions options[] =
{
	{ "+protocol", I18N_NOOP( "Protocol name" ), 0 },
	{ "+pool", I18N_NOOP( "Socket name" ), 0 },
	{ "+app", I18N_NOOP( "Socket name" ), 0 },
	KCmdLineLastOption
};

extern "C" {
	KDE_EXPORT int kdemain( int argc, char **argv )
	{
		// KApplication is necessary to use other ioslaves
		putenv(strdup("SESSION_MANAGER="));
		KCmdLineArgs::init(argc, argv, "kio_system", 0, 0, 0, 0);
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication app( false, false );
		// We want to be anonymous even if we use DCOP
		app.dcopClient()->attach();
		DCOPRef ref( "kded", "kded" );
		ref.call( "loadModule", "systemdirnotify" );

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		SystemProtocol slave( args->arg(0), args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


SystemProtocol::SystemProtocol(const Q3CString &protocol,
                               const Q3CString &pool, const Q3CString &app)
	: ForwardingSlaveBase(protocol, pool, app)
{
}

SystemProtocol::~SystemProtocol()
{
}

bool SystemProtocol::rewriteURL(const KURL &url, KURL &newUrl)
{
	QString name, path;

	if ( !m_impl.parseURL(url, name, path) )
	{
		error(KIO::ERR_MALFORMED_URL, url.prettyURL());
		return false;
	}

	if ( !m_impl.realURL(name, path, newUrl) )
	{
		error( m_impl.lastErrorCode(), m_impl.lastErrorMessage() );
		return false;
	}

	return true;
}


void SystemProtocol::stat(const KURL &url)
{
	kdDebug() << "SystemProtocol::stat: " << url << endl;

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
		error(KIO::ERR_MALFORMED_URL, url.prettyURL());
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
			error(KIO::ERR_DOES_NOT_EXIST, url.prettyURL());
		}
	}
	else
	{
		ForwardingSlaveBase::stat(url);
	}
}

void SystemProtocol::listDir(const KURL &url)
{
	kdDebug() << "SystemProtocol::listDir: " << url << endl;

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

	KIO::UDSEntryListIterator it = system_entries.begin();
	KIO::UDSEntryListIterator end = system_entries.end();

	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}


//#include "kio_system.moc"
