#ifndef __LDAP_H__
#define __LDAP_H__ "$Id$"

#include <qstring.h>
#include <qvaluelist.h>

#include <kio/slavebase.h>

class LDAPProtocol : public KIO::SlaveBase
{
  public:
    LDAPProtocol( const QCString &pool, const QCString &app );

    virtual void setHost( const QString& host, int port,
                          const QString& user, const QString& pass );

    virtual void get( const KURL& url );
    virtual void stat( const KURL& url );
    virtual void listDir( const KURL& url );

  private:
    QString mUrlPrefix;
    QString mUser;
    QString mPassword;
};

#endif
