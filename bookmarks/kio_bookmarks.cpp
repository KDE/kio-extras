/*
   SPDX-FileCopyrightText: 2008 Xavier Vello <xavier.vello@gmail.com>

   SPDX-License-Identifier: GPL-2.0-or-later

*/

#include "kio_bookmarks.h"

#include <stdio.h>
#include <stdlib.h>

#include <QRegularExpression>
#include <qtextdocument.h>
#include <qurlquery.h>

#include <kshell.h>

#include <KLocalizedString>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kimagecache.h>
#include <qdebug.h>
#include <kfileplacesmodel.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <QGuiApplication>
#include <KIO/ApplicationLauncherJob>

using namespace KIO;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.bookmarks" FILE "bookmarks.json")
};

BookmarksProtocol::BookmarksProtocol( const QByteArray &pool, const QByteArray &app )
    : SlaveBase( "bookmarks", pool, app )
{
    manager = KBookmarkManager::userBookmarksManager();
    cfg = new KConfig( "kiobookmarksrc" );
    config = cfg->group("General");
    cache = new KImageCache("kio_bookmarks", config.readEntry("CacheSize", 5 * 1024) * 1024);
    cache->setPixmapCaching(false);

    indent = 0;
    totalsize = 0;
    columns = 4;
}

BookmarksProtocol::~BookmarksProtocol()
{
    delete manager;
    delete cache;
    delete cfg;
}

void BookmarksProtocol::parseTree()
{
    totalsize = 0;

    cfg->reparseConfiguration();
    columns =  config.readEntry("Columns", 4);
    if (columns < 1)
        columns = 1;

    manager->notifyCompleteChange("kio_bookmarks");
    tree = manager->root();

    if(tree.first().isNull())
        return;

    if(config.readEntry("FlattenTree", false))
        flattenTree(tree);

    KBookmarkGroup root;
    if(config.readEntry("ShowRoot", true))
    {
        root = tree.createNewFolder(i18n("Root"));
        tree.moveBookmark(root, KBookmark());
        root.setIcon("konqueror");
    }

    KBookmark bm = tree.first();
    KBookmark next;
    while(!bm.isNull())
    {
        next = tree.next(bm);
        if (bm.isSeparator())
            tree.deleteBookmark(bm);
        else if (bm.isGroup())
            totalsize += sizeOfGroup(bm.toGroup());
        else
        {
            if(config.readEntry("ShowRoot", true))
                root.addBookmark(bm);

            tree.deleteBookmark(bm);
        }
        bm = next;
    }
    if(config.readEntry("ShowRoot", true))
        totalsize += sizeOfGroup(root);

    if(config.readEntry("ShowPlaces", true))
        totalsize += addPlaces();
}

int BookmarksProtocol::addPlaces()
{
    KFilePlacesModel placesModel;
    KBookmarkGroup folder = tree.createNewFolder(i18n("Places"));
    QList<Solid::Device> batteryList = Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString());

    if (batteryList.isEmpty()) {
        folder.setIcon("computer");
    } else {
        folder.setIcon("computer-laptop");
    }

    for (int row = 0; row < placesModel.rowCount(); ++row) {
        QModelIndex index = placesModel.index(row, 0);

        if (!placesModel.isHidden(index))
            folder.addBookmark(placesModel.bookmarkForIndex(index));
    }
    return sizeOfGroup(folder);
}

void BookmarksProtocol::flattenTree( const KBookmarkGroup &folder )
{
    KBookmark bm = folder.first();
    KBookmark prev = folder;
    KBookmark next;
    while (!bm.isNull())
    {
        if (bm.isGroup()) {
            flattenTree(bm.toGroup());
        }

        next = tree.next(bm);

        if (bm.isGroup() && bm.parentGroup().hasParent()) {
            bm.setFullText("| " + bm.parentGroup().fullText() + " > " + bm.fullText());
            tree.moveBookmark(bm, prev);
            prev = bm;
        }
        bm = next;
    }
}

// Should really go to KBookmarkGroup
int BookmarksProtocol::sizeOfGroup( const KBookmarkGroup &folder, bool real )
{
    int size = 1;  // counting the title line
    for (KBookmark bm = folder.first(); !bm.isNull(); bm = folder.next(bm))
    {
        if (bm.isGroup())
            size += sizeOfGroup(bm.toGroup());
        else
            size += 1;
    }

    // CSS sets a min-height for toplevel folders
    if (folder.parentGroup() == tree && size < 8 && real == false)
        size = 8;

    return size;
}

void BookmarksProtocol::get( const QUrl& url )
{
    QString path = url.path();
    const QRegularExpression regexp(QStringLiteral("^/(background|icon)/([\\S]+)"));
    QRegularExpressionMatch rmatch;

    if (path.isEmpty() || path == "/") {
        echoIndex();
    } else if (path == "/config") {
        const KService::Ptr bookmarksKCM = KService::serviceByDesktopName(QStringLiteral("bookmarks"));
        if (bookmarksKCM) {
            auto job = new KIO::ApplicationLauncherJob(bookmarksKCM);
            job->start();
        } else {
            error(KIO::ERR_SLAVE_DEFINED, i18n("Could not find bookmarks config"));
        }
        echoHead("bookmarks:/");
    } else if (path == "/editbookmarks") {
        const KService::Ptr keditbookmarks = KService::serviceByDesktopName(QStringLiteral("org.kde.keditbookmarks"));
        if (keditbookmarks) {
            auto job = new KIO::ApplicationLauncherJob(keditbookmarks);
            job->start();
        } else {
            error(KIO::ERR_SLAVE_DEFINED, i18n("Could not find bookmarks editor"));
        }
        echoHead("bookmarks:/");
    } else if (path.indexOf(regexp, 0, &rmatch) >= 0) {
        echoImage(rmatch.captured(1), rmatch.captured(2), QUrlQuery(url).queryItemValue("size"));
    } else {
        echoHead();
        echo("<p class=\"message\">" + i18n("Wrong request: %1", url.toDisplayString().toHtmlEscaped()) + "</p>");
    }
    finished();
}

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QLatin1String("kio_bookmarks"));

    if (argc != 4) {
        qCritical() << "Usage: kio_bookmarks protocol domain-socket1 domain-socket2";
        exit(-1);
    }

    BookmarksProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}

#include "kio_bookmarks.moc"
