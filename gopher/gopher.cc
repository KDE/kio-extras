// $Id$

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>

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
#include <kio/passdlg.h>

#include <klocale.h>

#include "gopher.h"

#include <iostream.h>


using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv)
{
  KInstance instance( "kio_gopher" );
  if (argc != 4) {
    fprintf(stderr, "usage statement needs to go here\n");
    exit(-1);
  }
  GopherProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();
  return 0;
}

GopherProtocol::GopherProtocol(const QCString &pool, const QCString &app)
  : SlaveBase( "gopher", pool, app)
{
  m_cmd = CMD_NONE;
  m_iSock = 0;
  m_tTimeout.tv_sec=10;
  m_tTimeout.tv_usec=0;
  fp = 0;
}

GopherProtocol::~GopherProtocol()
{
	gopher_close();
}

void GopherProtocol::gopher_close ()
{
  if (fp) {
    fclose(fp);
    m_iSock=0; fp=0;
  }
}

bool GopherProtocol::gopher_open( const KURL &_url )
{
  // This function is simply a wrapper to establish the connection
  // to the server.  It's a bit more complicated than ::connect
  // because we first have to check to see if the user specified
  // a port, and if so use it, otherwise we check to see if there
  // is a port specified in /etc/services, and if so use that
  // otherwise as a last resort use the "official" port of 70.

  unsigned short int port;
  ksockaddr_in server_name;
  memset(&server_name, 0, sizeof(server_name));
  // static char buf[512];

  // We want 70 as the default
  if (_url.port())
    port=_url.port();
  else {
    struct servent *srv=getservbyname("gopher", "tcp");
    if (srv) {
      port=ntohs(srv->s_port);
    } else
      port=70;
  }

  gopher_close();
  m_iSock = ::socket(PF_INET, SOCK_STREAM, 0);
  if (!KSocket::initSockaddr(&server_name, m_sServer.ascii(), port))
    return false;
  if (::connect(m_iSock, (struct sockaddr*)(&server_name), sizeof(server_name))) {
    error( ERR_COULD_NOT_CONNECT, _url.host());
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
      error(ERR_COULD_NOT_CONNECT, _url.host());
      return false;
    }
  } else {
    path.remove(0,1); // Remove the type identifier
    // It should not be empty here
    if (path.isEmpty()) {
      error(ERR_MALFORMED_URL, _url.host());
      gopher_close();
      return false;
    }
    // Otherwise we should send our request
    if (::write(m_iSock, path.ascii(), strlen(path.ascii())) != strlen(path.ascii())) {
      error(ERR_COULD_NOT_CONNECT, _url.host());
      gopher_close();
      return false;
    }
    if (::write(m_iSock, "\r\n", 2) != 2) {
      error(ERR_COULD_NOT_CONNECT, _url.host());
      gopher_close();
      return false;
    }
  }
  return true;
  
}

void GopherProtocol::setHost( const QString & _host, int _port, const QString &_user, const QString &_pass)
{
  m_sServer = _host;
  m_iPort = _port;
  m_sUser = _user;
  m_sPass = _pass;
}

void GopherProtocol::stat (const KURL &url)
{
  kdDebug() << "STAT CALLZ" << endl;
  QString _path = url.path();
  if (_path.at(0) == '/')
    _path.remove(0,1);
  UDSEntry entry;
  UDSAtom atom;
  atom.m_uds = KIO::UDS_NAME;
  atom.m_str = _path;
  entry.append( atom );

  atom.m_uds = KIO::UDS_FILE_TYPE;
  if ((url.path() == "/") || (_path.at(0) == '1') ) {
   kdDebug() << "Is a DIR" << endl;
   atom.m_long = S_IFDIR;
  } else {
   kdDebug() << "Is a FILE" << endl;
   atom.m_long = S_IFREG;
  }
  entry.append( atom );

      atom.m_uds = KIO::UDS_ACCESS;
      atom.m_long = S_IRUSR | S_IRGRP | S_IROTH; // readable by everybody
      entry.append( atom );

#if 0  
      atom.m_uds = KIO::UDS_SIZE;
      atom.m_long = m_iSize;
      entry.append( atom );
#endif

  // TODO: maybe get the size of the message?
  statEntry( entry );
  finished();
}


