#include <kdebug.h>
#include <kprocess.h>

#include "info.h"

using namespace KIO;

InfoProtocol::InfoProtocol( Connection *connection )
    : SlaveBase( "info", connection )
{
    kDebugInfo( 7106, "Michael's info-pages kioslave" );

    m_pProc = new KProcess();
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
    
    // send data ...
    data( array );
    
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
