// $Id$

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <qfile.h>
#include <kurl.h>
#include <kdebug.h>
#include <kinstance.h>
#include <ktar.h>
#include <kmimemagic.h>

#include "tar.h"

using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_tar" );

  kdDebug(7109) << "Starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_tar protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  TARProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  kdDebug(7109) << "Done" << endl;
  return 0;
}

TARProtocol::TARProtocol( const QCString &pool, const QCString &app ) : SlaveBase( "tar", pool, app )
{
  kdDebug( 7109 ) << "TarProtocol::TarProtocol" << endl;
  m_tarFile = 0L;
}

TARProtocol::~TARProtocol()
{
    delete m_tarFile;
}

bool TARProtocol::checkNewFile( const QString & fullPath, QString & path )
{
    kdDebug(7109) << "TARProtocol::checkNewFile " << fullPath << endl;
    // Are we already looking at that file ?
    if ( m_tarFile && m_tarFile->fileName() == fullPath.left(m_tarFile->fileName().length()) )
    {
        path = fullPath.mid( m_tarFile->fileName().length() + 1 );
        return true;
    }
    kdDebug(7109) << "Need to open a new file" << endl;

    // Close previous file
    if ( m_tarFile )
    {
        m_tarFile->close();
        delete m_tarFile;
        m_tarFile = 0L;
    }

    // Find where the tar file is in the full path
    int pos = 0;
    QString tarFile;
    path = QString::null;

    while ( (pos=fullPath.find( '/', pos+1 )) != -1 )
    {
        QString tryPath = fullPath.left( pos );
        kdDebug() << tryPath << endl;
        struct stat statbuf;
        if ( ::stat( QFile::encodeName(tryPath), &statbuf ) == 0 && !S_ISDIR(statbuf.st_mode) )
        {
            tarFile = tryPath;
            path = fullPath.mid( pos );
            kdDebug(7109) << "Found. tarFile=" << tarFile << " path=" << path << endl;
            break;
        }
    }
    if ( tarFile.isEmpty() )
        return false;

    // Open new file
    kdDebug(7109) << "Opening KTarGz on " << tarFile << endl;
    m_tarFile = new KTarGz( tarFile );
    if ( !m_tarFile->open( IO_ReadOnly ) )
    {
        kdDebug(7109) << "Opening " << tarFile << "failed." << endl;
        delete m_tarFile;
        m_tarFile = 0L;
        return false;
    }

    return true;
}


void TARProtocol::createUDSEntry( const KTarEntry * tarEntry, UDSEntry & entry )
{
    UDSAtom atom;
    entry.clear();
    atom.m_uds = UDS_NAME;
    atom.m_str = tarEntry->name();
    entry.append(atom);

    atom.m_uds = UDS_FILE_TYPE;
    atom.m_long = tarEntry->isFile() ? S_IFREG : S_IFDIR; // HACK
    entry.append( atom );

    atom.m_uds = UDS_SIZE;
    atom.m_long = tarEntry->isFile() ? ((KTarFile *)tarEntry)->size() : 0L ;
    entry.append( atom );

    atom.m_uds = UDS_MODIFICATION_TIME;
    atom.m_long = tarEntry->date();
    entry.append( atom );

    atom.m_uds = UDS_ACCESS;
    atom.m_long = tarEntry->permissions();
    entry.append( atom );

    atom.m_uds = UDS_USER;
    atom.m_str = tarEntry->user();
    entry.append( atom );

    atom.m_uds = UDS_GROUP;
    atom.m_str = tarEntry->group();
    entry.append( atom );

    atom.m_uds = UDS_LINK_DEST;
    atom.m_str = tarEntry->symlink();
    entry.append( atom );
}

void TARProtocol::listDir( const KURL & url )
{
    kdDebug( 7109 ) << "TarProtocol::listDir " << url.url() << endl;

    QString path;
    if ( !checkNewFile( url.path(), path ) )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.path() );
        return;
    }
    if ( path.isEmpty() )
    {
        KURL redir;
        ASSERT( m_tarFile );
        redir.setPath( m_tarFile->fileName() );
        redirection( redir );
        // TODO
        finished();
        return;
    }

    kdDebug( 7109 ) << "checkNewFile done" << endl;
    const KTarDirectory* root = m_tarFile->directory();
    const KTarDirectory* dir;
    if (!path.isEmpty() && path != "/")
    {
        kdDebug(7109) << QString("Looking for entry %1").arg(path) << endl;
        const KTarEntry* e = root->entry( path );
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
        dir = (KTarDirectory*)e;
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
        const KTarEntry* tarEntry = dir->entry( (*it) );

        createUDSEntry( tarEntry, entry );

        listEntry( entry, false );
    }

    listEntry( entry, true ); // ready

    finished();

    kdDebug( 7109 ) << "TarProtocol::listDir done" << endl;
}

void TARProtocol::stat( const KURL & url )
{
    QString path;
    if ( !checkNewFile( url.path(), path ) )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.path() );
        return;
    }
    UDSEntry entry;

    const KTarDirectory* root = m_tarFile->directory();
    const KTarEntry* tarEntry = root->entry( path );
    if ( !tarEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, path );
        return;
    }

    createUDSEntry( tarEntry, entry );
    statEntry( entry );

    finished();
}

void TARProtocol::get( const KURL & url )
{
    kdDebug( 7109 ) << "TarProtocol::get" << url.url() << endl;

    QString path;
    if ( !checkNewFile( url.path(), path ) )
    {
        error( KIO::ERR_DOES_NOT_EXIST, url.path() );
        return;
    }

    const KTarDirectory* root = m_tarFile->directory();
    const KTarEntry* tarEntry = root->entry( path );

    if ( !tarEntry )
    {
        error( KIO::ERR_DOES_NOT_EXIST, path );
        return;
    }
    if ( tarEntry->isDirectory() )
    {
        error( KIO::ERR_IS_DIRECTORY, path );
        return;
    }
    const KTarFile* tarFileEntry = static_cast<const KTarFile *>(tarEntry);

    totalSize( tarFileEntry->size() );

    QByteArray completeData = tarFileEntry->data();

    KMimeMagicResult * result = KMimeMagic::self()->findBufferFileType( completeData, path );
    kdDebug(7102) << "Emitting mimetype " << result->mimeType() << endl;
    mimeType( result->mimeType() );

    data( completeData );

    processedSize( tarFileEntry->size() );

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

