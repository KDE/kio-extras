// $Id$
/**********************************************************************
 *
 *   imap4.cc  - IMAP4rev1 KIOSlave
 *   Copyright (C) 1999  John Corey
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Send comments and bug fixes to jcorey@fruity.ath.cx
 *
 *********************************************************************/

/*
  References:
    RFC 2060 - Internet Message Access Protocol - Version 4rev1 - December 1996
    RFC 2192 - IMAP URL Scheme - September 1997
    RFC 1731 - IMAP Authentication Mechanisms - December 1994
               (Discusses KERBEROSv4, GSSAPI, and S/Key)
    RFC 2195 - IMAP/POP AUTHorize Extension for Simple Challenge/Response
             - September 1997 (CRAM-MD5 authentication method)
    RFC 2104 - HMAC: Keyed-Hashing for Message Authentication - February 1997

  Supported URLs:
    imap://server/ - Prompt for user/pass, list all folders in home directory
    imap://user:pass@server/ - Uses LOGIN to log in
    imap://user;AUTH=method:pass@server/ - Uses AUTHENTICATE to log in

    imap://server/folder/ - List messages in folder
 */

#include "imap4.h"
#include "base64md5.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

#include <qregexp.h>

#include <kprotocolmanager.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <kio/passdlg.h>
#include <klocale.h>

#ifndef MAX
#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#endif

using namespace KIO;

extern "C" {
	void sigalrm_handler(int);
	int kdemain(int argc, char **argv);
};

int kdemain(int argc, char **argv)
{
   kdDebug() << "IMAP4::kdemain" << endl;
/*
  int i;
  QString e64(encodeBase64("jcorey", &i));
  debug(QString("base64 encode jcorey = %1").arg(e64));
  QString b64(decodeBase64(e64, &i));
  debug(QString("base64 decode %1 = %2").arg(e64).arg(b64));
  QString m("jcorey");
  QString n = encodeMD5(m);
  debug(QString("md5 encode %1 = %2").arg(m).arg(n));
  exit(1);
*/

  KInstance instance( "kio_imap4" );
  if (argc != 4) {
    kdDebug() << " Usage: kio_imap4 protocol domain-socket1 domain-socket2" << endl;
    ::exit(-1);
  }
  IMAP4Protocol *slave = new IMAP4Protocol(argv[2], argv[3]);
  slave->dispatchLoop();
  delete slave;
  return 0;
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

IMAP4Protocol::IMAP4Protocol(const QCString &pool, const QCString &app)
  : SlaveBase( "imap4", pool, app)
{
   kdDebug() << "IMAP4::IMAP4Protocol" << endl;
   m_iSock = m_uLastCmd = 0;
   m_tTimeout.tv_sec=10;
   m_tTimeout.tv_usec=0;
   authState = 0;
   folderDelimiter = "/";
   pending.clear();
}


unsigned int IMAP4Protocol::command (enum IMAP_COMMAND type, const QString &_args)
{
   QString cmd_str;
   if (m_uLastCmd > 20) exit(-1);
   m_uLastCmd++;
   cmd_str.sprintf("KONQ%u", m_uLastCmd);
   kdDebug() << QString("--> cmd_str=%1 type=%2").arg(cmd_str).arg(type) << endl;
   CMD_Struct *cmd_s = new CMD_Struct;

   cmd_s->identifier = cmd_str.copy();
   cmd_s->type = type;
   cmd_s->args = _args;
   cmd_s->sent = false;

   pending.append(cmd_s);

   kdDebug() << "IMAP4: command put into list" << endl;
   return m_uLastCmd;
}

void IMAP4Protocol::imap4_close ()
{
   m_uLastCmd=0;
   exit(0);
}

bool IMAP4Protocol::imap4_open()
{
   kdDebug() << "IMAP4: imap4_open" << endl;
   unsigned short int port;
   ksockaddr_in server_name;
   struct servent *sent;
   memset(&server_name, sizeof(server_name), 0);

   // We want 143 as the default, but -1 means no port was specified.
   // Why 0 wasn't chosen is beyond me.
   sent = getservbyname("imap4", "tcp");
   port = (m_iPort != 0) ? m_iPort : (sent ? ntohs(sent->s_port) : 143);

   m_iSock = ::socket(PF_INET, SOCK_STREAM, 0);
   if (!KSocket::initSockaddr(&server_name, m_sServer.ascii(), m_iPort))
      return false;
   if (::connect(m_iSock, (struct sockaddr*)(&server_name), sizeof(server_name))) {
      error( ERR_COULD_NOT_CONNECT, m_sServer);
      m_cmd = CMD_NONE;
      return false;
   }
   if ((fp = fdopen(m_iSock, "w+")) == 0) {
      close(m_iSock);
      return false;
   }

   authType = "*";
  if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
      if (!openPassDlg(i18n("Username and password for your IMAP account:"),
         m_sUser, m_sPass)) {
         return false;
      } else {
         kdDebug() << "IMAP4: open_PassDlg: user=" << m_sUser << " pass=xx" << endl;
      }
   }

   int i = m_sUser.find(";AUTH=");
   if (i != -1) {
      kdDebug() << "AUTH " << i << endl;
      authType = m_sUser.mid(i+6);
      m_sUser = m_sUser.left(i);
   }
   kdDebug() << "IMAP4: connect " << m_sServer << ":" <<  m_iPort << "  user=" <<  m_sUser << " pass=xx authType=" << authType << endl;

   command(ICMD_CAPABILITY, "");
   return true;
}

