// $Id$
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
#include <kio/passdlg.h>

#include <klocale.h>

#include "nntp.h"

using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv)
{
        KInstance instance( "kio_nntp" );
        if (argc != 4)
        {
        fprintf(stderr, "usage statement needs to go here\n");
        exit(-1);
        }
        NNTPProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();
        return 0;
}

NNTPProtocol::NNTPProtocol(const QCString &pool, const QCString &app)
  : SlaveBase( "nntp", pool, app)
{
  kdDebug() << "NNTPProtocol::NNTPProtocol" << endl;
  m_cmd = CMD_NONE;
  m_iSock = m_iOldPort = 0;
  m_tTimeout.tv_sec=10;
  m_tTimeout.tv_usec=0;
  fp = 0;
}

NNTPProtocol::~NNTPProtocol()
{
  kdDebug() << "NNTPProtocol::~NNTPProtocol" << endl;
  nntp_close();
}

bool NNTPProtocol::getResponse (char *r_buf, unsigned int r_len)
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
    fprintf(stderr, "No response from NNTP server in 60 secs.\n");fflush(stderr);
    return false;
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

/*   CJM: CHANGE THIS!!!!
 *   From rfc1939:
 *
 *   Responses in the NNTP consist of a status indicator and a keyword
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
    fprintf(stderr, "Invalid NNTP response received!\n");
    if (r_buf && r_len) {
      memcpy(r_buf, buf, QMIN(r_len,recv_len));
    }
    if (buf) free(buf);
    return false;
  }
}

bool NNTPProtocol::command (const char *cmd, char *recv_buf, unsigned int len)
{

/*   CJM: CHANGE THIS!!!
 *   From rfc1939:
 *
 *   Commands in the NNTP consist of a case-insensitive keyword, possibly
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

void NNTPProtocol::nntp_close ()
{
  // If the file pointer exists, we can assume the socket is valid,
  // and to make sure that the server doesn't magically undo any of
  // our deletions and so-on, we should send a QUIT and wait for a
  // response.  We don't care if it's positive or negative.  Also
  // flush out any semblance of a persistant connection, i.e.: the
  // old username and password are now invalid.
  if (fp) {
    (void)command("QUIT");
    fclose(fp);
    m_iSock=0; fp=0;
    m_sOldUser = ""; m_sOldPass = ""; m_sOldServer = "";
  }
}

bool NNTPProtocol::nntp_open( const KURL &_url )
{
  // This function is simply a wrapper to establish the connection
  // to the server.  It's a bit more complicated than ::connect
  // because we first have to check to see if the user specified
  // a port, and if so use it, otherwise we check to see if there
  // is a port specified in /etc/services, and if so use that
  // otherwise as a last resort use the "official" port of 119.

  unsigned short int port;
  ksockaddr_in server_name;
  memset(&server_name, 0, sizeof(server_name));
  static char buf[512];

  // We want 119 as the default, but -1 means no port was specified.
  // Why 0 wasn't chosen is beyond me.
  port = _url.port() ? _url.port() : 119;
  if ( (m_iOldPort == port) && (m_sOldServer == _url.host()) && (m_sOldUser == _url.user()) && (m_sOldPass == _url.pass())) {
    fprintf(stderr,"Reusing old connection\n");
    return true;
  } else {
    nntp_close();
    m_iSock = ::socket(PF_INET, SOCK_STREAM, 0);
    if (!KSocket::initSockaddr(&server_name, _url.host().local8Bit(), port))
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
      return false;
    }

    QCString greeting (1024);
    if (!getResponse())  // If the server doesn't respond with a greeting
      return false;      // we've got major problems, and possibly the
                         // wrong port

    m_iOldPort = port;
    m_sOldServer = _url.host();

    QString usr, pass, one_string="USER ";
    if (_url.user().isEmpty() || _url.pass().isEmpty()) {
      // Prompt for usernames
      QString head=i18n("Username and password for your NNTP account:");
      if (!openPassDlg(head, usr, pass)) {
        return false;
        nntp_close();
      } else {
        one_string.append(usr);
        m_sOldUser=usr;
      }
    } else {
      one_string.append(_url.user());
      m_sOldUser = _url.user();
    }

    memset(buf, 0, sizeof(buf));

    if (!command(one_string.local8Bit(), buf, sizeof(buf))) {
      fprintf(stderr, "Couldn't login. Bad username Sorry\n");
      nntp_close();
      return false;
    }

    one_string="PASS ";
    if (_url.pass().isEmpty()) {
      m_sOldPass = pass;
      one_string.append(pass);
    } else {
      m_sOldPass = _url.pass();
      one_string.append(_url.pass());
    }
    if (!command(one_string.local8Bit(), buf, sizeof(buf))) {
      fprintf(stderr, "Couldn't login. Bad password Sorry\n");
      nntp_close();
      return false;
    }
    return true;
  }
}

size_t NNTPProtocol::realGetSize(unsigned int msg_num)
{
  char *buf;
  QCString cmd;
  size_t ret=0;
  buf=(char *)malloc(512);
  memset(buf, 0, 512);
  cmd.sprintf("LIST %u", msg_num);
  if (!command(cmd.data(), buf, 512)) {
    free(buf);
    return 0;
  } else {
    cmd=buf;
    cmd.remove(0, cmd.find(" "));
    ret=cmd.toLong();
  }
  free(buf);
  return ret;
}

void NNTPProtocol::get( const KURL& url )
{
// List of supported commands
//
// URI                                 Command   Result
// news://user:pass@domain/index       LIST      List message sizes
// news://user:pass@domain/uidl        UIDL      List message UIDs
// news://user:pass@domain/remove/#1   DELE #1   Mark a message for deletion
// news://user:pass@domain/download/#1 RETR #1   Get message header and body
// news://user:pass@domain/list/#1     LIST #1   Get size of a message
// news://user:pass@domain/uid/#1      UIDL #1   Get UID of a message
// news://user:pass@domain/commit      QUIT      Delete marked messages
// news://user:pass@domain/headers/#1  TOP #1    Get header of message
//
// Notes:
// Sizes are in bytes.
// No support for the STAT command has been implemented.
// commit closes the connection to the server after issuing the QUIT command.

  bool ok=true;
  static char buf[512];
  QByteArray array;
  QString path, cmd;

  path = url.path();

  if (path.at(0)=='/') path.remove(0,1);
  if (path.isEmpty()) {
    kdDebug() << "We should be a dir!!" << endl;
    error(ERR_IS_DIRECTORY, url.url());
    m_cmd=CMD_NONE; return;
  }

  if (((path.find("/") == -1) && (path != "index") &&
       (path != "uidl") && (path != "commit")) ) {
    error( ERR_MALFORMED_URL, url.url() );
    m_cmd = CMD_NONE;
    return;
  }

  cmd = path.left(path.find("/"));
  path.remove(0,path.find("/")+1);

  if (!nntp_open(url)) {
    fprintf(stderr,"nntp_open failed\n");
    error( ERR_COULD_NOT_CONNECT, url.host());
    nntp_close();
    return;
  }

  if ((cmd == "index") || (cmd == "uidl")) {
    unsigned long size=0;
    bool result;
    if (cmd == "index")
      result = command("LIST");
    else
      result = command("UIDL");
    if (result) {
      //ready();
      gettingFile(url.url());
      while (!feof(fp)) {
        memset(buf, 0, sizeof(buf));
        if (!fgets(buf, sizeof(buf)-1, fp))
          break;  // Error??
        // HACK: This assumes fread stops at the first \n and not \r
        if (strcmp(buf, ".\r\n")==0) break; // End of data
        // sanders, changed -2 to -1 below
        buf[strlen(buf)-2]='\0';
/*
LIST
+OK Mailbox scan listing follows
1 2979
2 1348
3 1213
4 1286
5 2363
6 1410
7 2048
8 958
9 91684
10 3770
11 1547
12 649
.
*/
        size+=strlen(buf);
        array.setRawData(buf, strlen(buf));
        data( array );
        array.resetRawData(buf, strlen(buf));
        totalSize(size);
      }
      fprintf(stderr,"Finishing up list\n");
      data( QByteArray() );
      speed(0); finished();
    }
  }

  else if (cmd == "headers") {
    (void)path.toInt(&ok);
    if (!ok) return; //  We fscking need a number!
    path.prepend("TOP ");
    path.append(" 0");
    if (command(path.ascii())) { // This should be checked, and a more hackish way of
                         // getting at the headers by d/l the whole message
                         // and stopping at the first blank line used if the
                         // TOP cmd isn't supported
      //ready();
      gettingFile(url.url());
      mimeType("text/plain");
      memset(buf, 0, sizeof(buf));
      while (!feof(fp)) {
        fprintf(stderr,"xxxxxxxxxxxFinishing up\n");
        memset(buf, 0, sizeof(buf));
        if (!fgets(buf, sizeof(buf)-1, fp))
          break;  // Error??

        // HACK: This assumes fread stops at the first \n and not \r
        if (strcmp(buf, ".\r\n")==0) break; // End of data
        // sanders, changed -2 to -1 below
        buf[strlen(buf)-1]='\0';
        if (strcmp(buf, "..")==0) {
          buf[0] = '.';
          array.setRawData(buf, 1);
          data( array );
          array.resetRawData(buf, 1);
        }
        else {
          array.setRawData(buf, strlen(buf));
          data( array );
          array.resetRawData(buf, strlen(buf));
        }
      }
      fprintf(stderr,"Finishing up\n");
      data( QByteArray() );
      speed(0); finished();
    }
  }

  else if (cmd == "remove") {
    (void)path.toInt(&ok);
    if (!ok) return; //  We fscking need a number!
    path.prepend("DELE ");
    command(path.ascii());
    finished();
    m_cmd = CMD_NONE;
  }

  else if (cmd == "download") {
    int p_size=0;
    unsigned int msg_len=0;
    (void)path.toInt(&ok);
    QString list_cmd("LIST ");
    if (!ok)
      return; //  We fscking need a number!
    list_cmd+= path;
    path.prepend("RETR ");
    memset(buf, 0, sizeof(buf));
    if (command(list_cmd.ascii(), buf, sizeof(buf)-1)) {
      list_cmd=buf;
      // We need a space, otherwise we got an invalid reply
      if (!list_cmd.find(" ")) {
        kdDebug(7105) << "List command needs a space? " << list_cmd << endl;
        nntp_close();
        return;
      }
      list_cmd.remove(0, list_cmd.find(" ")+1);
      msg_len = list_cmd.toUInt(&ok);
      if (!ok) {
        kdDebug(7105) << "LIST command needs to return a number? :" << list_cmd << ":" << endl;
        nntp_close();return;
      }
    } else {
      nntp_close(); return;
    }
    if (command(path.ascii())) {
      //ready();
      gettingFile(url.url());
      mimeType("message/rfc822");
      totalSize(msg_len);
      memset(buf, 0, sizeof(buf));
      while (!feof(fp)) {
        memset(buf, 0, sizeof(buf));
        if (!fgets(buf, sizeof(buf)-1, fp))
          break;  // Error??
        // HACK: This assumes fread stops at the first \n and not \r
        if (strcmp(buf, ".\r\n")==0) break; // End of data
        // sanders, changed -2 to -1 below
        buf[strlen(buf)-1]='\0';
        array.setRawData(buf, strlen(buf));
        data( array );
        array.resetRawData(buf, strlen(buf));
        p_size+=strlen(buf);
        processedSize(p_size);
      }
      fprintf(stderr,"Finishing up\n");
      data(QByteArray());
      speed(0); finished();
    } else {
      fprintf(stderr, "Couldn't login. Bad RETR Sorry\n");
      nntp_close();
      return;
    }
  }

  else if ((cmd == "uid") || (cmd == "list")) {
    QString qbuf;
    (void)path.toInt(&ok);
    if (!ok) return; //  We fscking need a number!
    if (cmd == "uid")
      path.prepend("UIDL ");
    else
      path.prepend("LIST ");
    memset(buf, 0, sizeof(buf));
    if (command(path.ascii(), buf, sizeof(buf)-1)) {
      const int len = strlen(buf);
      //ready();
      gettingFile(url.url());
      mimeType("text/plain");
      totalSize(len);
      array.setRawData(buf, len);
      data( array );
      array.resetRawData(buf, len);
      processedSize(len);
      kdDebug() << buf << endl;
      fprintf(stderr,"Finishing up uid\n");
      data(QByteArray());
      speed(0); finished();
    } else {
      nntp_close(); return;
    }
  }

  else if (cmd == "commit") {
    fprintf(stderr,"Issued QUIT\n");
    nntp_close();
    finished();
    m_cmd = CMD_NONE;
    return;
  }

}

