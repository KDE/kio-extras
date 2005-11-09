#include <config.h>

#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include <qfile.h>
//Added by qt3to4:
#include <Q3CString>

#include <kglobal.h>
#include <kurl.h>
#include <kdebug.h>
#include <kinstance.h>
#include <ktar.h>
#include <kzip.h>
#include <kar.h>
#include <kmimemagic.h>
#include <klocale.h>
#include <kde_file.h>
#include <kio/global.h>

#include <errno.h> // to be removed

#include "tar.h"

using namespace KIO;

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_tar" );

  kdDebug(7109) << "Starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_tar protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  ArchiveProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  kdDebug(7109) << "Done" << endl;
  return 0;
}

ArchiveProtocol::ArchiveProtocol( const QByteArray &pool, const QByteArray &app ) : SlaveBase( "tar", pool, app )
{
  kdDebug( 7109 ) << "ArchiveProtocol::ArchiveProtocol" << endl;
  m_archiveFile = 0L;
}

ArchiveProtocol::~ArchiveProtocol()
{
    delete m_archiveFile;
}

bool ArchiveProtocol::checkNewFile( const KURL & url, QString & path, KIO::Error& errorNum )
{
    QString fullPath = url.path();
    kdDebug(7109) << "ArchiveProtocol::checkNewFile " << fullPath << endl;


    // Are we already looking at that file ?
    if ( m_archiveFile && m_archiveName == fullPath.left(m_archiveName.length()) )
    {
        // Has it changed ?
        KDE_struct_stat statbuf;
        if ( KDE_stat( QFile::encodeName( m_archiveName ), &statbuf ) == 0 )
        {
            if ( m_mtime == statbuf.st_mtime )
            {
                path = fullPath.mid( m_archiveName.length() );
                kdDebug(7109) << "ArchiveProtocol::checkNewFile returning " << path << endl;
                return true;
            }
        }
    }
    kdDebug(7109) << "Need to open a new file" << endl;

    // Close previous file
    if ( m_archiveFile )
    {
        m_archiveFile->close();
        delete m_archiveFile;
        m_archiveFile = 0L;
    }

    // Find where the tar file is in the full path
    int pos = 0;
    QString archiveFile;
    path = QString::null;

    int len = fullPath.length();
    if ( len != 0 && fullPath[ len - 1 ] != '/' )
        fullPath += '/';

    kdDebug(7109) << "the full path is " << fullPath << endl;
    KDE_struct_stat statbuf;
    statbuf.st_mode = 0; // be sure to clear the directory bit
    while ( (pos=fullPath.find( '/', pos+1 )) != -1 )
    {
        QString tryPath = fullPath.left( pos );
        kdDebug(7109) << fullPath << "  trying " << tryPath << endl;
        if ( KDE_stat( QFile::encodeName(tryPath), &statbuf ) == -1 )
        {
            // We are not in the file system anymore, either we have already enough data or we will never get any useful data anymore
            break;
        }
        if ( !S_ISDIR(statbuf.st_mode) )
        {
            archiveFile = tryPath;
            m_mtime = statbuf.st_mtime;
            path = fullPath.mid( pos + 1 );
            kdDebug(7109) << "fullPath=" << fullPath << " path=" << path << endl;
            len = path.length();
            if ( len > 1 )
            {
                if ( path[ len - 1 ] == '/' )
                    path.truncate( len - 1 );
            }
            else
                path = QString::fromLatin1("/");
            kdDebug(7109) << "Found. archiveFile=" << archiveFile << " path=" << path << endl;
            break;
        }
    }
    if ( archiveFile.isEmpty() )
    {
        kdDebug(7109) << "ArchiveProtocol::checkNewFile: not found" << endl;
        if ( S_ISDIR(statbuf.st_mode) ) // Was the last stat about a directory?
        {
            // Too bad, it is a directory, not an archive.
            kdDebug(7109) << "Path is a directory, not an archive." << endl;
            errorNum = KIO::ERR_IS_DIRECTORY;
        }
        else
            errorNum = KIO::ERR_DOES_NOT_EXIST;
        return false;
    }

    // Open new file
    if ( url.protocol() == "tar" ) {
        kdDebug(7109) << "Opening KTar on " << archiveFile << endl;
        m_archiveFile = new KTar( archiveFile );
    } else if ( url.protocol() == "ar" ) {
        kdDebug(7109) << "Opening KAr on " << archiveFile << endl;
        m_archiveFile = new KAr( archiveFile );
    } else if ( url.protocol() == "zip" ) {
        kdDebug(7109) << "Opening KZip on " << archiveFile << endl;
        m_archiveFile = new KZip( archiveFile );
    } else {
        kdWarning(7109) << "Protocol " << url.protocol() << " not supported by this IOSlave" << endl;
        errorNum = KIO::ERR_UNSUPPORTED_PROTOCOL;
        return false;
    }

    if ( !m_archiveFile->open( QIODevice::ReadOnly ) )
    {
        kdDebug(7109) << "Opening " << archiveFile << "failed." << endl;
        delete m_archiveFile;
        m_archiveFile = 0L;
        errorNum = KIO::ERR_CANNOT_OPEN_FOR_READING;
        return false;
    }

    m_archiveName = archiveFile;
    return true;
}


