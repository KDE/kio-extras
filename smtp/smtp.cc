#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <qcstring.h>
#include <qglobal.h>

#include <kprotocolmanager.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <klocale.h>

#include "smtp.h"

using namespace KIO;

extern "C" { int kdemain (int argc, char **argv); }

#ifdef SSMTP

int SSL_readline(SSL *ssl, char *buf, int num) {
int c = 0;
  if (num <= 0) return -2;

  buf[num-1] = 0;

  for(c = 0; c < num-1; c++) {
    char x;
    int rc = SSL_read(ssl, &x, 1);
    if (rc <= 0)
      return rc;

    buf[c] = x;

    if (x == '\n') {
      buf[c+1] = 0;
      break;
    }
  }

  if (c == num-1)
    return c;
return c+1;
}

#endif

int kdemain( int argc, char **argv )
{
#ifdef SSMTP
  KInstance instance( "kio_ssmtp" );
#else
  KInstance instance( "kio_smtp" );
#endif

  if (argc != 4)
  {
#ifdef SSMTP
    fprintf(stderr, "Usage: kio_ssmtp protocol domain-socket1 domain-socket2\n");
#else
    fprintf(stderr, "Usage: kio_smtp protocol domain-socket1 domain-socket2\n");
#endif
    exit(-1);
  }

  SMTPProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();
  return 0;
}


SMTPProtocol::SMTPProtocol(const QCString &pool, const QCString &app)
#ifdef SSMTP
  : SlaveBase( "ssmtp", pool, app)
#else
  : SlaveBase( "smtp", pool, app)
#endif
{
  debug( "SMTPProtocol()" );
  

#ifdef SSMTP
  ssl = NULL;
  SSLeay_add_ssl_algorithms();
  meth = SSLv2_client_method();
  SSL_load_error_strings();
  ctx = SSL_CTX_new(meth);
#endif
}


SMTPProtocol::~SMTPProtocol()
{
  debug( "~SMTPProtocol()" );
  smtp_close();
#ifdef SSMTP
  SSL_CTX_free(ctx);
#endif
}


void SMTPProtocol::put( const QString& path, int permissions, bool overwrite, bool resume) {

}


QString SMTPProtocol::buildUrl(const QString& path)
{
  KURL url;
  url.setHost( m_sServer);
  url.setPort( m_iPort);
  url.setPath( path );
  return url.url();
}


void SMTPProtocol::setHost( const QString& host, int port, const QString& /*user*/, const QString& /*pass*/) {
  m_sServer = host;
  m_iPort = port;
}


bool SMTPProtocol::getResponse(char *buf, unsigned int len) {

}


bool SMTPProtocol::command(const char *buf, char *r_buf, unsigned int r_len) {

}


bool SMTPProtocol::smtp_open(KURL& url) {
unsigned short int port;
ksockaddr_in server_name;
memset(&server_name, 0, sizeof(server_name));

// get the port to use
// -1 means no port specified
#ifdef SSMTP
  if (url.port()) {
    port = url.port();
  } else {
    struct servent *sent = getservbyname("ssmtp", "tcp");
    if (sent) {
      port = ntohs(sent->s_port);
    } else {
      port = 465;
    }
  }
#else
  if (url.port()) {
    port = url.port();
  } else {
    struct servent *sent = getservbyname("smtp", "tcp");
    if (sent) {
      port = ntohs(sent->s_port);
    } else {
      port = 25;
    }
  }
#endif

  if ( (m_iOldPort == port) && (m_sOldServer == m_sServer) ) {
    fprintf(stderr,"Reusing old connection\n");
    return true;
  } else {
    smtp_close();
    m_iSock = ::socket(PF_INET, SOCK_STREAM, 0);
    if (!KSocket::initSockaddr(&server_name, m_sServer, port))
      return false;
    if (::connect(m_iSock, (struct sockaddr*)(&server_name), sizeof(server_name))) {
      error( ERR_COULD_NOT_CONNECT, m_sServer);
      return false;
    } 


// Either setup SSL or setup the stdin/stdout descriptor

#ifdef SSMTP
  // do the SSL negotiation
  ssl = SSL_new(ctx);
  if (!ssl) {
    error( ERR_COULD_NOT_CONNECT, m_sServer );
    close(m_iSock);
    return false;
  }
 
  SSL_set_fd(ssl, m_iSock);
  if (-1 == SSL_connect(ssl)) {
    error( ERR_COULD_NOT_CONNECT, m_sServer );
    close(m_iSock);
    return false;
  }

  server_cert = SSL_get_peer_certificate(ssl);
  if (!server_cert) {
    error( ERR_COULD_NOT_CONNECT, m_sServer );
    close(m_iSock);
    return false;
  }

  // we should verify the certificate here

  X509_free(server_cert);
#else
  if ((fp = fdopen(m_iSock, "w+")) == NULL) {
    close(m_iSock);
    return false;
  }
#endif

  QCString greeting(1024);
  if (!getResponse(greeting.data(), greeting.size()))
    return false;

  m_iOldPort = port;
  m_sOldServer = m_sServer;

  }
return true;
}


void SMTPProtocol::smtp_close() {
#ifdef SSMTP
  if (ssl) {
    command("QUIT");
    close(m_iSock);
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ssl = NULL;
    m_iSock = 0;
  }
#else
  if (fp) {
    command("QUIT");
    fclose(fp);
    m_iSock = 0;
    fp = 0;
  }
#endif
}


