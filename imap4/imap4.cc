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

#include <qregexp.h>

#include <kurl.h>
#include <kprotocolmanager.h>
#include <ksock.h>
#include <klocale.h>
#include <kio_pass_dlg.h>
#include <kinstance.h>

#ifndef MAX
#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#endif


extern "C" {
   void sigsegv_handler(int);
   void sigchld_handler(int);
   void sigalrm_handler(int);
};

int main(int argc, char **argv)
{
   debug("IMAP4: main");
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
   signal(SIGCHLD, sigchld_handler);
   signal(SIGSEGV, sigsegv_handler);

   KInstance instance( "kio_imap" );

   KIOConnection parent( 0, 1 );
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

IMAP4Protocol::IMAP4Protocol(KIOConnection *_conn) : KIOProtocol(_conn)
{
   debug("IMAP4: IMAP4Protocol");
   m_pJob = 0L;
   m_iSock = m_uLastCmd = 0;
   m_cmd = CMD_NONE;
   m_tTimeout.tv_sec=10;
   m_tTimeout.tv_usec=0;
   authState = 0;
   folderDelimiter = "/";
   pending.clear();
}


unsigned int IMAP4Protocol::command (enum IMAP_COMMAND type, const char *_args)
{
   QString cmd_str;
   if (m_uLastCmd > 20) exit(-1);
   m_uLastCmd++;
   cmd_str.sprintf("KONQ%u", m_uLastCmd);
   debug(QString("--> cmd_str=%1  type=%2").arg(cmd_str).arg(type));
   CMD_Struct *cmd_s = new CMD_Struct;

   cmd_s->identifier = cmd_str.copy();
   cmd_s->type = type;
   cmd_s->args = _args;
   cmd_s->sent = false;

   pending.append(cmd_s);

   debug(QString("IMAP4: command put into list"));
   return m_uLastCmd;
}

void IMAP4Protocol::imap4_close ()
{
   m_uLastCmd=0;
   exit(0);
}

bool IMAP4Protocol::imap4_open( KURL &_url )
{
   debug(QString("IMAP4: imap4_open"));
   unsigned short int port;
   struct sockaddr_in server_name;
   struct servent *sent;
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
      m_cmd = CMD_NONE;
      return false;
   }
   if ((fp = fdopen(m_iSock, "w+")) == 0) {
      close(m_iSock);
      return false;
   }

   authType = "*";
//  userName = _url.user().isEmpty() ? QString("anonymous") : _url.user();
   userName = _url.user();
   passWord = _url.pass();
   if (userName.isEmpty() || passWord.isEmpty()) {
      if (!open_PassDlg(i18n("Username and password for your IMAP account:"),
         userName, passWord)) {
         return false;
      } else {
         debug(QString("IMAP4: open_PassDlg: user=%1 pass=xx").arg(userName));
      }
   }
   int i = userName.find(";AUTH=");
   if (i != -1) {
      debug(QString("AUTH %1").arg(i));
      authType = userName.mid(i+6);
      userName = userName.left(i);
   }
   debug(QString("IMAP4: connect %1:%2  user=%3 pass=xx authType=%4").
      arg(_url.host()).arg(port).arg(userName).arg(authType));

   command(ICMD_CAPABILITY, "");
   return true;
}

void IMAP4Protocol::slotGetSize(const char *_url) {
   debug(QString("IMAP4: slotGetSize: %1").arg(_url));
   KURL ku(_url);
   if (ku.isMalformed()) {
      error(ERR_MALFORMED_URL, ku.url());
      m_cmd = CMD_NONE;
      return;
   }

   if (urlPath.right(1) == folderDelimiter) {
      debug("IMAP4: sending ERR_IS_DIRECTORY 1");
      error(ERR_IS_DIRECTORY, ku.url());
      m_cmd = CMD_NONE;
      return;
   }

   // TODO: Issue commands to get the size of the message...
}

