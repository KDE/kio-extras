#ifndef _GOPHER_H
#define _GOPHER_H "$Id$"

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include <qstring.h>

#include <kio/tcpslavebase.h>

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

class GopherProtocol : public KIO::TCPSlaveBase
{
public:
public:
  GopherProtocol (const QCString &pool, const QCString &app );
  virtual ~GopherProtocol();

  virtual void get( const KURL&);
  virtual void stat( const KURL& url );
  //virtual void del( const KURL& url, bool isfile);
  virtual void listDir( const KURL& url );

  void setHost(const QString &host, int port, const QString &user, const QString &pass);

 protected:

  bool readRawData(const QString &_url, const char *mimetype=0);

  /**
    *  Attempt to properly shut down the Gopher connection.
    */
  void gopher_close ();

  /**
    * Attempt to initiate a Gopher connection via a TCP socket.  If no port
    * is passed, port 70 is assumed.
    */
  bool gopher_open (const KURL &_url);

  QString m_sServer, m_sUser, m_sPass;
  int m_cmd, m_iSock, m_iPort;
  struct timeval m_tTimeout;
  FILE *fp;
  GopherType current_type;
  static const char *abouttext;
};

#endif
