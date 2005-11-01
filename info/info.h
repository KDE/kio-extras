#ifndef __info_h__
#define __info_h__

#include <qobject.h>

#include <kio/slavebase.h>

class KProcess;

class InfoProtocol : public KIO::SlaveBase
{
public:

    InfoProtocol( const QByteArray &pool, const QByteArray &app );
    virtual ~InfoProtocol();

    virtual void get( const KURL& url );
    virtual void stat( const KURL& url );
    virtual void mimetype( const KURL& url );

protected:

    void decodeURL( const KURL &url );
    void decodePath( QString path );

private:

    QString   m_page;
    QString   m_node;

    QString   m_perl;
    QString   m_infoScript;
    QString   m_infoConf;
};

#endif // __info_h__
