#ifndef __info_h__
#define __info_h__

#include <kio/slavebase.h>

class KProcess;

class InfoProtocol : public KIO::SlaveBase
{
 public:

    InfoProtocol( KIO::Connection *connection = 0 );
    virtual ~InfoProtocol();

    virtual void get( const QString& path, const QString& query, bool reload );
    virtual void mimetype( const QString& path );

 private:

    KProcess *m_pProc;
};

#endif // __info_h__
