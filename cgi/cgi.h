#ifndef KIO_CGI_H
#define KIO_CGI_H

#include <qobject.h>

#include <kio/slavebase.h>

class KProcess;

/*!
  This class implements an ioslave for viewing CGI script output without the
  need to run a web server.
*/
class CgiProtocol : public KIO::SlaveBase
{
  public:
    CgiProtocol( const QCString &pool, const QCString &app );
    virtual ~CgiProtocol();

    virtual void get( const KURL& url );

//    virtual void mimetype( const KURL& url );

  protected:
//    QCString errorMessage();

  private:
    QStringList mCgiPaths;
};

#endif
