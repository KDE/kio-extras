#include <stdio.h>
#include <stdlib.h>

#include <qdir.h>
#include <qregexp.h>

#include <kdebug.h>
#include <kprocess.h>
#include <kstddirs.h>
#include <kinstance.h>
#include <klocale.h>
#include <kconfig.h>

#include "cgi.h"

using namespace KIO;

CgiProtocol::CgiProtocol( const QCString &pool, const QCString &app )
    : SlaveBase( "cgi", pool, app )
{
  kdDebug() << "CgiProtocol::CgiProtocol" << endl;

  KConfig cfg( "kcmcgirc" );
  cfg.setGroup( "General" );
  mCgiPaths = cfg.readListEntry( "Paths" );
}

CgiProtocol::~CgiProtocol()
{
  kdDebug() << "CgiProtocol::~CgiProtocol" << endl;
}

void CgiProtocol::get( const KURL& url )
{
  kdDebug() << "CgiProtocol::get()" << endl;
  kdDebug() << " URL: " << url.url() << endl;
  kdDebug() << " Path: " << url.path() << endl;
  kdDebug() << " Query: " << url.query() << endl;
  kdDebug() << " Protocol: " << url.protocol() << endl;
  kdDebug() << " Filename: " << url.filename() << endl;

  QCString protocol = "SERVER_PROTOCOL=HTTP";
  putenv( protocol.data() );

  QCString query = url.query().mid( 1 ).local8Bit();
  query.prepend( "QUERY_STRING=" );
  putenv( query.data() );

  QString path = url.path();

  QString file;

  int pos = path.findRev('/');
  if ( pos >= 0 ) file = path.mid( pos + 1 );
  else file = path;

  QCString cmd;

  bool stripHeader = false;
  bool forwardFile = true;

  QStringList::ConstIterator it;
  for( it = mCgiPaths.begin(); it != mCgiPaths.end(); ++it ) {
    cmd = (*it).local8Bit() + "/" + file.local8Bit();
    if ( KStandardDirs::exists( cmd ) ) {
      forwardFile = false;
      stripHeader = true;
      break;
    }
  }
  
  FILE *fd;

  if ( forwardFile ) {
    kdDebug() << "Forwarding to '" << path << "'" << endl;

    QCString filepath = path.local8Bit();
  
    fd = fopen( filepath.data(), "r" );

    if ( !fd ) {
      kdDebug() << "Error opening '" << filepath << "'" << endl;
      error( KIO::ERR_CANNOT_OPEN_FOR_READING, filepath );
      return;
    }
  } else {
    kdDebug() << "Cmd: " << cmd << endl;

    fd = popen( cmd.data(), "r" );

    if ( !fd ) {
      kdDebug() << "Error running '" << cmd << "'" << endl;
      error( KIO::ERR_CANNOT_OPEN_FOR_READING, cmd );
      return;
    }
  }
  
  char buffer[ 4090 ];
  QByteArray array;

  QCString output;

  while ( !feof( fd ) )
  {
    int n = fread( buffer, 1, 2048, fd );

    if ( n == -1 )
    {
      // ERROR
      if ( forwardFile ) {
        fclose( fd );
      } else {
        pclose( fd );
      }
      return;
    }

    buffer[n] = 0;
    
    output.append( buffer );

    if ( stripHeader ) {
      int colon = output.find( ':' );
      int newline = output.find( '\n' );
      int semicolon = output.findRev( ';', newline );
      int end;
      if ( semicolon < 0 ) end = newline;
      else end = semicolon;

      kdDebug() << "  colon: " << colon << endl;
      kdDebug() << "  newline: " << newline << endl;
      kdDebug() << "  semicolon: " << semicolon << endl;
      kdDebug() << "  end: " << end << endl;
      
      QCString contentType = output.mid( colon + 1, end - colon - 1 );

      contentType = contentType.stripWhiteSpace();

      kdDebug() << "ContentType: '" << contentType << "'" << endl;

      mimeType( contentType );
      
      int start = output.find( "\r\n\r\n" );
      if ( start >= 0 ) start += 4;
      else {
        start = output.find( "\n\n" );
        if ( start >= 0 ) start += 2;
      }
      
      if ( start >= 0 ) output = output.mid( start );
    
      stripHeader = false;
    }
  }

  if ( forwardFile ) {
    fclose( fd );
  } else {
    pclose( fd );
  }

  data( output );

  finished();

  kdDebug() << "CgiProtocol::get - done" << endl;
}

extern "C" { int kdemain( int argc, char **argv ); }

/*! The kdemain function generates an instance of the ioslave and starts its
 * dispatch loop. */

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_cgi" );

  kdDebug() << "kio_cgi starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_cgi protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  CgiProtocol slave( argv[2], argv[3] );
  slave.dispatchLoop();

  return 0;
}
