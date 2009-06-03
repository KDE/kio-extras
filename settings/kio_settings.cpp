/* This file is part of the KDE project
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2008 David Faure <faure@kde.org>

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

#include <kservicetypetrader.h>
#include <kio/slavebase.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <klocale.h>
#include <sys/stat.h>
#include <time.h>
#include <kservice.h>
#include <kservicegroup.h>
#include <kstandarddirs.h>

class SettingsProtocol : public KIO::SlaveBase
{
public:
    SettingsProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app);
    virtual ~SettingsProtocol();
    virtual void get( const KUrl& url );
    virtual void stat(const KUrl& url);
    virtual void listDir(const KUrl& url);

private:
    void initSettingsData();

private:
    bool m_settingsDataLoaded;
    KService::List m_modules;
    QHash<QString, KService::Ptr> m_settingsServiceLookup;
    KService::List m_categories;
    QHash<QString, KService::Ptr> m_categoryLookup;
};

extern "C" {
    KDE_EXPORT int kdemain( int, char **argv )
    {
        kDebug() << "kdemain for settings kioslave";
        KComponentData componentData( "kio_settings" );
        SettingsProtocol slave(argv[1], argv[2], argv[3]);
        slave.dispatchLoop();
        return 0;
    }
}

static void createFileEntry(KIO::UDSEntry& entry, const KService::Ptr& service)
{
    entry.clear();
    entry.insert(KIO::UDSEntry::UDS_NAME, KIO::encodeFileName(service->desktopEntryName()));
    entry.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, service->name()); // translated name
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.insert(KIO::UDSEntry::UDS_ACCESS, 0500);
    entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, "application/x-desktop");
    entry.insert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.insert(KIO::UDSEntry::UDS_LOCAL_PATH, KStandardDirs::locate("services", service->entryPath()));
    entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, time(0));
    entry.insert(KIO::UDSEntry::UDS_ICON_NAME, service->icon());
}

static void createDirEntry(KIO::UDSEntry& entry, const QString& name, const QString& iconName)
{
    entry.clear();
    entry.insert( KIO::UDSEntry::UDS_NAME, name );
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry.insert( KIO::UDSEntry::UDS_ACCESS, 0500 );
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );
    entry.insert( KIO::UDSEntry::UDS_ICON_NAME, iconName );
}

SettingsProtocol::SettingsProtocol( const QByteArray &protocol, const QByteArray &pool, const QByteArray &app)
    : SlaveBase( protocol, pool, app ),
      m_settingsDataLoaded(false)
{
}

SettingsProtocol::~SettingsProtocol()
{
}

void SettingsProtocol::initSettingsData()
{
    if (m_settingsDataLoaded)
        return;

    // The code for settings:/ was inspired by kdebase/workspace/systemsettings/mainwindow.cpp readMenu().

    m_modules = KServiceTypeTrader::self()->query("KCModule");
    m_categories = KServiceTypeTrader::self()->query("SystemSettingsCategory");

    for (int i = 0; i < m_categories.size(); ++i) {
        const KService::Ptr service = m_categories.at(i);
        const QString category = service->property("X-KDE-System-Settings-Category").toString();
        m_categoryLookup.insert(category, service);
    }
    for (int i = 0; i < m_modules.size(); ++i) {
        const KService::Ptr service = m_modules.at(i);
        // Since modules have a unique name, we can just look them up by name,
        // no need to create a real hierarchical structure just for stat().
        //const QString category = service->property("X-KDE-System-Settings-Parent-Category").toString();
        m_settingsServiceLookup.insert(service->desktopEntryName(), service);
    }
}

void SettingsProtocol::stat(const KUrl& url)
{
    initSettingsData();
    const QString fileName = url.fileName();
    kDebug() << fileName;

    KIO::UDSEntry entry;
    // Root dir?
    if (fileName.isEmpty()) {
        createDirEntry(entry, ".", "preferences-system");
        statEntry(entry);
        finished();
        return;
    }

    // Is it a category?
    QHash<QString, KService::Ptr>::const_iterator it = m_categoryLookup.constFind(fileName);
    if (it != m_categoryLookup.constEnd()) {
        const KService::Ptr service = it.value();
        const QString parentCategory = service->property("X-KDE-System-Settings-Parent-Category").toString();
        const QString category = service->property("X-KDE-System-Settings-Category").toString();
        //kDebug() << "category" << service->desktopEntryName() << service->name() << "category=" << category << "parentCategory=" << parentCategory;
        createDirEntry(entry, category, service->icon());
        entry.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, service->name());
        statEntry(entry);
        finished();
        return;
    } else {
        // Is it a config module?
        it = m_settingsServiceLookup.constFind(fileName);
        if (it != m_settingsServiceLookup.constEnd()) {
            const KService::Ptr service = it.value();
            createFileEntry(entry, service);
            statEntry(entry);
            finished();
            return;
        }
    }

    error(KIO::ERR_DOES_NOT_EXIST, url.url());
}

void SettingsProtocol::listDir(const KUrl& url)
{
    initSettingsData();
    const QString fileName = url.fileName();
    if (!fileName.isEmpty() && !m_categoryLookup.contains(fileName)) {
        error(KIO::ERR_DOES_NOT_EXIST, fileName);
        return;
    }

    unsigned int count = 0;
    KIO::UDSEntry entry;

    // scan for any categories at this level and add them
    for (int i = 0; i < m_categories.size(); ++i) {
        const KService::Ptr service = m_categories.at(i);
        QString parentCategory = service->property("X-KDE-System-Settings-Parent-Category").toString();
        QString category = service->property("X-KDE-System-Settings-Category").toString();
        //kDebug() << "category" << service->desktopEntryName() << service->name() << "category=" << category << "parentCategory=" << parentCategory;
        if (parentCategory == fileName) {
            //KUrl dirUrl = url;
            //dirUrl.addPath(category);
            createDirEntry(entry, category, service->icon());
            entry.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, service->name());
            listEntry(entry, false);
            ++count;
        }
    }

    // scan for any modules at this level and add them
    for (int i = 0; i < m_modules.size(); ++i) {
        const KService::Ptr service = m_modules.at(i);
        const QString category = service->property("X-KDE-System-Settings-Parent-Category").toString();
        if (!fileName.isEmpty() && category == fileName) {
            createFileEntry(entry, service);
            listEntry(entry, false);
            ++count;
        }
    }

    totalSize(count);
    listEntry(entry, true);
    finished();
}

void SettingsProtocol::get( const KUrl & url )
{
    KService::Ptr service = KService::serviceByDesktopName(url.fileName());
    if (service && service->isValid()) {
        KUrl redirUrl;
        redirUrl.setPath(KStandardDirs::locate("services", service->entryPath()));
        redirection(redirUrl);
        finished();
    } else {
        error( KIO::ERR_IS_DIRECTORY, url.prettyUrl() );
    }
}
