#ifndef _TAR_H
#define _TAR_H "$Id$"

#include <kio_interface.h>
#include <kio_base.h>
#include <kio_filter.h>

class TARProtocol : public KIOProtocol
{
public:
  TARProtocol( KIOConnection *_conn );
  
  virtual void slotGet( const char *_url );
  virtual void slotPut( const char *_url, int _mode, bool _overwrite,
			bool _resume, unsigned int );
  virtual void slotCopy( const char *_source, const char *_dest );

  virtual void slotListDir( const char *_url );
  virtual void slotTestDir( const char *_url );

  virtual void slotData( void *_p, int _len );
  virtual void slotDataEnd();
  
  void jobData( void *_p, int _len );
  void jobError( int _errid, const char *_text );
  void jobDataEnd();
  void filterData( void *_p, int _len );
  
  KIOConnection* connection() { return KIOConnectionSignals::m_pConnection; }
  
 protected:
  
  int m_cmd;
  KIOFilter* m_pFilter;
  KIOJobBase* m_pJob;
};

class TARIOJob : public KIOJobBase
{
 public:
  TARIOJob( KIOConnection *_conn, TARProtocol *_tar );
  
  virtual void slotData( void *_p, int _len );
  virtual void slotDataEnd();
  virtual void slotError( int _errid, const char *_txt );
  
 protected:
  TARProtocol* m_pTAR;
};

class TARFilter : public KIOFilter
{
 public:
  TARFilter( TARProtocol *_tar, const char *_prg, const char **_argv);
  
  virtual void emitData( void *_p, int _len );
  
 protected:
  TARProtocol* m_pTAR;
};

#endif
