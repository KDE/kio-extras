// $Id$

#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include <qapplication.h>

#include <iostream>

#include <kurl.h>
#include <kprotocolmanager.h>
#include <kio_filter.h>
#include <kinstance.h>

#include "gzip.h"

int main( int , char ** )
{
  signal (SIGCHLD, KIOProtocol::sigchld_handler);

  KInstance instance( "kio_gzip" );

  KIOConnection parent( 0, 1 );

  GZipProtocol prot( &parent );
  prot.dispatchLoop();

  while(1);
  return 0;
}

GZipProtocol::GZipProtocol( KIOConnection *_conn ) : KIOProtocol( _conn )
{
  m_cmd = CMD_NONE;
  m_pFilter = 0L;
  m_pJob = 0L;
}


void GZipProtocol::slotCopy( const char *_source, const char *_dest )
{
  assert( m_cmd == CMD_NONE );

  m_cmd = CMD_COPY;

  /*****
   * Destination
   *****/

  KURL udest( _dest );
  if ( udest.isMalformed() )
  {
    error( ERR_MALFORMED_URL, strdup(_dest) );
    m_cmd = CMD_NONE;
    return;
  }

  QString dest_exec = KProtocolManager::self().executable( udest.protocol() );

  if ( dest_exec.isEmpty() ) {
    error( ERR_UNSUPPORTED_PROTOCOL, udest.protocol() );
    m_cmd = CMD_NONE;
    return;
  }

  // Start the dest slave
  KIOSlave dest_slave( dest_exec );
  if ( !dest_slave.isRunning()) {
    error( ERR_CANNOT_LAUNCH_PROCESS, dest_exec );
    m_cmd = CMD_NONE;
    return;
  }

  // Put a protocol on top of it
  GZipIOJob dest_job( &dest_slave, this );
  m_pJob = &dest_job;

  // Tell the "dest slave" that we want to write something
  dest_job.put( _dest, -1, false, false, 0 );
  while( !dest_job.isReady() && !dest_job.hasFinished() )
    dest_job.dispatch();

  if( dest_job.hasError() ) {
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  /*****
   * Source
   *****/

  KURL usrc( _source );
  if ( usrc.isMalformed() ) {
    error( ERR_MALFORMED_URL, strdup(_source) );
    m_cmd = CMD_NONE;
    return;
  }

  // Split the source URL
  KURL::List source_lst = KURL::split( _source );
  if ( source_lst.isEmpty() ) {
    error( ERR_MALFORMED_URL, strdup(_source) );
    m_cmd = CMD_NONE;
    return;
  }

  m_strProtocol = usrc.protocol().ascii();

  if (strcmp("gzip", m_strProtocol) != 0 &&
      strcmp("bzip", m_strProtocol) != 0 &&
      strcmp("bzip2", m_strProtocol) != 0) {
    error( ERR_INTERNAL, "kio_gzip got a URL which does not contain the gzip/bzip/bzip2 protocol" );
    m_cmd = CMD_NONE;
    return;
  }

  if ( source_lst.count() < 2 ) {
    error( ERR_NO_SOURCE_PROTOCOL,  m_strProtocol );
    m_cmd = CMD_NONE;
    return;
  }

  QString zip_cmd;
  QString path = usrc.path();
  if ( path == "/compress" ){
    if ( strcmp("gzip", m_strProtocol) == 0 )
      zip_cmd = "gzip";
    else if ( strcmp("bzip", m_strProtocol) == 0 )
      zip_cmd = "bzip";
    else if ( strcmp("bzip2", m_strProtocol) == 0 )
      zip_cmd = "bzip2";
  }
  else if ( path == "/decompress" ){
    if ( strcmp("gzip", m_strProtocol) == 0 )
      zip_cmd = "gunzip";
    else if ( strcmp("bzip", m_strProtocol) == 0 )
      zip_cmd = "bunzip";
    else if ( strcmp("bzip2", m_strProtocol) == 0 )
      zip_cmd = "bunzip2";
  }
  else
  {
    QString e = m_strProtocol;
    e += ":";
    e += path;
    error( ERR_UNSUPPORTED_ACTION, e );
    m_cmd = CMD_NONE;
    return;
  }

  // Strip the gzip URL part
  source_lst.remove( source_lst.begin() );

  // Find the new left most URL and a slave that can handle
  // this protocol
  QString source_exec = KProtocolManager::self().executable( (*source_lst.begin()).protocol() );

  // Did we find someone for this protocol ?
  if ( source_exec.isEmpty() ) {
    error( ERR_UNSUPPORTED_PROTOCOL, (*source_lst.begin()).protocol() );
    m_cmd = CMD_NONE;
    return;
  }

  // Start the source slave
  KIOSlave source_slave( source_exec );
  if ( !source_slave.isRunning()) {
    error( ERR_CANNOT_LAUNCH_PROCESS, source_exec );
    return;
  }

  /*****
   * ZIP
   *****/

  // Start GZIP
  GZipFilter filter( zip_cmd, this );
  m_pFilter = &filter;
  if ( filter.pid() == -1 ) {
    error( ERR_CANNOT_LAUNCH_PROCESS, zip_cmd );
    return;
  }

  GZipIOJob source_job( &source_slave, this );

  QString src = KURL::join( source_lst );

  source_job.get( src );
  while( !source_job.isReady() && !source_job.hasFinished() )
    source_job.dispatch();

  if( source_job.hasFinished() ) {
    finished();
    return;
  }

  while( !source_job.hasFinished() )
    source_job.dispatch();

  finished();

  m_cmd = CMD_NONE;
}

void GZipProtocol::slotTestDir( const char *_url )
{
  isFile();
  finished();
}

void GZipProtocol::slotGet( const char *_url )
{
  assert( m_cmd == CMD_NONE );

  m_cmd = CMD_GET;

  KURL::List lst = KURL::split( _url );
  if ( lst.isEmpty()  ) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    m_cmd = CMD_NONE;
    return;
  }

  m_strProtocol = (*lst.begin()).protocol();

  if (strcmp("gzip", m_strProtocol) != 0 &&
      strcmp("bzip", m_strProtocol) != 0 &&
      strcmp("bzip2", m_strProtocol) != 0) {
    error( ERR_INTERNAL, "kio_gzip got a URL which does not contain the gzip/bzip/bzip2 protocol" );
    m_cmd = CMD_NONE;
    return;
  }

  if ( lst.count() < 2 )
  {
    error( ERR_NO_SOURCE_PROTOCOL, m_strProtocol );
    m_cmd = CMD_NONE;
    return;
  }

  QString zip_cmd;
  QString path = (*lst.begin()).path();
  if ( path == "/compress" ){
    if ( strcmp("gzip", m_strProtocol) == 0 )
      zip_cmd = "gzip";
    else if ( strcmp("bzip", m_strProtocol) == 0 )
      zip_cmd = "bzip";
    else if ( strcmp("bzip2", m_strProtocol) == 0 )
      zip_cmd = "bzip2";
  }
  else if ( path == "/decompress" ){
    if ( strcmp("gzip", m_strProtocol) == 0 )
      zip_cmd = "gunzip";
    else if ( strcmp("bzip", m_strProtocol) == 0 )
      zip_cmd = "bunzip";
    else if ( strcmp("bzip2", m_strProtocol) == 0 )
      zip_cmd = "bunzip2";
  }
  else
  {
    error( ERR_UNSUPPORTED_ACTION, path );
    m_cmd = CMD_NONE;
    return;
  }

  if ( lst.count() < 2 ) {
    QString e = m_strProtocol;
    e += ":";
    e += path;
    error( ERR_NO_SOURCE_PROTOCOL, e );
    m_cmd = CMD_NONE;
    return;
  }

  // Remove gzip portion of URL
  lst.remove( lst.begin() );

  QString exec = KProtocolManager::self().executable( (*lst.begin()).protocol() );

  if ( exec.isEmpty() ) {
    error( ERR_UNSUPPORTED_PROTOCOL, (*lst.begin()).protocol() );
    m_cmd = CMD_NONE;
    return;
  }

  // Start the file protcol
  KIOSlave slave( exec );
  if ( !slave.isRunning() ) {
    error( ERR_CANNOT_LAUNCH_PROCESS, exec );
    return;
  }

  // Start GZIP
  GZipFilter filter( zip_cmd, this );
  m_pFilter = &filter;
  if ( filter.pid() == -1 ) {
    error( ERR_CANNOT_LAUNCH_PROCESS, zip_cmd );
    return;
  }

  GZipIOJob job( &slave, this );

  QString src = KURL::join( lst );

  qDebug( "kio_gzip : Nested fetching %s", src.ascii() );

  job.get( src );
  while( !job.isReady() && !job.hasFinished() )
    job.dispatch();

  if( job.hasFinished() ) {
    finished();
    return;
  }

  ready();

  while( !job.hasFinished() )
    job.dispatch();

  finished();

  m_cmd = CMD_NONE;
}