ssize_t IMAP4Protocol::getSize(const KURL &_url) {
   kdDebug() << "IMAP4:: slotGetSize: " << _url.url() << endl;
   if (_url.isMalformed()) {
      error(ERR_MALFORMED_URL, _url.url());
      return 0;
   }

   if (_url.path().right(1) == folderDelimiter) {
      kdDebug() << "IMAP4: sending ERR_IS_DIRECTORY 1" << endl;
      error(ERR_IS_DIRECTORY, _url.url());
      return 0;
   }

   // TODO: Issue commands to get the size of the message...
   return -1;
}

void IMAP4Protocol::get(const KURL &_url) {
   kdDebug() << "IMAP4::get: " << _url.url() << endl;
   QString path, cmd;
   if ( _url.isMalformed() ) {
      kdDebug() << "IMAP4: URL is malformed" << endl;
      error( ERR_MALFORMED_URL, _url.path() );
      return;
   }

   if (_url.protocol() != "imap") {
      kdDebug() << "IMAP4: URL protocol != imap" << endl;
      error( ERR_INTERNAL, i18n("kio_imap4 got non imap4 url") );
      return;
   }

   path = _url.path();
   if (path.at(0)=='/') path.remove(0,1);  // remove the first /, duh

   urlPath = path.copy();
   kdDebug() << "IMAP4: urlPath=" << urlPath << "folderDelimiter=" << folderDelimiter << endl;
   if ( (urlPath.right(1) == folderDelimiter) || (urlPath.isEmpty()) ) {
      kdDebug() << "IMAP4: sending ERR_IS_DIRECTORY 2" << endl;
      error(ERR_IS_DIRECTORY, _url.url());
      m_cmd = CMD_NONE;
      return;
   }

   if (!imap4_open()) {
      kdDebug() << "IMAP4: imap4_open failed" << endl;
      imap4_close();
      return;
   }
/*
  if (cmd == "index") {
    kdDebug("IMAP4: index");
  } else if (cmd == "download") {
    debug("IMAP4: We wanna download damnit :%s:", path.ascii());
    (void)path.toUInt(&ok);
    if (!ok)
      exit(0);
    command(ICMD_FETCH, QString("RFC822.SIZE ")+path);
    command(ICMD_FETCH, QString("RFC822 ")+path);
  }
*/
   startLoop();
}

