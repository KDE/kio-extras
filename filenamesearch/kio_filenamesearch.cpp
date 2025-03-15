/*
 *   SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *   SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kio_filenamesearch.h"
#include "kio_filenamesearch_p.h"

#include "kio_filenamesearch_debug.h"

#include <KIO/FileCopyJob>
#include <KIO/ListJob>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDir>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>
#include <QUrlQuery>

namespace
{

QString ensureTrailingSlash(const QString &str)
{
    QString path = str;
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }
    return path;
}

bool hasBeenVisited(const QString &path, std::set<QString> &seenDirs)
{
    // If the file or folder is within any of the seenDirs, it has already
    // been visited, skip the entry.

    for (auto iterator : seenDirs) {
        if (path.startsWith(iterator)) {
            return true;
        }
    }
    return false;
}

}

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.filenamesearch" FILE "filenamesearch.json")
};

static bool contentContainsPattern(const QUrl &url, const QRegularExpression &regex)
{
    auto fileContainsPattern = [&](const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (regex.match(line).hasMatch()) {
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

//  Search for a term in the filename and then, if SearchContent set, in the body of the file

bool FileNameSearchProtocol::match(const KIO::UDSEntry &entry, const QRegularExpression &regex, const SearchOptions options)
{
    if (options.testFlag(SearchFileName) && regex.match(entry.stringValue(KIO::UDSEntry::UDS_NAME)).hasMatch()) {
        return true;
    }
    if (options.testFlags(SearchContent)) {
        const QUrl entryUrl(entry.stringValue(KIO::UDSEntry::UDS_URL));
        QMimeDatabase mdb;
        QMimeType mimetype = mdb.mimeTypeForUrl(entryUrl);
        if (mimetype.inherits(QStringLiteral("text/plain"))) {
            return contentContainsPattern(entryUrl, regex);
        }
    }

    return false;
}

FileNameSearchProtocol::FileNameSearchProtocol(const QByteArray &pool, const QByteArray &app)
    : QObject()
    , WorkerBase("search", pool, app)
{
    QDBusInterface kded(QStringLiteral("org.kde.kded6"), QStringLiteral("/kded"), QStringLiteral("org.kde.kded6"));
    kded.call(QStringLiteral("loadModule"), QStringLiteral("filenamesearchmodule"));
}

FileNameSearchProtocol::~FileNameSearchProtocol() = default;

KIO::WorkerResult FileNameSearchProtocol::stat(const QUrl &url)
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

KIO::WorkerResult FileNameSearchProtocol::listDir(const QUrl &url)
{
    enum Engine {
        Unspecified,
        Internal,
        External,
    };

    listRootEntry();

    const QUrlQuery urlQuery(url);
    const QString search = urlQuery.queryItemValue(QStringLiteral("search"), QUrl::FullyDecoded);
    if (search.isEmpty()) {
        return KIO::WorkerResult::pass();
    }

    const QRegularExpression regex(search, QRegularExpression::CaseInsensitiveOption);
    if (!regex.isValid()) {
        qCWarning(KIO_FILENAMESEARCH) << "Invalid QRegularExpression/PCRE search pattern:" << search;
        return KIO::WorkerResult::pass();
    }

    QUrl dirUrl = QUrl(urlQuery.queryItemValue(QStringLiteral("url"), QUrl::FullyDecoded));

    // Canonicalise the search base directory
    if (dirUrl.isLocalFile()) {
        const QString canonicalPath = QFileInfo(dirUrl.toLocalFile()).canonicalFilePath();
        if (!canonicalPath.isEmpty()) {
            dirUrl = QUrl::fromLocalFile(canonicalPath);
        }
    }

    // Don't try to iterate the /proc directory of Linux
    if (dirUrl.isLocalFile() && dirUrl.toLocalFile() == QLatin1String("/proc")) {
        return KIO::WorkerResult::pass();
    }

    // "checkContent=yes" should also search for terms in the filename.
    SearchOptions options(SearchFileName);
    if (urlQuery.queryItemValue(QStringLiteral("checkContent")) == QLatin1String("yes")) {
        options.setFlag(SearchContent);
    }

    // "includeHidden=yes" should search for hidden files and within hidden folders
    if (urlQuery.queryItemValue(QStringLiteral("includeHidden")) == QLatin1String("yes")) {
        options.setFlag(IncludeHidden);
    }

    // These are placeholders, the longer term aim is something a bit more flexible.
    // The default behaviour is to assume the external script is present and fall back to using the
    // internal search if it is not.

    Engine useEngine = Unspecified;
    if (urlQuery.queryItemValue(QStringLiteral("src")) == QLatin1String("internal")) {
        useEngine = Internal;
    } else if (urlQuery.queryItemValue(QStringLiteral("src")) == QLatin1String("external")) {
        useEngine = External;
    }

    std::set<QString> iteratedDirs;
    std::queue<QUrl> pendingDirs;

#if !defined(Q_OS_WIN32)
    // Prefer using external tools if available
    if ((useEngine == Unspecified || useEngine == External) && options.testFlag(SearchContent) && dirUrl.isLocalFile()) {
        KIO::WorkerResult result = searchDirWithExternalTool(dirUrl, regex);
        if (result.error() != KIO::ERR_UNSUPPORTED_ACTION) {
            return result;
        }
        if (useEngine == Unspecified) {
            qCDebug(KIO_FILENAMESEARCH) << "External tool not available. Fall back to KIO.";
        } else {
            qCDebug(KIO_FILENAMESEARCH) << "External tool not available. Test fails.";
            return KIO::WorkerResult::fail(KIO::ERR_CANNOT_LAUNCH_PROCESS, QStringLiteral("External tool not available"));
        }
    }
#endif

    if (useEngine == Unspecified || useEngine == Internal) {
        searchDir(dirUrl, regex, options, iteratedDirs, pendingDirs);

        while (!pendingDirs.empty()) {
            const QUrl pendingUrl = pendingDirs.front();
            pendingDirs.pop();
            searchDir(pendingUrl, regex, options, iteratedDirs, pendingDirs);
        }
    }

    return KIO::WorkerResult::pass();
}

void FileNameSearchProtocol::searchDir(const QUrl &dirUrl,
                                       const QRegularExpression &regex,
                                       const SearchOptions options,
                                       std::set<QString> &iteratedDirs,
                                       std::queue<QUrl> &pendingDirs)
{
    //  If the directory is flagged in the iteratedDirs set, it has already been searched.
    //  Return to avoid circular recursion into symlinks.

    const QString dirPath = ensureTrailingSlash(QUrl(dirUrl).path());

    if (iteratedDirs.contains(dirPath)) {
        return;
    }

    KIO::ListJob::ListFlags listFlags = {};
    if (options.testAnyFlags(IncludeHidden)) {
        listFlags = KIO::ListJob::ListFlag::IncludeHidden;
    }
    KIO::ListJob *listJob = KIO::listRecursive(dirUrl, KIO::HideProgressInfo, listFlags);

    connect(this, &QObject::destroyed, listJob, [listJob]() {
        listJob->kill();
    });

    connect(listJob, &KIO::ListJob::entries, this, [&](KJob *, const KIO::UDSEntryList &list) {
        if (listJob->error()) {
            qCWarning(KIO_FILENAMESEARCH) << "Searching failed:" << listJob->errorText();
            return;
        }

        QUrl entryUrl(dirUrl);
        const QString path = ensureTrailingSlash(entryUrl.path());

        for (auto entry : list) {
            if (wasKilled()) { // File-by-file searches may take some time, call wasKilled before each file
                listJob->kill();
                return;
            }

            // UDS_NAME is e.g. "foo/bar/somefile.txt"
            const QString leaf = path + entry.stringValue(KIO::UDSEntry::UDS_NAME);

            if (!hasBeenVisited(leaf, iteratedDirs)) {
                entryUrl.setPath(leaf);

                const QString urlStr = entryUrl.toDisplayString();
                entry.replace(KIO::UDSEntry::UDS_URL, urlStr);

                const QString fileName = entryUrl.fileName();
                entry.replace(KIO::UDSEntry::UDS_NAME, fileName);

                //  There's no point in trying to search content if the entry is a folder, however the code should
                //  check the foldername. This is a "TODO" for when a content search also checks filenames.

                if (entry.isDir()) {
                    // Push the symlink destination into the queue, leaving the decision
                    // of whether to actually search the folder to later

                    if (const QString linkDest = entry.stringValue(KIO::UDSEntry::UDS_LINK_DEST); !linkDest.isEmpty()) {
                        pendingDirs.push(entryUrl.resolved(QUrl(linkDest)));
                    }
                }

                if (match(entry, regex, options)) {
                    // UDS_DISPLAY_NAME is e.g. "foo/bar/somefile.txt"
                    entry.replace(KIO::UDSEntry::UDS_DISPLAY_NAME, fileName);
                    listEntry(entry);
                }
            }
        }
    });

    listJob->exec();
    // Mark the folder when finished
    iteratedDirs.insert(dirPath);
}

#if !defined(Q_OS_WIN32)

KIO::WorkerResult FileNameSearchProtocol::searchDirWithExternalTool(const QUrl &dirUrl, const QRegularExpression &regex)
{
    qCDebug(KIO_FILENAMESEARCH) << "searchDirWithExternalTool dir:" << dirUrl << "pattern:" << regex.pattern();

    const QString programName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kio_filenamesearch/kio-filenamesearch-grep"));
    if (programName.isEmpty()) {
        const QString message = QStringLiteral("kio_filenamesearch/kio-filenamesearch-grep not found in ")
            + QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).join(QLatin1Char(':'));
        qCWarning(KIO_FILENAMESEARCH) << message;
        return KIO::WorkerResult::fail(KIO::ERR_CANNOT_LAUNCH_PROCESS, message);
    }

    QProcess process;
    process.setProgram(programName);
    process.setWorkingDirectory(dirUrl.toLocalFile());
    process.setArguments({QStringLiteral("--run"), regex.pattern()});

    qCDebug(KIO_FILENAMESEARCH) << "Start" << process.program() << "args:" << process.arguments() << "in:" << process.workingDirectory();
    process.start(QIODeviceBase::ReadWrite | QIODeviceBase::Unbuffered);
    if (!process.waitForStarted()) {
        qCWarning(KIO_FILENAMESEARCH) << programName << "failed to start:" << process.errorString();
        return KIO::WorkerResult::fail(KIO::ERR_CANNOT_LAUNCH_PROCESS, QStringLiteral("%1: %2").arg(programName, process.errorString()));
    }
    // Explicitly close the write channel, to avoid some tools waiting for input (e.g. ripgrep, when no path is given on cmdline)
    process.closeWriteChannel();
    qCDebug(KIO_FILENAMESEARCH) << "Close STDIN.";

    QDir rootDir(dirUrl.path());
    QUrl url(dirUrl);
    QByteArray output;
    const char sep = '\0';

    auto sendMatch = [this, &rootDir, &url](const QString &result) {
        qCDebug(KIO_FILENAMESEARCH) << "RESULT:" << result;
        QString relativePath = rootDir.cleanPath(result);
        QString fullPath = rootDir.filePath(relativePath);
        url.setPath(fullPath);
        KIO::UDSEntry uds;
        uds.reserve(4);
        uds.fastInsert(KIO::UDSEntry::UDS_NAME, url.fileName());
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, url.fileName());
        uds.fastInsert(KIO::UDSEntry::UDS_URL, url.url());
        uds.fastInsert(KIO::UDSEntry::UDS_LOCAL_PATH, fullPath);
        listEntry(uds);
    };

    do {
        if (!process.waitForReadyRead()) {
            continue;
        }
        output.append(process.readAllStandardOutput());
        qCDebug(KIO_FILENAMESEARCH) << "STDOUT:" << output;
        int begin = 0;
        while (begin < output.size()) {
            const int end = output.indexOf(sep, begin);
            if (end < 0) {
                // incomplete output, wait for more
                break;
            }

            if (end > begin) {
                QString s = QString::fromUtf8(output.mid(begin, end - begin));
                sendMatch(s);
            }

            begin = end + 1;
        }
        if (begin < output.size()) {
            output = output.mid(begin);
        } else {
            output.clear();
        }
    } while (process.state() == QProcess::Running);

    if (!output.isEmpty()) {
        qCDebug(KIO_FILENAMESEARCH) << "STDOUT:" << output;
        QString s = QString::fromUtf8(output);
        sendMatch(s);
    }

    const QString errors = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
    if (!errors.isEmpty()) {
        qCWarning(KIO_FILENAMESEARCH) << "STDERR:" << errors;
    }

    const int code = process.exitCode();
    qCDebug(KIO_FILENAMESEARCH) << programName << "stopped. Exit code:" << code;

    if (process.exitStatus() == QProcess::CrashExit) {
        qCWarning(KIO_FILENAMESEARCH) << "Crash exit:" << process.errorString();
        return KIO::WorkerResult::fail(KIO::ERR_UNKNOWN, QStringLiteral("%1: %2").arg(programName, process.errorString()));
    } else {
        if (code == 127) {
            qCDebug(KIO_FILENAMESEARCH) << "Search tool not found.";
            return KIO::WorkerResult::fail(KIO::ERR_UNSUPPORTED_ACTION);
        }
        if (code == 0 || errors.isEmpty()) {
            // `rg` returns 1 when no match, and 2 when it encounters broken links or no permission to read
            // a file, even if we suppressed the error message with `--no-messages`. We don't want to fail
            // in these cases.
            qCDebug(KIO_FILENAMESEARCH) << "Search success.";
            return KIO::WorkerResult::pass();
        } else {
            qCWarning(KIO_FILENAMESEARCH) << "Search failed. " << process.errorString();
            return KIO::WorkerResult::fail(
                KIO::ERR_UNKNOWN,
                i18nc("@info:%1 is the program used to do the search", "%1 failed, exit code: %2, error messages: %3", programName, code, errors));
        }
    }
}

#endif // !defined(Q_OS_WIN32)

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 4) {
        qCDebug(KIO_FILENAMESEARCH) << "Usage: kio_filenamesearch protocol domain-socket1 domain-socket2";
        return -1;
    }

    FileNameSearchProtocol worker(argv[2], argv[3]);
    worker.dispatchLoop();

    return 0;
}

#include "kio_filenamesearch.moc"
#include "moc_kio_filenamesearch.cpp"