void IMAP4Protocol::slotGet(const char *_url) {
   debug(QString("IMAP4: slotGet: %1").arg(_url));
   QString path, cmd;
   KURL usrc(_url);
   if ( usrc.isMalformed() ) {
      debug("IMAP4: URL is malformed");
      error( ERR_MALFORMED_URL, strdup(_url) );
      // M_CMD needs to be cmd_none
      return;
   }

   if (usrc.protocol() != "imap") {
      debug("IMAP4: URL protocol != imap");
      error( ERR_INTERNAL, i18n("kio_imap4 got non imap4 url") );
      // M_CMD needs to be cmd_none
      return;
   }

   path = usrc.path().copy();
   if (path.at(0)=='/') path.remove(0,1);  // remove the first /, duh

   urlPath = path.copy();
   debug(QString("IMAP4: urlPath=%1  folderDelimiter=%2").arg(urlPath).arg(folderDelimiter));
   if ( (urlPath.right(1) == folderDelimiter) || (urlPath.isEmpty()) ) {
      debug("IMAP4: sending ERR_IS_DIRECTORY 2");
      error(ERR_IS_DIRECTORY, usrc.url());
      m_cmd = CMD_NONE;
      return;
   }

   if (!imap4_open(usrc)) {
      debug("IMAP4: imap4_open failed");
      imap4_close();
      return;
   }
/*
  if (cmd == "index") {
    debug("IMAP4: index");
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
   debug("IMAP4: imap4_login begins");
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

   debug(QString("IMAP4: hasAuth=%1 authType=%2  authState=%3").
      arg(hasAuth ? "true":"false").arg(authType).arg(authState));

   if (authType == "*") {
      // We're supposed to choose the best method from those available in capabilities
      // For simplicity, I am just using the LOGIN command.
      command(ICMD_LOGIN, "\"" + userName + "\" \"" + passWord + "\"");
      sendNextCommand();
      return;
   }

   if (hasAuth) {
      if (authType == "LOGIN") {  // base64 encoded user/pass
         switch(authState) {
            case 0: {  // initial state - send AUTHENTICATE request
               command(ICMD_AUTHENTICATE, "LOGIN");
               debug(QString("IMAP4: authState = %1").arg(authState));
               sendNextCommand();
               break;
            }
            case 1: {  // send username
               int i;
               QString key(decodeBase64(authKey, &i));
               debug(QString("IMAP4: login(%1) key = %2").arg(authState).arg(key));
               command(ICMD_SEND_AUTH, encodeBase64(userName, &i));
               sendNextCommand();
               break;
            }
            case 2: {  // send password
               int i;
               QString key(decodeBase64(authKey, &i));
               debug(QString("IMAP4: login(%1) key = %2").arg(authState).arg(key));
               command(ICMD_SEND_AUTH, encodeBase64(passWord, &i));
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
         debug(QString("IMAP4: Unsupported Authentication method - %1").arg(authType));
         error(ERR_UNSUPPORTED_PROTOCOL, authType);
      }
   }
}

void IMAP4Protocol::imap4_exec() {
   // urlPath
   debug(QString("IMAP4: exec %1").arg(urlPath));
   // ??
}

void IMAP4Protocol::slotPut(const char *_url, int _mode, bool _overwrite,
                            bool _resume, unsigned int)
{
   debug(QString("IMAP4: slotPut: %1").arg(_url));
   KURL ku(_url);
   if (ku.isMalformed()) {
      error(ERR_MALFORMED_URL, ku.url());
      m_cmd = CMD_NONE;
      return;
   }
}

void IMAP4Protocol::slotListDir(const char *_url) {
   debug(QString("IMAP4: slotListDir: %1  authState=%2").arg(_url).arg(authState));
   KURL ku(_url);
   if (ku.isMalformed()) {
      error(ERR_MALFORMED_URL, ku.url());
      m_cmd = CMD_NONE;
      return;
   }
   sleep(20);
   if (!imap4_open(ku)) {
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

void IMAP4Protocol::slotTestDir(const char *_url) {
   debug(QString("IMAP4: slotTestDir: %1").arg(_url));
   KURL ku(_url);
   if (ku.isMalformed()) {
      error(ERR_MALFORMED_URL, ku.url());
      m_cmd = CMD_NONE;
      return;
   }
   // TODO: Do an actual check if this is a folder or file
   if (ku.path().right(1) == folderDelimiter) isDirectory();
   else isFile();
   finished();
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
   debug(QString("==> itemNum %1 Attributes: %1 Delimiter: %2  Name: %3").arg(itemNum).
      arg(Attributes).arg(folderDelimiter).arg(Name));

   KUDSEntry entry;
   KUDSAtom atom;
   QString filename;
  
   atom.m_uds = UDS_NAME;
//  atom.m_str = QString("Item_%1").arg(itemNum);
   atom.m_str = Name;
   atom.m_long = 0;
   entry.append(atom);
   debug(QString("=> Adding atom str=%1 long=%2").arg(atom.m_str).arg(atom.m_long));

   atom.m_uds = UDS_MIME_TYPE;
   atom.m_str = "inode/directory";
   atom.m_long = 0;
   entry.append(atom);
   debug(QString("=> Adding atom str=%1 long=%2").arg(atom.m_str).arg(atom.m_long));

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
   debug(QString("=> Adding atom str=%1 long=%2").arg(atom.m_str).arg(atom.m_long));

   atom.m_uds = UDS_FILE_TYPE;
   atom.m_str = "";
   // \Noinferiors = mailbox
   if (Attributes.find("\\Noselect", 0, false) != -1) {
      atom.m_long = S_IFREG;
   } else if (Attributes.find("\\Noinferiors", 0, false) != -1) {
      atom.m_long = S_IFDIR;
   } else
//   atom.m_long = S_IFREG;
   entry.append(atom);
   debug(QString("=> Adding atom str=%1 long=%2").arg(atom.m_str).arg(atom.m_long));

   atom.m_uds = UDS_SIZE;
   atom.m_str = "";
   atom.m_long = 5555;
   entry.append(atom);
   debug(QString("=> Adding atom str=%1 long=%2").arg(atom.m_str).arg(atom.m_long));

   listEntry(entry);
   entry.clear();
}

void IMAP4Protocol::sendNextCommand ()
{
   pending.first();
   while (pending.current()) {
   debug(QString("IMAP4: sendNextCommand: type=%1").arg(pending.current()->type));
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
            debug(QString("IMAP4: C: %1 CAPABILITY").arg(cmd->identifier));
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " CAPABILITY", 11);
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_LOGOUT: {
            debug(QString("IMAP4: C: %1 LOGOUT").arg(cmd->identifier));
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " LOGOUT", 7);
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         // Non-Authenticated State
         case ICMD_AUTHENTICATE: {
            debug(QString("IMAP4: C: %1 AUTHENTICATE %2").arg(cmd->identifier).
               arg(cmd->args));
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " AUTHENTICATE ", 14);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_SEND_AUTH: {  // must already have trailing \r\n
            debug(QString("IMAP4: (SEND_AUTH) C: %1").arg(cmd->args));
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            cmd->sent = true;
            break;
         }
         case ICMD_LOGIN: {
            debug(QString("IMAP4: C: %1 LOGIN %2").arg(cmd->identifier).arg(cmd->args));
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " LOGIN ", 7);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         // Authenticated state
         case ICMD_LIST: {
            debug(QString("IMAP4: C: %1 LIST %2").arg(cmd->identifier).
               arg(cmd->args.ascii()));
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " LIST ", 6);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_LSUB: {
            debug(QString("IMAP4: C: %1 LSUB %2").arg(cmd->identifier).
               arg(cmd->args.ascii()));
            write(m_iSock, cmd->identifier.ascii(), cmd->identifier.length());
            write(m_iSock, " LSUB ", 6);
            write(m_iSock, cmd->args.ascii(), cmd->args.length());
            write(m_iSock, "\r\n", 2);
            cmd->sent = true;
            break;
         }
         case ICMD_SELECT: {
            // <not tested>
            debug(QString("IMAP4: C: %1 SELECT %2").arg(cmd->identifier).arg(cmd->args.ascii()));
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

void IMAP4Protocol::jobError(int _errid, const char *_text)
{
   debug(QString("IMAP4: jobError"));
   error(_errid, _text);
}

void IMAP4Protocol::slotCopy(const char *_source, const char *_dest) {
   // TODO
   debug(QString("IMAP4: slotCopy"));
}

void IMAP4Protocol::slotData(void *_p, int _len) {
   // TODO
   debug(QString("IMAP4: slotData"));
   switch(m_cmd) {
      case CMD_PUT:
         // Send data here
         break;
      default:
         abort();
         break;
   }
}

void IMAP4Protocol::slotDataEnd() {
   // TODO
   debug(QString("IMAP4: slotDataEnd"));
   switch(m_cmd) {
      case CMD_GET:
         m_cmd = CMD_NONE;
         break;
      default:
         abort();
         break;
   }
}

void IMAP4Protocol::slotDel(QStringList& _source) {
   // TODO
   debug(QString("IMAP4: slotDel"));
}

void IMAP4Protocol::jobData(void *_p, int _len) {
   // TODO
   debug(QString("IMAP4: jobData"));
   switch(m_cmd) {
      case CMD_GET:
         break;
      case CMD_COPY:
         break;
      default:
         abort();
   }
}

void IMAP4Protocol::jobDataEnd() {
   // TODO
   m_cmd = CMD_NONE;
   debug(QString("IMAP4: jobDataEnd"));
   switch(m_cmd) {
      case CMD_GET:
         dataEnd();
         break;
      case CMD_COPY:
         m_pJob->dataEnd();
         break;
      default:
         abort();
         break;
   }
}

/*************************************
 *
 * IMAP4IOJob
 *
 *************************************/

IMAP4IOJob::IMAP4IOJob(KIOConnection *_conn, IMAP4Protocol *_imap4) :
        KIOJobBase(_conn)
{
   m_pIMAP4 = _imap4;
}

void IMAP4IOJob::slotError(int _errid, const char *_txt)
{
   debug(QString("IMAP4: IOJob::slotError"));
   KIOJobBase::slotError(_errid, _txt);
   m_pIMAP4->jobError(_errid, _txt );
}

void IMAP4IOJob::slotData(void *_p, int _len) {
   // TODO
   debug(QString("IMAP4: IOJob::slotData"));
   m_pIMAP4->jobData(_p, _len);
}

void IMAP4IOJob::slotDataEnd() {
   // TODO
   debug(QString("IMAP4: IOJob::slotDataEnd"));
   m_pIMAP4->jobDataEnd();
}