void IMAP4Protocol::imap4_login() {
   kdDebug() << "IMAP4: imap4_login begins" << endl;
   bool success = false;

   if (authState == 999) return;

   bool hasAuth = false;
   for(QStringList::Iterator it=capabilities.begin(); it!=capabilities.end(); it++) {
      QString cap = *it;
      if (cap.left(5) == "AUTH=") {
         if (cap.mid(5) == authType)
            hasAuth = true;
      }
   }

   kdDebug() << "IMAP4: hasAuth=" << (hasAuth ? "true":"false") << " authType=" << authType << " authState= " << authState << "." << endl;

   if (authType == "*") {
      // We're supposed to choose the best method from those available in capabilities
      // For simplicity, I am just using the LOGIN command.
      command(ICMD_LOGIN, "\"" + m_sUser + "\" \"" + m_sPass + "\"");
      sendNextCommand();
      return;
   }

   if (hasAuth) {
      if (authType == "LOGIN") {  // base64 encoded user/pass
         switch(authState) {
            case 0: {  // initial state - send AUTHENTICATE request
               command(ICMD_AUTHENTICATE, "LOGIN");
               kdDebug() << "IMAP4: authState = " << authState << endl;
               sendNextCommand();
               break;
            }
            case 1: {  // send username
               int i;
               QString key(decodeBase64(authKey, &i));
               kdDebug() << "IMAP4: login(" << authState << ") key=" << key << endl;
               command(ICMD_SEND_AUTH, encodeBase64(m_sUser, &i));
               sendNextCommand();
               break;
            }
            case 2: {  // send password
               int i;
               QString key(decodeBase64(authKey, &i));
               kdDebug() << "IMAP4: login(" << authState << ") key =" << key << endl;
               command(ICMD_SEND_AUTH, encodeBase64(m_sPass, &i));
               sendNextCommand();
               break;
            }
         } // switch
      } else if (authType == "CRAM-MD5") {  // MD5 encrypted user/pass
         // See RFC 2195
         /*
          * U = username, P = password, C = challenge
          * ipad = 0x36  opad = 0x5C
          *
          * dC = decoded base64 challenge, in the form "<12345.67890@host.domain>"
          * if len(P) > 64 then P = MD5(P)
          * if len(P) < 64 then pad with zeros to length 64
          * iP = P XOR ipad ; oP = P XOR opad
          * M1 = MD5( oP + MD5( iP + dC ) )
          * M2 = base64_encoded(U + " " + M1)
          * send M2 to server for authentication
          *
          */
         error(ERR_UNSUPPORTED_PROTOCOL, "CRAM-MD5");
      } else {
         kdDebug() << "IMAP4: Unsupported Authentication method - " << authType << endl;
         error(ERR_UNSUPPORTED_PROTOCOL, authType);
      }
   }
}

void IMAP4Protocol::imap4_exec() {
   // urlPath
   kdDebug() << "IMAP4: exec " << urlPath << endl;
   // ??
}

void IMAP4Protocol::listDir(const KURL &_url) {
   kdDebug() << "IMAP4: slotListDir: " << _url.url() << " authState=" << authState << endl;

   if (_url.isMalformed()) {
      error(ERR_MALFORMED_URL, _url.url());
      m_cmd = CMD_NONE;
      return;
   }
   // Is this necesary?
   sleep(20);
   if (!imap4_open()) {
      debug("IMAP4: imap4_open failed");
      imap4_close();
      return;
   } else {
      imap4_login();
   }

   bool LIST = true;  // use LIST vs LSUB
   if (urlPath.find(";TYPE=", 0, false) != -1) {
      int i = urlPath.find(";TYPE=", 0, false);
      QString t = urlPath.mid(i+6);
      if (t == "LSUB") LIST = false;
      else if (t == "LIST") LIST = true;
      else { error(ERR_UNSUPPORTED_ACTION, t); return; }
      urlPath = urlPath.left(i);
   }
   QString mailRef = urlPath;
   // further parse urlPath
   if (LIST) command(ICMD_LIST, "\"" + mailRef + "\" \"%\"");
   else command(ICMD_LSUB, "\"" + mailRef + "\" \"%\"");

   startLoop();
}