void NNTPProtocol::listDir( const KURL& url)
{
  bool isINT; int num_messages=0;
  char buf[512];
  QCString q_buf;
  KURL usrc( url );
  // Try and open a connection
  if (!nntp_open(usrc)) {
    fprintf(stderr,"nntp_open failed\n");
    error( ERR_COULD_NOT_CONNECT, usrc.host());
    nntp_close();
    return;
  }
  // Check how many messages we have. STAT is by law required to
  // at least return +OK num_messages total_size
  memset(buf, 0, 512);
  if (!command("STAT", buf, 512)) {
    error(ERR_INTERNAL, "??");
    return;
  }
  fprintf(stderr,"The stat buf is :%s:\n", buf);
  q_buf=buf;
  if (q_buf.find(" ")==-1) {
    error(ERR_INTERNAL, "Invalid NNTP response, we should have at least one space!");
    nntp_close();
    return;
  }
  q_buf.remove(q_buf.find(" "), q_buf.length());

  num_messages=q_buf.toUInt(&isINT);
  if (!isINT) {
    error(ERR_INTERNAL, "Invalid NNTP STAT response!");
    nntp_close();
    return;
  }
  UDSEntry entry;
  UDSAtom atom;
  QString fname;
  for (int i=0; i < num_messages; i++) {
    fname="Message %1";

    atom.m_uds = UDS_NAME;
    atom.m_long = 0;
    atom.m_str = fname.arg(i+1);
    entry.append(atom);

    atom.m_uds = UDS_MIME_TYPE;
    atom.m_long = 0;
    atom.m_str = "text/plain";
    entry.append(atom);
    fprintf(stderr,"Mimetype is %s\n", atom.m_str.ascii());

    atom.m_uds = UDS_URL;
    QString uds_url;
    if (usrc.user().isEmpty() || usrc.pass().isEmpty()) {
      uds_url="news://%1/download/%2";
      atom.m_str = uds_url.arg(usrc.host()).arg(i+1);
    } else {
      uds_url="news://%1:%2@%3/download/%3";
      atom.m_str = uds_url.arg(usrc.user()).arg(usrc.pass()).arg(usrc.host()).arg(i+1);
    }
    atom.m_long = 0;
    entry.append(atom);
    fprintf(stderr,"URL is %s\n", atom.m_str.ascii());

    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    atom.m_long = S_IFREG;
    entry.append(atom);

    atom.m_uds = UDS_SIZE;
    atom.m_str = "";
    atom.m_long = realGetSize(i+1);
    entry.append(atom);

    listEntry(entry, false);
    entry.clear();
  }
  listEntry( entry, true ); // ready

  finished();
}

void NNTPProtocol::stat( const KURL& url )
{
  QString _path = url.path();
  if (_path.at(0) == '/')
    _path.remove(0,1);

  UDSEntry entry;
  UDSAtom atom;
  atom.m_uds = KIO::UDS_NAME;
  atom.m_str = _path;
  entry.append( atom );

  // TODO: maybe get the size of the message?

  statEntry( entry );
  finished();
}

void NNTPProtocol::del( const KURL& url, bool /*isfile*/ )
{
  QString invalidURI=QString::null;
  bool isInt;

  if ( !nntp_open(url) ) {
    fprintf(stderr,"nntp_open failed\n");
    error( ERR_COULD_NOT_CONNECT, url.host());
    nntp_close();
    return;
  }

  QString _path = url.path();
  if (_path.at(0) == '/')
    _path.remove(0,1);
  (void)_path.toUInt(&isInt);
  if (!isInt) {
    invalidURI=_path;
  } else {
    _path.prepend("DELE ");
    if (!command(_path.ascii())) {
      invalidURI=_path;
    }
  }

  kdDebug() << "NNTPProtocol::del " << _path << endl;
  finished();
}


