// $Id$

#include "pop3.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>

#include <kurl.h>
#include <kprotocolmanager.h>
#include <ksock.h>

#ifndef MAX
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif


bool open_PassDlg( const QString& _head, QString& _user, QString& _pass );

extern "C" {
  void sigsegv_handler(int);
  void sigchld_handler(int);
  void sigalrm_handler(int);
};

int main(int , char **)
{
  signal(SIGCHLD, sigchld_handler);
  signal(SIGSEGV, sigsegv_handler);

  Connection parent( 0, 1 );

  POP3Protocol pop3( &parent );
  pop3.dispatchLoop();
}

void sigsegv_handler(int )
{
  // Debug and printf should be avoided because they might
  // call malloc.. and get in a nice recursive malloc loop
  write(2, "kio_pop3 : ###############SEG FAULT#############\n", 49);
  exit(1);
}

void sigchld_handler(int)
{
  int pid, status;

  while(true) {
    pid = waitpid(-1, &status, WNOHANG);
    if ( pid <= 0 ) {
      // Reinstall signal handler, since Linux resets to default after
      // the signal occured ( BSD handles it different, but it should do
      // no harm ).
      signal(SIGCHLD, sigchld_handler);
      return;
    }
  }
}

POP3Protocol::POP3Protocol(Connection *_conn) : IOProtocol(_conn)
{
  m_cmd = CMD_NONE;
  m_pJob = 0L;
  m_iSock = m_iOldPort = 0;
  m_tTimeout.tv_sec=10;
  m_tTimeout.tv_usec=0;
  fp = 0;
}

bool POP3Protocol::getResponse (char *r_buf, unsigned int r_len)
{
  char buf[r_len ? r_len : 512];
  unsigned int recv_len=0;
  fd_set FDs;

  // Wait for something to come from the server
  FD_ZERO(&FDs);
  FD_SET(m_iSock, &FDs);

  // And keep waiting if it timed out
  while (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0);

  // Clear out the buffer
  memset(&buf, 0, r_len);
  // And grab the data
  if (fgets(buf, sizeof(buf)-1, fp) == 0)
    return false;
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

  if (strncmp(buf, "+OK ", 4)==0) {
    if (r_buf && r_len) {
      memcpy(r_buf, buf+4, MIN(r_len,recv_len-4));
    }
    return true;
  } else if (strncmp(buf, "-ERR ", 5)==0) {
    if (r_buf && r_len) {
      memcpy(r_buf, buf+5, MIN(r_len,recv_len-5));
    }
    return false;
  } else {
    fprintf(stderr, "Invalid POP3 response received!\n");fflush(stderr);
    if (r_buf && r_len) {
      memcpy(r_buf, buf, MIN(r_len,recv_len));
    }
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
  if (::write(m_iSock, cmd, strlen(cmd)) != (ssize_t)strlen(cmd))
    return false;
  if (::write(m_iSock, "\r\n", 2) != 2)
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
  if (fp) {
    (void)command("QUIT");
    fclose(fp);
    m_iSock=0; fp=0;
    m_sOldUser = ""; m_sOldPass = ""; m_sOldServer = "";
  }
}

bool POP3Protocol::pop3_open( KURL &_url )
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
  port = (_url.port() != -1) ? _url.port() : 110;
  if ( (m_iOldPort == port) && (m_sOldServer == _url.host()) && (m_sOldUser == _url.user()) && (m_sOldPass == _url.pass())) {
    fprintf(stderr,"Reusing old connection\n");fflush(stderr);
    return true;
  } else {
    pop3_close();
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

    if (!getResponse())  // If the server doesn't respond with a greeting
      return false;      // we've got major problems, and possibly the
                         // wrong port

    m_iOldPort = port;
    m_sOldServer = _url.host();

    QString usr, pass, one_string="USER ";
    if (_url.user().isEmpty() || _url.pass().isEmpty()) {
      // Prompt for usernames
      QString head="Username and password for your POP3 account:";
      if (!open_PassDlg(head, usr, pass)) {
	return false;
	pop3_close();
      } else {
	one_string.append(usr);
	m_sOldUser=usr;
      }
    } else {
      one_string.append(_url.user());
      m_sOldUser = _url.user();
    }
    memset(buf, 0, sizeof(buf));
    if (!command(one_string, buf, sizeof(buf))) {
      fprintf(stderr, "Couldn't login. Bad username Sorry\n"); fflush(stderr);
      pop3_close();
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
    if (!command(one_string, buf, sizeof(buf))) {
      fprintf(stderr, "Couldn't login. Bad password Sorry\n"); fflush(stderr);
      pop3_close();
      return false;
    }
    return true;

  }
}

void POP3Protocol::slotGetSize( const char * _url )
{
  // This should I deally call the totalSize function for the URL,
  // but I haven't really tested this.
  // bool ok=true;
  // static char buf[512];
  QString path, cmd;
  KURL usrc(_url);

  // We can't work on an invalid URL.
  if ( usrc.isMalformed() ) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    m_cmd = CMD_NONE;
    return;
  }

  if (usrc.protocol() != "pop") {
    error( ERR_INTERNAL, "kio_pop3 got non pop3 url" );
    m_cmd = CMD_NONE;
    return;
  }

  if (path.left(1)=="/") path.remove(0,1);
  if (path.isEmpty()) {
    debug("We should be a dir!!");
    error(ERR_IS_DIRECTORY, strdup(_url));
    m_cmd=CMD_NONE; return;
  }

  if (((path.find("/") == -1) && (path != "index")) ) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    m_cmd = CMD_NONE;
    return; 
  }

  cmd = path.left(path.find("/"));
  path.remove(0,path.find("/")+1);
  if (!pop3_open(usrc)) {
    fprintf(stderr,"pop3_open failed\n");fflush(stderr);
    pop3_close();
    return;
  }

