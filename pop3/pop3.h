#ifndef _POP3_H
#define _POP3_H "$Id$"

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include <qstring.h>

#include <kio_interface.h>
#include <kio_base.h>
#include <kio_filter.h>
#include <kurl.h>

class POP3Protocol : public IOProtocol
{
public:
  POP3Protocol( Connection *_conn );
  
  virtual void slotGet( const char *_url );
  virtual void slotPut( const char *_url, int _mode, bool _overwrite,
			bool _resume, unsigned int );
  virtual void slotCopy( const char *_source, const char *_dest );

  virtual void slotData( void *_p, int _len );
  virtual void slotDataEnd();
  
  void jobData( void *_p, int _len );
  void jobError( int _errid, const char *_text );
  void jobDataEnd();
  
  Connection* connection() { return ConnectionSignals::m_pConnection; }
  
 protected:
  bool command(const char *, char *, unsigned int);
  void pop3_close ();
  bool pop3_open( KURL &_url );

  int m_cmd, m_iSock;
  struct timeval m_tTimeout;
  FILE *fp;
  IOJob* m_pJob;
  QString m_sServerInfo;
};

class POP3IOJob : public IOJob
{
 public:
  POP3IOJob( Connection *_conn, POP3Protocol *_pop3 );
  
  virtual void slotData( void *_p, int _len );
  virtual void slotDataEnd();
  virtual void slotError( int _errid, const char *_txt );
  
 protected:
  POP3Protocol* m_pPOP3;
};

#endif
