// $Id$

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

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

#include <kurl.h>
#include <kprotocolmanager.h>
#include <ksock.h>

#include "gopher.h"

bool open_PassDlg( const QString& _head, QString& _user, QString& _pass );

int main(int , char **)
{
  signal(SIGCHLD, IOProtocol::sigchld_handler);
  signal(SIGSEGV, IOProtocol::sigsegv_handler);

  Connection parent( 0, 1 );

  GopherProtocol gopher( &parent );
  gopher.dispatchLoop();
}

GopherProtocol::GopherProtocol(Connection *_conn) : IOProtocol(_conn)
{
  m_cmd = CMD_NONE;
  m_pJob = 0L;
  m_iSock = m_iOldPort = 0;
  m_tTimeout.tv_sec=10;
  m_tTimeout.tv_usec=0;
  fp = 0;
}

bool GopherProtocol::getResponse (char *r_buf, unsigned int r_len)
{
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

  // Wait for something to come from the server
  FD_ZERO(&FDs);
  FD_SET(m_iSock, &FDs);

  // And keep waiting if it timed out
  unsigned int wait_time=1;
  while (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0) {
    // Yes, it's true, Linux sucks.
    wait_time++;
    m_tTimeout.tv_sec=wait_time;
    m_tTimeout.tv_usec=0;
  }

  // Clear out the buffer
  memset(buf, 0, r_len);
  // And grab the data
  if (fgets(buf, r_len-1, fp) == 0) {
    if (buf) free(buf);
    return false;
  }
  // This is really a funky crash waiting to happen if something isn't
  // null terminated.
  recv_len=strlen(buf);

/*
 *   From rfc1939:
 *
 *   Responses in the Gopher consist of a status indicator and a keyword
 *   possibly followed by additional information.  All responses are
 *   terminated by a CRLF pair.  Responses may be up to 512 characters
 *   long, including the terminating CRLF.  There are currently two status
 *   indicators: positive ("+OK") and negative ("-ERR").  Servers MUST
 *   send the "+OK" and "-ERR" in upper case.
 */

  if (strncmp(buf, "+OK ", 4)==0) {
    if (r_buf && r_len) {
      memcpy(r_buf, buf+4, QMIN(r_len,recv_len-4));
    }
    if (buf) free(buf);
    return true;
  } else if (strncmp(buf, "-ERR ", 5)==0) {
    if (r_buf && r_len) {
      memcpy(r_buf, buf+5, QMIN(r_len,recv_len-5));
    }
    if (buf) free(buf);
    return false;
  } else {
    fprintf(stderr, "Invalid Gopher response received!\n");fflush(stderr);
    if (r_buf && r_len) {
      memcpy(r_buf, buf, QMIN(r_len,recv_len));
    }
    if (buf) free(buf);
    return false;
  }
}

bool GopherProtocol::command (const char *cmd, char *recv_buf, unsigned int len)
{

/*
 *   From rfc1939:
 *
 *   Commands in the Gopher consist of a case-insensitive keyword, possibly
 *   followed by one or more arguments.  All commands are terminated by a
 *   CRLF pair.  Keywords and arguments consist of printable ASCII
 *   characters.  Keywords and arguments are each separated by a single
 *   SPACE character.  Keywords are three or four characters long. Each
 *   argument may be up to 40 characters long.
 */

  // Write the command
  if (::write(m_iSock, cmd, strlen(cmd)) != (ssize_t)strlen(cmd))
    return false;
  if (::write(m_iSock, "\r\n", 2) != 2)
    return false;
  return getResponse(recv_buf, len);
}

void GopherProtocol::gopher_close ()
{
  if (fp) {
    fclose(fp);
    m_iSock=0; fp=0;
  }
}