void GZipProtocol::slotPut( const char *_url, int, bool _overwrite, bool /*_resume*/, unsigned int )
{
  assert( m_cmd == CMD_NONE );

  m_cmd = CMD_PUT;

  // Split up the URL
  KURL::List lst = KURL::split( _url );
  if ( lst.isEmpty() ) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  m_strProtocol = (*lst.begin()).protocol().ascii();	

  // Is the left most URL really the gzip protocol ?
    if (strcmp("gzip", m_strProtocol) != 0 &&
        strcmp("bzip", m_strProtocol) != 0 &&
        strcmp("bzip2", m_strProtocol) != 0) {
    error( ERR_INTERNAL, "kio_gzip got a URL which does not contain the gzip/bzip/bzip2 protocol" );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  // Since this is a filter there must be at least an additional source protocol
  if ( lst.count() < 2 ) {
    error( ERR_NO_SOURCE_PROTOCOL, m_strProtocol );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  // Find out what to do ( compres, decompress )
  QString zip_cmd;
  QString path = (*lst.begin()).path();
  if ( path == "/compress" ){
    if ( strcmp("gzip", m_strProtocol) == 0 )
      zip_cmd = "gzip";
    else if ( strcmp("bzip", m_strProtocol) == 0 )
      zip_cmd = "bzip";
    else if ( strcmp("bzip2", m_strProtocol) == 0 )
      zip_cmd = "bzip2";
  }
  else if ( path == "/decompress" ){
    if ( strcmp("gzip", m_strProtocol) == 0 )
      zip_cmd = "gunzip";
    else if ( strcmp("bzip", m_strProtocol) == 0 )
      zip_cmd = "bunzip";
    else if ( strcmp("bzip2", m_strProtocol) == 0 )
      zip_cmd = "bunzip2";
  }
  else
  {
    QString e = m_strProtocol;
    e += ":";
    e += path;
    error( ERR_UNSUPPORTED_ACTION, e );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  // Strip the gzip url
  lst.remove( lst.begin() );

  // Find someone who can handle the new left most protocol
  QString exec = KProtocolManager::self().executable( (*lst.begin()).protocol() );

  // Did we find someone to handle this protocol ?
  if ( exec.isEmpty() ) {
    error( ERR_UNSUPPORTED_PROTOCOL, (*lst.begin()).protocol() );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  // Start the protocol
  KIOSlave slave( exec );
  if ( !slave.isRunning() ) {
    error( ERR_CANNOT_LAUNCH_PROCESS, exec );
    finished();
    return;
  }

  // Start the ZIP process
  GZipFilter filter( zip_cmd, this );
  m_pFilter = &filter;
  if ( filter.pid() == -1 ) {
    error( ERR_CANNOT_LAUNCH_PROCESS, zip_cmd );
    finished();
    return;
  }

  // Put a protocol on top of the connection
  GZipIOJob job( &slave, this );
  m_pJob = &job;

  // Create what is left from the URL after stripping the
  // gzip part.
  QString src = KURL::join( lst );

  // Tell the other job that we want to write something
  job.put( src, -1, _overwrite, false, 0 );
  while( !job.isReady() && !job.hasFinished() )
    job.dispatch();

  if ( job.hasError() ) {
    finished();
    return;
  }

  // We are ready for receiving data
  ready();

  // Loop until we got 'dataEnd'
  while( m_cmd == CMD_PUT && dispatch() );

  // We have done our job => finish
  finished();
}


void GZipProtocol::slotData( void *_p, int _len )
{
  switch( m_cmd )
    {
    case CMD_PUT:
      assert( m_pFilter );
      m_pFilter->send( _p, _len );
      break;
    default:
      assert( 0 );
      break;
    }
}


void GZipProtocol::slotDataEnd()
{
  switch( m_cmd )
    {
    case CMD_PUT:
      assert( m_pFilter && m_pJob );
      m_pFilter->finish();
      m_pJob->dataEnd();
      m_cmd = CMD_NONE;
      break;
    default:
      assert( 0 );
      break;
    }
}


void GZipProtocol::jobData( void *_p, int _len )
{
  switch( m_cmd )
  {
  case CMD_GET:
    assert( m_pFilter );
    m_pFilter->send( _p, _len );
    break;
  case CMD_COPY:
    assert( m_pFilter );
    m_pFilter->send( _p, _len );
    break;
  default:
    assert( 0 );
  }
}


void GZipProtocol::jobDataEnd()
{
  switch( m_cmd )
  {
  case CMD_GET:
    assert( m_pFilter );
    m_pFilter->finish();
    dataEnd();
    break;
  case CMD_COPY:
    assert( m_pFilter );
    m_pFilter->finish();
    m_pJob->dataEnd();
    break;
  default:
    assert( 0 );
  }
}


void GZipProtocol::jobError( int _errid, const char *_text )
{
  error( _errid, _text );
}


void GZipProtocol::filterData( void *_p, int _len )
{
  switch( m_cmd )
  {
  case CMD_GET:
    data( _p, _len );
    break;
  case CMD_PUT:
    assert( m_pJob );
    m_pJob->data( _p, _len );
    break;
  case CMD_COPY:
    assert( m_pJob );
    m_pJob->data( _p, _len );
    break;
  default:
    assert( 0 );
  }
}



/*************************************
 *
 * GZipIOJob
 *
 *************************************/

GZipIOJob::GZipIOJob( KIOConnection *_conn, GZipProtocol *_gzip ) : KIOJobBase( _conn )
{
  m_pGZip = _gzip;
}

void GZipIOJob::slotData( void *_p, int _len )
{
  m_pGZip->jobData( _p, _len );

}

void GZipIOJob::slotDataEnd()
{
  m_pGZip->jobDataEnd();
}

void GZipIOJob::slotError( int _errid, const char *_txt )
{
  m_pGZip->jobError( _errid, _txt );
}



GZipFilter::GZipFilter( const char *_prg, GZipProtocol *_gzip ) : KIOFilter( _prg )
{
  m_pGZip = _gzip;
}

void GZipFilter::emitData( void *_p, int _len )
{
  m_pGZip->filterData( _p, _len );
}
