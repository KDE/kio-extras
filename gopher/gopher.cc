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

#include <iostream.h>

bool open_PassDlg( const QString& _head, QString& _user, QString& _pass );

int main(int , char **)
{
  signal(SIGCHLD, IOProtocol::sigchld_handler);
  //signal(SIGSEGV, IOProtocol::sigsegv_handler);

  Connection parent( 0, 1 );

  GopherProtocol gopher( &parent );
  gopher.dispatchLoop();
}

GopherProtocol::GopherProtocol(Connection *_conn) : IOProtocol(_conn)
{
  m_cmd = CMD_NONE;
  m_pJob = 0L;
  m_iSock = 0;
  m_tTimeout.tv_sec=10;
  m_tTimeout.tv_usec=0;
  fp = 0;
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
  // otherwise as a last resort use the "official" port of 70.

  unsigned short int port;
  struct sockaddr_in server_name;
  memset(&server_name, 0, sizeof(server_name));
  static char buf[512];

  // We want 70 as the default
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
    m_iSock=0;
    return false;
  }

  QString path=_url.path();
  if (path.at(0)=='/') path.remove(0,1);
  if (path.isEmpty()) {
    // We just want the initial listing
    if (::write(m_iSock, "\r\n", 2) != 2) {
      error(ERR_COULD_NOT_CONNECT, strdup(_url.host()));
      return false;
    }
  } else {
    path.remove(0,1); // Remove the type identifier
    // It should not be empty here
    if (path.isEmpty()) {
      error(ERR_MALFORMED_URL, strdup(_url.host()));
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
  }
  return true;
  
}

void GopherProtocol::slotListDir( const char *_url )
{
  KURL dest(_url);
  QString path = dest.path();
  if ( dest.isMalformed() ) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    m_cmd = CMD_NONE;
    return;
  }

  if (dest.protocol() != "gopher") {
    error( ERR_INTERNAL, "kio_gopher got non gopher url" );
    m_cmd = CMD_NONE;
    return;
  }
  gopher_open(dest);
  if (path.at(0) == '/') path.remove(0,1);

  UDSEntry entry;
  UDSAtom atom;
  QString line;
  char buf[128];
  while (fgets(buf, 127, fp)) {
    line = buf+1;
    switch ((GopherType)buf[0]) {
    case GOPHER_MENU: {
      entry.clear();
      atom.m_uds = UDS_NAME;
      atom.m_long = 0;
      atom.m_str = line.mid(0,line.find("\t"));
      entry.append(atom);

      atom.m_uds = UDS_FILE_TYPE;
      atom.m_str = "";
      atom.m_long = S_IFDIR;
      entry.append(atom);

      atom.m_uds = UDS_SIZE;
      atom.m_str = QString::null;
      atom.m_long = 0;
      entry.append(atom);

      listEntry(entry);
      break;
    }
    default: {
      break;
    }
    }
  }
  finished();
  return;
}

void GopherProtocol::slotTestDir( const char *_url )
{
  KURL usrc(_url);
  // An empty path is essentially a request for an index...
  QString path = usrc.path();
  if (path.at(0)=='/') path.remove(0,1);
  if (path.isEmpty() || path.at(0) == '1')
    isDirectory();
  else
    isFile();
  finished();
}

void GopherProtocol::slotGetSize( const char *_url )
{
  KURL target(_url);
  if (target.path() == "/aboutme.txt") {
    totalSize(strlen(GopherProtocol::abouttext));
    finished();
    m_cmd = CMD_NONE;
  }
  
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
    error( ERR_INTERNAL, "kio_gopher got non gopher url" );
    m_cmd = CMD_NONE;
    return;
  }

  path = usrc.path();

  if (path == "/aboutme.txt") {
    ready();
    gettingFile(_url);
    mimeType("text/plain");
    data(GopherProtocol::abouttext, strlen(GopherProtocol::abouttext));
    dataEnd();
    processedSize(strlen(GopherProtocol::abouttext));
    finished();
  }
  if (path.at(0)=='/') path.remove(0,1);
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
  //fprintf(stderr,"Type is:");
  current_type=(GopherType)type;
  switch ((GopherType)type) {
  case GOPHER_GIF:  {
    gopher_open(usrc);
    if(!readRawData(_url, "image/gif")) {
      error(ERR_INTERNAL, "rawReadData failed");
      return;
    }
  }
  case GOPHER_TEXT: {
    gopher_open(usrc);
    if(!readRawData(_url, "text/plain")) {
      error(ERR_INTERNAL, "rawReadData failed");
      return;
    }
  }
  }
}

bool GopherProtocol::readRawData(const char *_url, const char *mimetype)
{
  char buf[1024];
  ready();
  gettingFile(_url);
  mimeType(mimetype);
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
  finished();
  return true;
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

const char *GopherProtocol::abouttext=
"gopher  n.  1. Any of various short tailed, burrowing mammals of the
family Geomyidae, of North America.  2. (Amer. colloq.) Native or
inhabitant of Minnesota: the Gopher State.  3. (Amer. colloq.) One
who runs errands, does odd-jobs, fetches or delivers documents for
office staff.  4. (computer tech.) software following a simple
protocol for burrowing through a TCP/IP internet.";