void ArchiveProtocol::createUDSEntry( const KArchiveEntry * archiveEntry, UDSEntry & entry )
{
    entry.clear();
    entry.insert( UDS_NAME, archiveEntry->name() );
    entry.insert( UDS_FILE_TYPE, archiveEntry->permissions() & S_IFMT ); // keep file type only
    entry.insert( UDS_SIZE, archiveEntry->isFile() ? ((KArchiveFile *)archiveEntry)->size() : 0L );
    entry.insert( UDS_MODIFICATION_TIME, archiveEntry->date());
    entry.insert( UDS_ACCESS, archiveEntry->permissions() & 07777 ); // keep permissions only
    entry.insert( UDS_USER, archiveEntry->user());
    entry.insert( UDS_GROUP, archiveEntry->group());
    entry.insert( UDS_LINK_DEST, archiveEntry->symlink());
}

void ArchiveProtocol::listDir( const KURL & url )
{
    kdDebug( 7109 ) << "ArchiveProtocol::listDir " << url.url() << endl;

    QString path;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            error( KIO::ERR_SLAVE_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1")
                           .arg( url.prettyURL() ) );
            return;
        }
        else if ( errorNum != ERR_IS_DIRECTORY )
        {
            // We have any other error
            error( errorNum, url.prettyURL() );
            return;
        }
        // It's a real dir -> redirect
        KURL redir;
        redir.setPath( url.path() );
        kdDebug( 7109 ) << "Ok, redirection to " << redir.url() << endl;
        redirection( redir );
        finished();
        // And let go of the tar file - for people who want to unmount a cdrom after that
        delete m_archiveFile;
        m_archiveFile = 0L;
        return;
    }

    if ( path.isEmpty() )
    {
        KURL redir( url.protocol() + QString::fromLatin1( ":/") );
        kdDebug( 7109 ) << "url.path()==" << url.path() << endl;
        redir.setPath( url.path() + QString::fromLatin1("/") );
        kdDebug( 7109 ) << "ArchiveProtocol::listDir: redirection " << redir.url() << endl;
        redirection( redir );
        finished();
        return;
    }

    kdDebug( 7109 ) << "checkNewFile done" << endl;
    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveDirectory* dir;
    if (!path.isEmpty() && path != "/")
    {
        kdDebug(7109) << QString("Looking for entry %1").arg(path) << endl;
        const KArchiveEntry* e = root->entry( path );
        if ( !e )
        {
            error( KIO::ERR_DOES_NOT_EXIST, url.prettyURL() );
            return;
        }
        if ( ! e->isDirectory() )
        {
            error( KIO::ERR_IS_FILE, url.prettyURL() );
            return;
        }
        dir = (KArchiveDirectory*)e;
    } else {
        dir = root;
    }

    QStringList l = dir->entries();
    totalSize( l.count() );

    UDSEntry entry;
    QStringList::Iterator it = l.begin();
    for( ; it != l.end(); ++it )
    {
        kdDebug(7109) << (*it) << endl;
        const KArchiveEntry* archiveEntry = dir->entry( (*it) );

        createUDSEntry( archiveEntry, entry );

        listEntry( entry, false );
    }

    listEntry( entry, true ); // ready

    finished();

    kdDebug( 7109 ) << "ArchiveProtocol::listDir done" << endl;
}

void ArchiveProtocol::stat( const KURL & url )
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
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1")
                           .arg( url.prettyURL() ) );
            return;
        }
        else if ( errorNum != ERR_IS_DIRECTORY )
        {
            // We have any other error
            error( errorNum, url.prettyURL() );
            return;
        }
        // Real directory. Return just enough information for KRun to work
        entry.insert( KIO::UDS_NAME, url.fileName());
        kdDebug( 7109 ) << "ArchiveProtocol::stat returning name=" << url.fileName() << endl;

        KDE_struct_stat buff;
        if ( KDE_stat( QFile::encodeName( url.path() ), &buff ) == -1 )
        {
            // Should not happen, as the file was already stated by checkNewFile
            error( KIO::ERR_COULD_NOT_STAT, url.prettyURL() );
            return;
        }

        entry.insert( KIO::UDS_FILE_TYPE, buff.st_mode & S_IFMT);

        statEntry( entry );

        finished();

        // And let go of the tar file - for people who want to unmount a cdrom after that
        delete m_archiveFile;
        m_archiveFile = 0L;
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
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyURL() );
        return;
    }

    createUDSEntry( archiveEntry, entry );
    statEntry( entry );

    finished();
}

