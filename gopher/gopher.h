#ifndef _GOPHER_H
#define _GOPHER_H "$Id$"

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include <qstring.h>

#include <kio_interface.h>
#include <kio_base.h>
#include <kio_filter.h>
#include <kurl.h>

enum GopherType
{
  GOPHER_TEXT       = '0',
  GOPHER_MENU       = '1',
  GOPHER_PHONEBOOK  = '2',
  GOPHER_ERROR      = '3',
  GOPHER_BINHEX     = '4',
  GOPHER_PCBINARY   = '5',
  GOPHER_UUENCODE   = '6',
  GOPHER_INDEX      = '7',
  GOPHER_TELNET     = '8',
  GOPHER_BINARY     = '9',
  GOPHER_DUPLICATE  = '+',
  GOPHER_GIF        = 'g',
  GOPHER_IMAGE      = 'I',
  GOPHER_TN3270     = 'T',
  GOPHER_NONE       = '*'
};

class GopherProtocol : public KIOProtocol
{
public:
  GopherProtocol (KIOConnection *_conn);
  
  virtual void slotGet (const char *_url);
  virtual void slotGetSize (const char *_url );
  virtual void slotCopy (const char *_source, const char *_dest);

  virtual void slotData (void *_p, int _len);
  virtual void slotDataEnd ();

  virtual void slotListDir( const char *_url );
  virtual void slotTestDir( const char *_url );
  
  void jobData (void *_p, int _len);
  void jobError (int _errid, const char *_text);
  void jobDataEnd ();
  
  KIOConnection* connection () { return KIOConnectionSignals::m_pConnection; }
  
 protected:

  bool readRawData(const char *_url, const char *mimetype=0);

  /**
    *  Attempt to properly shut down the Gopher connection.
    */
  void gopher_close ();

  /**
    * Attempt to initiate a Gopher connection via a TCP socket.  If no port
    * is passed, port 70 is assumed.
    */
  bool gopher_open (KURL &_url);

  int m_cmd, m_iSock;
  struct timeval m_tTimeout;
  FILE *fp;
  KIOJobBase* m_pJob;
  GopherType current_type;
  static const char *abouttext;
};

class GopherIOJob : public KIOJobBase
{
 public:
  GopherIOJob (KIOConnection *_conn, GopherProtocol *_gopher);
  
  virtual void slotData (void *_p, int _len);
  virtual void slotDataEnd ();
  virtual void slotError (int _errid, const char *_txt);
  
 protected:
  GopherProtocol *m_pGopher;
};

#endif
