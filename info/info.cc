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

    QByteArray array;
    
    decodePath( path );
    
    if( m_page.isEmpty() )
    {
	//error( 1, "Syntax error in URL" );
	
	array = QCString( "<html><body bgcolor=\"#FFFFFF\">An error occured</body></html>" );
	data( array );
	finished();

	return;
    }
    
/*
    m_pProc << "perl" << m_infoScript << "dir" << "Top" ;
    
    m_pProc->start();
    m_pProc->kill();
*/
    
    // test data
    array = QCString( "<html><body bgcolor=\"#FFFFFF\">Test by Michael<p>" + path + "<p>" + m_page + " " + m_node + "</body></html>" );
    data( array );
    
    // finish action
    finished();
}

void InfoProtocol::mimetype( const QString& /*path*/ )
{
    // to get rid of those "Open with" dialogs...
    mimeType( "text/html" );

    // finish action
    finished();
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

extern "C"
{
    SlaveBase *init_info()
    {
	return new InfoProtocol();
    }
}