bool GopherProtocol::gopher_open( KURL &_url )
{
  // This function is simply a wrapper to establish the connection
  // to the server.  It's a bit more complicated than ::connect
  // because we first have to check to see if the user specified
  // a port, and if so use it, otherwise we check to see if there
  // is a port specified in /etc/services, and if so use that
  // otherwise as a last resort use the "official" port of 110.

  unsigned int port;
  struct sockaddr_in server_name;
  memset(&server_name, 0, sizeof(server_name));
  static char buf[512];

  // We want 110 as the default, but -1 means no port was specified.
  // Why 0 wasn't chosen is beyond me.
  port = _url.port() ? _url.port() : 70;
  gopher_close();
  m_iSock = ::socket(PF_INET, SOCK_STREAM, 0);
  if (!KSocket::initSockaddr(&server_name, _url.host(), port))
    return false;
  if (::connect(m_iSock, (struct sockaddr*)(&server_name), sizeof(server_name))) {
    error( ERR_COULD_NOT_CONNECT, strdup(_url.host()));
    return false;
  }

  // Since we want to use stdio on the socket,
  // we must fdopen it to get a file pointer,
  // if it fails, close everything up
  if ((fp = fdopen(m_iSock, "w+")) == 0) {
    close(m_iSock);
    return false;
  }

  QString path=_url.path();
  if (path.left(1)=="/") path.remove(0,1);
  if (path.isEmpty()) {
    // We just want the initial listing
    if (::write(m_iSock, "\r\n", 2) != 2) {
      error(ERR_COULD_NOT_CONNECT, strdup(_url.host()));
      return false;
    }
  }
  path.remove(0,1); // Remove the type identifier
  // It sholud not be empty here
  if (path.isEmpty()) {
    gopher_close();
    return false;
  }
  // Otherwise we should send our request
  if (::write(m_iSock, path.ascii(), strlen(path.ascii())) != strlen(path.ascii())) {
    error(ERR_COULD_NOT_CONNECT, strdup(_url.host()));
    gopher_close();
    return false;
  }
  if (::write(m_iSock, "\r\n", 2) != 2) {
    error(ERR_COULD_NOT_CONNECT, strdup(_url.host()));
    gopher_close();
    return false;
  }
  return true;
  
}

void GopherProtocol::slotListDir( const char *_url )
{
}

void GopherProtocol::slotTestDir( const char *_url )
{
}

void GopherProtocol::slotGetSize( const char *_url )
{
}

void GopherProtocol::slotGet(const char *_url)
{
  bool ok=true;
  static char buf[512];
  QString path, cmd;
  KURL usrc(_url);
  if ( usrc.isMalformed() ) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    m_cmd = CMD_NONE;
    return;
  }

  if (usrc.protocol() != "gopher") {
    error( ERR_INTERNAL, "kio_gopher got non pop3 url" );
    m_cmd = CMD_NONE;
    return;
  }

  path = usrc.path();

  if (path.left(1)=="/") path.remove(0,1);
  if (path.isEmpty()) {
    debug("We should be a dir!!");
    error(ERR_IS_DIRECTORY, strdup(_url));
    m_cmd=CMD_NONE; return;
  }
  if (path.length() < 2) {
    error(ERR_MALFORMED_URL, strdup(_url));
    return;
  }
  char type = path.ascii()[0];
  fprintf(stderr,"Type is:");
  current_type=type;
  switch (type) {
  case GOPHER_TEXT: {
    gopher_open(usrc);
    char buf[1024];
    ready();
    gettingFile(_url);
    mimeType("text/plain");
    ssize_t read_ret=0;
    size_t total_size=0;
    while ((read_ret=::read(m_iSock, buf, 1024))>0) {
      data(buf, read_ret);
      total_size+=read_ret;
    }
    dataEnd();
    processedSize(total_size);
    finished();
    gopher_close();
  }
  }

  finished();
}

void GopherProtocol::slotCopy(const char *, const char *)
{
  fprintf(stderr, "GopherProtocol::slotCopy\n");
  fflush(stderr);
}

void GopherProtocol::slotData(void *, int)
{
  switch (m_cmd) {
    case CMD_PUT:
	// Send data here
      break;
    default:
      abort();
      break;
    }
}

void GopherProtocol::slotDataEnd()
{
  switch (m_cmd) {
    case CMD_PUT:
      m_cmd = CMD_NONE;
      break;
    default:
      abort();
      break;
    }
}

void GopherProtocol::jobData(void *, int )
{
  switch (m_cmd) {
  case CMD_GET:
    break;
  case CMD_COPY:
    break;
  default:
    abort();
  }
}

void GopherProtocol::jobError(int _errid, const char *_text)
{
  error(_errid, _text);
}

void GopherProtocol::jobDataEnd()
{
  switch (m_cmd) {
  case CMD_GET:
    dataEnd();
    break;
  case CMD_COPY:
    m_pJob->dataEnd();
    break;
  default:
    abort();
  }
}

/*************************************
 *
 * GopherIOJob
 *
 *************************************/

GopherIOJob::GopherIOJob(Connection *_conn, GopherProtocol *_gopher) :
	IOJob(_conn)
{
  m_pGopher = _gopher;
}
  
void GopherIOJob::slotData(void *_p, int _len)
{
  m_pGopher->jobData( _p, _len );
}

void GopherIOJob::slotDataEnd()
{
  m_pGopher->jobDataEnd();
}

void GopherIOJob::slotError(int _errid, const char *_txt)
{
  IOJob::slotError( _errid, _txt );
  m_pGopher->jobError(_errid, _txt );
}
