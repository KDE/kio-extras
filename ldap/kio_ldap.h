#ifndef __LDAP_H__
#define __LDAP_H__ "$Id$"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <unistd.h>

#include <qstring.h>
#include <qvaluelist.h>

#include <kio_interface.h>
#include <kio_base.h>

class LDAPProtocol : public IOProtocol
{
public:
  LDAPProtocol( Connection *_conn );
  
  virtual void slotGet( const char *_url );

  virtual void slotTestDir( const char *_url );
  virtual void slotListDir( const char *_url );

  Connection* connection() { return ConnectionSignals::m_pConnection; }

  void jobError( int _errid, const char *_txt );

private:
  
  bool m_bIgnoreJobErrors;

};


class LDAPIOJob : public IOJob
{
public:
  LDAPIOJob( Connection *_conn, LDAPProtocol *_File );
  
  virtual void slotError( int _errid, const char *_txt );

protected:
  LDAPProtocol* m_pFile;
};

#endif
