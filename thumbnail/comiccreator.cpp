/**
 * This file is part of the KDE libraries
 *
 * Comic Book Thumbnailer for KDE 4 v0.1
 * Creates cover page previews for comic-book files (.cbr/z/t).
 * Copyright (c) 2009 Harsh J <harsh@harshj.com>
 *
 * Some code borrowed from Okular's comicbook generators,
 * by Tobias Koenig <tokoe@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// comiccreator.cpp

#include "comiccreator.h"
#include "thumbnail-comic-logsettings.h"

#include <kzip.h>
#include <ktar.h>
#include <k7zip.h>
#include <kprocess.h>

#include <memory>

#include <QFile>
#include <QEventLoop>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <QTemporaryDir>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new ComicCreator;
    }
}

ComicCreator::ComicCreator() : m_loop(nullptr) {}

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
        qCDebug(KIO_THUMBNAIL_COMIC_LOG) << "Error creating the comic book thumbnail.";
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
        if (entry.endsWith(QLatin1String(".gif"), Qt::CaseInsensitive) ||
                entry.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) ||
                entry.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive) ||
                entry.endsWith(QLatin1String(".png"), Qt::CaseInsensitive)) {
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
    QString unrar = unrarPath();
    if (unrar.isEmpty()) {
        qCDebug(KIO_THUMBNAIL_COMIC_LOG) << "A suitable version of unrar is not available.";
        return QImage();
    }

    // Get the files and filter the images out.
    QStringList entries = getRARFileList(path, unrar);
    filterImages(entries);
    if (entries.isEmpty()) {
        return QImage();
    }

    // Clear previously used data arrays.
    m_stdOut.clear();
    m_stdErr.clear();

    // Extract the cover file alone. Use verbose paths.
    // unrar x -n<file> path/to/archive /path/to/temp
    QTemporaryDir cUnrarTempDir;
    startProcess(unrar, QStringList() << "x" << "-n" + entries[0] << path << cUnrarTempDir.path());

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
    startProcess(unrarPath, QStringList() << "vb" << path);
    entries = QString::fromLocal8Bit(m_stdOut).split('\n', QString::SkipEmptyParts);
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
        proc.start(unrar, QStringList() << "-version");
        proc.waitForFinished(-1);
        const QStringList lines = QString::fromLocal8Bit(proc.readAllStandardOutput()).split
            ('\n', QString::SkipEmptyParts);
        if (!lines.isEmpty()) {
            if (lines.first().startsWith(QLatin1String("RAR ")) || lines.first().startsWith(QLatin1String("UNRAR "))) {
                return unrar;
            }
        }
    }
    return QString();
}

void ComicCreator::readProcessOut()
{
    /// Read all std::out data and store to the data array.
    if (!m_process)
        return;

    m_stdOut += m_process->readAllStandardOutput();
}

void ComicCreator::readProcessErr()
{
    /// Read available std:err data and kill process if there is any.
    if (!m_process)
        return;

    m_stdErr += m_process->readAllStandardError();
    if (!m_stdErr.isEmpty())
    {
        m_process->kill();
        return;
    }
}

void ComicCreator::finishedProcess(int exitCode, QProcess::ExitStatus exitStatus)
{
    /// Run when process finishes.
    Q_UNUSED(exitCode)
    if (m_loop)
    {
        m_loop->exit(exitStatus == QProcess::CrashExit ? 1 : 0);
    }
}

int ComicCreator::startProcess(const QString& processPath, const QStringList& args)
{
    /// Run a process and store std::out, std::err data in their respective buffers.
    int ret = 0;

#if defined(Q_OS_WIN)
    m_process.reset(new QProcess(this));
#else
    m_process.reset(new KPtyProcess(this));
    m_process->setOutputChannelMode(KProcess::SeparateChannels);
#endif

    connect(m_process.data(), SIGNAL(readyReadStandardOutput()), SLOT(readProcessOut()));
    connect(m_process.data(), SIGNAL(readyReadStandardError()), SLOT(readProcessErr()));
    connect(m_process.data(), SIGNAL(finished(int,QProcess::ExitStatus)),
        SLOT(finishedProcess(int,QProcess::ExitStatus)));

#if defined(Q_OS_WIN)
    m_process->start(processPath, args, QIODevice::ReadWrite | QIODevice::Unbuffered);
    ret = m_process->waitForFinished(-1) ? 0 : 1;
#else
    m_process->setProgram(processPath, args);
    m_process->setNextOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered);
    m_process->start();
    QEventLoop loop;
    m_loop = &loop;
    ret = loop.exec(QEventLoop::WaitForMoreEvents);
    m_loop = nullptr;
#endif

    return ret;
}
