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

#ifdef SSMTP
  SSL_CTX_free(ctx);
#endif
}



