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

#include <K7Zip>
#include <KConfigGroup>
#include <KIO/Global>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTar>
#include <KZip>

#include <memory>

#include <QEventLoop>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

K_PLUGIN_CLASS_WITH_JSON(ComicCreator, "comicbookthumbnail.json")

ComicCreator::ComicCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult ComicCreator::create(const KIO::ThumbnailRequest &request)
{
    const QString path = request.url().toLocalFile();
    QImage cover;

    // Detect mime type.
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForName(request.mimeType());

    if (mime.inherits("application/x-cbz") || mime.inherits("application/zip")) {
        // ZIP archive.
        cover = extractArchiveImage(path, ZIP);
    } else if (mime.inherits("application/x-cbt") || mime.inherits("application/x-gzip") || mime.inherits("application/x-tar")) {
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
        return KIO::ThumbnailResult::fail();
    }

    return KIO::ThumbnailResult::pass(cover);
}

void ComicCreator::filterImages(QStringList &entries)
{
    /// Sort case-insensitive, then remove non-image entries.
    QMap<QString, QString> entryMap;
    for (const QString &entry : qAsConst(entries)) {
        // Skip MacOS resource forks
        if (entry.startsWith(QLatin1String("__MACOSX"), Qt::CaseInsensitive) || entry.startsWith(QLatin1String(".DS_Store"), Qt::CaseInsensitive)) {
            continue;
        }
        if (entry.endsWith(QLatin1String(".avif"), Qt::CaseInsensitive) || entry.endsWith(QLatin1String(".bmp"), Qt::CaseInsensitive)
            || entry.endsWith(QLatin1String(".gif"), Qt::CaseInsensitive) || entry.endsWith(QLatin1String(".heif"), Qt::CaseInsensitive)
            || entry.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) || entry.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive)
            || entry.endsWith(QLatin1String(".jxl"), Qt::CaseInsensitive) || entry.endsWith(QLatin1String(".png"), Qt::CaseInsensitive)
            || entry.endsWith(QLatin1String(".webp"), Qt::CaseInsensitive)) {
            entryMap.insert(entry.toLower(), entry);
        }
    }
    entries = entryMap.values();
}

QImage ComicCreator::extractArchiveImage(const QString &path, const ComicCreator::Type type)
{
    /// Extracts the cover image out of the .cbz or .cbt file.
    QScopedPointer<KArchive> cArchive;

    if (type == ZIP) {
        // Open the ZIP archive.
        cArchive.reset(new KZip(path));
    } else if (type == TAR) {
        // Open the TAR archive.
        cArchive.reset(new KTar(path));
    } else if (type == SEVENZIP) {
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
    const KArchiveDirectory *cArchiveDir = nullptr;
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

    const KConfigGroup globalConfig(KSharedConfig::openConfig(), QStringLiteral("PreviewSettings"));
    const KIO::filesize_t maxFileSize = globalConfig.readEntry("MaximumSize", std::numeric_limits<KIO::filesize_t>::max());

    // Extract the cover file.
    for (const QString &entry : entries) {
        const KArchiveFile *coverFile = static_cast<const KArchiveFile *>(cArchiveDir->entry(entry));
        if (!coverFile || coverFile->size() < 0) {
            continue;
        }

        const KIO::filesize_t coverFileSize_t = static_cast<KIO::filesize_t>(coverFile->size());
        if (coverFileSize_t > maxFileSize) {
            continue;
        }

        return QImage::fromData(coverFile->data());
    }

    return {};
}

void ComicCreator::getArchiveFileList(QStringList &entries, const QString &prefix, const KArchiveDirectory *dir)
{
    /// Recursively list all files in the ZIP archive into 'entries'.
    const auto dirEntries = dir->entries();
    for (const QString &entry : dirEntries) {
        const KArchiveEntry *e = dir->entry(entry);
        if (e->isDirectory()) {
            getArchiveFileList(entries, prefix + entry + '/', static_cast<const KArchiveDirectory *>(e));
        } else if (e->isFile()) {
            entries.append(prefix + entry);
        }
    }
}

QImage ComicCreator::extractRARImage(const QString &path)
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

QStringList ComicCreator::getRARFileList(const QString &path, const QString &unrarPath)
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
        const QStringList lines = QString::fromLocal8Bit(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            if (lines.first().startsWith(QLatin1String("RAR ")) || lines.first().startsWith(QLatin1String("UNRAR "))) {
                return unrar;
            }
        }
    }
    qCWarning(KIO_THUMBNAIL_COMIC_LOG) << "A suitable version of unrar is not available.";
    return QString();
}

int ComicCreator::runProcess(const QString &processPath, const QStringList &args)
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

#include "comiccreator.moc"
#include "moc_comiccreator.cpp"
