#ifndef __LDAP_H__
#define __LDAP_H__ "$Id$"

//#include <sys/types.h>
//#include <sys/stat.h>

//#include <stdio.h>
//#include <unistd.h>

#include <qstring.h>
#include <qvaluelist.h>

#include <kio/slavebase.h>

class LDAPProtocol : public KIO::SlaveBase
{
public:
  LDAPProtocol( const QCString &pool, const QCString &app );
//  virtual ~LDAPProtocol();

  virtual void setHost(const QString& host, int port,
		       const QString& user, const QString& pass);

  virtual void get( const KURL& url );
  virtual void stat( const KURL& url );
//  virtual void mimetype( const KURL& url );
  virtual void listDir( const KURL& url );

private:
  QString urlPrefix;
};


#endif
