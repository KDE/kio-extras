#ifndef _SMTP_H
#define _SMTP_H

#include <qstring.h>
#include <kio/tcpslavebase.h>

class SMTPProtocol : public KIO::TCPSlaveBase {
 public:
  SMTPProtocol( const QCString &pool, const QCString &app, bool SSL );
  virtual ~SMTPProtocol();

  virtual void setHost(const QString& host, int port, const QString& user, const QString& pass);

  virtual void put( const KURL& url, int permissions, bool overwrite, bool resume);

 private:

  bool smtp_open(KURL &url);
  void smtp_close();
  bool command(const char *buf, char *r_buf = NULL, unsigned int r_len = 0);
  bool getResponse(char *r_buf = NULL, unsigned int r_len = 0, const char *cmd = NULL);

  int m_iSock;
  struct timeval m_tTimeout;
  FILE *fp;
  bool opened;
  QString m_sServer, m_sOldServer;
  unsigned short int m_iPort, m_iOldPort;
  QString m_sError;
};

#endif
