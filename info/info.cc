#include <kdebug.h>
#include <kprocess.h>
#include <kstddirs.h>

#include "info.h"

using namespace KIO;

InfoProtocol::InfoProtocol( Connection *connection )
    : SlaveBase( "info", connection )
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

/*
    m_pProc << "perl" << m_infoScript << "dir" << "Top" ;
    
    m_pProc->start();
    m_pProc->kill();
*/
    
    // test data
    QString htmldata = "<html><body bgcolor=\"#FFFFFF\">Test by Michael</body></html>";
    array.assign( htmldata.ascii(), htmldata.length() );
    data( array );
    
    // ready();
    
    // finish action
    finished();
}

void InfoProtocol::mimetype( const QString& /*path*/ )
{
    // to get rid of those "Open with" dialogs...
    mimeType( "text/html" );

    // finisch action
    finished();
}

extern "C"
{
    SlaveBase *init_info()
    {
	return new InfoProtocol();
    }
}