void IMAP4Protocol::processList(QString str) {
   static unsigned int itemNum = 0;
   // Sample: (\NoInferiors \Marked) "/" "~/.imap/Item"
   // (Attributes) "<hierarchy delimiter>" "<item name>"
   QString tmp_str = str.copy();
   int i = tmp_str.find("("), j = tmp_str.find(")");
   QString Attributes = tmp_str.mid(i, j-i+1);
   tmp_str.remove(i, j-i+1);
   tmp_str = tmp_str.simplifyWhiteSpace();

   if (tmp_str[0] == QChar('"')) {
      j = tmp_str.find("\"", 1);
   } else {
      j = tmp_str.find(" ");
   }
   folderDelimiter = tmp_str.mid(0, j+1);
   if (folderDelimiter[0] == QChar('"'))
      folderDelimiter.remove(0, 1);
   if (folderDelimiter[folderDelimiter.length()-1] == QChar('"'))
      folderDelimiter.truncate(1);
   tmp_str.remove(0, j+1);
   tmp_str = tmp_str.simplifyWhiteSpace();

/*
if (tmp_str[0] == QChar('"')) {
    j = tmp_str.find("\"", 1);
  } else {
    j = tmp_str.find(" ");
  }
*/
   QString Name = tmp_str.copy();
   if (Attributes.isEmpty() || folderDelimiter.isEmpty() || Name.isEmpty()) {
      return;
   }
   itemNum++;
   kdDebug() << "==> itemNum " << itemNum << " Attributes: " << Attributes << " Delimiter: " << folderDelimiter << " Name: " << Name << endl;

   UDSEntry entry;
   UDSAtom atom;
   QString filename;
  
   atom.m_uds = UDS_NAME;
   //atom.m_str = QString("Item_%1").arg(itemNum);
   atom.m_str = Name;
   atom.m_long = 0;
   entry.append(atom);
   kdDebug() << "=> Adding atom str=" << atom.m_str << " long=" << atom.m_long << endl;

   atom.m_uds = UDS_MIME_TYPE;
   atom.m_str = "inode/directory";
   atom.m_long = 0;
   entry.append(atom);
   kdDebug() << "=> Adding atom str=" << atom.m_str << " long=" << atom.m_long << endl;

   // \Noselect = directory
   if (Attributes.find("\\Noselect", 0, false) != -1) {
      atom.m_str = "NOSELECT_";
   } else if (Attributes.find("\\Noinferiors", 0, false) != -1) {
      atom.m_str = "NOINFERIORS_";
   } else {
      atom.m_str = "";
   }
   atom.m_uds = UDS_URL;
   atom.m_str += "/messageXX";
   atom.m_long = 0;
   entry.append(atom);
   kdDebug() << "=> Adding atom str=" << atom.m_str << " long=" << atom.m_long << endl;   

   atom.m_uds = UDS_FILE_TYPE;
   atom.m_str = "";
   // \Noinferiors = mailbox
   if (Attributes.find("\\Noselect", 0, false) != -1) {
      atom.m_long = S_IFREG;
   } else if (Attributes.find("\\Noinferiors", 0, false) != -1) {
      atom.m_long = S_IFDIR;
   } else
      atom.m_long = S_IFREG;
   entry.append(atom);
   kdDebug() << "=> Adding atom str=" << atom.m_str << " long=" << atom.m_long << endl;   

   atom.m_uds = UDS_SIZE;
   atom.m_str = "";
   atom.m_long = 5555;
   entry.append(atom);
   kdDebug() << "=> Adding atom str=" << atom.m_str << " long=" << atom.m_long << endl;   

   listEntry(entry, false);
   entry.clear();
   listEntry(entry, true);
}

