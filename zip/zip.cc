// $Id$

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <qfile.h>
#include <kurl.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kzip.h>
#include <kmimemagic.h>

#include <errno.h> // to be removed

#include "zip.h"

using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_zip" );

  kdDebug(7122) << "Starting holgikio_zip " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_zip protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  ZIPProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  kdDebug(7122) << "Done" << endl;
  return 0;
}

ZIPProtocol::ZIPProtocol( const QCString &pool, const QCString &app ) 
    : SlaveBase( "zip", pool, app )
{
  kdDebug( 7122 ) << "ZipProtocol::ZipProtocol" << endl;
  m_zipFile = 0L;
}

ZIPProtocol::~ZIPProtocol()
{
    delete m_zipFile;
}

bool ZIPProtocol::checkNewFile( QString fullPath, QString & path )
{
    kdDebug(7122) << "ZIPProtocol::checkNewFile " << fullPath << endl;


    // Are we already looking at that file ?
    if ( m_zipFile && m_zipFile->fileName() == fullPath.left(m_zipFile->fileName().length()) )
    {
        // Has it changed ?
        struct stat statbuf;
        if ( ::stat( QFile::encodeName( m_zipFile->fileName() ), &statbuf ) == 0 )
        {
            if ( m_mtime == statbuf.st_mtime )
            {
                path = fullPath.mid( m_zipFile->fileName().length() );
                kdDebug(7122) << "ZIPProtocol::checkNewFile returning " << path << endl;
                return true;
            }
        }
    }
    kdDebug(7122) << "Need to open a new file" << endl;

    // Close previous file
    if ( m_zipFile )
    {
        m_zipFile->close();
        delete m_zipFile;
        m_zipFile = 0L;
    }

    // Find where the zip file is in the full path
    int pos = 0;
    QString zipFile;
    path = QString::null;

    int len = fullPath.length();
    if ( len != 0 && fullPath[ len - 1 ] != '/' )
        fullPath += '/';

    kdDebug(7122) << "the full path is " << fullPath << endl;
    while ( (pos=fullPath.find( '/', pos+1 )) != -1 )
    {
        QString tryPath = fullPath.left( pos );
        kdDebug(7122) << fullPath << "  trying " << tryPath << endl;
        struct stat statbuf;
        if ( ::stat( QFile::encodeName(tryPath), &statbuf ) == 0 && !S_ISDIR(statbuf.st_mode) )
        {
            zipFile = tryPath;
            m_mtime = statbuf.st_mtime;
            path = fullPath.mid( pos + 1 );
            kdDebug(7122) << "fullPath=" << fullPath << " path=" << path << endl;
            len = path.length();
            if ( len > 1 )
            {
                if ( path[ len - 1 ] == '/' )
                    path.truncate( len - 1 );
            }
            else
                path = QString::fromLatin1("/");
            kdDebug(7122) << "Found. zipFile=" << zipFile << " path=" << path << endl;
            break;
        }
    }
    if ( zipFile.isEmpty() )
    {
        kdDebug(7122) << "ZIPProtocol::checkNewFile: not found" << endl;
        return false;
    }

    // Open new file
    kdDebug(7122) << "Opening KZip on " << zipFile << endl;
    m_zipFile = new KZip( zipFile );
    if ( !m_zipFile->open( IO_ReadOnly ) )
    {
        kdDebug(7122) << "Opening " << zipFile << "failed." << endl;
        delete m_zipFile;
        m_zipFile = 0L;
        return false;
    }
        kdDebug(7122) << "Opening " << zipFile << "done." << endl;

    return true;
}


void ZIPProtocol::createUDSEntry( const KArchiveEntry * archiveEntry, UDSEntry & entry )
{
    UDSAtom atom;
    entry.clear();
    atom.m_uds = UDS_NAME;
    atom.m_str = archiveEntry->name();
    entry.append(atom);

    atom.m_uds = UDS_FILE_TYPE;
    atom.m_long = archiveEntry->permissions() & S_IFMT; // keep file type only
    entry.append( atom );

    atom.m_uds = UDS_SIZE;
    atom.m_long = archiveEntry->isFile() ? ((KArchiveFile *)archiveEntry)->size() : 0L ;
    entry.append( atom );

    atom.m_uds = UDS_MODIFICATION_TIME;
    atom.m_long = archiveEntry->date();
    entry.append( atom );

    atom.m_uds = UDS_ACCESS;
    atom.m_long = archiveEntry->permissions() & 07777; // keep permissions only
    entry.append( atom );

    atom.m_uds = UDS_USER;
    atom.m_str = archiveEntry->user();
    entry.append( atom );

    atom.m_uds = UDS_GROUP;
    atom.m_str = archiveEntry->group();
    entry.append( atom );

    atom.m_uds = UDS_LINK_DEST;
    atom.m_str = archiveEntry->symlink();
    entry.append( atom );
}

