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

#include "pop3.h"

#define APOP

#ifdef APOP
extern "C" {
#include "md5.h"
};
#endif

using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

#ifdef SPOP3

int SSL_readline(SSL *ssl, char *buf, int num) {
int c = 0;
  if (num <= 0) return -2;

  buf[num-1] = 0;

  for (c = 0; c < num-1; c++) {
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
  KInstance instance( "kio_pop3" );

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_pop3 protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  POP3Protocol *slave;
  if (strcasecmp(argv[1], "pop3s") == 0)
	slave = new POP3Protocol(argv[2], argv[3], true);
  else
	slave = new POP3Protocol(argv[2], argv[3], false);
  slave->dispatchLoop();
  delete slave;
  return 0;
}


POP3Protocol::POP3Protocol(const QCString &pool, const QCString &app, bool isSSL)
   : TCPSlaveBase((isSSL ? 995 : 110), (isSSL ? "pop3s" : "pop3"), pool, app)
{
  m_bIsSSL=isSSL;
  kdDebug() << "POP3Protocol()" << endl;
  m_cmd = CMD_NONE;
  m_iOldPort = 0;
  m_tTimeout.tv_sec=10;
  m_tTimeout.tv_usec=0;
  m_try_apop = true;
}

POP3Protocol::~POP3Protocol()
{
  kdDebug() << "~POP3Protocol()" << endl;
  pop3_close();
}

void POP3Protocol::setHost( const QString& _host, int _port, const QString& _user, const QString& _pass )
{
  m_sServer = _host;
  m_iPort = _port;
  m_sUser = _user;
  m_sPass = _pass;
}

bool POP3Protocol::getResponse (char *r_buf, unsigned int r_len)
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
    fprintf(stderr, "No response from POP3 server in 60 secs.\n");
    return false;
  }

  // Clear out the buffer
  memset(buf, 0, r_len);
  ReadLine(buf, r_len-1);

  // This is really a funky crash waiting to happen if something isn't
  // null terminated.
  recv_len=strlen(buf);

/*
 *   From rfc1939:
 *
 *   Responses in the POP3 consist of a status indicator and a keyword
 *   possibly followed by additional information.  All responses are
 *   terminated by a CRLF pair.  Responses may be up to 512 characters
 *   long, including the terminating CRLF.  There are currently two status
 *   indicators: positive ("+OK") and negative ("-ERR").  Servers MUST
 *   send the "+OK" and "-ERR" in upper case.
 */

  if (strncmp(buf, "+OK", 3)==0) {
    if (r_buf && r_len) {
      memcpy(r_buf, (buf[3] == ' ' ? buf+4 : buf+3),
             QMIN(r_len, (buf[3] == ' ' ? recv_len-4 : recv_len-3)));
    }
    if (buf) free(buf);
    return true;
  } else if (strncmp(buf, "-ERR", 4)==0) {
    if (r_buf && r_len) {
      memcpy(r_buf, (buf[4] == ' ' ? buf+5 : buf+4),
             QMIN(r_len, (buf[4] == ' ' ? recv_len-5 : recv_len-4)));
    }
    if (buf) free(buf);
    return false;
  } else {
    fprintf(stderr, "Invalid POP3 response received!\n");
    if (r_buf && r_len) {
      memcpy(r_buf, buf, QMIN(r_len,recv_len));
    }
    if (buf) free(buf);
    return false;
  }
}

bool POP3Protocol::command (const char *cmd, char *recv_buf, unsigned int len)
{

/*
 *   From rfc1939:
 *
 *   Commands in the POP3 consist of a case-insensitive keyword, possibly
 *   followed by one or more arguments.  All commands are terminated by a
 *   CRLF pair.  Keywords and arguments consist of printable ASCII
 *   characters.  Keywords and arguments are each separated by a single
 *   SPACE character.  Keywords are three or four characters long. Each
 *   argument may be up to 40 characters long.
 */

  // Write the command
  if (Write(cmd, strlen(cmd)) != (ssize_t)strlen(cmd))
    return false;
  if (Write("\r\n", 2) != 2)
    return false;
  return getResponse(recv_buf, len);
}

