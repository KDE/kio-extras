#include <kdebug.h>
#include <kprocess.h>
#include <kstddirs.h>

#include "info.h"

using namespace KIO;

InfoProtocol::InfoProtocol( Connection *connection )
    : SlaveBase( "info", connection )
    , m_page( "" )
    , m_node( "" )
{
    kDebugInfo( 7106, "InfoProtocol::InfoProtocol" );

    m_pProc = new KProcess();
    connect( m_pProc, SIGNAL( processExited( KProcess* ) ),
	     this ,SLOT( slotProcessExited( KProcess* ) ) );
    connect( m_pProc, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
	     this, SLOT( slotReceivedStdout( KProcess*, char*, int ) ) );

    m_infoScript = locate( "data", "kinfobrowser/kde-info2html" );

    if( m_infoScript.isEmpty() )
	kDebugFatal( 7106, "Critical error: Cannot locate 'kde-info2html' for HTML-conversion" );
    
    kDebugInfo( 7106, "InfoProtocol::InfoProtocol - done" );
}

InfoProtocol::~InfoProtocol()
{
    kDebugInfo( 7106, "InfoProtocol::~InfoProtocol" );

    delete m_pProc;
    
    kDebugInfo( 7106, "InfoProtocol::~InfoProtocol - done" );
}

void InfoProtocol::get( const QString& path, const QString& /*query*/, bool /*reload*/ )
{
    kDebugInfo( 7106, "InfoProtocol::get" );
    kDebugInfo( 7106, path.data() );

    decodePath( path );

    if( m_page.isEmpty() )
    {
	//error( 1, "Syntax error in URL" );
	
	QByteArray array = errorMessage();
	
	data( array );
	finished();

	return;
    }

    /*
    data( QByteArray( errorMessage() ) );
    finished();
    */

    if( m_pProc->isRunning() )
	m_pProc->kill();

    m_pProc->clearArguments();

    *m_pProc << "perl" << "/home/devel/src/kde-2.0/kdebase/khelpcenter/kinfobrowser/kde-info2html" << "dir" << "Top";
    //*m_pProc << "perl" << m_infoScript.latin1() << m_page.latin1() << m_node.latin1();

    m_pProc->start( KProcess::NotifyOnExit, KProcess::Stdout );

    kDebugInfo( 7106, "InfoProtocol::get - done" );
}

void InfoProtocol::mimetype( const QString& /*path*/ )
{
    kDebugInfo( 7106, "InfoProtocol::mimetype" );

    // to get rid of those "Open with" dialogs...
    mimeType( "text/html" );

    // finish action
    finished();
    
    kDebugInfo( 7106, "InfoProtocol::mimetype - done" );
}

void InfoProtocol::slotProcessExited( KProcess* )
{
    kDebugInfo( 7106, "InfoProtocol::slotProcessExited" );

    // I dont know if this is needed
    m_pProc->kill();

    // finish kioslave
    finished();
    
    kDebugInfo( 7106, "InfoProtocol::slotProcessExited - done" );
}

void InfoProtocol::slotReceivedStdout( KProcess*, char* htmldata, int len )
{
    kDebugInfo( 7106, "InfoProtocol::slotReceivedStdout" );

    QByteArray array;

    array = QCString( htmldata, len );
    data( array );
    
    kDebugInfo( 7106, "InfoProtocol::slotReceivedStdout - done" );
}

void InfoProtocol::decodePath( QString path )
{
    kDebugInfo( 7106, "InfoProtocol::decodePath" );

    m_page = "";
    m_node = "";

/*
    TODO: why is this wrong ?

    // test leading slash
    if( path.left( 1 ) == "/" )
	return; // error
*/

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
    m_node = path.right( path.length() - slashPos - 1 );

    kDebugInfo( 7106, "InfoProtocol::decodePath - done" );
}

QCString InfoProtocol::errorMessage()
{
    kDebugInfo( 7106, "InfoProtocol::errorMessage" );

    return QCString( "<html><body bgcolor=\"#FFFFFF\">An error occured during converting an info-page to HTML</body></html>" );
    
    kDebugInfo( 7106, "InfoProtocol::errorMessage - done" );
}

extern "C"
{
    SlaveBase *init_info()
    {
	return new InfoProtocol();
    }
}
