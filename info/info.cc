#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>

#include <kdebug.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
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
	kdError( 7108 ) << "Critical error: Cannot locate 'kde-info2html' for HTML-conversion" << endl;

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
    cmd += KProcess::quote(locate("data", "kio_info/kde-info2html.conf"));
    cmd += " ";
    cmd += KProcess::quote(path);
    cmd += " ";
    cmd += KProcess::quote(m_page);
    cmd += " ";
    cmd += KProcess::quote(m_node);

    kdDebug( 7108 ) << "cmd: " << cmd << endl;

    FILE *fd = popen( QFile::encodeName(cmd), "r" );

    char buffer[ 4096 ];
    QByteArray array;

    while ( !feof( fd ) )
    {
      int n = fread( buffer, 1, sizeof( buffer ), fd );
      if ( n < 0 )
      {
        // ERROR
	kdDebug( 7108 ) << "InfoProtocol::get ERROR!" << endl;
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

void InfoProtocol::decodeURL( const KURL &url )
{
    kdDebug( 7108 ) << "InfoProtocol::decodeURL" << endl;

    QString dirstr = url.path(); // there HAS to be a url.path() at least
    //kdDebug( 7108 ) << "dirstring: " << dirstr << endl;

    int slashPos = dirstr.find( "/", 1 );
    int oldPos = 0;
    QDir dir(dirstr.left(slashPos));
    //kdDebug( 7108 ) << "dirpath: " << dir.path() << endl;
    while (dir.exists()) {
      oldPos = slashPos;
      slashPos = dirstr.find( "/", oldPos+1 );
      if (-1 == slashPos) {
	// no more '/' found,
	// the whole string is a valid path ?
	break;
      }
      dir.setPath(dirstr.left(slashPos));
      //kdDebug( 7108 ) << "dirpath-loop: " << dir.path() << endl;
    }

    // oldPos now has the last dir '/'
    //kdDebug( 7108 ) << "dirstr_ length = " <<dirstr.length() << ", pos = " << oldPos << endl;
    //kdDebug( 7108 ) << "info_ request: " << dirstr.right(dirstr.length() - oldPos) << endl;
    decodePath(dirstr.right(1+dirstr.length() - oldPos));

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

/*void InfoProtocol::listDir( const KURL &url )
{
    kdDebug( 7108 ) << "InfoProtocol::listDir" << endl;

    if ( !url.directory(true,true).isEmpty()
         && url.directory(true,true) != QString("/") )
    {
        error( KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path() );
        return;
    }

    // Match info nodes in the 'dir' file
    // "* infopage:" at the start of a line
    QRegExp regex( "^\\*  *[^: ][^:]*:", false );

    QFile f( "/usr/info/dir" );

    if ( f.open(IO_ReadOnly) ) {
        QTextStream t( &f );
        QString s;

        int start, len;

        UDSEntryList uds_entry_list;
        UDSEntry     uds_entry;
        UDSAtom      uds_atom;

        uds_atom.m_uds = KIO::UDS_NAME; // we only do names...
        uds_entry.append( uds_atom );

        while ( !t.eof() ) {
            s = t.readLine();

            start = regex.match( s, 0, &len );

            if ( start != -1 ) {
            // Found "* infonode:", add "infonode" to matches

                int pos = 1;
                while ( pos < len && s[pos] == ' ')
                    pos++;

                QString name = s.mid( pos, (len-pos-1) ).lower();

                if ( !name.isEmpty() ) {
                    uds_entry[0].m_str = name;
                    uds_entry_list.append( uds_entry );
                }
            }
        }
        f.close();

        listEntries( uds_entry_list );
        finished();
    }
    else {
        kdError(7108) << "cannot open file '/usr/info/dir'" << endl;
    }
    kdDebug( 7108 ) << "InfoProtocol::listDir - done" << endl;
}*/

extern "C" { int kdemain( int argc, char **argv ); }

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
