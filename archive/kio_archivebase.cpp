/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kio_archivebase.h"
#include <kio_archive_debug.h>

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <KIO/StatJob>
#include <KTar>
#include <KZip>
#include <KAr>
#include <K7Zip>
#include <KUser>
#include <KLocalizedString>

#include <QFile>
#include <QDir>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>

#include <memory>

#ifdef Q_OS_WIN
#define S_ISDIR(m) (((m & S_IFMT) == S_IFDIR))
#endif

using namespace KIO;

ArchiveProtocolBase::ArchiveProtocolBase( const QByteArray &proto, const QByteArray &pool, const QByteArray &app ) : WorkerBase( proto, pool, app )
{
    qCDebug(KIO_ARCHIVE_LOG);
    m_archiveFile = nullptr;
}

ArchiveProtocolBase::~ArchiveProtocolBase()
{
    delete m_archiveFile;
}

bool ArchiveProtocolBase::checkNewFile( const QUrl & url, QString & path, KIO::Error& errorNum )
{
#ifndef Q_OS_WIN
    QString fullPath = url.path();
#else
    QString fullPath = url.path().remove(0, 1);
#endif
    qCDebug(KIO_ARCHIVE_LOG) << fullPath;

    // Are we already looking at that file ?
    if ( m_archiveFile && m_archiveName == fullPath.left(m_archiveName.length()) )
    {
        // Has it changed ?
        QT_STATBUF statbuf;
        if ( QT_STAT( QFile::encodeName(m_archiveName).constData(), &statbuf ) == 0 )
        {
            if ( m_mtime == statbuf.st_mtime )
            {
                path = fullPath.mid( m_archiveName.length() );
                qCDebug(KIO_ARCHIVE_LOG) << "returning" << path;
                return true;
            }
        }
    }
    qCDebug(KIO_ARCHIVE_LOG) << "Need to open a new file";

    // Close previous file
    if ( m_archiveFile )
    {
        m_archiveFile->close();
        delete m_archiveFile;
        m_archiveFile = nullptr;
    }

    // Find where the tar file is in the full path
    int pos = 0;
    QString archiveFile;
    path.clear();

    if (!fullPath.isEmpty() && !fullPath.endsWith(QLatin1Char('/')))
        fullPath += QLatin1Char('/');

    qCDebug(KIO_ARCHIVE_LOG) << "the full path is" << fullPath;
    QT_STATBUF statbuf;
    statbuf.st_mode = 0; // be sure to clear the directory bit
    while ( (pos=fullPath.indexOf(QLatin1Char('/'), pos+1 )) != -1 )
    {
        QString tryPath = fullPath.left( pos );
        qCDebug(KIO_ARCHIVE_LOG) << fullPath << "trying" << tryPath;
        if ( QT_STAT( QFile::encodeName(tryPath).constData(), &statbuf ) == -1 )
        {
            if (errno == ENOENT)
            {
                // The current path is no longer part of the local filesystem.
                // Either we already have enough of the pathname, or we will
                // not get anything more useful.
                statbuf.st_mode = 0;            // do not trust the result
                break;
            }

            if (errno == EACCES)
                errorNum = KIO::ERR_ACCESS_DENIED;
            else
                errorNum = KIO::ERR_CANNOT_STAT;
            return false;
        }

        if ( !S_ISDIR(statbuf.st_mode) )
        {
            archiveFile = tryPath;
            m_mtime = statbuf.st_mtime;
#ifdef Q_OS_WIN // st_uid and st_gid provides no information
            m_user.clear();
            m_group.clear();
#else
            KUser user(statbuf.st_uid);
            m_user = user.loginName();
            KUserGroup group(statbuf.st_gid);
            m_group = group.name();
#endif
            path = fullPath.mid( pos + 1 );
            qCDebug(KIO_ARCHIVE_LOG).nospace() << "fullPath=" << fullPath << " path=" << path;
            if ( path.length() > 1 )
            {
                if (path.endsWith(QLatin1Char('/')))
                    path.chop(1);
            }
            else
                path = QStringLiteral("/");
            qCDebug(KIO_ARCHIVE_LOG).nospace() << "Found. archiveFile=" << archiveFile << " path=" << path;
            break;
        }
    }
    if ( archiveFile.isEmpty() )
    {
        qCDebug(KIO_ARCHIVE_LOG) << "not found";
        if ( S_ISDIR(statbuf.st_mode) ) // Did the last stat() find a directory?
        {
            // Too bad, it is a directory, not an archive.
            qCDebug(KIO_ARCHIVE_LOG) << "Path is a directory, not an archive.";
            errorNum = KIO::ERR_IS_DIRECTORY;
        }
        else
            errorNum = KIO::ERR_DOES_NOT_EXIST;
        return false;
    }

    // Open new file
    m_archiveFile = this->createArchive( url.scheme(), archiveFile );
    if ( !m_archiveFile ) {
        qCWarning(KIO_ARCHIVE_LOG) << "Protocol" << url.scheme() << "not supported by this IOWorker" ;
        errorNum = KIO::ERR_UNSUPPORTED_PROTOCOL;
        return false;
    }

    if ( !m_archiveFile->open( QIODevice::ReadOnly ) )
    {
        qCDebug(KIO_ARCHIVE_LOG) << "Opening" << archiveFile << "failed.";
        delete m_archiveFile;
        m_archiveFile = nullptr;
        errorNum = KIO::ERR_CANNOT_OPEN_FOR_READING;
        return false;
    }

    m_archiveName = archiveFile;
    return true;
}

