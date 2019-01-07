/*  This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kio_archivebase.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <QFile>
#include <QDir>
#include <QMimeDatabase>
#include <QMimeType>

#include <QUrl>
#include <ktar.h>
#include <kzip.h>
#include <kar.h>
#include <kio/global.h>
#include <kio_archive_debug.h>
#include <kuser.h>
#include <KLocalizedString>

#ifdef Q_OS_WIN
#define S_ISDIR(m) (((m & S_IFMT) == S_IFDIR))
#endif

using namespace KIO;

ArchiveProtocolBase::ArchiveProtocolBase( const QByteArray &proto, const QByteArray &pool, const QByteArray &app ) : SlaveBase( proto, pool, app )
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
    qCDebug(KIO_ARCHIVE_LOG) << "ArchiveProtocolBase::checkNewFile" << fullPath;


    // Are we already looking at that file ?
    if ( m_archiveFile && m_archiveName == fullPath.left(m_archiveName.length()) )
    {
        // Has it changed ?
        QT_STATBUF statbuf;
        if ( QT_STAT( QFile::encodeName( m_archiveName ), &statbuf ) == 0 )
        {
            if ( m_mtime == statbuf.st_mtime )
            {
                path = fullPath.mid( m_archiveName.length() );
                qCDebug(KIO_ARCHIVE_LOG) << "ArchiveProtocolBase::checkNewFile returning" << path;
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

    int len = fullPath.length();
    if ( len != 0 && fullPath[ len - 1 ] != '/' )
        fullPath += '/';

    qCDebug(KIO_ARCHIVE_LOG) << "the full path is" << fullPath;
    QT_STATBUF statbuf;
    statbuf.st_mode = 0; // be sure to clear the directory bit
    while ( (pos=fullPath.indexOf( '/', pos+1 )) != -1 )
    {
        QString tryPath = fullPath.left( pos );
        qCDebug(KIO_ARCHIVE_LOG) << fullPath << "trying" << tryPath;
        if ( QT_STAT( QFile::encodeName(tryPath), &statbuf ) == -1 )
        {
            // We are not in the file system anymore, either we have already enough data or we will never get any useful data anymore
            break;
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
            len = path.length();
            if ( len > 1 )
            {
                if ( path[ len - 1 ] == '/' )
                    path.truncate( len - 1 );
            }
            else
                path = QString::fromLatin1("/");
            qCDebug(KIO_ARCHIVE_LOG).nospace() << "Found. archiveFile=" << archiveFile << " path=" << path;
            break;
        }
    }
    if ( archiveFile.isEmpty() )
    {
        qCDebug(KIO_ARCHIVE_LOG) << "ArchiveProtocolBase::checkNewFile: not found";
        if ( S_ISDIR(statbuf.st_mode) ) // Was the last stat about a directory?
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
        qCWarning(KIO_ARCHIVE_LOG) << "Protocol" << url.scheme() << "not supported by this IOSlave" ;
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


void ArchiveProtocolBase::createRootUDSEntry( KIO::UDSEntry & entry )
{
    entry.clear();
    entry.fastInsert( KIO::UDSEntry::UDS_NAME, "." );
    entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
    entry.fastInsert( KIO::UDSEntry::UDS_MODIFICATION_TIME, m_mtime );
    //entry.fastInsert( KIO::UDSEntry::UDS_ACCESS, 07777 ); // fake 'x' permissions, this is a pseudo-directory
    entry.fastInsert( KIO::UDSEntry::UDS_USER, m_user);
    entry.fastInsert( KIO::UDSEntry::UDS_GROUP, m_group);
}

void ArchiveProtocolBase::createUDSEntry( const KArchiveEntry * archiveEntry, UDSEntry & entry )
{
    entry.clear();
    entry.fastInsert( KIO::UDSEntry::UDS_NAME, archiveEntry->name() );
    entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, archiveEntry->permissions() & S_IFMT ); // keep file type only
    entry.fastInsert( KIO::UDSEntry::UDS_SIZE, archiveEntry->isFile() ? ((KArchiveFile *)archiveEntry)->size() : 0L );
    entry.fastInsert( KIO::UDSEntry::UDS_MODIFICATION_TIME, archiveEntry->date().toTime_t());
    entry.fastInsert( KIO::UDSEntry::UDS_ACCESS, archiveEntry->permissions() & 07777 ); // keep permissions only
    entry.fastInsert( KIO::UDSEntry::UDS_USER, archiveEntry->user());
    entry.fastInsert( KIO::UDSEntry::UDS_GROUP, archiveEntry->group());
    entry.fastInsert( KIO::UDSEntry::UDS_LINK_DEST, archiveEntry->symLinkTarget());
}

void ArchiveProtocolBase::listDir( const QUrl & url )
{
    qCDebug(KIO_ARCHIVE_LOG) << "ArchiveProtocolBase::listDir" << url.url();

    QString path;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            error( KIO::ERR_SLAVE_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                             url.toDisplayString() ) );
            return;
        }
        else if ( errorNum != ERR_IS_DIRECTORY )
        {
            // We have any other error
            error( errorNum, url.toDisplayString() );
            return;
        }
        // It's a real dir -> redirect
        QUrl redir;
        redir.setPath( url.path() );
        qCDebug(KIO_ARCHIVE_LOG) << "Ok, redirection to" << redir.url();
        redirection( redir );
        finished();
        // And let go of the tar file - for people who want to unmount a cdrom after that
        delete m_archiveFile;
        m_archiveFile = nullptr;
        return;
    }

    if ( path.isEmpty() )
    {
        QUrl redir( url.scheme() + QString::fromLatin1( ":/") );
        qCDebug(KIO_ARCHIVE_LOG) << "url.path()=" << url.path();
        redir.setPath( url.path() + QString::fromLatin1("/") );
        qCDebug(KIO_ARCHIVE_LOG) << "ArchiveProtocolBase::listDir: redirection" << redir.url();
        redirection( redir );
        finished();
        return;
    }

    qCDebug(KIO_ARCHIVE_LOG) << "checkNewFile done";
    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveDirectory* dir;
    if (!path.isEmpty() && path != "/")
    {
        qCDebug(KIO_ARCHIVE_LOG) << "Looking for entry" << path;
        const KArchiveEntry* e = root->entry( path );
        if ( !e )
        {
            error( KIO::ERR_DOES_NOT_EXIST, url.toDisplayString() );
            return;
        }
        if ( ! e->isDirectory() )
        {
            error( KIO::ERR_IS_FILE, url.toDisplayString() );
            return;
        }
        dir = (KArchiveDirectory*)e;
    } else {
        dir = root;
    }

    const QStringList l = dir->entries();
    totalSize( l.count() );

    UDSEntry entry;
    if (!l.contains(".")) {
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

    finished();

    qCDebug(KIO_ARCHIVE_LOG) << "ArchiveProtocolBase::listDir done";
}

void ArchiveProtocolBase::stat( const QUrl & url )
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
            error( KIO::ERR_SLAVE_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                             url.toDisplayString() ) );
            return;
        }
        else if ( errorNum != ERR_IS_DIRECTORY )
        {
            // We have any other error
            error( errorNum, url.toDisplayString() );
            return;
        }
        // Real directory. Return just enough information for KRun to work
        entry.fastInsert( KIO::UDSEntry::UDS_NAME, url.fileName());
        qCDebug(KIO_ARCHIVE_LOG).nospace() << "ArchiveProtocolBase::stat returning name=" << url.fileName();

        QT_STATBUF buff;
#ifdef Q_OS_WIN
        QString fullPath = url.path().remove(0, 1);
#else
        QString fullPath = url.path();
#endif

        if ( QT_STAT( QFile::encodeName( fullPath ), &buff ) == -1 )
        {
            // Should not happen, as the file was already stated by checkNewFile
            error( KIO::ERR_COULD_NOT_STAT, url.toDisplayString() );
            return;
        }

        entry.fastInsert( KIO::UDSEntry::UDS_FILE_TYPE, buff.st_mode & S_IFMT);

        statEntry( entry );

        finished();

        // And let go of the tar file - for people who want to unmount a cdrom after that
        delete m_archiveFile;
        m_archiveFile = nullptr;
        return;
    }

    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveEntry* archiveEntry;
    if ( path.isEmpty() )
    {
        path = QString::fromLatin1( "/" );
        archiveEntry = root;
    } else {
        archiveEntry = root->entry( path );
    }
    if ( !archiveEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.toDisplayString() );
        return;
    }

    createUDSEntry( archiveEntry, entry );
    statEntry( entry );

    finished();
}

void ArchiveProtocolBase::get( const QUrl & url )
{
    qCDebug(KIO_ARCHIVE_LOG) << "ArchiveProtocolBase::get" << url.url();

    QString path;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            error( KIO::ERR_SLAVE_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1",
                             url.toDisplayString() ) );
            return;
        }
        else
        {
            // We have any other error
            error( errorNum, url.toDisplayString() );
            return;
        }
    }

    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveEntry* archiveEntry = root->entry( path );

    if ( !archiveEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.toDisplayString() );
        return;
    }
    if ( archiveEntry->isDirectory() )
    {
        error( KIO::ERR_IS_DIRECTORY, url.toDisplayString() );
        return;
    }
    const KArchiveFile* archiveFileEntry = static_cast<const KArchiveFile *>(archiveEntry);
    if ( !archiveEntry->symLinkTarget().isEmpty() )
    {
      const QString target = archiveEntry->symLinkTarget();
      qCDebug(KIO_ARCHIVE_LOG) << "Redirection to" << target;
      const QUrl realURL = url.resolved(QUrl(target));
      qCDebug(KIO_ARCHIVE_LOG) << "realURL=" << realURL;
      redirection( realURL );
      finished();
      return;
    }

    //qCDebug(KIO_ARCHIVE_LOG) << "Preparing to get the archive data";

    /*
     * The easy way would be to get the data by calling archiveFileEntry->data()
     * However this has drawbacks:
     * - the complete file must be read into the memory
     * - errors are skipped, resulting in an empty file
     */

    QIODevice* io = archiveFileEntry->createDevice();

    if (!io)
    {
        error( KIO::ERR_SLAVE_DEFINED,
            i18n( "The archive file could not be opened, perhaps because the format is unsupported.\n%1" ,
                      url.toDisplayString() ) );
        return;
    }

    if ( !io->open( QIODevice::ReadOnly ) )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, url.toDisplayString() );
        delete io;
        return;
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
        error( KIO::ERR_OUT_OF_MEMORY, url.toDisplayString() );
        delete io;
        return;
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
            error( KIO::ERR_COULD_NOT_READ, url.toDisplayString() );
            delete io;
            return;
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
    delete io;

    data( QByteArray() );

    finished();
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
