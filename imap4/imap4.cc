// $Id$

#include "imap4.h"

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

#include <qregexp.h>

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

int main(int argc, char **argv)
{
  signal(SIGCHLD, sigchld_handler);
  signal(SIGSEGV, sigsegv_handler);

  Connection parent( 0, 1 );

  IMAP4Protocol imap4( &parent );
  imap4.dispatchLoop();
}

void sigsegv_handler(int signo)
{
  // Debug and printf should be avoided because they might
  // call malloc.. and get in a nice recursive malloc loop
  write(2, "kio_imap4 : ###############SEG FAULT#############\n", 50);
  exit(1);
}

void sigchld_handler(int signo)
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

IMAP4Protocol::IMAP4Protocol(Connection *_conn) : IOProtocol(_conn)
{
  m_pJob = 0L;
  m_iSock =  m_uLastCmd = 0;
  m_tTimeout.tv_sec=10;
  m_tTimeout.tv_usec=0;
  pending.clear();
}


unsigned int IMAP4Protocol::command (enum IMAP_COMMAND type, const char *_args)
{
  QString cmd_str;
  m_uLastCmd++;
  cmd_str.sprintf("KONQ%u", m_uLastCmd);
  CMD_Struct *cmd_s = new CMD_Struct;

  cmd_s->identifier = cmd_str.copy();
  cmd_s->type = type;
  cmd_s->args = _args;
  cmd_s->sent = false;

  pending.append(cmd_s);

  return m_uLastCmd;
}

void IMAP4Protocol::imap4_close ()
{
  m_uLastCmd=0;
}

bool IMAP4Protocol::imap4_open( KURL &_url )
{
  unsigned short int port;
  struct sockaddr_in server_name;
  struct servent *sent;
  QString login_str;
  memset(&server_name, sizeof(server_name), 0);

  // We want 143 as the default, but -1 means no port was specified.
  // Why 0 wasn't chosen is beyond me.
  sent = getservbyname("imap4", "tcp");
  port = (_url.port() != 0) ? _url.port() : (sent ? ntohs(sent->s_port) : 143);

  m_iSock = ::socket(PF_INET, SOCK_STREAM, 0);
  if (!KSocket::initSockaddr(&server_name, _url.host(), port))
    return false;
  if (::connect(m_iSock, (struct sockaddr*)(&server_name), sizeof(server_name))) {
      error( ERR_COULD_NOT_CONNECT, strdup(_url.host()));
      return false;
  }
  if ((fp = fdopen(m_iSock, "w+")) == 0) {
    close(m_iSock);
    return false;
  }
  if (!_url.user().isEmpty() && !_url.pass().isEmpty()) {
    login_str.append(_url.user());
    login_str.append(" ");
    login_str.append(_url.pass());
  }

  command(ICMD_LOGIN, login_str.ascii());
  command(ICMD_SELECT, "INBOX");
  return true;
}

void IMAP4Protocol::slotGet(const char *_url)
{
  fprintf(stderr,"slotGet\n"); fflush(stderr);
  bool ok;
  QString path, cmd;
  KURL usrc(_url);
  if ( usrc.isMalformed() ) {
    fprintf(stderr,"slotGet2\n"); fflush(stderr);
    error( ERR_MALFORMED_URL, strdup(_url) );
    // M_CMD needs to be cmd_none
    fprintf(stderr,"slotGet2\n"); fflush(stderr);
    return;
  }

  if (usrc.protocol() != "imap") {
    fprintf(stderr,"slotGet3\n"); fflush(stderr);
    error( ERR_INTERNAL, "kio_imap4 got non imap4 url" );
    // M_CMD needs to be cmd_none
    return;
  }

  path = usrc.path().copy();

  if (path.left(1)=="/") path.remove(0,1);
  if (path.isEmpty() || (path.find("/") == -1)) {
    error( ERR_MALFORMED_URL, strdup(_url) );
    // M_CMD needs to be cmd_none
    return; 
  }

  cmd = path.left(path.find("/"));
  path.remove(0,path.find("/")+1);

  if (!imap4_open(usrc)) {
    fprintf(stderr,"imap4_open failed\n");fflush(stderr);
    imap4_close();
    return;
  }

  if (cmd == "index") {
  } else if (cmd == "download") {
    debug("We wanna download damnit :%s:", path.ascii());
    (void)path.toUInt(&ok);
    if (!ok)
      exit(0);
    command(ICMD_FETCH, QString("RFC822.SIZE ")+path);
    command(ICMD_FETCH, QString("RFC822 ")+path);
  }
  startLoop();
}

void IMAP4Protocol::slotPut(const char *_url, int _mode, bool _overwrite,
			    bool _resume, unsigned int)
{
}