uint ArchiveProtocolBase::computeArchiveDirSize(const KArchiveDirectory *dir)
{
    // compute size of archive content
    uint totalSize = 0;
    const auto entries = dir->entries();
    for (const auto &entryName: entries) {
        auto entry = dir->entry(entryName);
        if (entry->isFile()) {
            auto fileEntry = static_cast<const KArchiveFile *>(entry);
            totalSize += fileEntry->size();
        }
        else if (entry->isDirectory()) {
            const auto dirEntry = static_cast<const KArchiveDirectory *>(entry);
            // recurse
            totalSize += computeArchiveDirSize(dirEntry);
        }
    }
    return totalSize;
}

KIO::StatDetails ArchiveProtocolBase::getStatDetails()
{
    // takes care of converting old metadata details to new StatDetails
    // TODO KF6 : remove legacy "details" code path
    KIO::StatDetails details;
#if KIOCORE_BUILD_DEPRECATED_SINCE(5, 69)
    if (hasMetaData(QStringLiteral("statDetails"))) {
#endif
        const QString statDetails = metaData(QStringLiteral("statDetails"));
        details = statDetails.isEmpty() ? KIO::StatDefaultDetails : static_cast<KIO::StatDetails>(statDetails.toInt());
#if KIOCORE_BUILD_DEPRECATED_SINCE(5, 69)
    } else {
        const QString sDetails = metaData(QStringLiteral("details"));
        // silence deprecation warning for KIO::detailsToStatDetails
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
        QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
        details = sDetails.isEmpty() ? KIO::StatDefaultDetails : KIO::detailsToStatDetails(sDetails.toInt());
        QT_WARNING_POP
    }
#endif
    return details;
}

void ArchiveProtocolBase::createRootUDSEntry( KIO::UDSEntry & entry )
{
    entry.clear();
    entry.reserve(7);

    auto path = m_archiveFile->fileName();
    path = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);

    entry.fastInsert( KIO::UDSEntry::UDS_NAME, QStringLiteral(".") );
    entry.fastInsert( KIO::UDSEntry::UDS_DISPLAY_NAME, path );
    entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry.fastInsert( KIO::UDSEntry::UDS_MODIFICATION_TIME, m_mtime );
    //entry.fastInsert( KIO::UDSEntry::UDS_ACCESS, 07777 ); // fake 'x' permissions, this is a pseudo-directory
    entry.fastInsert( KIO::UDSEntry::UDS_USER, m_user);
    entry.fastInsert( KIO::UDSEntry::UDS_GROUP, m_group);

    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(m_archiveFile->fileName());
    if (mt.isValid()) {
        entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, mt.name());
    }
}

