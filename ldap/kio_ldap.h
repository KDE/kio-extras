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

class LDAPProtocol : public KIOProtocol
{
public:
  LDAPProtocol( KIOConnection *_conn );
  
  virtual void slotGet( const char *_url );

  virtual void slotTestDir( const char *_url );
  virtual void slotListDir( const char *_url );

  KIOConnection* connection() { return KIOConnectionSignals::m_pConnection; }

  void jobError( int _errid, const char *_txt );

private:
  
  bool m_bIgnoreJobErrors;

};


class LDAPIOJob : public KIOJobBase
{
public:
  LDAPIOJob( KIOConnection *_conn, LDAPProtocol *_File );
  
  virtual void slotError( int _errid, const char *_txt );

protected:
  LDAPProtocol* m_pFile;
};

#endif
