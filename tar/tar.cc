// $Id$

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <kurl.h>
#include <kdebug.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <ktar.h>

#include "tar.h"

using namespace KIO;

TARProtocol::TARProtocol(Connection *connection)
    : SlaveBase( "tar", connection)
{
  kdDebug( 7109 ) << "TarProtocol::TarProtocol" << endl;
}

TARProtocol::~TARProtocol()
{
}

void TARProtocol::get( const QString & path, const QString & /*query*/, bool /*reload*/ )
{
  kdDebug( 7109 ) << "TarProtocol::get" << endl;
  SlaveBase::get( path, QString::null, true ); // NOT IMPLEMENTED

  /*
  KURL::List lst = KURL::split(_url);
  if (lst.isEmpty())  {
    error(ERR_MALFORMED_URL, _url);
    m_cmd = CMD_NONE;
    return;
  }

  if ((*lst.begin()).protocol() != "tar")  {
    error( ERR_INTERNAL, "kio_tar got a URL which does not contain the tar protocol");
    m_cmd = CMD_NONE;
    return;
  }

  if (lst.count() < 2) {
    error(ERR_NO_SOURCE_PROTOCOL, "tar");
    m_cmd = CMD_NONE;
    return;
  }

  QString path=(*lst.begin()).path();

  // Remove tar protocol
  lst.remove(lst.begin());

  QString exec = KProtocolManager::self().executable((*lst.begin()).protocol());

  if (exec.isEmpty()) {
    error(ERR_UNSUPPORTED_PROTOCOL, (*lst.begin()).protocol());
    m_cmd = CMD_NONE;
    return;
  }

  // Start the file protcol
  KIOSlave slave(exec);
  if (!slave.isRunning()) {
    error(ERR_CANNOT_LAUNCH_PROCESS, exec);
    return;
  }

  QString tar_cmd = "tar";
  // x eXtract
  // O standard Output
  // f from a file
  // - standard input
  const char *argv[4] = {"-xOf","-", path.ascii()+1, 0};
  debug("argv : -x0f - %s",  path.ascii()+1);

  // Start the TAR process
  TARFilter filter(this, tar_cmd, argv);
  m_pFilter = &filter;
  if ( filter.pid() == -1 ) {
    error(ERR_CANNOT_LAUNCH_PROCESS, tar_cmd);
    finished();
    return;
  }

  TARIOJob job(&slave, this);
  QString src = KURL::join( lst );
  debug("kio_tar : Nested fetching %s", src.ascii());

  job.get(src);
  while(!job.isReady() && !job.hasFinished())
    job.dispatch();

  if( job.hasFinished()) {
    finished();
    return;
  }

  while(!job.hasFinished())
    job.dispatch();

  m_cmd = CMD_NONE;
  */
  finished();
}

void TARProtocol::listDir( const QString & path )
{
  kdDebug( 7109 ) << "TarProtocol::listDir " << path << endl;
  SlaveBase::listDir( path ); // NOT IMPLEMENTED
  /*
  // Split url
  KURL::List lst = KURL::split( _url );
  if ( lst.isEmpty() ) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    m_cmd = CMD_NONE;
    return;
  }

  QString protocol = (*lst.begin()).protocol();

  if ( lst.count() < 2 )
  {
    error( ERR_NO_SOURCE_PROTOCOL, protocol );
    m_cmd = CMD_NONE;
    return;
  }

  if ( strcmp( protocol, "tar" ) != 0L ) {
    error( ERR_INTERNAL, "kio_tar got non tar file in list command" );
    m_cmd = CMD_NONE;
    return;
  }

  // Save path required
  QString path = (*lst.begin()).path();
  // Remove tar portion of URL
  lst.remove( lst.begin() );

  // HACK ktar supports tar.gz, so strip gzip
  if ( (*lst.begin()).protocol() == "gzip" )
  {
    debug("HACK");
    lst.remove( lst.begin() );
  }

  // HACK : only supports local files for now
  if ( (*lst.begin()).protocol() != "file" || lst.count() != 1 )
  {
    error( ERR_UNSUPPORTED_PROTOCOL, (*lst.begin()).protocol() );
    m_cmd = CMD_NONE;
    return;
  }

  kdebug( KDEBUG_INFO, 0, QString("Opening KTarGz on %1").arg((*lst.begin()).path()));
  KTarGz ktar( (*lst.begin()).path() );
  if ( !ktar.open( IO_ReadOnly ) ) {
    error( ERR_CANNOT_ENTER_DIRECTORY, strdup(_url) );
    m_cmd = CMD_NONE;
    return;
  }

  const KTarDirectory* root = ktar.directory();
  const KTarDirectory* dir;
  if (!path.isEmpty() && path != "/")
  {
    kdebug( KDEBUG_INFO, 0, QString("Looking for entry %1").arg(path) );
    const KTarEntry* e = root->entry( path );
    ASSERT( e && e->isDirectory() );
    dir = (KTarDirectory*)e;
  } else {
    dir = root;
  }

  QStringList l = dir->entries();
  totalFiles( l.count() );

  KUDSEntry entry;
  KUDSAtom atom;
  QStringList::Iterator it = l.begin();
  for( ; it != l.end(); ++it )
  {
    printf("%s\n", (*it).ascii());
    const KTarEntry* tarEntry = dir->entry( (*it) );

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

    listEntry( entry );
  }
  */

  finished();

  kdDebug( 7109 ) << "TarProtocol::listDir done" << endl;
}

void TARProtocol::stat( const QString & path )
{
    SlaveBase::stat( path ); // NOT IMPLEMENTED
}

/*
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

extern "C" {
    SlaveBase *init_tar() {
        return new TARProtocol();
    }
}
