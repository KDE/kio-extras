#ifndef __info_h__
#define __info_h__

#include <kio/slavebase.h>

class ManProtocol : public KIO::SlaveBase
{
public:

    ManProtocol( KIO::Connection *connection = 0 );
    virtual ~ManProtocol();

    virtual void get( const QString& path, const QString& query, bool reload );
    virtual void mimetype( const QString& path );

protected:

    void decodePath( QString path );
    QCString errorMessage();

private:

    QString   m_page;
    QString   m_node;
};

#endif // __info_h__
