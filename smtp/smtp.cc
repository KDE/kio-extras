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



//
//   COMMANDS WE SUPPORT:
//
// 
//   COMMANDS WE SHOULD SUPPORT:
//   QUIT,       EXPN,       HELO,       EHLO,       HELP,       NOOP, 
//   VRFY,       RSET,       DATA,       RCPT TO:,   MESG FROM:, ETRN,
//   VERB,       DSN
//
//   COMMANDS THAT DON'T SEEM SUPPORTED ANYMORE:
//   SAML FROM:, SOML FROM:, SEND FROM:, TURN
//   

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



//
// REPLY CODES
//       (is this up to date?)
// 5xx - errors
// 500 Syntax error, command unrecognized, parameter required
// 501 Syntax error in parameters or arguments
// 502 Command not implemented
// 503 Bad sequence of commands
// 504 Command parameter not implemented
//
//     - status
// 211 System status or help reply
// 214 Help message
// 220 <domain> Service ready
// 221 <domain> Service closing transmission channel
// 421 <domain> Service not available, closing transmission channel
//
//     - success or error
// 250 Requested mail action okay, completed
// 251 User not local; will forward to <addr>
// 450 Requested mail action not taken: mailbox unavailable (busy)
// 550 Requested action not taken: mailbox unavailable (not found)
// 451 Requested action aborted: error in processing
// 551 User not local; please try <addr>
// 452 Requested action not taken: insufficient storage
// 553 Requested action not taken: mailbox name not allowed
// 354 Start mail input; end with <CRLF>.<CRLF>
// 554 Transaction failed
//
// Commands with their replies:
//
// CONNECTION: Success: 220; Fail: 421; Error:
// HELO:       Success: 250; Fail: ; Error: 500, 501, 504, 421
// MAIL FROM:: Success: 250; Fail: 552, 451, 452; Error: 500, 501, 421
// RCPT FROM:: Success: 250, 251; Fail: 550, 551, 552, 553, 450, 451, 452;
//             Error: 500, 501, 503, 421
// DATA:       Initial: 354;  data -> Success: 250; Fail: 552, 554, 451, 452
//             Fail: 451, 554; Error: 500, 501, 503, 421
// RSET:       Success: 250; Fail: ; Error: 500, 501, 504, 421
// VRFY:       Success: 250, 251; Fail: 550, 551, 553; 
//             Error: 500, 501, 502, 504, 421
// EXPN:       Success: 250; Fail: 550; Error: 500, 501, 502, 504, 421
// HELP:       Success: ; Fail: ; Error: 500, 501, 502, 504, 421
// NOOP:       Success: 250; Fail: ; Error: 500, 421
// QUIT:       Success: 221; Fail: ; Error: 500
// EHLO:       ???
// VERB:       ???
// ETRN:       ???
// DSN:        ???
// 

bool SMTPProtocol::getResponse(char *r_buf, unsigned int r_len) {
  char *buf=0;
  unsigned int recv_len=0;
  fd_set FDs;
 
  // Give the buffer the appropiate size
  if (r_len)
    buf=(char *)malloc(r_len);
  else {
    buf=(char *)malloc(512);
    r_len=512;
  }
 
  // And keep waiting if it timed out
  unsigned int wait_time=60; // Wait 60sec. max.
  do
  {
      // Wait for something to come from the server
    FD_ZERO(&FDs);
    FD_SET(m_iSock, &FDs);
    // Yes, it's true, Linux sucks. (And you can't program --Waba)
    wait_time--;
    m_tTimeout.tv_sec=1;
    m_tTimeout.tv_usec=0;
  }
  while (wait_time && (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0));
 
  if (wait_time == 0)
  {
    fprintf(stderr, "No response from SMTP server in 60 secs.\n");
    return false;
  }
 
  // Clear out the buffer
  memset(buf, 0, r_len);
  // And grab the data
#ifdef SPOP3
  int rc = SSL_readline(ssl, buf, r_len-1);
  if (rc <= 0) {
    if (buf) free(buf);
    return false;
  }
  buf[rc] = 0;
#else
  if (fgets(buf, r_len-1, fp) == 0) {
    if (buf) free(buf);
    return false;
  }
#endif
  // This is really a funky crash waiting to happen if something isn't
  // null terminated.
  recv_len=strlen(buf);
 
/*
 *   From std010:
 *

 *
 */
 
  // HERE WE CHECK THE SUCCESSFUL RESPONSES FIRST 1xy,2xy,3xy
  // AND THE ERROR RESPONSES SECOND               4xy,5xy
  if (strncmp(buf, "250 ", 4)==0) {           // standard "OK"
    if (r_buf && r_len) {
      memcpy(r_buf, buf+4, QMIN(r_len,recv_len-4));
    }
    if (buf) free(buf);
    return true;
  } else if (strncmp(buf, "354 ", 4)==0) {    // after DATA, says to continue
  } else if (strncmp(buf, "220 ", 4)==0) {    // opening connection
  } else if (strncmp(buf, "221 ", 4)==0) {    // closing connection
  } else if (strncmp(buf, "-ERR ", 5)==0) {
    if (r_buf && r_len) {
      memcpy(r_buf, buf+5, QMIN(r_len,recv_len-5));
    }
    if (buf) free(buf);
    return false;
  } else {
    fprintf(stderr, "Invalid SMTP response received!\n");
    if (r_buf && r_len) {
      memcpy(r_buf, buf, QMIN(r_len,recv_len));
    }
    if (buf) free(buf);
    return false;
  }
}


bool SMTPProtocol::command(const char *buf, char *r_buf, unsigned int r_len) {
#ifdef SPOP3
  // Write the command
  int rc = SSL_write(ssl, buf, strlen(buf));
  if (rc <= 0) return false;
 
  rc = SSL_write(ssl, "\r\n", 2);
  if (rc <= 0) return false;
#else
  // Write the command
  unsigned int x = strlen(buf);
  if (::write(m_iSock, buf, x) != (ssize_t)x)
    return false;
  if (::write(m_iSock, "\r\n", 2) != 2)
    return false;
#endif
  return getResponse(r_buf, r_len);
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
    m_sOldServer = ""; 
    m_iOldPort = 0;
  }
#else
  if (fp) {
    command("QUIT");
    fclose(fp);
    m_iSock = 0;
    fp = NULL;
    m_sOldServer = "";
    m_iOldPort = 0;
  }
#endif
}