//  if (cmd == "index") {

}

void POP3Protocol::slotGet(const char *_url)
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

  if (usrc.protocol() != "pop") {
    error( ERR_INTERNAL, "kio_pop3 got non pop3 url" );
    m_cmd = CMD_NONE;
    return;
  }

  path = usrc.path().copy();

  if (path.left(1)=="/") path.remove(0,1);
  if (path.isEmpty()) {
    debug("We should be a dir!!");
    error(ERR_IS_DIRECTORY, strdup(_url));
    m_cmd=CMD_NONE; return;
  }

  if (((path.find("/") == -1) && (path != "index")) ) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    m_cmd = CMD_NONE;
    return; 
  }

  cmd = path.left(path.find("/"));
  path.remove(0,path.find("/")+1);

  if (!pop3_open(usrc)) {
    fprintf(stderr,"pop3_open failed\n");fflush(stderr);
    pop3_close();
    return;
  }

  if (cmd == "index") {
    if (command("LIST")) {
      ready();
      gettingFile(_url);
      while (!feof(fp)) {
	memset(buf, 0, sizeof(buf));
	if (!fgets(buf, sizeof(buf)-1, fp))
	  break;  // Error??
	// HACK: This assumes fread stops at the first \n and not \r
	buf[strlen(buf)-2]='\0';
	if (strcmp(buf, ".")==0)  break; // End of data.	
	else {
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
		data(buf, strlen(buf));
	}
      }
      fprintf(stderr,"Finishing up list\n");fflush(stderr);
      dataEnd();
      speed(0); finished();
    }
  }

  else if (cmd == "headers") {
    (void)path.toInt(&ok);
    if (!ok) return; //  We fscking need a number!
    path.prepend("TOP ");
    path.append(" 0");
    if (command(path)) { // This should be checked, and a more hackish way of
                         // getting at the headers by d/l the whole message
                         // and stopping at the first blank line used if the
                         // TOP cmd isn't supported
      ready();
      gettingFile(_url);
      memset(buf, 0, sizeof(buf));
      while (!feof(fp)) {
	memset(buf, 0, sizeof(buf));
	if (!fgets(buf, sizeof(buf)-1, fp))
	  break;  // Error??
	// HACK: This assumes fread stops at the first \n and not \r
	buf[strlen(buf)-2]='\0';
	if (strcmp(buf, ".")==0)  break; // End of data.
	else if (strcmp(buf, "..")==0)
		data (".", 1);
	else
		data(buf, strlen(buf));
      }
      fprintf(stderr,"Finishing up\n");fflush(stderr);
      dataEnd();
      speed(0); finished();
    }
  }

  else if (cmd == "remove") {
    (void)path.toInt(&ok);
    if (!ok) return; //  We fscking need a number!
    path.prepend("DELE ");
    command(path);
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
    if (command(list_cmd, buf, sizeof(buf)-1)) {
      list_cmd=buf;
      // We need a space, otherwise we got an invalid reply
      if (!list_cmd.find(" ")) {
	debug("List command needs a space? %s", list_cmd.data());
        pop3_close();
        return;
      }
      list_cmd.remove(0, list_cmd.find(" ")+1);
      msg_len = list_cmd.toUInt(&ok);
      if (!ok) {
	debug("LIST command needs to return a number? :%s:", list_cmd.data());
	pop3_close();return;
      }
    } else {
      pop3_close(); return;
    }
    if (command(path)) {
      ready();
      gettingFile(_url);
      mimeType("message/rfc822");
      totalSize(msg_len);
      memset(buf, 0, sizeof(buf));
      while (!feof(fp)) {
	memset(buf, 0, sizeof(buf));
	if (!fgets(buf, sizeof(buf)-1, fp))
	  break;  // Error??
	// HACK: This assumes fread stops at the first \n and not \r
	buf[strlen(buf)-2]='\0';
	if (strcmp(buf, ".")==0)  break; // End of data.
	data(buf, strlen(buf));
	p_size+=strlen(buf);
	processedSize(p_size);
      }
      fprintf(stderr,"Finishing up\n");fflush(stderr);
      dataEnd();
      speed(0); finished();
    } else {
      fprintf(stderr, "Couldn't login. Bad RETR Sorry\n");
      fflush(stderr);
      pop3_close();
      return;
    }
  }
}