void ZIPProtocol::listDir( const KURL & url )
{
    kdDebug( 7122 ) << "ZipProtocol::listDir " << url.url() << endl;

    QString path;
    if ( !checkNewFile( url.path(), path ) )
    {
        QCString _path( QFile::encodeName(url.path()));
        kdDebug( 7122 ) << "Checking (stat) on " << _path << endl;
        struct stat buff;
        if ( ::stat( _path.data(), &buff ) == -1 || !S_ISDIR( buff.st_mode ) ) {
            error( KIO::ERR_DOES_NOT_EXIST, url.path() );
            return;
        }
        // It's a real dir -> redirect
        KURL redir;
        redir.setPath( url.path() );
        kdDebug( 7122 ) << "Ok, redirection to " << redir.url() << endl;
        redirection( redir );
        finished();
        // And let go of the zip file - for people who want to unmount a cdrom after that
        delete m_zipFile;
        m_zipFile = 0L;
        return;
    }

    if ( path.isEmpty() )
    {
        KURL redir( QString::fromLatin1( "zip:/") );
        kdDebug() << "url.path()==" << url.path() << endl;
        redir.setPath( url.path() + QString::fromLatin1("/") );
        kdDebug() << "ZIPProtocol::listDir: redirection " << redir.url() << endl;
        redirection( redir );
        finished();
        return;
    }

    kdDebug( 7122 ) << "checkNewFile done" << endl;
    const KArchiveDirectory* root = m_zipFile->directory();
    const KArchiveDirectory* dir;
    if (!path.isEmpty() && path != "/")
    {
        kdDebug(7122) << QString("Looking for entry %1").arg(path) << endl;
        const KArchiveEntry* e = root->entry( path );
        if ( !e )
        {
            error( KIO::ERR_DOES_NOT_EXIST, path );
            return;
        }
        if ( ! e->isDirectory() )
        {
            error( KIO::ERR_IS_FILE, path );
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
        kdDebug(7122) << (*it) << endl;
        const KArchiveEntry* archiveEntry = dir->entry( (*it) );

        createUDSEntry( archiveEntry, entry );

        listEntry( entry, false );
    }

    listEntry( entry, true ); // ready

    finished();

    kdDebug( 7122 ) << "ZipProtocol::listDir done" << endl;
}

void ZIPProtocol::stat( const KURL & url )
{
    QString path;
    UDSEntry entry;
    if ( !checkNewFile( url.path(), path ) )
    {
        // We may be looking at a real directory - this happens
        // when pressing up after being in the root of an archive
        QCString _path( QFile::encodeName(url.path()));
        kdDebug( 7122 ) << "ZIPProtocol::stat (stat) on " << _path << endl;
        struct stat buff;
        if ( ::stat( _path.data(), &buff ) == -1 || !S_ISDIR( buff.st_mode ) ) {
            kdDebug() << "isdir=" << S_ISDIR( buff.st_mode ) << "  errno=" << strerror(errno) << endl;
            error( KIO::ERR_DOES_NOT_EXIST, url.path() );
            return;
        }
        // Real directory. Return just enough information for KRun to work
        UDSAtom atom;
        atom.m_uds = KIO::UDS_NAME;
        atom.m_str = url.fileName();
        entry.append( atom );
        kdDebug( 7122 ) << "ZIPProtocol::stat returning name=" << url.fileName() << endl;

        atom.m_uds = KIO::UDS_FILE_TYPE;
        atom.m_long = buff.st_mode & S_IFMT;
        entry.append( atom );

        statEntry( entry );

        finished();

        // And let go of the zip file - for people who want to unmount a cdrom after that
        delete m_zipFile;
        m_zipFile = 0L;
        return;
    }

    const KArchiveDirectory* root = m_zipFile->directory();
    const KArchiveEntry* zipEntry;
    if ( path.isEmpty() )
    {
        path = QString::fromLatin1( "/" );
        zipEntry = root;
    } else {
        zipEntry = root->entry( path );
    }
    if ( !zipEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, path );
        return;
    }

    createUDSEntry( zipEntry, entry );
    statEntry( entry );

    finished();
}

void ZIPProtocol::get( const KURL & url )
{
    kdDebug( 7122 ) << "ZipProtocol::get" << url.url() << endl;

    QString path;
    if ( !checkNewFile( url.path(), path ) )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.path() );
        return;
    }

    const KArchiveDirectory* root = m_zipFile->directory();
    const KArchiveEntry* zipEntry = root->entry( path );

    if ( !zipEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, path );
        return;
    }
    if ( zipEntry->isDirectory() )
    {
        error( KIO::ERR_IS_DIRECTORY, path );
        return;
    }
    const KArchiveFile* zipFileEntry = static_cast<const KArchiveFile *>(zipEntry);
    if ( !zipEntry->symlink().isEmpty() )
    {
      kdDebug(7102) << "Redirection to " << zipEntry->symlink() << endl;
      KURL realURL( url, zipEntry->symlink() );
      kdDebug(7102) << "realURL= " << realURL.url() << endl;
      redirection( realURL.url() );
      finished();
      return;
    }

    totalSize( zipFileEntry->size() );

    QByteArray completeData = zipFileEntry->data();

    KMimeMagicResult * result = KMimeMagic::self()->findBufferFileType( completeData, path );
    kdDebug(7102) << "Emitting mimetype " << result->mimeType() << endl;
    mimeType( result->mimeType() );

    data( completeData );

    processedSize( zipFileEntry->size() );

    finished();
}

