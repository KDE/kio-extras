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

class GopherProtocol : public IOProtocol
{
public:
  GopherProtocol (Connection *_conn);
  
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
  
  Connection* connection () { return ConnectionSignals::m_pConnection; }
  
 protected:

  /**
    *  Send a command to the server, and wait for the one-line-status
    *  reply via getResponse.  Similar rules apply.  If no buffer is
    *  specified, no data is passed back.
    */
  bool command (const char *buf, char *r_buf=0, unsigned int r_len=0);

  /**
    *  All Gopher commands will generate a response.  Each response will
    *  either be prefixed with a "+OK " or a "-ERR ".  The getResponse 
    *  fucntion will wait until there's data to be read, and then read in
    *  the first line (the response), and copy the response sans +OK/-ERR
    *  into a buffer (up to len bytes) if one was passed to it.  It will
    *  return true if the response was affirmitave, or false otherwise.
    */
  bool getResponse (char *buf=0, unsigned int len=0);

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
  uint m_iOldPort;
  struct timeval m_tTimeout;
  FILE *fp;
  IOJob* m_pJob;
  GopherType current_type;
};

class GopherIOJob : public IOJob
{
 public:
  GopherIOJob (Connection *_conn, GopherProtocol *_gopher);
  
  virtual void slotData (void *_p, int _len);
  virtual void slotDataEnd ();
  virtual void slotError (int _errid, const char *_txt);
  
 protected:
  GopherProtocol *m_pGopher;
};

#endif