void POP3Protocol::slotListDir (const char *_url)
{
	debug( "kio_pop3 : listDir 1 %s", _url);
	KURL usrc( _url );
	if ( usrc.isMalformed() ) {
		error( ERR_MALFORMED_URL, _url );
		return;
	}

}

void POP3Protocol::slotTestDir (const char *_url)
{
        debug( "kio_pop3 : testing");
	KURL usrc(_url);
        debug( "kio_pop3 : testing");
	if (usrc.path() == "/")
		isDirectory();
	else
		isFile();
        debug( "kio_pop3 : testing");
	finished();
        debug( "kio_pop3 : testing");
}

void POP3Protocol::slotCopy(const char *, const char *)
{
  fprintf(stderr, "POP3Protocol::slotCopy\n");
  fflush(stderr);
}

void POP3Protocol::slotData(void *, int)
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

void POP3Protocol::slotDataEnd()
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

void POP3Protocol::jobData(void *, int )
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

void POP3Protocol::jobError(int _errid, const char *_text)
{
  error(_errid, _text);
}

void POP3Protocol::jobDataEnd()
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

void POP3Protocol::slotDel( QStringList&  )
{
}

/*************************************
 *
 * POP3IOJob
 *
 *************************************/

POP3IOJob::POP3IOJob(Connection *_conn, POP3Protocol *_pop3) :
	IOJob(_conn)
{
  m_pPOP3 = _pop3;
}
  
void POP3IOJob::slotData(void *_p, int _len)
{
  m_pPOP3->jobData( _p, _len );
}

void POP3IOJob::slotDataEnd()
{
  m_pPOP3->jobDataEnd();
}

void POP3IOJob::slotError(int _errid, const char *_txt)
{
  IOJob::slotError( _errid, _txt );
  m_pPOP3->jobError(_errid, _txt );
}
