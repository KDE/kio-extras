#ifndef _SMTP_H
#define _SMTP_H

#include <qstring.h>
#include <kio/slavebase.h>

#ifdef SSMTP
  #ifndef HAVE_SSL
    #undef SSMTP
  #endif
#endif  


#ifdef SSMTP
  extern "C" { 
  #include <openssl/crypto.h> 
  #include <openssl/x509.h> 
  #include <openssl/pem.h> 
  #include <openssl/ssl.h> 
  #include <openssl/err.h> 
  };
#endif                                                                                 

class SMTPProtocol : public KIO::SlaveBase {
 public:
  SMTPProtocol( const QCString &pool, const QCString &app );
  virtual ~SMTPProtocol();

  virtual void setHost(const QString& host, int port, const QString& user, const QString& pass);

  virtual void put( const QString& path, int permissions, bool overwrite, bool resume);

 private:

  bool smtp_open(KURL &url);
  void smtp_close();
  bool command(const char *buf, char *r_buf = NULL, unsigned int r_len = 0);
  bool getResponse(char *r_buf = NULL, unsigned int r_len = 0);
  QString buildUrl(const QString &path);

  int m_iSock;
  struct timeval m_tTimeout;
  FILE *fp;
  QString urlPrefix;
  QString m_sServer, m_sOldServer;
  unsigned short int m_iPort, m_iOldPort;
#ifdef SSMTP
  SSL_CTX *ctx;
  SSL *ssl;
  X509 *server_cert;
  SSL_METHOD *meth;
#endif

};

#endif
