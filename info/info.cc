#include "info.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegExp>

#include <kdebug.h>
#include <k3process.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <klocale.h>

using namespace KIO;

InfoProtocol::InfoProtocol( const QByteArray &pool, const QByteArray &app )
    : SlaveBase( "info", pool, app )
    , m_page( "" )
    , m_node( "" )
{
    kDebug( 7108 ) << "InfoProtocol::InfoProtocol" << endl;
    m_iconLoader = new KIconLoader(KGlobal::mainComponent().componentName(), KGlobal::mainComponent().dirs());
    m_perl = KGlobal::dirs()->findExe( "perl" );
    m_infoScript = KStandardDirs::locate( "data", "kio_info/kde-info2html" );
    m_infoConf = KStandardDirs::locate("data", "kio_info/kde-info2html.conf");

    if( m_perl.isNull() || m_infoScript.isNull() || m_infoConf.isNull() ) {
	kError( 7108 ) << "Critical error: Cannot locate files for HTML-conversion" << endl;
	QString errorStr;
	if ( m_perl.isNull() ) {
		errorStr = "perl.";
	} else {
		QString missing =m_infoScript.isNull() ?  "kio_info/kde-info2html" : "kio_info/kde-info2html.conf";
		errorStr = "kde-info2html" + i18n( "\nUnable to locate file %1 which is necessary to run this service. "
				"Please check your software installation" ,  missing );
	}
	error( KIO::ERR_CANNOT_LAUNCH_PROCESS, errorStr );
	exit();
    }

    kDebug( 7108 ) << "InfoProtocol::InfoProtocol - done" << endl;
}

InfoProtocol::~InfoProtocol()
{
    kDebug( 7108 ) << "InfoProtocol::~InfoProtocol" << endl;
    delete m_iconLoader;
    kDebug( 7108 ) << "InfoProtocol::~InfoProtocol - done" << endl;
}

void InfoProtocol::get( const KUrl& url )
{
    kDebug( 7108 ) << "InfoProtocol::get" << endl;
    kDebug( 7108 ) << "URL: " << url.prettyUrl() << " , Path :" << url.path() << endl;

    if (url.path()=="/")
    {
       KUrl newUrl("info:/dir");
       redirection(newUrl);
       finished();
       return;
    };

    // some people write info://autoconf instead of info:/autoconf
    if (!url.host().isEmpty()) {
        KUrl newURl(url);
        newURl.setPath(url.host()+url.path());
        newURl.setHost(QString());
        redirection(newURl);
        finished();
        return;
    }

    if ( url.path().right(1) == "/" )
    {
        // Trailing / are not supported, so we need to remove them.
        KUrl newUrl( url );
        QString newPath( url.path() );
        newPath.chop( 1 );
        newUrl.setPath( newPath );
        redirection( newUrl );
        finished();
        return;
    }

    mimeType("text/html");
    // extract the path and node from url
    decodeURL( url );

    QString path = m_iconLoader->iconPath("go-up", K3Icon::Toolbar, true);
    int revindex = path.lastIndexOf('/');
    path = path.left(revindex);

    QString cmd = K3Process::quote(m_perl);
    cmd += ' ';
    cmd += K3Process::quote(m_infoScript);
    cmd += ' ';
    cmd += K3Process::quote(m_infoConf);
    cmd += ' ';
    cmd += K3Process::quote(path);
    cmd += ' ';
    cmd += K3Process::quote(m_page);
    cmd += ' ';
    cmd += K3Process::quote(m_node);

    kDebug( 7108 ) << "cmd: " << cmd << endl;

    FILE *file = popen( QFile::encodeName(cmd), "r" );
    if ( !file ) {
        kDebug( 7108 ) << "InfoProtocol::get popen failed" << endl;
        error( ERR_CANNOT_LAUNCH_PROCESS, cmd );
        return;
    }

    char buffer[ 4096 ];

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
	kDebug( 7108 ) << "InfoProtocol::get ERROR!" << endl;
        pclose( file );
	return;
      }

      empty = false;
      data( QByteArray::fromRawData( buffer, n ) );
    }

    pclose( file );

    finished();

    kDebug( 7108 ) << "InfoProtocol::get - done" << endl;
}

void InfoProtocol::mimetype( const KUrl& /* url */ )
{
    kDebug( 7108 ) << "InfoProtocol::mimetype" << endl;

    // to get rid of those "Open with" dialogs...
    mimeType( "text/html" );

    // finish action
    finished();

    kDebug( 7108 ) << "InfoProtocol::mimetype - done" << endl;
}

void InfoProtocol::decodeURL( const KUrl &url )
{
    kDebug( 7108 ) << "InfoProtocol::decodeURL" << endl;

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

    if ( url == KUrl( "info:/browse_by_file?special=yes" ) ) {
	    m_page = "#special#";
	    m_node = "browse_by_file";
	    kDebug( 7108 ) << "InfoProtocol::decodeURL - special - browse by file" << endl;
	    return;
    }

    decodePath( url.path() );

    kDebug( 7108 ) << "InfoProtocol::decodeURL - done" << endl;
}

void InfoProtocol::decodePath( QString path )
{
    kDebug( 7108 ) << "InfoProtocol::decodePath(-" <<path<<"-)"<< endl;

    m_page = "dir";  //default
    m_node = "";

    // remove leading slash
    if ('/' == path[0]) {
      path = path.mid( 1 );
    }
    //kDebug( 7108 ) << "Path: " << path << endl;

    int slashPos = path.indexOf( "/" );

    if( slashPos < 0 )
    {
	m_page = path;
	m_node = "Top";
	return;
    }

    m_page = path.left( slashPos );

    // remove leading+trailing whitespace
    m_node = path.right( path.length() - slashPos - 1).trimmed ();

    kDebug( 7108 ) << "InfoProtocol::decodePath - done" << endl;
}

// A minimalistic stat with only the file type
// This seems to be enough for konqueror
void InfoProtocol::stat( const KUrl & )
{
	UDSEntry uds_entry;

	// Regular file with rwx permission for all
	uds_entry.insert( KIO::UDS_FILE_TYPE, S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO );

	statEntry( uds_entry );

	finished();
}

extern "C" { int KDE_EXPORT kdemain( int argc, char **argv ); }

int kdemain( int argc, char **argv )
{
  KComponentData componentData( "kio_info" );

  kDebug() << "kio_info starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_info protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  InfoProtocol slave( argv[2], argv[3] );
  slave.dispatchLoop();

  return 0;
}
