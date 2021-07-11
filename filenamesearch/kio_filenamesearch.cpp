/*
 *   SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kio_filenamesearch.h"

#include "kio_filenamesearch_debug.h"

#include <KIO/Job>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QDBusInterface>
#include <QMimeDatabase>
#include <QTemporaryFile>
#include <QUrl>
#include <QUrlQuery>

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.filenamesearch" FILE "filenamesearch.json")
};

FileNameSearchProtocol::FileNameSearchProtocol(const QByteArray &pool, const QByteArray &app)
    : QObject()
    , SlaveBase("search", pool, app)
{
    QDBusInterface kded(QStringLiteral("org.kde.kded5"), QStringLiteral("/kded"), QStringLiteral("org.kde.kded5"));
    kded.call(QStringLiteral("loadModule"), QStringLiteral("filenamesearchmodule"));

    m_regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
}

FileNameSearchProtocol::~FileNameSearchProtocol() = default;

void FileNameSearchProtocol::stat(const QUrl &url)
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
    finished();
}

// Create a UDSEntry for "."
void FileNameSearchProtocol::listRootEntry()
{
    KIO::UDSEntry entry;
    entry.reserve(4);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
    listEntry(entry);
}

void FileNameSearchProtocol::listDir(const QUrl &url)
{
    m_iteratedDirs.clear();

    listRootEntry();

    const QUrlQuery urlQuery(url);
    const QString search = urlQuery.queryItemValue(QStringLiteral("search"));
    if (search.isEmpty()) {
        finished();
        return;
    }

    m_regex.setPattern(search);
    if (!m_regex.isValid()) {
        qCWarning(KIO_FILENAMESEARCH) << "Invalid QRegularExpression/PCRE search pattern:" << search;
        finished();
        return;
    }

    m_url = QUrl(urlQuery.queryItemValue(QStringLiteral("url")));
    const bool isContent = urlQuery.queryItemValue(QStringLiteral("checkContent")) == QLatin1String("yes");
    m_searchType = isContent ? SearchContent : SearchFileNames;

    searchDir(m_url);

    finished();
}

void FileNameSearchProtocol::searchDir(const QUrl &dirUrl)
{
    if (dirUrl.path() == QLatin1String("/proc")) {
        // Don't try to iterate the /proc directory of Linux
        return;
    }

    KIO::ListJob *listJob = KIO::listRecursive(dirUrl, KIO::HideProgressInfo, false /* hidden */);

    connect(this, &QObject::destroyed, listJob, [listJob]() {
        listJob->kill();
    });

    std::vector<QUrl> pendingDirs;

    connect(listJob, &KIO::ListJob::entries, this, [&](KJob *, const KIO::UDSEntryList &list) mutable {
        if (listJob->error()) {
            qCWarning(KIO_FILENAMESEARCH) << "Searching failed:" << listJob->errorText();
            return;
        }

        KIO::UDSEntryList entryList = list;

        for (auto &entry : entryList) {
            QUrl entryUrl(dirUrl);
            QString path = entryUrl.path();
            if (!path.endsWith(QLatin1Char('/'))) {
                path += QLatin1Char('/');
            }
            // UDS_NAME is e.g. "foo/bar/somefile.txt"
            entryUrl.setPath(path + entry.stringValue(KIO::UDSEntry::UDS_NAME));

            const QString urlStr = entryUrl.toDisplayString();
            entry.replace(KIO::UDSEntry::UDS_URL, urlStr);

            const QString fileName = entryUrl.fileName();
            entry.replace(KIO::UDSEntry::UDS_NAME, fileName);

            if (entry.isDir()) {
                // Also search the target of a dir symlink
                if (const QString linkDest = entry.stringValue(KIO::UDSEntry::UDS_LINK_DEST); !linkDest.isEmpty()) {
                    // Remember the dir to prevent endless loops
                    if (const auto [it, isInserted] = m_iteratedDirs.insert(linkDest); isInserted) {
                        pendingDirs.push_back(entryUrl.resolved(QUrl(linkDest)));
                    }
                }

                m_iteratedDirs.insert(urlStr);
            }

            if (match(entry)) {
                // UDS_DISPLAY_NAME is e.g. "foo/bar/somefile.txt"
                entry.replace(KIO::UDSEntry::UDS_DISPLAY_NAME, fileName);
                listEntry(entry);
            }
        }
    });

    listJob->exec();

    for (const auto &u : pendingDirs) {
        searchDir(u);
    }
}

bool FileNameSearchProtocol::match(const KIO::UDSEntry &entry) const
{
    if (m_searchType == SearchFileNames) {
        return m_regex.match(entry.stringValue(KIO::UDSEntry::UDS_NAME)).hasMatch();
    } else {
        const QUrl entryUrl(entry.stringValue(KIO::UDSEntry::UDS_URL));
        QMimeDatabase mdb;
        QMimeType mimetype = mdb.mimeTypeForUrl(entryUrl);
        if (mimetype.inherits(QStringLiteral("text/plain"))) {
            return contentContainsPattern(entryUrl);
        }
    }

    return false;
}

bool FileNameSearchProtocol::contentContainsPattern(const QUrl &url) const
{
    auto fileContainsPattern = [this](const QString &path) -> bool {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (m_regex.match(line).hasMatch()) {
                return true;
            }
        }

        return false;
    };

    if (url.isLocalFile()) {
        return fileContainsPattern(url.toLocalFile());
    } else {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            const QString tempName = tempFile.fileName();
            KIO::Job *getJob = KIO::file_copy(url, QUrl::fromLocalFile(tempName), -1, KIO::Overwrite | KIO::HideProgressInfo);
            if (getJob->exec()) {
                // The non-local file was downloaded successfully.
                return fileContainsPattern(tempName);
            }
        }
    }

    return false;
}

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 4) {
        qCDebug(KIO_FILENAMESEARCH) << "Usage: kio_filenamesearch protocol domain-socket1 domain-socket2";
        return -1;
    }

    FileNameSearchProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}

#include "kio_filenamesearch.moc"