void ArchiveProtocolBase::createUDSEntry( const KArchiveEntry * archiveEntry, UDSEntry & entry )
{
    entry.clear();

    entry.reserve(8);
    entry.fastInsert( KIO::UDSEntry::UDS_NAME, archiveEntry->name() );
    entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, archiveEntry->isFile() ? archiveEntry->permissions() & S_IFMT : S_IFDIR ); // keep file type only
    if (archiveEntry->isFile()) {
        entry.fastInsert( KIO::UDSEntry::UDS_SIZE, ((KArchiveFile *)archiveEntry)->size() );
    }
    entry.fastInsert( KIO::UDSEntry::UDS_MODIFICATION_TIME, archiveEntry->date().toSecsSinceEpoch());
    entry.fastInsert( KIO::UDSEntry::UDS_ACCESS, archiveEntry->permissions() & 07777 ); // keep permissions only
    entry.fastInsert( KIO::UDSEntry::UDS_USER, archiveEntry->user());
    entry.fastInsert( KIO::UDSEntry::UDS_GROUP, archiveEntry->group());
    entry.fastInsert( KIO::UDSEntry::UDS_LINK_DEST, archiveEntry->symLinkTarget());
}

KIO::WorkerResult ArchiveProtocolBase::listDir( const QUrl & url )
{
    qCDebug(KIO_ARCHIVE_LOG) << url.url();

    QString path;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            return KIO::WorkerResult::fail( KIO::ERR_WORKER_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                         url.toDisplayString() ) );
        }
        else if ( errorNum != ERR_IS_DIRECTORY )
        {
            // We have any other error
            return KIO::WorkerResult::fail( errorNum, url.toDisplayString() );
        }
        // It's a real dir -> redirect
        QUrl redir = QUrl::fromLocalFile(url.path());
        qCDebug(KIO_ARCHIVE_LOG) << "Ok, redirection to" << redir.url();
        redirection( redir );
        // And let go of the tar file - for people who want to unmount a cdrom after that
        delete m_archiveFile;
        m_archiveFile = nullptr;
        return KIO::WorkerResult::pass();
    }

    if ( path.isEmpty() )
    {
        QUrl redir;
        redir.setScheme(url.scheme());
        qCDebug(KIO_ARCHIVE_LOG) << "url.path()=" << url.path();
        redir.setPath( url.path() + QLatin1Char('/') );
        qCDebug(KIO_ARCHIVE_LOG) << "redirection" << redir.url();
        redirection( redir );
        return KIO::WorkerResult::pass();
    }

    qCDebug(KIO_ARCHIVE_LOG) << "checkNewFile done";
    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveDirectory* dir;
    if (!path.isEmpty() && path != QLatin1String("/"))
    {
        qCDebug(KIO_ARCHIVE_LOG) << "Looking for entry" << path;
        const KArchiveEntry* e = root->entry( path );
        if ( !e )
        {
            return KIO::WorkerResult::fail( KIO::ERR_DOES_NOT_EXIST, url.toDisplayString() );
        }
        if ( ! e->isDirectory() )
        {
            return KIO::WorkerResult::fail( KIO::ERR_IS_FILE, url.toDisplayString() );
        }
        dir = (KArchiveDirectory*)e;
    } else {
        dir = root;
    }

    const QStringList l = dir->entries();
    totalSize( l.count() );

    UDSEntry entry;
    if (!l.contains(QLatin1String("."))) {
        createRootUDSEntry(entry);
        listEntry(entry);
    }

    QStringList::const_iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
        qCDebug(KIO_ARCHIVE_LOG) << (*it);
        const KArchiveEntry* archiveEntry = dir->entry( (*it) );

        createUDSEntry( archiveEntry, entry );

        listEntry(entry);
    }

    qCDebug(KIO_ARCHIVE_LOG) << "done";
    return KIO::WorkerResult::pass();
}

