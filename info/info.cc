#include <stdio.h>
#include <stdlib.h>

#include <kdebug.h>
#include <kprocess.h>
#include <kstddirs.h>
#include <kinstance.h>

#include "info.h"

using namespace KIO;

InfoProtocol::InfoProtocol( const QCString &pool, const QCString &app )
    : SlaveBase( "info", pool, app )
    , m_page( "" )
    , m_node( "" )
{
    kdDebug( 7108 ) << "InfoProtocol::InfoProtocol" << endl;

    m_infoScript = locate( "data", "kio_info/kde-info2html" );

    m_perl = KGlobal::dirs()->findExe( "perl" );
    
    if( m_infoScript.isEmpty() )
	kDebugFatal( 7108, "Critical error: Cannot locate 'kde-info2html' for HTML-conversion" );
    
    kdDebug( 7108 ) << "InfoProtocol::InfoProtocol - done" << endl;
}

InfoProtocol::~InfoProtocol()
{
    kdDebug( 7108 ) << "InfoProtocol::~InfoProtocol" << endl;

    kdDebug( 7108 ) << "InfoProtocol::~InfoProtocol - done" << endl;
}

void InfoProtocol::get( const KURL& url, bool /*reload*/ )
{
    kdDebug( 7108 ) << "InfoProtocol::get" << endl;
    kdDebug( 7108 ) << url.path() << endl;

    decodePath( url.path() );
    /*
    if( m_page.isEmpty() )
    {
	//error( 1, "Syntax error in URL" );
	
	QByteArray array = errorMessage();
	
	data( array );
	finished();

	return;
    }
    */
    if ( m_page.isEmpty() )
      m_page = "dir";

    QString cmds("%1 %2 %3 %4 \"%5\" \"%6\"");
    QCString cmd = cmds.arg(m_perl).arg(m_infoScript).arg(locate("data", "kio_info/kde-info2html.conf")).arg(KGlobal::dirs()->findResourceDir("icon", "hicolor/22x22/actions/up.png")).arg(m_page).arg(m_node).latin1();
    
    FILE *fd = popen( cmd.data(), "r" );
    
    char buffer[ 4090 ];
    QByteArray array;
    
    while ( !feof( fd ) )
    {
      int n = fread( buffer, 1, 2048, fd );
      if ( n == -1 )
      {
        // ERROR
        pclose( fd );
	return;
      }
      array.setRawData( buffer, n );
      data( array );
      array.resetRawData( buffer, n );
    }
    
    pclose( fd );

    finished();
    
    kdDebug( 7108 ) << "InfoProtocol::get - done" << endl;
}

void InfoProtocol::mimetype( const KURL& /* url */ )
{
    kdDebug( 7108 ) << "InfoProtocol::mimetype" << endl;

    // to get rid of those "Open with" dialogs...
    mimeType( "text/html" );

    // finish action
    finished();

    kdDebug( 7108 ) << "InfoProtocol::mimetype - done" << endl;
}

void InfoProtocol::decodePath( QString path )
{
    kdDebug( 7108 ) << "InfoProtocol::decodePath" << endl;

    m_page = "";
    m_node = "";

    // remove leading slash
    path = path.right( path.length() - 1 );

    int slashPos = path.find( "/" );

    if( slashPos < 0 )
    {
	m_page = path;
	m_node = "Top";
	return;
    }

    m_page = path.left( slashPos );
    m_node = path.right( path.length() - slashPos - 1);

    kdDebug( 7108 ) << "InfoProtocol::decodePath - done" << endl;
}

QCString InfoProtocol::errorMessage()
{
    kdDebug( 7108 ) << "InfoProtocol::errorMessage" << endl;

    // i18n !!!!!!!!!!!!!!!!!!
    return QCString( "<html><body bgcolor=\"#FFFFFF\">An error occured during converting an info-page to HTML</body></html>" );
}

extern "C" { int kdemain( int argc, char **argv ); }

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_info" );

  kdDebug() << "kio_info starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_file protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  InfoProtocol slave( argv[2], argv[3] );
  slave.dispatchLoop();

  return 0;
}
