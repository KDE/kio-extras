/* This file is part of the KDE project
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>

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

#include <kio/slavebase.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <QTextStream>
#include <klocale.h>
#include <sys/stat.h>
#include <QDataStream>
#include <time.h>
#include <kservice.h>
#include <kservicegroup.h>
#include <kstandarddirs.h>

class SettingsProtocol : public KIO::SlaveBase
{
public:
	enum RunMode { SettingsMode, ProgramsMode, ApplicationsMode };
	SettingsProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app);
	virtual ~SettingsProtocol();
	virtual void get( const KUrl& url );
	virtual void stat(const KUrl& url);
	virtual void listDir(const KUrl& url);
	void listRoot();
	KServiceGroup::Ptr findGroup(const QString &relPath);

private:
	RunMode m_runMode;
};

extern "C" {
	KDE_EXPORT int kdemain( int, char **argv )
	{
	  kDebug() << "kdemain for settings kioslave" << endl;
	  KComponentData componentData( "kio_settings" );
	  SettingsProtocol slave(argv[1], argv[2], argv[3]);
	  slave.dispatchLoop();
	  return 0;
	}
}


static void createFileEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime, const QString& iconName, const QString& localPath)
{
	entry.clear();
	entry.insert( KIO::UDSEntry::UDS_NAME, KIO::encodeFileName(name) );
	entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
	entry.insert( KIO::UDSEntry::UDS_URL, url );
	entry.insert( KIO::UDSEntry::UDS_ACCESS, 0500);
	entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, mime );
	entry.insert( KIO::UDSEntry::UDS_SIZE, 0);
	entry.insert( KIO::UDSEntry::UDS_LOCAL_PATH, localPath );
	entry.insert( KIO::UDSEntry::UDS_CREATION_TIME, 1);
	entry.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, time(0) );
	entry.insert( KIO::UDSEntry::UDS_ICON_NAME, iconName  );
}

static void createDirEntry(KIO::UDSEntry& entry, const QString& name, const QString& url, const QString& mime,const QString& iconName)
{
	entry.clear();
	entry.insert( KIO::UDSEntry::UDS_NAME, name );
	entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
	entry.insert( KIO::UDSEntry::UDS_ACCESS, 0500 );
	entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, mime );
	entry.insert( KIO::UDSEntry::UDS_URL, url );
	//entry.insert( KIO::UDS_SIZE, 0 );
	entry.insert( KIO::UDSEntry::UDS_ICON_NAME, iconName );
}

SettingsProtocol::SettingsProtocol( const QByteArray &protocol, const QByteArray &pool, const QByteArray &app): SlaveBase( protocol, pool, app )
{
	// Adjusts which part of the K Menu to virtualize.
	if ( protocol == "programs" )
		m_runMode = ProgramsMode;
	else
		if (protocol == "applications")
			m_runMode = ApplicationsMode;
		else
			m_runMode = SettingsMode;
}

SettingsProtocol::~SettingsProtocol()
{
}

KServiceGroup::Ptr SettingsProtocol::findGroup(const QString &relPath)
{
	QString nextPart;
	QString alreadyFound("Settings/");
	QStringList rest = relPath.split( '/');

	kDebug() << "Trying harder to find group " << relPath << endl;
	for ( int i=0; i<rest.count(); i++)
		kDebug() << "Item (" << rest.at(i) << ")" << endl;

	while (!rest.isEmpty()) {
		KServiceGroup::Ptr tmp = KServiceGroup::group(alreadyFound);
		if (!tmp || !tmp->isValid())
			return KServiceGroup::Ptr();

		bool found = false;
		foreach (const KSycocaEntry::Ptr &e, tmp->entries(true, true)) {
			if (e->isType(KST_KServiceGroup)) {
			    KServiceGroup::Ptr g(KServiceGroup::Ptr::staticCast(e));
			    if ((g->caption()==rest.front()) || (g->name()==alreadyFound+rest.front())) {
				kDebug() << "Found group with caption " << g->caption()
					  << " with real name: " << g->name() << endl;
				found = true;
				rest.erase(rest.begin());
				alreadyFound = g->name();
				kDebug() << "ALREADY FOUND: " << alreadyFound << endl;
				break;
			    }
			}
		}

		if (!found) {
			kDebug() << "Group with caption " << rest.front() << " not found within "
				  << alreadyFound << endl;
			return KServiceGroup::Ptr();
		}

	}
	return KServiceGroup::group(alreadyFound);
}

void SettingsProtocol::get( const KUrl & url )
{
	KService::Ptr service = KService::serviceByDesktopName(url.fileName());
	if (service && service->isValid()) {
		KUrl redirUrl;
		redirUrl.setPath(KStandardDirs::locate("apps", service->desktopEntryPath()));
		redirection(redirUrl);
		finished();
	} else {
		error( KIO::ERR_IS_DIRECTORY, url.prettyUrl() );
	}
}


void SettingsProtocol::stat(const KUrl& url)
{
	KIO::UDSEntry entry;

	QString servicePath( url.path( KUrl::AddTrailingSlash ) );
	servicePath.remove(0, 1); // remove starting '/'

	if ( m_runMode == SettingsMode)
		servicePath = "Settings/" + servicePath;

	KServiceGroup::Ptr grp = KServiceGroup::group(servicePath);

	if (grp && grp->isValid()) {
		createDirEntry(entry, (m_runMode == SettingsMode) ? i18n("Settings") : ( (m_runMode==ApplicationsMode) ? i18n("Applications") : i18n("Programs")),
			url.url(), "inode/directory",grp->icon() );
	} else {
		KService::Ptr service = KService::serviceByDesktopName( url.fileName() );
		if (service && service->isValid()) {
//			KUrl newUrl;
//			newUrl.setPath(KStandardDirs::locate("apps", service->desktopEntryPath()));
//			createFileEntry(entry, service->name(), newUrl, "application/x-desktop", service->icon());

			createFileEntry(entry, service->name(), url.url( KUrl::AddTrailingSlash )+service->desktopEntryName(),
                            "application/x-desktop", service->icon(), KStandardDirs::locate("apps", service->desktopEntryPath()) );
		} else {
			error(KIO::ERR_SLAVE_DEFINED,i18n("Unknown settings folder"));
			return;
		}
	}

	statEntry(entry);
	finished();
	return;
}


void SettingsProtocol::listDir(const KUrl& url)
{
	QString groupPath = url.path( KUrl::AddTrailingSlash );
	groupPath.remove(0, 1); // remove starting '/'

	if ( m_runMode == SettingsMode)
		groupPath.prepend("Settings/");

	KServiceGroup::Ptr grp = KServiceGroup::group(groupPath);

	if (!grp || !grp->isValid()) {
		grp = findGroup(groupPath);
		if (!grp || !grp->isValid()) {
		    error(KIO::ERR_SLAVE_DEFINED,i18n("Unknown settings folder"));
		    return;
		}
	}

	unsigned int count = 0;
	KIO::UDSEntry entry;

	foreach (const KSycocaEntry::Ptr &e, grp->entries(true, true)) {
		if (e->isType(KST_KServiceGroup)) {
			KServiceGroup::Ptr g(KServiceGroup::Ptr::staticCast(e));
			QString groupCaption = g->caption();

			// Avoid adding empty groups.
			KServiceGroup::Ptr subMenuRoot = KServiceGroup::group(g->relPath());
			if (subMenuRoot->childCount() == 0)
			    continue;

			// Ignore dotfiles.
			if ((g->name().at(0) == '.'))
			    continue;

			QString relPath = g->relPath();

			// Do not display the "Settings" menu group in Programs Mode.
			if( (m_runMode == ProgramsMode) && relPath.startsWith( "Settings" ) )
			{
				kDebug() << "SettingsProtocol: SKIPPING entry programs:/" << relPath << endl;
				continue;
			}

			switch( m_runMode )
			{
			  case( SettingsMode ):
				relPath.remove(0, 9); // length("Settings/") ==9
				kDebug() << "SettingsProtocol: adding entry settings:/" << relPath << endl;
				createDirEntry(entry, groupCaption, "settings:/"+relPath, "inode/directory",g->icon());
				break;
			  case( ProgramsMode ):
				kDebug() << "SettingsProtocol: adding entry programs:/" << relPath << endl;
				createDirEntry(entry, groupCaption, "programs:/"+relPath, "inode/directory",g->icon());
				break;
			  case( ApplicationsMode ):
				kDebug() << "SettingsProtocol: adding entry applications:/" << relPath << endl;
				createDirEntry(entry, groupCaption, "applications:/"+relPath, "inode/directory",g->icon());
				break;
		    }

		} else {
			KService::Ptr s(KService::Ptr::staticCast(e));
			kDebug() << "SettingsProtocol: adding file entry " << url.url( KUrl::AddTrailingSlash )+s->name() << endl;
			createFileEntry(entry,s->name(),url.url( KUrl::AddTrailingSlash )+s->desktopEntryName(), "application/x-desktop",s->icon(),KStandardDirs::locate("apps", s->desktopEntryPath()));
		}

		listEntry(entry, false);
		count++;
	}

	totalSize(count);
	listEntry(entry, true);
	finished();
}

// vim: ts=4 sw=4 et