KIO::WorkerResult ArchiveProtocolBase::stat( const QUrl & url )
{
    QString path;
    UDSEntry entry;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        // We may be looking at a real directory - this happens
        // when pressing up after being in the root of an archive
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            return KIO::WorkerResult::fail( KIO::ERR_WORKER_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                         url.toDisplayString() ) );
        }
        else if ( errorNum != ERR_IS_DIRECTORY )
        {
            // We have any other error
            return KIO::WorkerResult::fail( errorNum, url.toDisplayString() );
        }
        entry.reserve(2);
        // Real directory. Return just enough information for KRun to work
        entry.fastInsert( KIO::UDSEntry::UDS_NAME, url.fileName());
        qCDebug(KIO_ARCHIVE_LOG) << "returning name" << url.fileName();

        QT_STATBUF buff;
#ifdef Q_OS_WIN
        QString fullPath = url.path().remove(0, 1);
#else
        QString fullPath = url.path();
#endif

        if ( QT_STAT( QFile::encodeName(fullPath).constData(), &buff ) == -1 )
        {
            // Should not happen, as the file was already stated by checkNewFile
            return KIO::WorkerResult::fail( KIO::ERR_CANNOT_STAT, url.toDisplayString() );
        }

        entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, buff.st_mode & S_IFMT);

        statEntry( entry );

        // And let go of the tar file - for people who want to unmount a cdrom after that
        delete m_archiveFile;
        m_archiveFile = nullptr;
        return KIO::WorkerResult::pass();
    }

    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveEntry* archiveEntry;
    if ( path.isEmpty() )
    {
        path = QStringLiteral("/");
        archiveEntry = root;
    } else {
        archiveEntry = root->entry( path );
    }
    if ( !archiveEntry )
    {
        return KIO::WorkerResult::fail( KIO::ERR_DOES_NOT_EXIST, url.toDisplayString() );
    }

    if (archiveEntry == root) {
        createRootUDSEntry( entry );
    } else {
        createUDSEntry( archiveEntry, entry );
    }

    if (archiveEntry->isDirectory()) {
        auto details = getStatDetails();
        if (details & KIO::StatRecursiveSize) {
            const auto directoryEntry = static_cast<const KArchiveDirectory *>(archiveEntry);
            entry.fastInsert(KIO::UDSEntry::UDS_RECURSIVE_SIZE, static_cast<long long>(computeArchiveDirSize(directoryEntry)));
        }
    }
    statEntry( entry );

    return KIO::WorkerResult::pass();
}

