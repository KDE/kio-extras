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
#include <dcopref.h>
#include <kcmdlineargs.h>
#include <kglobal.h>


#include "kio_remote.h"
//Added by qt3to4:
#include <Q3CString>

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
		KCmdLineArgs::init(argc, argv, "kio_remote", 0L, 0L, false);
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication app( false, false );
		// We want to be anonymous even if we use DCOP
		app.dcopClient()->attach();
		DCOPRef ref( "kded", "kded" );
		ref.call( "loadModule", "remotedirnotify" );

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		RemoteProtocol slave( args->arg(0), args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


RemoteProtocol::RemoteProtocol(const QByteArray &protocol,
                               const QByteArray &pool, const QByteArray &app)
	: SlaveBase(protocol, pool, app)
{
}

RemoteProtocol::~RemoteProtocol()
{
}

void RemoteProtocol::listDir(const KURL &url)
{
	kdDebug(1220) << "RemoteProtocol::listDir: " << url << endl;

	if ( url.path().length() <= 1 )
	{
		listRoot();
		return;
	}

	int second_slash_idx = url.path().find( '/', 1 );
	QString root_dirname = url.path().mid( 1, second_slash_idx-1 );
	
	KURL target = m_impl.findBaseURL( root_dirname );
	kdDebug(1220) << "possible redirection target : " << target << endl;
	if( target.isValid() )
	{
		target.addPath( url.path().remove(0, second_slash_idx) );
		redirection(target);
		finished();
		return;
	}

	error(KIO::ERR_MALFORMED_URL, url.prettyURL());
}

void RemoteProtocol::listRoot()
{
	KIO::UDSEntry entry;

	KIO::UDSEntryList remote_entries;
        m_impl.listRoot(remote_entries);

	totalSize(remote_entries.count()+2);

	m_impl.createTopLevelEntry(entry);
	listEntry(entry, false);

	m_impl.createWizardEntry(entry);
	listEntry(entry, false);

	KIO::UDSEntryList::ConstIterator it = remote_entries.begin();
	const KIO::UDSEntryList::ConstIterator end = remote_entries.end();
	for(; it!=end; ++it)
	{
		listEntry(*it, false);
	}

	entry.clear();
	listEntry(entry, true);

	finished();
}

void RemoteProtocol::stat(const KURL &url)
{
	kdDebug(1220) << "RemoteProtocol::stat: " << url << endl;

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

	if (m_impl.isWizardURL(url))
	{
		KIO::UDSEntry entry;
		if (m_impl.createWizardEntry(entry))
		{
			statEntry(entry);
			finished();
		}
		else
		{
			error(KIO::ERR_DOES_NOT_EXIST, url.prettyURL());
		}
		return;
	}

	int second_slash_idx = url.path().find( '/', 1 );
	QString root_dirname = url.path().mid( 1, second_slash_idx-1 );
	
	if ( second_slash_idx==-1 || ( (int)url.path().length() )==second_slash_idx+1 )
	{
		KIO::UDSEntry entry;
		if (m_impl.statNetworkFolder(entry, root_dirname))
		{
			statEntry(entry);
			finished();
			return;
		}
	}
	else
	{
		KURL target = m_impl.findBaseURL(  root_dirname );
		kdDebug( 1220 ) << "possible redirection target : " << target << endl;
		if (  target.isValid() )
		{
			target.addPath( url.path().remove( 0, second_slash_idx ) );
			redirection( target );
			finished();
			return;
		}
	}
	
	error(KIO::ERR_MALFORMED_URL, url.prettyURL());
}

void RemoteProtocol::del(const KURL &url, bool /*isFile*/)
{
	kdDebug(1220) << "RemoteProtocol::del: " << url << endl;

	if (!m_impl.isWizardURL(url)
	 && m_impl.deleteNetworkFolder(url.fileName()))
	{
		finished();
		return;
	}

	error(KIO::ERR_CANNOT_DELETE, url.prettyURL());
}

void RemoteProtocol::get(const KURL &url)
{
	kdDebug(1220) << "RemoteProtocol::get: " << url << endl;

	QString file = m_impl.findDesktopFile( url.fileName() );
	kdDebug(1220) << "desktop file : " << file << endl;

	if (!file.isEmpty())
	{
		KURL desktop;
		desktop.setPath(file);

		redirection(desktop);
		finished();
		return;
	}

	error(KIO::ERR_MALFORMED_URL, url.prettyURL());
}

void RemoteProtocol::rename(const KURL &src, const KURL &dest,
                            bool overwrite)
{
	if (src.protocol()!="remote" || dest.protocol()!="remote"
         || m_impl.isWizardURL(src) || m_impl.isWizardURL(dest))
	{
		error(KIO::ERR_UNSUPPORTED_ACTION, src.prettyURL());
		return;
	}

	if (m_impl.renameFolders(src.fileName(), dest.fileName(), overwrite))
	{
		finished();
		return;
	}

	error(KIO::ERR_CANNOT_RENAME, src.prettyURL());
}