void ArchiveProtocol::get( const KURL & url )
{
    kdDebug( 7109 ) << "ArchiveProtocol::get" << url.url() << endl;

    QString path;
    KIO::Error errorNum;
    if ( !checkNewFile( url, path, errorNum ) )
    {
        if ( errorNum == KIO::ERR_CANNOT_OPEN_FOR_READING )
        {
            // If we cannot open, it might be a problem with the archive header (e.g. unsupported format)
            // Therefore give a more specific error message
            error( KIO::ERR_SLAVE_DEFINED,
                   i18n( "Could not open the file, probably due to an unsupported file format.\n%1")
                           .arg( url.prettyURL() ) );
            return;
        }
        else
        {
            // We have any other error
            error( errorNum, url.prettyURL() );
            return;
        }
    }

    const KArchiveDirectory* root = m_archiveFile->directory();
    const KArchiveEntry* archiveEntry = root->entry( path );

    if ( !archiveEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyURL() );
        return;
    }
    if ( archiveEntry->isDirectory() )
    {
        error( KIO::ERR_IS_DIRECTORY, url.prettyURL() );
        return;
    }
    const KArchiveFile* archiveFileEntry = static_cast<const KArchiveFile *>(archiveEntry);
    if ( !archiveEntry->symlink().isEmpty() )
    {
      kdDebug(7109) << "Redirection to " << archiveEntry->symlink() << endl;
      KURL realURL( url, archiveEntry->symlink() );
      kdDebug(7109) << "realURL= " << realURL.url() << endl;
      redirection( realURL );
      finished();
      return;
    }

    //kdDebug(7109) << "Preparing to get the archive data" << endl;

    /*
     * The easy way would be to get the data by calling archiveFileEntry->data()
     * However this has drawbacks:
     * - the complete file must be read into the memory
     * - errors are skipped, resulting in an empty file
     */

    QIODevice* io = 0;
    // Getting the device is hard, as archiveFileEntry->device() is not virtual!
    if ( url.protocol() == "tar" )
    {
        io = archiveFileEntry->device();
    }
    else if ( url.protocol() == "ar" )
    {
        io = archiveFileEntry->device();
    }
    else if ( url.protocol() == "zip" )
    {
        io = ((KZipFileEntry*) archiveFileEntry)->device();
    }
    else
    {
        // Wrong protocol? Why was this not catched by checkNewFile?
        kdWarning(7109) << "Protocol " << url.protocol() << " not supported by this IOSlave; " << k_funcinfo << endl;
        error( KIO::ERR_UNSUPPORTED_PROTOCOL, url.protocol() );
        return;
    }

    if (!io)
    {
        error( KIO::ERR_SLAVE_DEFINED,
            i18n( "The archive file could not be opened, perhaps because the format is unsupported.\n%1" )
                    .arg( url.prettyURL() ) );
        return;
    }

    if ( !io->open( QIODevice::ReadOnly ) )
    {
        error( KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyURL() );
        return;
    }

    totalSize( archiveFileEntry->size() );

    // Size of a QIODevice read. It must be large enough so that the mime type check will not fail
    const int maxSize = 0x100000; // 1MB

    int bufferSize = qMin( maxSize, archiveFileEntry->size() );
    QByteArray buffer ( bufferSize );
    if ( buffer.isEmpty() && bufferSize > 0 )
    {
        // Something went wrong
        error( KIO::ERR_OUT_OF_MEMORY, url.prettyURL() );
        return;
    }

    bool firstRead = true;

    // How much file do we still have to process?
    int fileSize = archiveFileEntry->size();
    KIO::filesize_t processed = 0;

    while ( !io->atEnd() && fileSize > 0 )
    {
        if ( !firstRead )
        {
            bufferSize = qMin( maxSize, fileSize );
            buffer.resize( bufferSize );
        }
        const Q_LONG read = io->readBlock( buffer.data(), buffer.size() ); // Avoid to use bufferSize here, in case something went wrong.
        if ( read != bufferSize )
        {
            kdWarning(7109) << "Read " << read << " bytes but expected " << bufferSize << endl;
            error( KIO::ERR_COULD_NOT_READ, url.prettyURL() );
            return;
        }
        if ( firstRead )
        {
            // We use the magic one the first data read
            // (As magic detection is about fixed positions, we can be sure that it is enough data.)
            KMimeMagicResult * result = KMimeMagic::self()->findBufferFileType( buffer, path );
            kdDebug(7109) << "Emitting mimetype " << result->mimeType() << endl;
            mimeType( result->mimeType() );
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
