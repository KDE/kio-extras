#include "gzip.h"

#include <kio_filter.h>
#include <kio_manager.h>

#include <qapplication.h>

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>

#include <kurl.h>

void sig_handler( int signum );
void sig_handler2( int signum );

int main( int argc, char **argv )
{
  signal(SIGCHLD,sig_handler);

  ProtocolManager manager;
  
  Connection parent( 0, 1 );

  GZipProtocol prot( &parent );
  prot.dispatchLoop();
  
  while(1);
}

void sig_handler( int )
{
  int status;
  int saved_errno = errno;

  while( waitpid( -1, &status, WNOHANG ) > 0 );

  errno = saved_errno;
  signal( SIGCHLD, sig_handler );

  return;
}


GZipProtocol::GZipProtocol( Connection *_conn ) : IOProtocol( _conn )
{
  m_cmd = CMD_NONE;
  m_pFilter = 0L;
  m_pJob = 0L;
}


void GZipProtocol::slotCopy( const char *_source, const char *_dest )
{
  assert( m_cmd == CMD_NONE );

  string source = _source;
  string dest = _dest;
  
  m_cmd = CMD_COPY;

  /*****
   * Destination
   *****/

  // Split up the destination URL
  KURLList dest_lst;
  if ( !KURL::split( _dest, dest_lst )  )
  {
    error( ERR_MALFORMED_URL, dest.c_str() );
    m_cmd = CMD_NONE;
    return;
  }

  string dest_exec = ProtocolManager::self()->find( dest_lst.getLast()->protocol() );

  if ( dest_exec.empty() )
  {
    error( ERR_UNSUPPORTED_PROTOCOL, dest_lst.getLast()->protocol() );
    m_cmd = CMD_NONE;
    return;
  }

  // Start the dest slave
  Slave dest_slave( dest_exec.c_str() );
  if ( dest_slave.pid() == -1 )
  {
    error( ERR_CANNOT_LAUNCH_PROCESS, dest_exec.c_str() );
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

  if( dest_job.hasError() )
  {
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  /*****
   * Source
   *****/

  // Split the source URL
  KURLList source_lst;
  if ( !KURL::split( _source, source_lst )  )
  {
    error( ERR_MALFORMED_URL, source.c_str() );
    m_cmd = CMD_NONE;
    return;
  }

  if ( strcmp( source_lst.getLast()->protocol(), "gzip" ) != 0L )
  {
    error( ERR_INTERNAL, "kio_gzip got a URL which has not the gzip protocol" );
    m_cmd = CMD_NONE;
    return;
  }
  
  if ( source_lst.count() < 2 )
  {
    error( ERR_NO_SOURCE_PROTOCOL, "gzip" );
    m_cmd = CMD_NONE;
    return;
  }
  
  string zip_cmd;
  const char * path = source_lst.getLast()->path();
  if ( strcmp( path, "/compress" ) == 0 )
    zip_cmd = "/usr/bin/gzip";
  else if ( strcmp( path, "/decompress" ) == 0 )
    zip_cmd = "/usr/bin/gunzip";
  else if ( strcmp( path, "/bzip/compress" ) == 0 )
    zip_cmd = "/usr/local/bin/bzip";
  else if ( strcmp( path, "/bzip/decompress" ) == 0 )
    zip_cmd = "/usr/local/bin/bunzip";
  else
  {
    string e = "gzip:";
    e += path;
    error( ERR_UNSUPPORTED_ACTION, e.c_str() );
    m_cmd = CMD_NONE;
    return;
  }
     
  // Strip the gzip URL part
  source_lst.removeLast();
  
  // Find the new right most URL and a slave that can handle
  // this protocol
  string source_exec = ProtocolManager::self()->find( source_lst.getLast()->protocol() );

  // Did we find someone for this protocol ?
  if ( source_exec.empty() )
  {
    error( ERR_UNSUPPORTED_PROTOCOL, source_lst.getLast()->protocol() );
    m_cmd = CMD_NONE;
    return;
  }

  // Start the source slave
  Slave source_slave( source_exec.c_str() );
  if ( source_slave.pid() == -1 )
  {
    error( ERR_CANNOT_LAUNCH_PROCESS, source_exec.c_str() );
    return;
  }

  /*****
   * ZIP
   *****/

  // Start GZIP
  GZipFilter filter( zip_cmd.c_str(), this );
  m_pFilter = &filter;
  if ( filter.pid() == -1 )
  {
    error( ERR_CANNOT_LAUNCH_PROCESS, zip_cmd.c_str() );
    return;
  }

  GZipIOJob source_job( &source_slave, this );

  QString src;
  KURL * it = source_lst.first();
  for( ; it ; it = source_lst.next() )
  {
    src += it->url();
  }
  
  source_job.get( src );
  while( !source_job.isReady() && !source_job.hasFinished() )
    source_job.dispatch();

  if( source_job.hasFinished() )
  {
    finished();
    return;
  }

  while( !source_job.hasFinished() )
    source_job.dispatch();
  
  finished();

  m_cmd = CMD_NONE;
}

  
void GZipProtocol::slotGet( const char *_url )
{
  assert( m_cmd == CMD_NONE );

  string url = _url;
  
  m_cmd = CMD_GET;

  KURLList lst;
  if ( !KURL::split( _url, lst )  )
  {
    error( ERR_MALFORMED_URL, url.c_str() );
    m_cmd = CMD_NONE;
    return;
  }

  if ( strcmp( lst.getLast()->protocol(), "gzip" ) != 0L )
  {
    error( ERR_INTERNAL, "kio_gzip got a URL which has not the gzip protocol" );
    m_cmd = CMD_NONE;
    return;
  }
  
  if ( lst.count() < 2 )
  {
    error( ERR_NO_SOURCE_PROTOCOL, "gzip" );
    m_cmd = CMD_NONE;
    return;
  }
  
  string zip_cmd;
  const char * path = lst.getLast()->path();
  if ( strcmp( path, "/compress" ) == 0 )
    zip_cmd = "/usr/bin/gzip";
  else if ( strcmp( path, "/decompress" ) == 0 )
    zip_cmd = "/usr/bin/gunzip";
  else if ( strcmp( path, "/bzip/compress" ) == 0 )
    zip_cmd = "/usr/local/bin/bzip";
  else if ( strcmp( path, "/bzip/decompress" ) == 0 )
    zip_cmd = "/usr/local/bin/bunzip";
  else
  {
    error( ERR_UNSUPPORTED_ACTION, path );
    m_cmd = CMD_NONE;
    return;
  }
  
  if ( lst.count() < 2 )
  {
    string e = "gzip:";
    e += path;
    error( ERR_NO_SOURCE_PROTOCOL, e.c_str() );
    m_cmd = CMD_NONE;
    return;
  }
   
  lst.removeLast();
  
  string exec = ProtocolManager::self()->find( lst.getLast()->protocol() );

  if ( exec.empty() )
  {
    error( ERR_UNSUPPORTED_PROTOCOL, lst.getLast()->protocol() );
    m_cmd = CMD_NONE;
    return;
  }

  // Start the file protcol
  Slave slave( exec.c_str() );
  if ( slave.pid() == -1 )
  {
    error( ERR_CANNOT_LAUNCH_PROCESS, exec.c_str() );
    return;
  }

  // Start GZIP
  GZipFilter filter( zip_cmd.c_str(), this );
  m_pFilter = &filter;
  if ( filter.pid() == -1 )
  {
    error( ERR_CANNOT_LAUNCH_PROCESS, zip_cmd.c_str() );
    return;
  }

  GZipIOJob job( &slave, this );

  QString src;
  KURL * it = lst.first();
  for( ; it ; it = lst.next() )
  {
    src += it->url();
  }
  
  debug( "kio_gzip : Nested fetching %s", src.data() );
  
  job.get( src );
  while( !job.isReady() && !job.hasFinished() )
    job.dispatch();

  if( job.hasFinished() )
  {
    finished();
    return;
  }

  ready();

  while( !job.hasFinished() )
    job.dispatch();
  
  finished();

  m_cmd = CMD_NONE;
}


void GZipProtocol::slotPut( const char *_url, int, bool _overwrite, bool _resume, unsigned int )
{
  assert( m_cmd == CMD_NONE );
  
  string url = _url;
  
  m_cmd = CMD_PUT;

  // Split up the URL
  KURLList lst;
  if ( !KURL::split( _url, lst )  )
  {
    error( ERR_MALFORMED_URL, url.c_str() );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  // Since this is a filter there must be at least an additional source protocol
  if ( lst.count() < 2 )
  {
    error( ERR_NO_SOURCE_PROTOCOL, "gzip" );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  // Has the right most URL really the gzip protocol ?
  if ( strcmp( lst.getLast()->protocol(), "gzip" ) != 0L )
  {
    error( ERR_INTERNAL, "kio_gzip got a URL which has not the gzip protocol" );
    finished();
    m_cmd = CMD_NONE;
    return;
  }
    
  // Find out what to do ( compres, decompress )
  string zip_cmd;
  const char * path = lst.getLast()->path();
  if ( strcmp( path, "/compress" ) == 0 )
    zip_cmd = "/usr/bin/gzip";
  else if ( strcmp( path, "/decompress" ) == 0 )
    zip_cmd = "/usr/bin/gunzip";
  else if ( strcmp( path, "/bzip/compress" ) == 0 )
    zip_cmd = "/usr/local/bin/bzip";
  else if ( strcmp( path, "/bzip/decompress" ) == 0 )
    zip_cmd = "/usr/local/bin/bunzip";
  else
  {
    string e = "gzip:";
    e += path;
    error( ERR_UNSUPPORTED_ACTION, e.c_str() );
    finished();
    m_cmd = CMD_NONE;
    return;
  }
  
  // Strip the gzip url
  lst.removeLast();
  
  // Find someone who can handle the new right most protocol
  string exec = ProtocolManager::self()->find( lst.getLast()->protocol() );

  // Did we find someone to handle this protocol ?
  if ( exec.empty() )
  {
    error( ERR_UNSUPPORTED_PROTOCOL, lst.getLast()->protocol() );
    finished();
    m_cmd = CMD_NONE;
    return;
  }

  // Start the protocol
  Slave slave( exec.c_str() );
  if ( slave.pid() == -1 )
  {
    error( ERR_CANNOT_LAUNCH_PROCESS, exec.c_str() );
    finished();
    return;
  }

  // Start the ZIP process
  GZipFilter filter( zip_cmd.c_str(), this );
  m_pFilter = &filter;
  if ( filter.pid() == -1 )
  {
    error( ERR_CANNOT_LAUNCH_PROCESS, zip_cmd.c_str() );
    finished();
    return;
  }

  // Put a protocol on top of the connection
  GZipIOJob job( &slave, this );
  m_pJob = &job;
  
  // Create what is left from the URL after stripping the
  // gzip part.
  QString src;
  KURL * it = lst.first();
  for( ; it ; it = lst.next() )
  {
    src += it->url();
  }

  // Tell the other job that we want to write something
  job.put( src, -1, _overwrite, false, 0 );
  while( !job.isReady() && !job.hasFinished() )
    job.dispatch();

  if ( job.hasError() )
  {
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

GZipIOJob::GZipIOJob( Connection *_conn, GZipProtocol *_gzip ) : IOJob( _conn )
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



GZipFilter::GZipFilter( const char *_prg, GZipProtocol *_gzip ) : Filter( _prg )
{
  m_pGZip = _gzip;
}  

void GZipFilter::emitData( void *_p, int _len )
{
  m_pGZip->filterData( _p, _len );
}