KIO::WorkerResult ArchiveProtocolBase::get( const QUrl & url )
{
    qCDebug(KIO_ARCHIVE_LOG) << url.url();

    QString path;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            return KIO::WorkerResult::fail( KIO::ERR_WORKER_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                         url.toDisplayString() ) );
        }
        else
        {
            // We have any other error
            return KIO::WorkerResult::fail( errorNum, url.toDisplayString() );
        }
    }

    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveEntry* archiveEntry = root->entry( path );

    if ( !archiveEntry )
    {
        return KIO::WorkerResult::fail( KIO::ERR_DOES_NOT_EXIST, url.toDisplayString() );
    }
    if ( archiveEntry->isDirectory() )
    {
        return KIO::WorkerResult::fail( KIO::ERR_IS_DIRECTORY, url.toDisplayString() );
    }
    const KArchiveFile* archiveFileEntry = static_cast<const KArchiveFile *>(archiveEntry);
    if ( !archiveEntry->symLinkTarget().isEmpty() )
    {
        const QString target = archiveEntry->symLinkTarget();
        qCDebug(KIO_ARCHIVE_LOG) << "Redirection to" << target;
        const QUrl realURL = url.resolved(QUrl(target));
        qCDebug(KIO_ARCHIVE_LOG) << "realURL=" << realURL;
        redirection( realURL );
        return KIO::WorkerResult::pass();
    }

    //qCDebug(KIO_ARCHIVE_LOG) << "Preparing to get the archive data";

    /*
     * The easy way would be to get the data by calling archiveFileEntry->data()
     * However this has drawbacks:
     * - the complete file must be read into the memory
     * - errors are skipped, resulting in an empty file
     */

    std::unique_ptr<QIODevice> io(archiveFileEntry->createDevice());

    if (!io)
    {
        return KIO::WorkerResult::fail( KIO::ERR_WORKER_DEFINED,
               i18n( "The archive file could not be opened, perhaps because the format is unsupported.\n%1",
                     url.toDisplayString() ) );
    }

    if ( !io->open( QIODevice::ReadOnly ) )
    {
        return KIO::WorkerResult::fail( KIO::ERR_CANNOT_OPEN_FOR_READING, url.toDisplayString() );
    }

    totalSize( archiveFileEntry->size() );

    // Size of a QIODevice read. It must be large enough so that the mime type check will not fail
    const qint64 maxSize = 0x100000; // 1MB

    qint64 bufferSize = qMin( maxSize, archiveFileEntry->size() );
    QByteArray buffer;
    buffer.resize( bufferSize );
    if ( buffer.isEmpty() && bufferSize > 0 )
    {
        // Something went wrong
        return KIO::WorkerResult::fail( KIO::ERR_OUT_OF_MEMORY, url.toDisplayString() );
    }

    bool firstRead = true;

    // How much file do we still have to process?
    qint64 fileSize = archiveFileEntry->size();
    KIO::filesize_t processed = 0;

    while ( !io->atEnd() && fileSize > 0 )
    {
        if ( !firstRead )
        {
            bufferSize = qMin( maxSize, fileSize );
            buffer.resize( bufferSize );
        }
        const qint64 read = io->read( buffer.data(), buffer.size() ); // Avoid to use bufferSize here, in case something went wrong.
        if ( read != bufferSize )
        {
            qCWarning(KIO_ARCHIVE_LOG) << "Read" << read << "bytes but expected" << bufferSize ;
            return KIO::WorkerResult::fail( KIO::ERR_CANNOT_READ, url.toDisplayString() );
        }
        if ( firstRead )
        {
            // We use the magic one the first data read
            // (As magic detection is about fixed positions, we can be sure that it is enough data.)
            QMimeDatabase db;
            QMimeType mime = db.mimeTypeForFileNameAndData( path, buffer );
            qCDebug(KIO_ARCHIVE_LOG) << "Emitting mimetype" << mime.name();
            mimeType( mime.name() );
            firstRead = false;
        }
        data( buffer );
        processed += read;
        processedSize( processed );
        fileSize -= bufferSize;
    }
    io->close();

    data( QByteArray() );

    return KIO::WorkerResult::pass();
}

/*
  In case someone wonders how the old filter stuff looked like :    :)
void TARProtocol::slotData(void *_p, int _len)
{
  switch (m_cmd) {
    case CMD_PUT:
      assert(m_pFilter);
      m_pFilter->send(_p, _len);
      break;
    default:
      abort();
      break;
    }
}

void TARProtocol::slotDataEnd()
{
  switch (m_cmd) {
    case CMD_PUT:
      assert(m_pFilter && m_pJob);
      m_pFilter->finish();
      m_pJob->dataEnd();
      m_cmd = CMD_NONE;
      break;
    default:
      abort();
      break;
    }
}

void TARProtocol::jobData(void *_p, int _len)
{
  switch (m_cmd) {
  case CMD_GET:
    assert(m_pFilter);
    m_pFilter->send(_p, _len);
    break;
  case CMD_COPY:
    assert(m_pFilter);
    m_pFilter->send(_p, _len);
    break;
  default:
    abort();
  }
}

void TARProtocol::jobDataEnd()
{
  switch (m_cmd) {
  case CMD_GET:
    assert(m_pFilter);
    m_pFilter->finish();
    dataEnd();
    break;
  case CMD_COPY:
    assert(m_pFilter);
    m_pFilter->finish();
    m_pJob->dataEnd();
    break;
  default:
    abort();
  }
}

void TARProtocol::filterData(void *_p, int _len)
{
debug("void TARProtocol::filterData");
  switch (m_cmd) {
  case CMD_GET:
    data(_p, _len);
    break;
  case CMD_PUT:
    assert (m_pJob);
    m_pJob->data(_p, _len);
    break;
  case CMD_COPY:
    assert(m_pJob);
    m_pJob->data(_p, _len);
    break;
  default:
    abort();
  }
}
*/

// kate: space-indent on; indent-width 4; replace-tabs on;
