#ifndef __info_h__
#define __info_h__

#include <kio/slavebase.h>

class KProcess;

class InfoProtocol : public KIO::SlaveBase
                   , public QObject
{
public:

    InfoProtocol( KIO::Connection *connection = 0 );
    virtual ~InfoProtocol();

    virtual void get( const QString& path, const QString& query, bool reload );
    virtual void mimetype( const QString& path );

protected:

    void decodePath( QString path );
    QCString errorMessage();

protected slots:

    void slotProcessExited( KProcess* );
    void slotReceivedStdout( KProcess*, char*, int );

private:

    QString   m_page;
    QString   m_node;

    KProcess *m_pProc;
    QString   m_infoScript;
};

#endif // __info_h__
