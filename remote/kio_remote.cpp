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

#include <stdlib.h>

#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kservice.h>
#include <kmimetype.h>
#include <kdesktopfile.h>

#include <qdir.h>

#include "kio_remote.h"

#define WIZARD_SERVICE "knetattach"

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
		KCmdLineArgs::init(argc, argv, "kio_remote", 0, 0, 0, 0);
		KCmdLineArgs::addCmdLineOptions( options );
		KApplication app( false, false );
		// We want to be anonymous even if we use DCOP
		app.dcopClient()->attach();

		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		RemoteProtocol slave( args->arg(0), args->arg(1), args->arg(2) );
		slave.dispatchLoop();
		return 0;
	}
}


RemoteProtocol::RemoteProtocol(const QCString &protocol,
                               const QCString &pool, const QCString &app)
	: ForwardingSlaveBase(protocol, pool, app)
{
	KGlobal::dirs()->addResourceType("remote_entries",
		KStandardDirs::kde_default("data") + "remoteview");

	QString path = KGlobal::dirs()->saveLocation("remote_entries");

	QDir dir = path;
	if (!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("remoteview");
	}

	m_baseURL.setPath(path);
}

RemoteProtocol::~RemoteProtocol()
{
}

static void addAtom(KIO::UDSEntry& entry, unsigned int ID,
                    long l, const QString& s = QString::null)
{
	KIO::UDSAtom atom;
	atom.m_uds = ID;
	atom.m_long = l;
	atom.m_str = s;
	entry.append(atom);
}

static KURL findWizardRealURL()
{
	KURL url;
	KService::Ptr service = KService::serviceByDesktopName(WIZARD_SERVICE);

	if (service && service->isValid())
	{
		url.setPath( locate("apps",
				    service->desktopEntryPath())
				);
	}

	return url;
}

static bool createWizardEntry(KIO::UDSEntry &entry)
{
	entry.clear();

	KURL url = findWizardRealURL();

	if (!url.isValid())
	{
		return false;
	}

	addAtom(entry, KIO::UDS_NAME, 0, i18n("Add a Network Folder"));
	addAtom(entry, KIO::UDS_FILE_TYPE, S_IFREG);
	addAtom(entry, KIO::UDS_URL, 0, url.url());
	addAtom(entry, KIO::UDS_ACCESS, 0500);
	addAtom(entry, KIO::UDS_MIME_TYPE, 0, "application/x-desktop");
	addAtom(entry, KIO::UDS_ICON_NAME, 0, "wizard");

	return true;
}


bool RemoteProtocol::rewriteURL(const KURL &url, KURL &newUrl)
{
	newUrl = m_baseURL;
	newUrl.addPath(url.path());

	return true;
}

void RemoteProtocol::prepareUDSEntry(KIO::UDSEntry &entry, bool listing) const
{
	KIO::UDSEntry::iterator it = entry.begin();
	KIO::UDSEntry::iterator end = entry.end();

	QString name;

	for(; it!=end; ++it)
	{
		if ( (*it).m_uds == KIO::UDS_NAME )
		{
			name = (*it).m_str;
			break;
		}
	}

	KURL remote = requestedURL();
	KURL real = processedURL();

	if (listing)
	{
		remote.addPath(name);
		real.addPath(name);
	}

	QString mime = KMimeType::findByPath(real.path())->name();

	if (mime == "application/x-desktop")
	{
		KDesktopFile desktop(real.path());

		entry.clear();

		addAtom(entry, KIO::UDS_NAME, 0, desktop.readName());
		addAtom(entry, KIO::UDS_FILE_TYPE, S_IFDIR);
		addAtom(entry, KIO::UDS_URL, 0, remote.url());
		addAtom(entry, KIO::UDS_ACCESS, 0500);
		addAtom(entry, KIO::UDS_MIME_TYPE, 0, "inode/directory");
		addAtom(entry, KIO::UDS_ICON_NAME, 0, desktop.readIcon());

		return;
	}

	ForwardingSlaveBase::prepareUDSEntry(entry, listing);
}


void RemoteProtocol::listDir(const KURL &url)
{
	kdDebug() << "RemoteProtocol::listDir: " << url << endl;

	if ( url.path().length() <= 1 )
	{
		KIO::UDSEntry entry;
		if ( createWizardEntry(entry) )
		{
			listEntry(entry, false);
			listEntry(entry, true);
		}
	}
	else
	{
		KURL new_url;
		rewriteURL(url, new_url);
		QString mime = KMimeType::findByPath(new_url.path())->name();
		if (mime =="application/x-desktop")
		{
			KDesktopFile desktop(new_url.path(), true);

			redirection(desktop.readURL());
			finished();
			return;
		}
	}

	ForwardingSlaveBase::listDir(url);
}


#include "kio_remote.moc"
