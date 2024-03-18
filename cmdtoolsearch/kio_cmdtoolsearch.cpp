/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kio_cmdtoolsearch.h"
#include "cmdtool.h"
#include "cmdtool_manager.h"
#include "kio_cmdtoolsearch_debug.h"
#include "kio_cmdtoolsearch_p.h"

#include "config.h"

#include <KIO/FileCopyJob>
#include <KIO/ListJob>
#include <KIO/StatJob>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QUrl>
#include <QUrlQuery>

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.cmdtoolsearch" FILE "cmdtoolsearch.json")
};

CmdToolSearchProtocol::CmdToolSearchProtocol(const QByteArray &pool, const QByteArray &app)
    : ForwardingWorkerBase("cmdtoolsearch", pool, app)
{
}

KIO::WorkerResult CmdToolSearchProtocol::stat(const QUrl &url)
{
    KIO::UDSEntry uds;
    uds.reserve(9);
    uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, 0700);
    uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
    uds.fastInsert(KIO::UDSEntry::UDS_ICON_OVERLAY_NAMES, QStringLiteral("baloo"));
    uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n("Search Folder"));
    uds.fastInsert(KIO::UDSEntry::UDS_URL, url.url());

    QUrlQuery query(url);
    QString title = query.queryItemValue(QStringLiteral("title"), QUrl::FullyDecoded);
    if (!title.isEmpty()) {
        uds.fastInsert(KIO::UDSEntry::UDS_NAME, title);
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, title);
    }

    statEntry(uds);
    return KIO::WorkerResult::pass();
}

// Create a UDSEntry for "."
void CmdToolSearchProtocol::listRootEntry()
{
    KIO::UDSEntry entry;
    entry.reserve(4);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR);
    listEntry(entry);
}

KIO::WorkerResult CmdToolSearchProtocol::listDir(const QUrl &url)
{
    const QUrlQuery urlQuery(url);

    const QString toolName = urlQuery.queryItemValue(QStringLiteral("tool"));
    const QString searchPattern = urlQuery.queryItemValue(QStringLiteral("search"));
    const bool searchFileContents = urlQuery.queryItemValue(QStringLiteral("checkContent")) == QLatin1String("yes");
    const QUrl dirUrl = QUrl(urlQuery.queryItemValue(QStringLiteral("url")));

    if (searchPattern.isEmpty() && toolName.isEmpty()) {
        // Some tool may not need search pattern, so we only require a search pattern for the default tool.
        return KIO::WorkerResult::fail(KIO::ERR_MALFORMED_URL, i18nc("@info:search failed because of missing argument", "No search pattern specified"));
    }

    CmdTool *tool = nullptr;
    CmdToolManager manager;

    if (!toolName.isEmpty()) {
        if (toolName == QLatin1String("plain")) {
            QUrlQuery newUrlQuery = urlQuery;
            newUrlQuery.removeQueryItem(QStringLiteral("tool"));
            QUrl newUrl = url;
            newUrl.setQuery(newUrlQuery.query());
            return ForwardingWorkerBase::listDir(newUrl);
        }

        if (!dirUrl.isLocalFile()) {
            return KIO::WorkerResult::fail(KIO::ERR_UNSUPPORTED_ACTION, i18nc("@info", "Tool \"%1\" can only run in local directories", toolName));
        }

        tool = manager.getTool(toolName);
        if (!tool) {
            return KIO::WorkerResult::fail(KIO::ERR_UNSUPPORTED_ACTION, i18nc("@info", "Tool \"%1\" not found", toolName));
        } else if (!tool->isAvailable()) {
            return KIO::WorkerResult::fail(KIO::ERR_UNSUPPORTED_ACTION, i18nc("@info", "Tool \"%1\" not available", toolName));
        }
    } else {
        // Decide default plugin
        if (!dirUrl.isLocalFile()) {
            qCDebug(KIO_CMDTOOLSEARCH) << "Cmdtool can only run in local directories, fallback to kio_filenamesearch";
            return ForwardingWorkerBase::listDir(url);
        }
        if (searchFileContents) {
            tool = manager.getDefaultFileContentSearchTool();
        } else {
            tool = manager.getDefaultFileNameSearchTool();
        }
        if (!tool) {
            qCDebug(KIO_CMDTOOLSEARCH) << "Default plugin not available, fallback to kio_filenamesearch";
            return ForwardingWorkerBase::listDir(url);
        }
    }

    qCDebug(KIO_CMDTOOLSEARCH) << "Running tool" << tool->name() << "with search pattern" << searchPattern;

    listRootEntry();
    QDir rootDir(dirUrl.toLocalFile());
    connect(tool, &CmdTool::result, [this, rootDir](const QString &result) {
        QString relativePath = rootDir.cleanPath(result);
        QString fullPath = rootDir.filePath(relativePath);
        QUrl url = QUrl::fromLocalFile(fullPath);
        KIO::UDSEntry uds;
        uds.reserve(4);
        uds.fastInsert(KIO::UDSEntry::UDS_NAME, url.fileName());
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, url.fileName());
        uds.fastInsert(KIO::UDSEntry::UDS_URL, url.url());
        uds.fastInsert(KIO::UDSEntry::UDS_LOCAL_PATH, fullPath);
        listEntry(uds);
    });

    tool->run(rootDir.absolutePath(), searchPattern, searchFileContents);

    return KIO::WorkerResult::pass();
}

bool CmdToolSearchProtocol::rewriteUrl(const QUrl &url, QUrl &newURL)
{
    newURL = url;
    newURL.setScheme(QStringLiteral("filenamesearch"));
    return true;
}

void CmdToolSearchProtocol::adjustUDSEntry(KIO::UDSEntry &entry, UDSEntryCreationMode creationMode) const
{
    // ForwardingWorkerBase::adjustUDSEntry() rewrites the URL to our scheme. We don't want that.
}

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 4) {
        qCDebug(KIO_CMDTOOLSEARCH) << "Usage: kio_cmdtoolsearch protocol domain-socket1 domain-socket2";
        return -1;
    }

    CmdToolSearchProtocol worker(argv[2], argv[3]);
    worker.dispatchLoop();

    return 0;
}

#include "kio_cmdtoolsearch.moc"
