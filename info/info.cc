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
    kDebugInfo( 7106, "Michael's info-pages kioslave" );

    m_pProc = new KProcess();

    connect( m_pProc, SIGNAL( processExited( KProcess* ) ), 
	     this, SLOT( slotProcessExited( KProcess* ) ) );
    connect( m_pProc, SIGNAL( receivedStdout( KProcess*, char*, int ) ),
	     this, SLOT( slotReceivedStdout( KProcess*, char*, int ) ) );
    
    m_infoScript = locate( "data", "kinfobrowser/kde-info2html" );

    if( m_infoScript.isEmpty() )
	kDebugFatal( 7106, "Cannot locate 'kde-info2html' for HTML-conversion" );
}

InfoProtocol::~InfoProtocol()
{
    delete m_pProc;
}

void InfoProtocol::get( const QString& path, const QString& /*query*/, bool /*reload*/ )
{
    kDebugInfo( 7106, "Michael's info-pages kioslave" );
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

    m_pProc->kill();
    m_pProc->clearArguments();

    *m_pProc << "perl" << m_infoScript.latin1() << m_page.latin1() << m_node.latin1();

    m_pProc->start( KProcess::NotifyOnExit, KProcess::Stdout );
}

void InfoProtocol::mimetype( const QString& /*path*/ )
{
    // to get rid of those "Open with" dialogs...
    mimeType( "text/html" );

    // finish action
    finished();
}

void InfoProtocol::slotProcessExited( KProcess* )
{
    // I dont know if this is needed
    m_pProc->kill();
    
    // finish kioslave
    finished();
}

void InfoProtocol::slotReceivedStdout( KProcess*, char* htmldata, int len )
{
    QByteArray array;

    array = QCString( htmldata, len );
    
    data( array );
}

void InfoProtocol::decodePath( QString path )
{
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
}

QCString InfoProtocol::errorMessage()
{
    return QCString( "<html><body bgcolor=\"#FFFFFF\">An error occured during converting an info-page to HTML</body></html>" ); 
}

extern "C"
{
    SlaveBase *init_info()
    {
	return new InfoProtocol();
    }
}
