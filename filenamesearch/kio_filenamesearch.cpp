/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "kio_filenamesearch.h"

#include <KCoreDirLister>
#include <KFileItem>
#include <KIO/Job>

#include <QTemporaryFile>
#include <QScopedPointer>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

Q_LOGGING_CATEGORY(KIO_FILENAMESEARCH, "kio_filenamesearch")

FileNameSearchProtocol::FileNameSearchProtocol(const QByteArray &pool, const QByteArray &app)
    : SlaveBase("search", pool, app)
{
}

FileNameSearchProtocol::~FileNameSearchProtocol()
{
}

void FileNameSearchProtocol::listDir(const QUrl &url)
{
    const QUrlQuery urlQuery(url);
    const QString search = urlQuery.queryItemValue("search");
    if (search.isEmpty()) {
        finished();
        return;
    }

    const QRegularExpression pattern(search, QRegularExpression::CaseInsensitiveOption);

    std::function<bool(const KFileItem &)> validator;
    if (urlQuery.queryItemValue("checkContent") == QStringLiteral("yes")) {
        validator = [pattern](const KFileItem &item) -> bool {
            return item.determineMimeType().inherits(QStringLiteral("text/plain")) &&
                   contentContainsPattern(item.url(), pattern);
        };
    } else {
        validator = [pattern](const KFileItem &item) -> bool {
            return item.text().contains(pattern);
        };
    }

    QSet<QString> iteratedDirs;
    const QUrl directory(urlQuery.queryItemValue("url"));
    searchDirectory(directory, validator, iteratedDirs);

    finished();
}

void FileNameSearchProtocol::searchDirectory(const QUrl &directory,
                                             const std::function<bool(const KFileItem &)> &itemValidator,
                                             QSet<QString> &iteratedDirs)
{
    if (directory.path() == QStringLiteral("/proc")) {
        // Don't try to iterate the /proc directory of Linux
        return;
    }

    // Get all items of the directory
    QScopedPointer<KCoreDirLister> dirLister(new KCoreDirLister);
    dirLister->setDelayedMimeTypes(true);
    dirLister->openUrl(directory);

    QEventLoop eventLoop;
    QObject::connect(dirLister.data(), static_cast<void(KCoreDirLister::*)()>(&KCoreDirLister::canceled),
                     &eventLoop, &QEventLoop::quit);
    QObject::connect(dirLister.data(), static_cast<void(KCoreDirLister::*)()>(&KCoreDirLister::completed),
                     &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    // Visualize all items that match the search pattern
    QList<QUrl> pendingDirs;
    const KFileItemList items = dirLister->items();
    foreach (const KFileItem &item, items) {
        if (itemValidator(item)) {
            KIO::UDSEntry entry = item.entry();
            entry.insert(KIO::UDSEntry::UDS_URL, item.url().url());
            listEntry(entry);
        }

        if (item.isDir()) {
            if (item.isLink()) {
                // Assure that no endless searching is done in directories that
                // have already been iterated.
                const QUrl linkDest = item.url().resolved(QUrl::fromLocalFile(item.linkDest()));
                if (!iteratedDirs.contains(linkDest.path())) {
                    pendingDirs.append(linkDest);
                }
            } else {
                pendingDirs.append(item.url());
            }
        }
    }

    iteratedDirs.insert(directory.path());

    dirLister.reset();

    // Recursively iterate all sub directories
    foreach (const QUrl &pendingDir, pendingDirs) {
        searchDirectory(pendingDir, itemValidator, iteratedDirs);
    }
}

bool FileNameSearchProtocol::contentContainsPattern(const QUrl &fileName, const QRegularExpression &pattern)
{
    auto fileContainsPattern = [&pattern](const QString &path) -> bool {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.contains(pattern)) {
                return true;
            }
        }

        return false;
    };

    if (fileName.isLocalFile()) {
        return fileContainsPattern(fileName.toLocalFile());
    } else {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            KIO::Job* getJob = KIO::file_copy(fileName,
                                              QUrl::fromLocalFile(tempFile.fileName()),
                                              -1,
                                              KIO::Overwrite | KIO::HideProgressInfo);
            if (getJob->exec()) {
                // The non-local file was downloaded successfully.
                return fileContainsPattern(tempFile.fileName());
            }
        }
    }

    return false;
}

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 4) {
        qCDebug(KIO_FILENAMESEARCH) << "Usage: kio_filenamesearch protocol domain-socket1 domain-socket2"
                                    << endl;
        return -1;
    }

    FileNameSearchProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}