void POP3Protocol::pop3_close ()
{
  // If the file pointer exists, we can assume the socket is valid,
  // and to make sure that the server doesn't magically undo any of
  // our deletions and so-on, we should send a QUIT and wait for a
  // response.  We don't care if it's positive or negative.  Also
  // flush out any semblance of a persistant connection, i.e.: the
  // old username and password are now invalid.
  (void)command("QUIT");
  CloseDescriptor();
  m_sOldUser = ""; m_sOldPass = ""; m_sOldServer = "";
}

bool POP3Protocol::pop3_open()
{
  static char buf[512];
  if ( (m_iOldPort == GetPort(m_iPort)) && (m_sOldServer == m_sServer) &&
       (m_sOldUser == m_sUser) && (m_sOldPass == m_sPass)) {
    fprintf(stderr,"Reusing old connection\n");
    return true;
  } else {
    pop3_close();
    ConnectToHost(m_sServer.ascii(), m_iPort);

    QCString greeting( 1024 );
    // If the server doesn't respond with a greeting
    if (!getResponse(greeting.data(), greeting.size()))
      return false;      // we've got major problems, and possibly the
                         // wrong port

#ifdef APOP
    //
    // Does the server support APOP?
    //
    QString apop_cmd;
    QRegExp re("<[A-Za-z0-9\\.\\-_]+@[A-Za-z0-9\\.\\-_]+>$", false);
    if(greeting.length() > 0)
      greeting.truncate(greeting.length() - 2);
    int apop_pos = greeting.find(re);
    bool apop = (bool)(apop_pos != -1);
#endif

    m_iOldPort = m_iPort;
    m_sOldServer = m_sServer;

    QString usr, pass, one_string="USER ";
#ifdef APOP
    QString apop_string = "APOP ";
#endif
    if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
      // Prompt for usernames
      QString head=i18n("Username and password for your POP3 account:");
      if (!openPassDlg(head, usr, pass)) {
	pop3_close();
	return false;
      } else {
#ifdef APOP
	apop_string.append(usr);
#endif
	one_string.append(usr);
	m_sOldUser=usr;
      }
    } else {
#ifdef APOP
      apop_string.append(m_sUser);
#endif
      one_string.append(m_sUser);
      m_sOldUser = m_sUser;
    }

    memset(buf, 0, sizeof(buf));
#ifdef APOP
    if(apop && m_try_apop) {
      char *c = greeting.data() + apop_pos;
      unsigned char digest[16];
      char ascii_digest[33];
      Bin_MD5Context ctx;

      if ( m_sPass.isEmpty())
	m_sOldPass = pass;
      else
	m_sOldPass = m_sPass;

      // Generate digest
      Bin_MD5Init(&ctx);
      Bin_MD5Update(&ctx,
		    (unsigned char *)c,
		    (unsigned)strlen(c));
      Bin_MD5Update(&ctx,
		    (unsigned char *)m_sOldPass.data(),
		    (unsigned)m_sOldPass.length());
      Bin_MD5Final(digest, &ctx);
      for(int i = 0; i < 16; i++)
	sprintf(ascii_digest+2*i, "%02x", digest[i]);

      // Genenerate APOP command
      apop_string.append(" ");
      apop_string.append(ascii_digest);

      if(command(apop_string, buf, sizeof(buf)))
	return true;

      fprintf(stderr, "Couldn't login via APOP. Falling back to USER/PASS\n");
      pop3_close();
      m_try_apop = false;
      return pop3_open();
    }
#endif

    // Fall back to conventional USER/PASS scheme
    if (!command(one_string, buf, sizeof(buf))) {
      fprintf(stderr, "Couldn't login. Bad username Sorry\n");
      error( ERR_COULD_NOT_CONNECT, "Couldn't login. Bad username Sorry\n" );
      pop3_close();
      return false;
    }

    one_string="PASS ";
    if (m_sPass.isEmpty()) {
      m_sOldPass = pass;
      one_string.append(pass);
    } else {
      m_sOldPass = m_sPass;
      one_string.append(m_sPass);
    }
    if (!command(one_string, buf, sizeof(buf))) {
      fprintf(stderr, "Couldn't login. Bad password Sorry\n");
      error( ERR_COULD_NOT_CONNECT, "Couldn't login. Bad password Sorry\n" );
      pop3_close();
      return false;
    }
    return true;
  }
}

