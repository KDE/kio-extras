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
#include <kglobal.h>

#include <QFile>

#include "kio_remote.h"

extern "C" {
	int KDE_EXPORT kdemain( int argc, char **argv )
	{
		// KApplication is necessary to use other ioslaves
		putenv(strdup("SESSION_MANAGER="));
		KCmdLineArgs::init(argc, argv, "kio_remote", 0, ki18n(0L), false, ki18n(0L));

		KCmdLineOptions options;
		options.add("+protocol", ki18n( "Protocol name" ));
		options.add("+pool", ki18n( "Socket name" ));
		options.add("+app", ki18n( "Socket name" ));
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication app(  false );

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		RemoteProtocol slave( QFile::encodeName( args->arg(0) ), 
		                      QFile::encodeName( args->arg(1) ),
		                      QFile::encodeName( args->arg(2) ) );
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

void RemoteProtocol::listDir(const KUrl &url)
{
	kDebug(1220) << "RemoteProtocol::listDir: " << url << endl;

	if ( url.path().length() <= 1 )
	{
		listRoot();
		return;
	}

	int second_slash_idx = url.path().indexOf( '/', 1 );
	QString root_dirname = url.path().mid( 1, second_slash_idx-1 );
	
	KUrl target = m_impl.findBaseURL( root_dirname );
	kDebug(1220) << "possible redirection target : " << target << endl;
	if( target.isValid() )
	{
		target.addPath( url.path().remove(0, second_slash_idx) );
		redirection(target);
		finished();
		return;
	}

	error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
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

void RemoteProtocol::stat(const KUrl &url)
{
	kDebug(1220) << "RemoteProtocol::stat: " << url << endl;

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
			error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
		}
		return;
	}

	int second_slash_idx = url.path().indexOf( '/', 1 );
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
		KUrl target = m_impl.findBaseURL(  root_dirname );
		kDebug( 1220 ) << "possible redirection target : " << target << endl;
		if (  target.isValid() )
		{
			target.addPath( url.path().remove( 0, second_slash_idx ) );
			redirection( target );
			finished();
			return;
		}
	}
	
	error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
}

void RemoteProtocol::del(const KUrl &url, bool /*isFile*/)
{
	kDebug(1220) << "RemoteProtocol::del: " << url << endl;

	if (!m_impl.isWizardURL(url)
	 && m_impl.deleteNetworkFolder(url.fileName()))
	{
		finished();
		return;
	}

	error(KIO::ERR_CANNOT_DELETE, url.prettyUrl());
}

void RemoteProtocol::get(const KUrl &url)
{
	kDebug(1220) << "RemoteProtocol::get: " << url << endl;

	QString file = m_impl.findDesktopFile( url.fileName() );
	kDebug(1220) << "desktop file : " << file << endl;

	if (!file.isEmpty())
	{
		KUrl desktop;
		desktop.setPath(file);

		redirection(desktop);
		finished();
		return;
	}

	error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
}

void RemoteProtocol::rename(const KUrl &src, const KUrl &dest,
                            bool overwrite)
{
	if (src.protocol()!="remote" || dest.protocol()!="remote"
         || m_impl.isWizardURL(src) || m_impl.isWizardURL(dest))
	{
		error(KIO::ERR_UNSUPPORTED_ACTION, src.prettyUrl());
		return;
	}

	if (m_impl.renameFolders(src.fileName(), dest.fileName(), overwrite))
	{
		finished();
		return;
	}

	error(KIO::ERR_CANNOT_RENAME, src.prettyUrl());
}
