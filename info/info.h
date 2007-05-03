#ifndef __info_h__
#define __info_h__

#include <QObject>

#include <kio/slavebase.h>

class K3Process;
class KIconLoader;

class InfoProtocol : public KIO::SlaveBase
{
public:

    InfoProtocol( const QByteArray &pool, const QByteArray &app );
    virtual ~InfoProtocol();

    virtual void get( const KUrl& url );
    virtual void stat( const KUrl& url );
    virtual void mimetype( const KUrl& url );

protected:

    void decodeURL( const KUrl &url );
    void decodePath( QString path );

private:

    QString   m_page;
    QString   m_node;

    QString   m_perl;
    QString   m_infoScript;
    QString   m_infoConf;
    KIconLoader* m_iconLoader;
};

#endif // __info_h__
