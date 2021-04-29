/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QDebug>
#include <QStandardPaths>

#include <kservicetypetrader.h>
#include <kio/slavebase.h>
#include <kservice.h>
#include <kservicegroup.h>

#include <time.h>

class SettingsProtocol : public KIO::SlaveBase
{
public:
    SettingsProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app);
    ~SettingsProtocol() override;
    void get( const QUrl& url ) override;
    void stat(const QUrl& url) override;
    void listDir(const QUrl& url) override;

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
    Q_DECL_EXPORT int kdemain( int argc, char **argv )
    {
        QCoreApplication app(argc, argv);
        app.setApplicationName("kio_settings");
        qDebug() << "kdemain for settings kioslave";
        SettingsProtocol slave(argv[1], argv[2], argv[3]);
        slave.dispatchLoop();
        return 0;
    }
}

static void createFileEntry(KIO::UDSEntry& entry, const KService::Ptr& service)
{
    entry.clear();
    entry.reserve(9);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, KIO::encodeFileName(service->desktopEntryName()));
    entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, service->name()); // translated name
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, 0500);
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, "application/x-desktop");
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_LOCAL_PATH, QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("kservices5/") + service->entryPath()));
    entry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, time(nullptr));
    entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, service->icon());
}

static void createDirEntry(KIO::UDSEntry& entry, const QString& name, const QString& iconName)
{
    entry.clear();
    entry.reserve(6); // +1 for UDS_DISPLAY_NAME
    entry.fastInsert( KIO::UDSEntry::UDS_NAME, name );
    entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry.fastInsert( KIO::UDSEntry::UDS_ACCESS, 0500 );
    entry.fastInsert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );
    entry.fastInsert( KIO::UDSEntry::UDS_ICON_NAME, iconName );
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

void SettingsProtocol::stat(const QUrl& url)
{
    initSettingsData();
    const QString fileName = url.fileName();
    qDebug() << fileName;

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
        const QString category = service->property("X-KDE-System-Settings-Category").toString();
        createDirEntry(entry, category, service->icon());
        entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, service->name());
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

void SettingsProtocol::listDir(const QUrl& url)
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
        if (parentCategory == fileName) {
            //KUrl dirUrl = url;
            //dirUrl.addPath(category);
            createDirEntry(entry, category, service->icon());
            entry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, service->name());
            listEntry(entry);
            ++count;
        }
    }

    // scan for any modules at this level and add them
    for (int i = 0; i < m_modules.size(); ++i) {
        const KService::Ptr service = m_modules.at(i);
        const QString category = service->property("X-KDE-System-Settings-Parent-Category").toString();
        if (!fileName.isEmpty() && category == fileName) {
            createFileEntry(entry, service);
            listEntry(entry);
            ++count;
        }
    }

    totalSize(count);
    finished();
}

void SettingsProtocol::get( const QUrl & url )
{
    KService::Ptr service = KService::serviceByDesktopName(url.fileName());
    if (service && service->isValid()) {
        QUrl redirUrl = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("kservices5/") + service->entryPath()));
        redirection(redirUrl);
        finished();
    } else {
        error( KIO::ERR_IS_DIRECTORY, url.toDisplayString() );
    }
}
