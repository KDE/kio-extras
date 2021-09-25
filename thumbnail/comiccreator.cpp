/**
 * This file is part of the KDE libraries
 *
 * Comic Book Thumbnailer for KDE 4 v0.1
 * Creates cover page previews for comic-book files (.cbr/z/t).
 * SPDX-FileCopyrightText: 2009 Harsh J <harsh@harshj.com>
 *
 * Some code borrowed from Okular's comicbook generators,
 * by Tobias Koenig <tokoe@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

// comiccreator.cpp

#include "comiccreator.h"
#include "thumbnail-comic-logsettings.h"

#include <kzip.h>
#include <ktar.h>
#include <k7zip.h>

#include <memory>

#include <QFile>
#include <QEventLoop>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new ComicCreator;
    }
}

ComicCreator::ComicCreator() {}

bool ComicCreator::create(const QString& path, int width, int height, QImage& img)
{
    Q_UNUSED(width);
    Q_UNUSED(height);

    QImage cover;

    // Detect mime type.
    QMimeDatabase db;
    db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    const QMimeType mime = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);

    if (mime.inherits("application/x-cbz") || mime.inherits("application/zip")) {
        // ZIP archive.
        cover = extractArchiveImage(path, ZIP);
    } else if (mime.inherits("application/x-cbt") ||
               mime.inherits("application/x-gzip") ||
               mime.inherits("application/x-tar")) {
        // TAR archive
        cover = extractArchiveImage(path, TAR);
    } else if (mime.inherits("application/x-cb7") || mime.inherits("application/x-7z-compressed")) {
        cover = extractArchiveImage(path, SEVENZIP);
    } else if (mime.inherits("application/x-cbr") || mime.inherits("application/x-rar")) {
        // RAR archive.
        cover = extractRARImage(path);
    }

    if (cover.isNull()) {
        qCDebug(KIO_THUMBNAIL_COMIC_LOG) << "Error creating the comic book thumbnail for" << path;
        return false;
    }

    // Copy the extracted cover to KIO::ThumbCreator's img reference.
    img = cover;

    return true;
}

void ComicCreator::filterImages(QStringList& entries)
{
    /// Sort case-insensitive, then remove non-image entries.
    QMap<QString, QString> entryMap;
    for (const QString& entry : qAsConst(entries)) {
        // Skip MacOS resource forks
        if (entry.startsWith(QLatin1String("__MACOSX"), Qt::CaseInsensitive) ||
                entry.startsWith(QLatin1String(".DS_Store"), Qt::CaseInsensitive)) {
            continue;
        }
        if (entry.endsWith(QLatin1String(".gif"), Qt::CaseInsensitive) ||
                entry.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) ||
                entry.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive) ||
                entry.endsWith(QLatin1String(".png"), Qt::CaseInsensitive) ||
                entry.endsWith(QLatin1String(".webp"), Qt::CaseInsensitive)) {
            entryMap.insert(entry.toLower(), entry);
        }
    }
    entries = entryMap.values();
}

QImage ComicCreator::extractArchiveImage(const QString& path, const ComicCreator::Type type)
{
    /// Extracts the cover image out of the .cbz or .cbt file.
    QScopedPointer<KArchive> cArchive;

    if (type==ZIP) {
        // Open the ZIP archive.
        cArchive.reset(new KZip(path));
    } else if (type==TAR) {
        // Open the TAR archive.
        cArchive.reset(new KTar(path));
    } else if (type==SEVENZIP) {
        // Open the 7z archive.
        cArchive.reset(new K7Zip(path));
    } else {
        // Reject all other types for this method.
        return QImage();
    }

    // Can our archive be opened?
    if (!cArchive->open(QIODevice::ReadOnly)) {
        return QImage();
    }

    // Get the archive's directory.
    const KArchiveDirectory* cArchiveDir = nullptr;
    cArchiveDir = cArchive->directory();
    if (!cArchiveDir) {
        return QImage();
    }

    QStringList entries;

    // Get and filter the entries from the archive.
    getArchiveFileList(entries, QString(), cArchiveDir);
    filterImages(entries);
    if (entries.isEmpty()) {
        return QImage();
    }

    // Extract the cover file.
    const KArchiveFile *coverFile = static_cast<const KArchiveFile*>
                                    (cArchiveDir->entry(entries[0]));
    if (!coverFile) {
        return QImage();
    }

    return QImage::fromData(coverFile->data());
}



void ComicCreator::getArchiveFileList(QStringList& entries, const QString& prefix,
                                      const KArchiveDirectory *dir)
{
    /// Recursively list all files in the ZIP archive into 'entries'.
    const auto dirEntries = dir->entries();
    for (const QString& entry : dirEntries) {
        const KArchiveEntry *e = dir->entry(entry);
        if (e->isDirectory()) {
            getArchiveFileList(entries, prefix + entry + '/',
                               static_cast<const KArchiveDirectory*>(e));
        } else if (e->isFile()) {
            entries.append(prefix + entry);
        }
    }
}

QImage ComicCreator::extractRARImage(const QString& path)
{
    /// Extracts the cover image out of the .cbr file.

    // Check if unrar is available. Get its path in 'unrarPath'.
    static const QString unrar = unrarPath();
    if (unrar.isEmpty()) {
        return QImage();
    }

    // Get the files and filter the images out.
    QStringList entries = getRARFileList(path, unrar);
    filterImages(entries);
    if (entries.isEmpty()) {
        return QImage();
    }

    // Extract the cover file alone. Use verbose paths.
    // unrar x -n<file> path/to/archive /path/to/temp
    QTemporaryDir cUnrarTempDir;
    runProcess(unrar, {"x", "-n" + entries[0], path, cUnrarTempDir.path()});

    // Load cover file data into image.
    QImage cover;
    cover.load(cUnrarTempDir.path() + QDir::separator() + entries[0]);

    return cover;
}

QStringList ComicCreator::getRARFileList(const QString& path,
        const QString& unrarPath)
{
    /// Get a verbose unrar listing so we can extract a single file later.
    // CMD: unrar vb /path/to/archive
    QStringList entries;
    runProcess(unrarPath, {"vb", path});
    entries = QString::fromLocal8Bit(m_stdOut).split('\n', Qt::SkipEmptyParts);
    return entries;
}

QString ComicCreator::unrarPath() const
{
    /// Check the standard paths to see if a suitable unrar is available.
    QString unrar = QStandardPaths::findExecutable("unrar");
    if (unrar.isEmpty()) {
        unrar = QStandardPaths::findExecutable("unrar-nonfree");
    }
    if (unrar.isEmpty()) {
        unrar = QStandardPaths::findExecutable("rar");
    }
    if (!unrar.isEmpty()) {
        QProcess proc;
        proc.start(unrar, {"-version"});
        proc.waitForFinished(-1);
        const QStringList lines = QString::fromLocal8Bit(proc.readAllStandardOutput()).split
                                  ('\n', Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            if (lines.first().startsWith(QLatin1String("RAR ")) || lines.first().startsWith(QLatin1String("UNRAR "))) {
                return unrar;
            }
        }
    }
    qCWarning(KIO_THUMBNAIL_COMIC_LOG) << "A suitable version of unrar is not available.";
    return QString();
}

int ComicCreator::runProcess(const QString& processPath, const QStringList& args)
{
    /// Run a process and store stdout data in a buffer.

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);

    process.setProgram(processPath);
    process.setArguments(args);
    process.start(QIODevice::ReadWrite | QIODevice::Unbuffered);

    auto ret = process.waitForFinished(-1);
    m_stdOut = process.readAllStandardOutput();

    return ret;
}
