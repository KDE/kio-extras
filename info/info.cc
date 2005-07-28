#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>
//Added by qt3to4:
#include <Q3CString>

#include <kdebug.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <klocale.h>

#include "info.h"

using namespace KIO;

InfoProtocol::InfoProtocol( const Q3CString &pool, const Q3CString &app )
    : SlaveBase( "info", pool, app )
    , m_page( "" )
    , m_node( "" )
{
    kdDebug( 7108 ) << "InfoProtocol::InfoProtocol" << endl;

    m_perl = KGlobal::dirs()->findExe( "perl" );
    m_infoScript = locate( "data", "kio_info/kde-info2html" );
    m_infoConf = locate("data", "kio_info/kde-info2html.conf");

    if( m_perl.isNull() || m_infoScript.isNull() || m_infoConf.isNull() ) {
	kdError( 7108 ) << "Critical error: Cannot locate files for HTML-conversion" << endl;
	QString errorStr;
	if ( m_perl.isNull() ) {
		errorStr = "perl.";
	} else {
		QString missing =m_infoScript.isNull() ?  "kio_info/kde-info2html" : "kio_info/kde-info2html.conf";
		errorStr = "kde-info2html" + i18n( "\nUnable to locate file %1 which is necessary to run this service. "
				"Please check your software installation" ).arg( missing );
	}
	error( KIO::ERR_CANNOT_LAUNCH_PROCESS, errorStr );
	exit();
    }

    kdDebug( 7108 ) << "InfoProtocol::InfoProtocol - done" << endl;
}

InfoProtocol::~InfoProtocol()
{
    kdDebug( 7108 ) << "InfoProtocol::~InfoProtocol" << endl;

    kdDebug( 7108 ) << "InfoProtocol::~InfoProtocol - done" << endl;
}

void InfoProtocol::get( const KURL& url )
{
    kdDebug( 7108 ) << "InfoProtocol::get" << endl;
    kdDebug( 7108 ) << "URL: " << url.prettyURL() << " , Path :" << url.path() << endl;

    if (url.path()=="/")
    {
       KURL newUrl("info:/dir");
       redirection(newUrl);
       finished();
       return;
    };

    // some people write info://autoconf instead of info:/autoconf
    if (!url.host().isEmpty()) {
        KURL newURl(url);
        newURl.setPath(url.host()+url.path());
        newURl.setHost(QString::null);
        redirection(newURl);
        finished();
        return;
    }

    mimeType("text/html");
    // extract the path and node from url
    decodeURL( url );

    QString path = KGlobal::iconLoader()->iconPath("up", KIcon::Toolbar, true);
    int revindex = path.findRev('/');
    path = path.left(revindex);

    QString cmd = KProcess::quote(m_perl);
    cmd += " ";
    cmd += KProcess::quote(m_infoScript);
    cmd += " ";
    cmd += KProcess::quote(m_infoConf);
    cmd += " ";
    cmd += KProcess::quote(path);
    cmd += " ";
    cmd += KProcess::quote(m_page);
    cmd += " ";
    cmd += KProcess::quote(m_node);

    kdDebug( 7108 ) << "cmd: " << cmd << endl;

    FILE *file = popen( QFile::encodeName(cmd), "r" );
    if ( !file ) {
        kdDebug( 7108 ) << "InfoProtocol::get popen failed" << endl;
        error( ERR_CANNOT_LAUNCH_PROCESS, cmd );
        return;
    }

    char buffer[ 4096 ];
    QByteArray array;

    bool empty = true;
    while ( !feof( file ) )
    {
      int n = fread( buffer, 1, sizeof( buffer ), file );
      if ( !n && feof( file ) && empty ) {
	      error( ERR_CANNOT_LAUNCH_PROCESS, cmd );
	      return;
      }
      if ( n < 0 )
      {
        // ERROR
	kdDebug( 7108 ) << "InfoProtocol::get ERROR!" << endl;
        pclose( file );
	return;
      }

      empty = false;
      array.setRawData( buffer, n );
      data( array );
      array.resetRawData( buffer, n );
    }

    pclose( file );

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

void InfoProtocol::decodeURL( const KURL &url )
{
    kdDebug( 7108 ) << "InfoProtocol::decodeURL" << endl;

    /* Notes:
     * 
     * I cleaned up the URL decoding and chose not to support URLs in the
     * form "info:/usr/local/share/info/libc.info.gz" or similar which the
     * older code attempted (and failed, maybe it had worked once) to do.
     *
     * The reason is that an obvious use such as viewing a info file off your
     * infopath would work for the first page, but then all the links would be
     * wrong. Of course, one could change kde-info2html to make it work, but I don't
     * think it worthy, others are free to disagree and write the necessary code ;)
     *
     * luis pedro
     */

    if ( url == KURL( "info:/browse_by_file?special=yes" ) ) {
	    m_page = "#special#";
	    m_node = "browse_by_file";
	    kdDebug( 7108 ) << "InfoProtocol::decodeURL - special - browse by file" << endl;
	    return;
    }

    decodePath( url.path() );

    kdDebug( 7108 ) << "InfoProtocol::decodeURL - done" << endl;
}

void InfoProtocol::decodePath( QString path )
{
    kdDebug( 7108 ) << "InfoProtocol::decodePath(-" <<path<<"-)"<< endl;

    m_page = "dir";  //default
    m_node = "";

    // remove leading slash
    if ('/' == path[0]) {
      path = path.mid( 1 );
    }
    //kdDebug( 7108 ) << "Path: " << path << endl;

    int slashPos = path.find( "/" );

    if( slashPos < 0 )
    {
	m_page = path;
	m_node = "Top";
	return;
    }

    m_page = path.left( slashPos );

    // remove leading+trailing whitespace
    m_node = path.right( path.length() - slashPos - 1).stripWhiteSpace ();

    kdDebug( 7108 ) << "InfoProtocol::decodePath - done" << endl;
}

// A minimalistic stat with only the file type
// This seems to be enough for konqueror
void InfoProtocol::stat( const KURL & )
{
	UDSEntry uds_entry;
	UDSAtom  uds_atom;

	// Regular file with rwx permission for all
	uds_atom.m_uds = KIO::UDS_FILE_TYPE;
	uds_atom.m_long = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

	uds_entry.append( uds_atom );

	statEntry( uds_entry );

	finished();
}

extern "C" { int KDE_EXPORT kdemain( int argc, char **argv ); }

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_info" );

  kdDebug() << "kio_info starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_info protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  InfoProtocol slave( argv[2], argv[3] );
  slave.dispatchLoop();

  return 0;
}
