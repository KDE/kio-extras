#include <kdebug.h>
#include <kprocess.h>
#include <kstddirs.h>

#include "man.h"
#include "manslave.h"

using namespace KIO;

ManProtocol::ManProtocol( Connection *connection )
    : SlaveBase( "info", connection )
    , m_page( "" )
    , m_node( "" )
{
    kDebugInfo( 7106, "Michael's info-pages kioslave" );
}

ManProtocol::~ManProtocol()
{
}

void ManProtocol::get( const QString& path, const QString& /*query*/, bool /*reload*/ )
{
    kDebugInfo( 7106, "Michael's info-pages kioslave" );
    kDebugInfo( 7106, path.data() );

    decodePath( path );

    QByteArray array;
    
    if( m_page.isEmpty() )
    {
	//error( 1, "Syntax error in URL" );
	
	array = errorMessage();
	
	data( array );
	finished();

	return;
    }
    
    ManParser parser();
    
    data( errorMessage() );
    
    finished();
}

void ManProtocol::mimetype( const QString& /*path*/ )
{
    // to get rid of those "Open with" dialogs...
    mimeType( "text/html" );

    // finish action
    finished();
}

void ManProtocol::decodePath( QString path )
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

QCString ManProtocol::errorMessage()
{
    return QCString( "<html><body bgcolor=\"#FFFFFF\">An error occured during converting a man-page to HTML</body></html>" );
}

extern "C"
{
    SlaveBase *init_info()
    {
	return new ManProtocol();
    }
}
