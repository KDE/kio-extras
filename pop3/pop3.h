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
  POP3Protocol (Connection *_conn);
  
  virtual void slotGet (const char *_url);
  virtual void slotGetSize (const char *_url );
  virtual void slotCopy (const char *_source, const char *_dest);

  virtual void slotData (void *_p, int _len);
  virtual void slotDataEnd ();

  virtual void slotDel( QStringList& _source );

  virtual void slotListDir( const char *_url );
  virtual void slotTestDir( const char *_url );
  
  void jobData (void *_p, int _len);
  void jobError (int _errid, const char *_text);
  void jobDataEnd ();
  
  Connection* connection () { return ConnectionSignals::m_pConnection; }
  
 protected:

  /**
    * This returns the size of a message as a long integer.
    * This is useful as an internal member, because the "other"
    * getSize command will emit a signal, which would be harder
    * to trap when doing something like listing a directory.
    */
  size_t realGetSize(unsigned int msg_num);

  /**
    *  Send a command to the server, and wait for the one-line-status
    *  reply via getResponse.  Similar rules apply.  If no buffer is
    *  specified, no data is passed back.
    */
  bool command (const char *buf, char *r_buf=0, unsigned int r_len=0);

  /**
    *  All POP3 commands will generate a response.  Each response will
    *  either be prefixed with a "+OK " or a "-ERR ".  The getResponse 
    *  fucntion will wait until there's data to be read, and then read in
    *  the first line (the response), and copy the response sans +OK/-ERR
    *  into a buffer (up to len bytes) if one was passed to it.  It will
    *  return true if the response was affirmitave, or false otherwise.
    */
  bool getResponse (char *buf=0, unsigned int len=0);

  /**
    *  Attempt to properly shut down the POP3 connection by sending
    *  "QUIT\r\n" before closing the socket.
    */
  void pop3_close ();

  /**
    * Attempt to initiate a POP3 connection via a TCP socket.  If no port
    * is passed, port 110 is assumed, if no user || password is
    * specified, the user is prompted for them.
    */
  bool pop3_open (KURL &_url);

  int m_cmd, m_iSock;
  uint m_iOldPort;
  struct timeval m_tTimeout;
  QString m_sOldServer, m_sOldPass, m_sOldUser;
  FILE *fp;
  IOJob* m_pJob;
};

class POP3IOJob : public IOJob
{
 public:
  POP3IOJob (Connection *_conn, POP3Protocol *_pop3);
  
  virtual void slotData (void *_p, int _len);
  virtual void slotDataEnd ();
  virtual void slotError (int _errid, const char *_txt);
  
 protected:
  POP3Protocol *m_pPOP3;
};

#endif
