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

int main(int , char **)
{
  signal(SIGCHLD, KIOProtocol::sigchld_handler);
  signal(SIGSEGV, KIOProtocol::sigsegv_handler);

  KInstance instance( "kio_tar" );

  KIOConnection parent( 0, 1 );

  TARProtocol tar( &parent );
  tar.dispatchLoop();
}

TARProtocol::TARProtocol(KIOConnection *_conn) : KIOProtocol(_conn)
{
debug("TARProtocol::TARProtocol");
  m_cmd = CMD_NONE;
  m_pFilter = 0L;
  m_pJob = 0L;
}

void TARProtocol::slotGet(const char *_url)
{
debug("void TARProtocol::slotGet(%s) ",_url);
  assert(m_cmd == CMD_NONE);

  m_cmd = CMD_GET;

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

  finished();

  m_cmd = CMD_NONE;
}

void TARProtocol::slotPut(const char *, int , bool ,
			  bool , unsigned int)
{
}

void TARProtocol::slotCopy(const char *, const char *)
{
  fprintf(stderr, "TARProtocol::slotCopy\n");
  fflush(stderr);
}

void TARProtocol::slotListDir( const char *_url )
{
  kdebug( KDEBUG_INFO, 0, "=============== LIST %s ===============", _url  );
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

  m_cmd = CMD_LIST;

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

    /*
      TODO
    atom.m_uds = UDS_LINK_DEST;
    atom.m_str = slink;
    entry.append( atom );
    */

    listEntry( entry );
  }

  m_cmd = CMD_NONE;

  finished();

  kdebug( KDEBUG_INFO, 0, "=============== BYE ===========" );
}

void TARProtocol::slotTestDir( const char *_url )
{
  // OUCH, this is major duplicated code from slotListDir
  // Perhaps the KTar instance should be a member variable
  // but then we need to detect when we switch to another tar file...

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
  if (!path.isEmpty() && path != "/")
  {
    kdebug( KDEBUG_INFO, 0, QString("Looking for entry %1").arg(path) );
    const KTarEntry* e = root->entry( path );
    if (e->isDirectory())
      isDirectory();
    else
      isFile();
  } else {
    isDirectory();
  }

  finished();
}

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

void TARProtocol::jobError(int _errid, const char *_text)
{
  error(_errid, _text);
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


/*************************************
 *
 * TARIOJob
 *
 *************************************/

TARIOJob::TARIOJob(KIOConnection *_conn, TARProtocol *_tar) :
	KIOJobBase(_conn)
{
debug("TARIOJob::TARIOJob");
  m_pTAR = _tar;
}

void TARIOJob::slotData(void *_p, int _len)
{
  m_pTAR->jobData( _p, _len );
}

void TARIOJob::slotDataEnd()
{
  m_pTAR->jobDataEnd();
}

void TARIOJob::slotError(int _errid, const char *_txt)
{
  m_pTAR->jobError(_errid, _txt );
}


/*************************************
 *
 * TARFilter
 *
 *************************************/

TARFilter::TARFilter(TARProtocol *_tar, const char *_prg, const char **_argv)
  : KIOFilter(_prg, _argv)
{
debug("TARFilter::TARFilter()");
  m_pTAR = _tar;
}

void TARFilter::emitData(void *_p, int _len)
{
  m_pTAR->filterData(_p, _len);
}
