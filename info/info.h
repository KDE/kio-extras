#ifndef __info_h__
#define __info_h__

#include <qobject.h>

#include <kio/slavebase.h>

class KProcess;

class InfoProtocol : public KIO::SlaveBase
{
public:

    InfoProtocol( const QCString &pool, const QCString &app );
    virtual ~InfoProtocol();

    virtual void get( const QString& path, const QString& query, bool reload );
    virtual void mimetype( const QString& path, const QString& query );

protected:

    void decodePath( QString path );
    QCString errorMessage();

private:

    QString   m_page;
    QString   m_node;

    QString   m_infoScript;
    QString   m_perl;
};

#endif // __info_h__