void GopherProtocol::listDir( const KURL &dest )
{
  QString path = dest.path();
  if ( dest.isMalformed() ) {
    error( ERR_MALFORMED_URL, dest.url() );
    m_cmd = CMD_NONE;
    return;
  }

  if (dest.protocol() != "gopher") {
    error( ERR_INTERNAL, "kio_gopher got non gopher url" );
    m_cmd = CMD_NONE;
    return;
  }
  if (!gopher_open(dest)) {
    gopher_close();
    return;
  }
  if (path.at(0) == '/') path.remove(0,1);

  UDSEntry entry;
  UDSAtom atom;
  QString line;
  char buf[128];
  while (fgets(buf, 127, fp)) {

    line = buf+1;
    if (strcmp(buf, ".\r\n")==0) {
      finished();
      return;
    }
    entry.clear();
    atom.m_uds = UDS_NAME;
    atom.m_long = 0;
    atom.m_str = line.mid(0,line.find("\t"));
    entry.append(atom);

    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    switch ((GopherType)buf[0]) {
    case GOPHER_MENU:{
      atom.m_long = S_IFDIR;
      break;
    }
    default: {
      atom.m_long = S_IFREG;
    }
    }
    entry.append(atom);

    atom.m_uds = UDS_MIME_TYPE;
    atom.m_long = 0;
    switch ((GopherType)buf[0]) {
    case GOPHER_MENU:{
      atom.m_str="inode/directory";
      break;
    }
    case GOPHER_GIF:{
      atom.m_str="image/gif";
      break;
    }
    case GOPHER_TEXT:{
      atom.m_str="text/plain";
      break;
    }
    default: {
      atom.m_str="application/ocet-stream";
      break;
    }
    }
    entry.append(atom);

    atom.m_uds = UDS_URL;
    KURL uds;
    uds.setProtocol("gopher");
    QString path("/");
    path.append(buf[0]);
    line.remove(0, line.find("\t")+1);
    path.append(line.mid(0,line.find("\t")));
    if (path == "//") path="/";
    uds.setPath(path);
    line.remove(0, line.find("\t")+1);
    uds.setHost(line.mid(0,line.find("\t")));
    line.remove(0, line.find("\t")+1);
    uds.setPort(line.mid(0,line.find("\t")).toUShort());
    atom.m_long = 0;
    atom.m_str = uds.url();
    entry.append(atom);

    atom.m_uds = UDS_SIZE;
    atom.m_str = QString::null;
    atom.m_long = 0;
    entry.append(atom);

    listEntry(entry, false);
    entry.clear();
  }
  listEntry(entry, true);
  finished();
  return;
}

void GopherProtocol::get(const KURL &usrc)
{
  QByteArray array;
  QString path, cmd;
  //KURL usrc(_url);
  if ( usrc.isMalformed() ) {
    error( ERR_MALFORMED_URL, usrc.url() );
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
    gettingFile(usrc.url());
    mimeType("text/plain");
    array.setRawData(GopherProtocol::abouttext, strlen(GopherProtocol::abouttext));
    data(array);
    array.resetRawData(GopherProtocol::abouttext, strlen(GopherProtocol::abouttext));
    data(QByteArray());
    processedSize(strlen(GopherProtocol::abouttext));
    finished();
  }
  if (path.at(0)=='/') path.remove(0,1);
  if (path.isEmpty()) {
    debug("We should be a dir!!");
    error(ERR_IS_DIRECTORY, usrc.url());
    m_cmd=CMD_NONE; return;
  }
  if (path.length() < 2) {
    error(ERR_MALFORMED_URL, usrc.url());
    return;
  }
  char type = path.ascii()[0];
  //fprintf(stderr,"Type is:");
  current_type=(GopherType)type;
  switch ((GopherType)type) {
  case GOPHER_GIF:  {
    gopher_open(usrc);
    if(!readRawData(usrc.url(), "image/gif")) {
      error(ERR_INTERNAL, "rawReadData failed");
      return;
    }
    break;
  }
  case GOPHER_UUENCODE: {
    gopher_open(usrc);
    if (!readRawData(usrc.url(), "text/plain")) {
      error(ERR_INTERNAL, "rawReadData failed");
      return;
    }
    break;
  }
  case GOPHER_BINARY:
  case GOPHER_PCBINARY: {
    gopher_open(usrc);
    if(!readRawData(usrc.url(), "application/ocet-stream")) {
      error(ERR_INTERNAL, "rawReadData failed");
      return;
    }
    break;
  }
  case GOPHER_TEXT: {
    gopher_open(usrc);
    if(!readRawData(usrc.url(), "text/plain")) {
      error(ERR_INTERNAL, "rawReadData failed");
      return;
    }
    break;
  }
  default:
    break;
  }
}

bool GopherProtocol::readRawData(const QString &_url, const char *mimetype)
{
  QByteArray array;
  char buf[1024];
  gettingFile(_url);
  mimeType(mimetype);
  ssize_t read_ret=0;
  size_t total_size=0;
  while ((read_ret=::read(m_iSock, buf, 1024))>0) {
      total_size+=read_ret;
      array.setRawData(buf, read_ret);
      data( array );
      array.resetRawData(buf, read_ret);
      totalSize(total_size);
  }
  processedSize(total_size);
  finished();
  gopher_close();
  finished();
  return true;
}

const char *GopherProtocol::abouttext=
"gopher  n.  1. Any of various short tailed, burrowing mammals of the\n"
"family Geomyidae, of North America.  2. (Amer. colloq.) Native or\n"
"inhabitant of Minnesota: the Gopher State.  3. (Amer. colloq.) One\n"
"who runs errands, does odd-jobs, fetches or delivers documents for\n"
"office staff.  4. (computer tech.) software following a simple\n"
"protocol for burrowing through a TCP/IP internet.";
