/*
   Copyright (C) 2002 Cornelius Schumacher <schumacher@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdio.h>
#include <stdlib.h>

#include <QDir>
#include <QRegExp>

#include <kdebug.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kinstance.h>
#include <klocale.h>
#include <kconfig.h>

#include "cgi.h"

using namespace KIO;

CgiProtocol::CgiProtocol( const QByteArray &pool, const QByteArray &app )
    : SlaveBase( "cgi", pool, app )
{
  kDebug(7124) << "CgiProtocol::CgiProtocol" << endl;

  KConfig cfg( "kcmcgirc" );
  cfg.setGroup( "General" );
  mCgiPaths = cfg.readEntry( "Paths" , QStringList() );
}

CgiProtocol::~CgiProtocol()
{
  kDebug(7124) << "CgiProtocol::~CgiProtocol" << endl;
}

void CgiProtocol::get( const KUrl& url )
{
  kDebug(7124) << "CgiProtocol::get()" << endl;
  kDebug(7124) << " URL: " << url.url() << endl;
#if 0
  kDebug(7124) << " Path: " << url.path() << endl;
  kDebug(7124) << " Query: " << url.query() << endl;
  kDebug(7124) << " Protocol: " << url.protocol() << endl;
  kDebug(7124) << " Filename: " << url.filename() << endl;
#endif
  QByteArray protocol = "SERVER_PROTOCOL=HTTP";
  putenv( protocol.data() );

  QByteArray requestMethod = "REQUEST_METHOD=GET";
  putenv( requestMethod.data() );

  QByteArray query = url.query().mid( 1 ).toLocal8Bit();
  query.prepend( "QUERY_STRING=" );
  putenv( query.data() );

  QString path = url.path();

  QString file;

  int pos = path.lastIndexOf('/');
  if ( pos >= 0 ) file = path.mid( pos + 1 );
  else file = path;

  QString cmd;

  bool stripHeader = false;
  bool forwardFile = true;

  QStringList::ConstIterator it;
  for( it = mCgiPaths.begin(); it != mCgiPaths.end(); ++it ) {
    cmd = *it;
    if ( !(*it).endsWith("/") )
        cmd += '/';
    cmd += file;
    if ( KStandardDirs::exists( cmd ) ) {
      forwardFile = false;
      stripHeader = true;
      break;
    }
  }

  FILE *fd;

  if ( forwardFile ) {
    kDebug(7124) << "Forwarding to '" << path << "'" << endl;

    QByteArray filepath = QFile::encodeName( path );

    fd = fopen( filepath.data(), "r" );

    if ( !fd ) {
      kDebug(7124) << "Error opening '" << filepath << "'" << endl;
      error( KIO::ERR_CANNOT_OPEN_FOR_READING, filepath );
      return;
    }
  } else {
    kDebug(7124) << "Cmd: " << cmd << endl;

    fd = popen( QFile::encodeName(KProcess::quote( cmd )).data(), "r" );

    if ( !fd ) {
      kDebug(7124) << "Error running '" << cmd << "'" << endl;
      error( KIO::ERR_CANNOT_OPEN_FOR_READING, cmd );
      return;
    }
  }

  char buffer[ 4090 ];

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

    if ( stripHeader ) {
      QByteArray output = buffer; // this assumes buffer is text and not binary
      int colon = output.indexOf( ':' );
      int newline = output.indexOf( '\n' );
      int semicolon = output.lastIndexOf( ';', newline );
      int end;
      if ( semicolon < 0 ) end = newline;
      else end = semicolon;

#if 0
      kDebug(7124) << "  colon: " << colon << endl;
      kDebug(7124) << "  newline: " << newline << endl;
      kDebug(7124) << "  semicolon: " << semicolon << endl;
      kDebug(7124) << "  end: " << end << endl;
#endif

      QByteArray contentType = output.mid( colon + 1, end - colon - 1 );

      contentType = contentType.trimmed();

      kDebug(7124) << "ContentType: '" << contentType << "'" << endl;

      mimeType( contentType );

      int start = output.indexOf( "\r\n\r\n" );
      if ( start >= 0 ) start += 4;
      else {
        start = output.indexOf( "\n\n" );
        if ( start >= 0 ) start += 2;
      }

      if ( start >= 0 ) output = output.mid( start );

      stripHeader = false;
      data( output );
    } else {
      QByteArray array;
      array.setRawData( buffer, n );
      data( array );
      array.resetRawData( buffer, n );
    }
  }

  if ( forwardFile ) {
    fclose( fd );
  } else {
    pclose( fd );
  }

  finished();

  kDebug(7124) << "CgiProtocol::get - done" << endl;
}

extern "C" { int KDE_EXPORT kdemain( int argc, char **argv ); }

/*! The kdemain function generates an instance of the ioslave and starts its
 * dispatch loop. */

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_cgi" );

  kDebug(7124) << "kio_cgi starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_cgi protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  CgiProtocol slave( argv[2], argv[3] );
  slave.dispatchLoop();

  return 0;
}