void IMAP4Protocol::startLoop ()
{
  char buf[1024];
  bool gotGreet = false, loggedin=false, isNum=false;
  struct timeval m_tTimeout = {1, 0};
  FILE *fp = fdopen(m_iSock, "w+");
  QString s_buf, s_identifier;
  fd_set FDs;
  while (1) {
    fprintf(stderr,"Top of the loop\n");fflush(stderr);
    FD_ZERO(&FDs);
    FD_SET(m_iSock, &FDs);
    QString s_cmd;
    memset(&buf, sizeof(buf), 0);
    //
    if (fgets(buf, sizeof(buf)-1, fp) == 0) {
      if (ferror(fp)) {
	fprintf(stderr,"Error while freading something\n");fflush(stderr);
	break;
      } else {
	while (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0) {fprintf(stderr,"Sleeping\n");fflush(stderr);sleep(10);}
	if (fgets(buf, sizeof(buf)-1, fp) == 0) {
	  fprintf(stderr,"Error while freading something, and yes we already waited on select\n");fflush(stderr);
	  break;
	}
      }  
    }
    s_buf=buf;
    if (s_buf.find(" ") == -1) {
      fprintf(stderr,"Got an invalid response: %s\n", buf);fflush(stderr);
      break;
    }
    s_identifier=s_buf.mid(0, s_buf.find(" "));
    s_buf.remove(0, s_buf.find(" ")+1);
    //fprintf(stderr,"First token is :%s:\n", s_identifier.data());fflush(stderr);
    if (s_identifier == "*") {
      fprintf(stderr,"Got a star\n"); fflush(stderr);
      if (!gotGreet) {
	// We just kinda assume it's the greeting
	gotGreet=true;
	sendNextCommand();
      }
      QString s_token;
      if (s_buf.find(" ") == -1) {
        if ( (s_buf != "NO") && (s_buf != "OK")) {
	  fprintf(stderr,"We got a weird response, :%s:\n", buf);fflush(stderr);
	}  
      } else {
	s_token = s_buf.mid(0, s_buf.find(" "));
	s_buf.remove(0, s_buf.find(" ")+1);
	//fprintf(stderr,"Token is:%s:\n", s_token.ascii());fflush(stderr);
	(void)s_token.toInt(&isNum);

	QRegExp r("\r\n");
	s_buf.replace(r, "");

	if (isNum) {
	  if (s_buf.find(" ") != -1) {
	    s_cmd = s_buf.mid(0, s_buf.find(" "));
	    s_buf.remove(0, s_buf.find(" ")+1);
	  } else {
	    s_cmd = s_buf.copy();
	    s_buf = "";
	  }
	  if (s_cmd == "EXISTS") {
	    fprintf(stderr, "%s messages exist in the current mbx\n", s_token.ascii()); fflush(stderr);
	  } else if (s_cmd == "RECENT") {
	    fprintf(stderr, "%s messages have the recent flag in current mbx\n", s_token.ascii()); fflush(stderr);
	  } else if (s_cmd == "FETCH") {
	    ;
	  } else {
	    fprintf(stderr,"Got unknown untokened response :%s:\n", buf); fflush(stderr);
	  }
	} else {
	  if (s_token == "FLAGS") {
	    fprintf(stderr,"Flags are :%s:\n", s_buf.ascii()); fflush(stderr);
	  }
	} else {
// 	  if (s_buf.find(" ") != -1)
// 	    s_cmd = s_buf.mid(0, s_buf.find(" "));
// 	  else
// 	    s_cmd = s_buf.copy();
// 	  if (s_cmd = 
	}
      }
    } else if (s_identifier == "+") {
      fprintf(stderr,"Got a plus\n");fflush(stderr);
    } else {
      fprintf(stderr,"Looking for a match\n");fflush(stderr);
      pending.first();
      while (pending.current()) {
	//fprintf(stderr,"p.c.i==:%s: s_i==:%s:\n", pending.current()->identifier.ascii(), s_identifier.ascii());fflush(stderr);
	if (pending.current()->identifier.copy() == s_identifier){ 
	  fprintf(stderr,"Got a match!\n");fflush(stderr);
	  switch (pending.current()->type) {
	  case ICMD_LOGIN: {
	    loggedin=true;
	    sendNextCommand();
	    break;
	  }
	  case ICMD_SELECT: {
	    if (s_buf.left(3) == "OK ") {
	      s_cmd = s_buf.mid(3, s_buf.length());
	      if (s_cmd.left(11)=="[READ-ONLY]") {
		fprintf(stderr,"Warning mbx opened readonly\n");fflush(stderr);
		s_cmd.remove(0, 12);
	      }
	      debug("ICMD_SELECT completed fine");
	      sendNextCommand();
	    } else {
	      debug("ICMD_SELECT failed!");
	    }
	    break;
	  }
	  default: {
	    break;
	  }
	  }
	}
	pending.next();
      }
    }
  }
}

void IMAP4Protocol::sendNextCommand ()
{
  pending.first();
  while (pending.current()) {
    if (!pending.current()->sent) {
      switch (pending.current()->type) {
      case ICMD_LOGIN: {
	write(m_iSock, pending.current()->identifier.ascii(), pending.current()->identifier.length());
	write(m_iSock, " LOGIN ", 7);
	write(m_iSock, pending.current()->args.ascii(), pending.current()->args.length());
	write(m_iSock, "\r\n", 2);
	pending.current()->sent=true;
	break;
      }
      case ICMD_SELECT: {
	write(m_iSock, pending.current()->identifier.ascii(), pending.current()->identifier.length());
	write(m_iSock, " SELECT ", 8);
	write(m_iSock, pending.current()->args.ascii(), pending.current()->args.length());
	write(m_iSock, "\r\n", 2);
	pending.current()->sent=true;
	break;
      }
      }
    }
    pending.next();
  }
}

void IMAP4Protocol::jobError(int _errid, const char *_text)
{
  error(_errid, _text);
}

/*************************************
 *
 * IMAP4IOJob
 *
 *************************************/

IMAP4IOJob::IMAP4IOJob(Connection *_conn, IMAP4Protocol *_imap4) :
	IOJob(_conn)
{
  m_pIMAP4 = _imap4;
}

void IMAP4IOJob::slotError(int _errid, const char *_txt)
{
  m_pIMAP4->jobError(_errid, _txt );
}