void IMAP4Protocol::sendNextCommand ()
{
   pending.first();
   while (pending.current()) {
   kdDebug() << "IMAP4: sendNextCommand: type=" << pending.current()->type << endl;
   CMD_Struct *cmd = pending.current();
   if (!pending.current()->sent) {
      switch (pending.current()->type) {
        // Any State
         case ICMD_NOOP: {
//          debug(QString("IMAP4: C: %1 NOOP").arg(cmd->identifier));
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " NOOP", 5);
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_CAPABILITY: {
            kdDebug() << "IMAP4: C: " << cmd->identifier << " CAPABILITY" << endl;
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " CAPABILITY", 11);
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_LOGOUT: {
            kdDebug() << "IMAP4: C: " << cmd->identifier << " LOGOUT" << endl;
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " LOGOUT", 7);
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         // Non-Authenticated State
         case ICMD_AUTHENTICATE: {
            kdDebug() << "IMAP4: C: " << cmd->identifier << " AUTHENTICATE " << cmd->args << endl;
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " AUTHENTICATE ", 14);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_SEND_AUTH: {  // must already have trailing \r\n
            kdDebug() << "IMAP4: (SEND_AUTH) C: " << cmd->args << endl;
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            cmd->sent = true;
            break;
         }
         case ICMD_LOGIN: {
            kdDebug() << "IMAP4: C: " << cmd->identifier << " LOGIN " << cmd->args << endl;
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " LOGIN ", 7);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         // Authenticated state
         case ICMD_LIST: {
            kdDebug() << "IMAP4: C: " << cmd->identifier << " LIST " << cmd->args << endl;
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " LIST ", 6);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_LSUB: {
            kdDebug() << "IMAP4: C: " << cmd->identifier << " LSUB " << cmd->args << endl;
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " LSUB ", 6);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_SELECT: {
            // <not tested>
            kdDebug() << "IMAP4: C: " << cmd->identifier << " SELECT " << cmd->args << endl;
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " SELECT ", 8);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
      } // switch
   } // if
   pending.next();
   } // while
}

void IMAP4Protocol::del( const KURL &_url, bool isFile) {
   // TODO
   kdDebug() << "IMAP4::del" << endl;
}

void IMAP4Protocol::setHost(const QString &_host, int _port, const QString &_user, const QString &_pass)
{
	m_sServer = _host;
	m_iPort = _port;
	m_sUser = _user;
	m_sPass = _pass;
}

void IMAP4Protocol::stat(const KURL &_url)
{
   if (_url.isMalformed()) {
      error(ERR_MALFORMED_URL, _url.url());
      m_cmd = CMD_NONE;
      return;
   }

   UDSEntry entry;
   UDSAtom atom;

   atom.m_uds = UDS_NAME;
   atom.m_str = _url.path();
   if (atom.m_str.at(0) == '/')
       atom.m_str.remove(0,1);
   entry.append( atom );

   // TODO: Do an actual check if this is a folder or file
   if (_url.path().right(1) == folderDelimiter) {
       atom.m_uds = UDS_MIME_TYPE;
       atom.m_str = "inode/directory";
       entry.append( atom );

       atom.m_uds = UDS_FILE_TYPE;
       atom.m_str = "";
       atom.m_long = S_IFDIR;
       entry.append( atom );
   } else {
       atom.m_uds = UDS_MIME_TYPE;
       atom.m_str = "message/rfc822"; // Yes, a bit naive, but oh well
       entry.append( atom );

       atom.m_uds = UDS_FILE_TYPE;
       atom.m_str = "";
       atom.m_long = S_IFREG;
	//isFile();
   }

   statEntry( entry );
   finished();
}
