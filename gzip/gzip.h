#ifndef __gzip_h__
#define __gzip_h__

#include <kio_interface.h>
#include <kio_base.h>
#include <kio_filter.h>

class GZipProtocol : public KIOProtocol
{
public:
  GZipProtocol( Connection *_conn );
  
  virtual void slotGet( const char *_url );
  virtual void slotPut( const char *_url, int _mode, bool _overwrite,
			bool _resume, unsigned int );
  virtual void slotCopy( const char *_source, const char *_dest );

  virtual void slotData( void *_p, int _len );
  virtual void slotDataEnd();
  
  void jobData( void *_p, int _len );
  void jobError( int _errid, const char *_text );
  void jobDataEnd();
  void filterData( void *_p, int _len );
  
  Connection* connection() { return ConnectionSignals::m_pConnection; }
  
protected:

  int m_cmd;
  KIOFilter* m_pFilter;
  IOJob* m_pJob;
};

class GZipIOJob : public KIOJobBase
{
public:
  GZipIOJob( Connection *_conn, GZipProtocol *_gzip );
  
  virtual void slotData( void *_p, int _len );
  virtual void slotDataEnd();
  virtual void slotError( int _errid, const char *_txt );

protected:
  GZipProtocol* m_pGZip;
};

class GZipFilter : public KIOFilter
{
public:
  GZipFilter( const char *_prg, GZipProtocol *_gzip );

  virtual void emitData( void *_p, int _len );

protected:
  GZipProtocol* m_pGZip;
};

#endif