size_t POP3Protocol::realGetSize(unsigned int msg_num)
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

void POP3Protocol::get( const KURL& url )
{
// List of supported commands
//
// URI                                 Command   Result
// pop3://user:pass@domain/index       LIST      List message sizes
// pop3://user:pass@domain/uidl        UIDL      List message UIDs
// pop3://user:pass@domain/remove/#1   DELE #1   Mark a message for deletion
// pop3://user:pass@domain/download/#1 RETR #1   Get message header and body
// pop3://user:pass@domain/list/#1     LIST #1   Get size of a message
// pop3://user:pass@domain/uid/#1      UIDL #1   Get UID of a message
// pop3://user:pass@domain/commit      QUIT      Delete marked messages
// pop3://user:pass@domain/headers/#1  TOP #1    Get header of message
//
// Notes:
// Sizes are in bytes.
// No support for the STAT command has been implemented.
// commit closes the connection to the server after issuing the QUIT command.

  bool ok=true;
  static char buf[512];
  QByteArray array;

  QString cmd;
  QString path = url.path();

  if (path.at(0)=='/') path.remove(0,1);
  if (path.isEmpty()) {
    kdDebug() << "We should be a dir!!" << endl;
    error(ERR_IS_DIRECTORY, url.url());
    m_cmd=CMD_NONE; return;
  }

  if (((path.find('/') == -1) && (path != "index") &&
       (path != "uidl") && (path != "commit")) ) {
    error( ERR_MALFORMED_URL, url.url() );
    m_cmd = CMD_NONE;
    return;
  }

  cmd = path.left(path.find('/'));
  path.remove(0,path.find('/')+1);

  if (!pop3_open()) {
    fprintf(stderr,"pop3_open failed\n");
    error( ERR_COULD_NOT_CONNECT, m_sServer);
    pop3_close();
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
#ifdef SPOP3
      while (SSL_pending(ssl)) {
        memset(buf, 0, sizeof(buf));
        SSL_readline(ssl, buf, sizeof(buf)-1);
#else
      while (!feof(fp)) {
	memset(buf, 0, sizeof(buf));
	if (!fgets(buf, sizeof(buf)-1, fp))
	  break;  // Error??
#endif
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
    }
    fprintf(stderr,"Finishing up list\n");
    data( QByteArray() );
    speed(0); finished();
  }

  else if (cmd == "headers") {
    (void)path.toInt(&ok);
    if (!ok)
    {
       error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
       return; //  We fscking need a number!
    }
    path.prepend("TOP ");
    path.append(" 0");
    if (command(path)) { // This should be checked, and a more hackish way of
                         // getting at the headers by d/l the whole message
                         // and stopping at the first blank line used if the
                         // TOP cmd isn't supported
      //ready();
      gettingFile(url.url());
      mimeType("text/plain");
      memset(buf, 0, sizeof(buf));
      while (AtEOF()) {
        memset(buf, 0, sizeof(buf));
	ReadLine(buf, sizeof(buf)-1);

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
      speed(0);finished();
    }
  }

  else if (cmd == "remove") {
    (void)path.toInt(&ok);
    if (!ok)
    {
       error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
       return; //  We fscking need a number!
    }
    path.prepend("DELE ");
    command(path);
    finished();
    m_cmd = CMD_NONE;
  }

  else if (cmd == "download") {
    int p_size=0;
    unsigned int msg_len=0;
    (void)path.toInt(&ok);
    QString list_cmd("LIST ");
    if (!ok)
    {
       error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
       return; //  We fscking need a number!
    }
    list_cmd+= path;
    path.prepend("RETR ");
    memset(buf, 0, sizeof(buf));
    if (command(list_cmd, buf, sizeof(buf)-1)) {
      list_cmd=buf;
      // We need a space, otherwise we got an invalid reply
      if (!list_cmd.find(" ")) {
	kdDebug(7105) << "List command needs a space? " << list_cmd << endl;
        pop3_close();
        error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
        return;
      }
      list_cmd.remove(0, list_cmd.find(" ")+1);
      msg_len = list_cmd.toUInt(&ok);
      if (!ok) {
	kdDebug(7105) << "LIST command needs to return a number? :" << list_cmd << ":" << endl;
	pop3_close();
        error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
	return;
      }
    } else {
      pop3_close();
      error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
      return;
    }
    if (command(path)) {
      //ready();
      gettingFile(url.url());
      mimeType("message/rfc822");
      totalSize(msg_len);
      memset(buf, 0, sizeof(buf));
#ifdef SPOP3
      while (SSL_pending(ssl)) {
        memset(buf, 0, sizeof(buf));
        SSL_readline(ssl, buf, sizeof(buf)-1);
#else
      while (!feof(fp)) {
	memset(buf, 0, sizeof(buf));
	if (!fgets(buf, sizeof(buf)-1, fp))
	  break;  // Error??
#endif
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
      pop3_close();
      error( ERR_INTERNAL, i18n("Couldn't login."));
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
    if (command(path, buf, sizeof(buf)-1)) {
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
      pop3_close();
      error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
      return;
    }
  }

  else if (cmd == "commit") {
    fprintf(stderr,"Issued QUIT\n");
    pop3_close();
    finished();
    m_cmd = CMD_NONE;
    return;
  }

}

void POP3Protocol::listDir( const KURL & /* url*/ )
{
  bool isINT; int num_messages=0;
  char buf[512];
  QCString q_buf;

  // Try and open a connection
  if (!pop3_open()) {
#ifdef SPOP3
    fprintf(stderr,"spop3_open failed\n");
#else
    fprintf(stderr,"pop3_open failed\n");
#endif
    error( ERR_COULD_NOT_CONNECT, m_sServer);
    pop3_close();
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
    error(ERR_INTERNAL, "Invalid POP3 response, we should have at least one space!");
    pop3_close();
    return;
  }
  q_buf.remove(q_buf.find(" "), q_buf.length());

  num_messages=q_buf.toUInt(&isINT);
  if (!isINT) {
    error(ERR_INTERNAL, "Invalid POP3 STAT response!");
    pop3_close();
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
    KURL uds_url;
#ifdef SPOP3
    uds_url.setProtocol("spop3");
#else
    uds_url.setProtocol("pop3");
#endif
    uds_url.setUser(m_sUser);
    uds_url.setPass(m_sPass);
    uds_url.setHost(m_sServer);
    uds_url.setPath(QString::fromLatin1("/download/%1").arg(i+1));
    atom.m_str = uds_url.url();
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

void POP3Protocol::stat( const KURL & url )
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

void POP3Protocol::del( const KURL& url, bool /*isfile*/ )
{
  QString invalidURI=QString::null;
  bool isInt;

  if ( !pop3_open() ) {
#ifdef SPOP3
    fprintf(stderr,"spop3_open failed\n");
#else
    fprintf(stderr,"pop3_open failed\n");
#endif
    error( ERR_COULD_NOT_CONNECT, m_sServer );
    pop3_close();
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
    if (!command(_path)) {
      invalidURI=_path;
    }
  }

  kdDebug() << "POP3Protocol::del " << _path << endl;
  finished();
}
