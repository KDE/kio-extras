/*
 * Copyright (c) 1999-2001 Alex Zepeda
 * Copyright (c) 2001-2002 Michael Haeckel <haeckel@kde.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <errno.h>
#include <stdio.h>

#include <qcstring.h>
#include <qglobal.h>
#include <qregexp.h>

#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>
#include <kmdcodec.h>
#include <kprotocolmanager.h>
#include <ksock.h>
#include <kio/kdesasl.h>

#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <kio/passdlg.h>

#include "pop3.h"

#define GREETING_BUF_LEN 1024
#define MAX_RESPONSE_LEN 512
#define MAX_COMMANDS 10

#define POP3_DEBUG kdDebug(7105)

extern "C" {
  int kdemain(int argc, char **argv);
}

using namespace KIO;

int kdemain(int argc, char **argv)
{

  if (argc != 4) {
    POP3_DEBUG << "Usage: kio_pop3 protocol domain-socket1 domain-socket2"
        << endl;
    return -1;
  }

  KInstance instance("kio_pop3");
  POP3Protocol *slave;

  // Are we looking to use SSL?
  if (strcasecmp(argv[1], "pop3s") == 0) {
    slave = new POP3Protocol(argv[2], argv[3], true);
  } else {
    slave = new POP3Protocol(argv[2], argv[3], false);
  }

  slave->dispatchLoop();
  delete slave;
  return 0;
}

POP3Protocol::POP3Protocol(const QCString & pool, const QCString & app,
                           bool isSSL)
:  TCPSlaveBase((isSSL ? 995 : 110), (isSSL ? "pop3s" : "pop3"), pool, app,
             isSSL)
{
  POP3_DEBUG << "POP3Protocol::POP3Protocol()" << endl;
  m_bIsSSL = isSSL;
  m_cmd = CMD_NONE;
  m_iOldPort = 0;
  m_tTimeout.tv_sec = 10;
  m_tTimeout.tv_usec = 0;
  supports_apop = false;
  m_try_apop = true;
  m_try_sasl = true;
  opened = false;
  readBufferLen = 0;
}

POP3Protocol::~POP3Protocol()
{
  POP3_DEBUG << "POP3Protocol::~POP3Protocol()" << endl;
  closeConnection();
}

void POP3Protocol::setHost(const QString & _host, int _port,
                           const QString & _user, const QString & _pass)
{
  m_sServer = _host;
  m_iPort = _port;
  m_sUser = _user;
  m_sPass = _pass;
}

ssize_t POP3Protocol::myRead(void *data, ssize_t len)
{
  if (readBufferLen) {
    ssize_t copyLen = (len < readBufferLen) ? len : readBufferLen;
    memcpy(data, readBuffer, copyLen);
    readBufferLen -= copyLen;
    if (readBufferLen)
      memcpy(readBuffer, &readBuffer[copyLen], readBufferLen);
    return copyLen;
  }
  waitForResponse(600);
  return Read(data, len);
}

ssize_t POP3Protocol::myReadLine(char *data, ssize_t len)
{
  ssize_t copyLen = 0, readLen = 0;
  while (true) {
    while (copyLen < readBufferLen && readBuffer[copyLen] != '\n')
      copyLen++;
    if (copyLen < readBufferLen || copyLen == len) {
      copyLen++;
      memcpy(data, readBuffer, copyLen);
      data[copyLen] = '\0';
      readBufferLen -= copyLen;
      if (readBufferLen)
        memcpy(readBuffer, &readBuffer[copyLen], readBufferLen);
      return copyLen;
    }
    waitForResponse(600);
    readLen = read(&readBuffer[readBufferLen], len - readBufferLen);
    readBufferLen += readLen;
    if (readLen <= 0) {
      data[0] = '\0';
      return 0;
    }
  }
}

bool POP3Protocol::getResponse(char *r_buf, unsigned int r_len,
                               const char *cmd)
{
  char *buf = 0;
  unsigned int recv_len = 0;
  // fd_set FDs;

  // Give the buffer the appropriate size
  r_len = r_len ? r_len : MAX_RESPONSE_LEN;

  buf = new char[r_len];

  // Clear out the buffer
  memset(buf, 0, r_len);
  myReadLine(buf, r_len - 1);

  // This is really a funky crash waiting to happen if something isn't
  // null terminated.
  recv_len = strlen(buf);

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

  if (strncmp(buf, "+OK", 3) == 0) {
    if (r_buf && r_len) {
      memcpy(r_buf, (buf[3] == ' ' ? buf + 4 : buf + 3),
             QMIN(r_len, (buf[3] == ' ' ? recv_len - 4 : recv_len - 3)));
    }

    if (buf) {
      delete[]buf;
    }

    return true;
  } else if (strncmp(buf, "-ERR", 4) == 0) {
    if (r_buf && r_len) {
      memcpy(r_buf, (buf[4] == ' ' ? buf + 5 : buf + 4),
             QMIN(r_len, (buf[4] == ' ' ? recv_len - 5 : recv_len - 4)));
    }

    QString command = QString::fromLatin1(cmd);
    QString serverMsg = QString::fromLatin1(buf).mid(5).stripWhiteSpace();

    if (command.left(4) == "PASS") {
      command = i18n("PASS <your password>");
    }

    m_sError = i18n("The server said: \"%1\"").arg(serverMsg);

    if (buf) {
      delete[]buf;
    }

    return false;
  } else if (strncmp(buf, "+ ", 2) == 0) {
    if (r_buf && r_len) {
      memcpy(r_buf, buf + 2, QMIN(r_len, recv_len - 4));
      r_buf[QMIN(r_len - 1, recv_len - 4)] = '\0';
    }

    if (buf) {
      delete[]buf;
    }

    return true;
  } else {
    POP3_DEBUG << "Invalid POP3 response received!" << endl;

    if (r_buf && r_len) {
      memcpy(r_buf, buf, QMIN(r_len, recv_len));
    }

    if (!buf || !*buf) {
      m_sError = i18n("The server terminated the connection.");
    } else {
      m_sError = i18n("Invalid response from server:\n\"%1\"").arg(buf);
    }

    if (buf) {
      delete[]buf;
    }

    return false;
  }
}

bool POP3Protocol::sendCommand(const char *cmd)
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

  if (!isConnectionValid()) return false;

  char *cmdrn = new char[strlen(cmd) + 3];
  sprintf(cmdrn, "%s\r\n", (cmd) ? cmd : "");

  if (write(cmdrn, strlen(cmdrn)) != static_cast < ssize_t >
      (strlen(cmdrn))) {
    m_sError = i18n("Could not send to server.\n");
    delete[]cmdrn;
    return false;
  }

  delete[]cmdrn;
  return true;
}

bool POP3Protocol::command(const char *cmd, char *recv_buf,
                           unsigned int len)
{
  sendCommand(cmd);
  return getResponse(recv_buf, len, cmd);
}

void POP3Protocol::openConnection()
{
  m_try_apop = !hasMetaData("auth") || metaData("auth") == "APOP";
  m_try_sasl = !hasMetaData("auth") || metaData("auth") == "SASL";

  if (!pop3_open()) {
    POP3_DEBUG << "pop3_open failed" << endl;
    closeConnection();
  } else {
    connected();
  }
}

void POP3Protocol::closeConnection()
{
  // If the file pointer exists, we can assume the socket is valid,
  // and to make sure that the server doesn't magically undo any of
  // our deletions and so-on, we should send a QUIT and wait for a
  // response.  We don't care if it's positive or negative.  Also
  // flush out any semblance of a persistant connection, i.e.: the
  // old username and password are now invalid.
  if (!opened) {
    return;
  }

  command("QUIT");
  closeDescriptor();
  readBufferLen = 0;
  m_sOldUser = m_sOldPass = m_sOldServer = "";
  opened = false;
}

bool POP3Protocol::pop3_open()
{
  char buf[512], *greeting_buf;
  if ((m_iOldPort == port(m_iPort)) && (m_sOldServer == m_sServer) &&
      (m_sOldUser == m_sUser) && (m_sOldPass == m_sPass)) {
    POP3_DEBUG << "Reusing old connection" << endl;
    return true;
  } else {
    closeConnection();

    if (!connectToHost(m_sServer.ascii(), m_iPort)) {
      // error(ERR_COULD_NOT_CONNECT, m_sServer);
      // ConnectToHost has already send an error message.
      return false;
    }
    opened = true;

    greeting_buf = new char[GREETING_BUF_LEN];
    memset(greeting_buf, 0, GREETING_BUF_LEN);

    // If the server doesn't respond with a greeting
    if (!getResponse(greeting_buf, GREETING_BUF_LEN, "")) {
      m_sError =
          i18n("Could not login to %1.\n\n").arg(m_sServer) +
          ((!greeting_buf
            || !*greeting_buf) ?
           i18n("The server terminated the connection immediately.") :
           i18n("Server does not respond properly:\n%1\n").
           arg(greeting_buf));
      error(ERR_COULD_NOT_LOGIN, m_sError);
      delete[]greeting_buf;
      return false;             // we've got major problems, and possibly the
      // wrong port
    }
    QCString greeting(greeting_buf);
    delete[]greeting_buf;

    // Does the server support APOP?
    QString apop_cmd;
    QRegExp re("<[A-Za-z0-9\\.\\-_]+@[A-Za-z0-9\\.\\-_]+>$", false);

    if (greeting.length() > 0) {
      greeting.truncate(greeting.length() - 2);
    }

    int apop_pos = greeting.find(re);
    supports_apop = (bool) (apop_pos != -1);
    if (metaData("nologin") == "on")
      return true;

    if (metaData("auth") == "APOP" && !supports_apop) {
      error(ERR_COULD_NOT_LOGIN,
            i18n("Your POP3 server does not support APOP.\n"
                 "Choose a different authentication method."));
      return false;
    }

    m_iOldPort = m_iPort;
    m_sOldServer = m_sServer;

    // Try to go into TLS mode
    if ((metaData("tls") == "on" || (canUseTLS() &&
                                     metaData("tls") != "off"))
        && command("STLS")) {
      int tlsrc = startTLS();
      if (tlsrc == 1) {
        POP3_DEBUG << "TLS mode has been enabled." << endl;
      } else {
        if (tlsrc != -3) {
          POP3_DEBUG << "TLS mode setup has failed. Aborting." << endl;
          error(ERR_COULD_NOT_CONNECT,
                i18n("Your POP3 server claims to "
                     "support TLS but negotiation "
                     "was unsuccessful. You can "
                     "disable TLS in KDE using the "
                     "crypto settings module."));
        }
        return false;
      }
    } else if (metaData("tls") == "on") {
      error(ERR_COULD_NOT_CONNECT,
            i18n("Your POP3 server does not support TLS. Disable "
                 "TLS, if you want to connect without encryption."));
      return false;
    }

    KIO::AuthInfo authInfo;
    authInfo.username = m_sUser;
    QString one_string = QString::fromLatin1("USER ");
    QString apop_string = QString::fromLatin1("APOP ");
    if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
      // Prompt for usernames
      authInfo.prompt = i18n("Username and password for your POP3 account:");
      if (!openPassDlg(authInfo)) {
        closeConnection();
        return false;
      } else {
        apop_string.append(authInfo.username);
        one_string.append(authInfo.username);
        m_sOldUser = authInfo.username;
        m_sUser = authInfo.username;
        m_sPass = authInfo.password;
      }
    } else {
      apop_string.append(m_sUser);
      one_string.append(m_sUser);
      m_sOldUser = m_sUser;
    }
    memset(buf, 0, sizeof(buf));
    if (supports_apop && m_try_apop) {
      char *c = greeting.data() + apop_pos;
      KMD5 ctx;

      if (m_sPass.isEmpty()) {
        m_sOldPass = authInfo.password;
      } else {
        m_sOldPass = m_sPass;
      }

      // Generate digest
      ctx.update(c, strlen(c));
      ctx.update(m_sOldPass.latin1());

      // Genenerate APOP command
      apop_string.append(" ");
      apop_string.append(ctx.hexDigest());

      if (command(apop_string.local8Bit(), buf, sizeof(buf))) {
        return true;
      }

      POP3_DEBUG << "Couldn't login via APOP. Falling back to USER/PASS" <<
          endl;
      closeConnection();
      m_try_apop = false;
      if (metaData("auth") == "APOP") {
        error(ERR_COULD_NOT_LOGIN,
              i18n
              ("Login via APOP failed. The server may not support APOP, although it claims to support it, or the password may be wrong.\n\n%1").
              arg(m_sError));
        return false;
      } else {
        return pop3_open();
      }
    }
    // Let's try SASL stuff first.. it might be more secure
    QString sasl_auth, sasl_buffer = QString::fromLatin1("AUTH");

    // We need to check what methods the server supports...
    // This is based on RFC 1734's wisdom
    if (m_try_sasl
        && (hasMetaData("sasl") || command(sasl_buffer.local8Bit()))) {
      QStrIList sasl_list;
      if (hasMetaData("sasl")) {
        sasl_list.append(metaData("sasl").latin1());
      } else
        while (true /* !AtEOF() */ ) {
          memset(buf, 0, sizeof(buf));
          myReadLine(buf, sizeof(buf) - 1);

          // HACK: This assumes fread stops at the first \n and not \r
          if (strcmp(buf, ".\r\n") == 0) {
            break;              // End of data
          }
          // sanders, changed -2 to -1 below
          buf[strlen(buf) - 2] = '\0';

          sasl_list.append(buf);
        }

      KDESasl sasl(m_sUser, m_sPass, (m_bIsSSL) ? "pop3s" : "pop3");
      sasl_buffer = sasl.chooseMethod(sasl_list);
      sasl_auth = sasl_buffer;

      if (sasl_buffer == QString::null) {
      } else {
        // Yich character arrays..
        QByteArray challenge(2049);
        sasl_buffer.prepend("AUTH ");
        if (command(sasl_buffer.latin1(), challenge.data(), 2049)) {
          challenge.resize(challenge.find(0));
          bool ret = command(sasl.getResponse(challenge));
          if (sasl_auth.upper() == "LOGIN"
              || sasl_auth.upper() == "DIGEST-MD5")
            ret = command(sasl.getResponse(challenge));
          if (ret) {
            m_sOldUser = m_sUser;
            m_sOldPass = m_sPass;
            return true;
          }
        }

        if (metaData("auth") == "SASL") {
          error(ERR_COULD_NOT_LOGIN,
                i18n
                ("Login via SASL (%1) failed. The server may not support %2, or the password may be wrong.\n\n%3").
                arg(sasl_auth).arg(sasl_auth).arg(m_sError));
          return false;
        }
      }
    } else if (m_try_sasl) {
      closeConnection();
      m_try_sasl = false;
      if (metaData("auth") != "SASL") {
        return pop3_open();
      }
    }

    if (metaData("auth") == "SASL") {
      error(ERR_COULD_NOT_LOGIN,
            i18n("Your POP3 server does not support SASL.\n"
                 "Choose a different authentication method."));
      return false;
    }
    // Fall back to conventional USER/PASS scheme
    if (!command(one_string.local8Bit(), buf, sizeof(buf))) {
      POP3_DEBUG << "Couldn't login. Bad username Sorry" << endl;

      m_sError =
          i18n("Could not login to %1.\n\n").arg(m_sServer) + m_sError;
      error(ERR_COULD_NOT_LOGIN, m_sError);
      closeConnection();

      return false;
    }

    one_string = QString::fromLatin1("PASS ");
    if (m_sPass.isEmpty()) {
      m_sOldPass = authInfo.password;
      one_string.append(authInfo.password);
    } else {
      m_sOldPass = m_sPass;
      one_string.append(m_sPass);
    }

    if (!command(one_string.local8Bit(), buf, sizeof(buf))) {
      POP3_DEBUG << "Couldn't login. Bad password Sorry." << endl;
      m_sError =
          i18n
          ("Could not login to %1. The password may be wrong.\n\n%2").
          arg(m_sServer).arg(m_sError);
      error(ERR_COULD_NOT_LOGIN, m_sError);
      closeConnection();
      return false;
    }
    return true;
  }
}

size_t POP3Protocol::realGetSize(unsigned int msg_num)
{
  char *buf;
  QCString cmd;
  size_t ret = 0;

  buf = new char[MAX_RESPONSE_LEN];
  memset(buf, 0, MAX_RESPONSE_LEN);
  cmd.sprintf("LIST %u", msg_num);
  if (!command(cmd.data(), buf, MAX_RESPONSE_LEN)) {
    delete[]buf;
    return 0;
  } else {
    cmd = buf;
    cmd.remove(0, cmd.find(" "));
    ret = cmd.toLong();
  }
  delete[]buf;
  return ret;
}

void POP3Protocol::special(const QByteArray & aData)
{
  QString result;
  char buf[MAX_PACKET_LEN];
  QDataStream stream(aData, IO_ReadOnly);
  int tmp;
  stream >> tmp;

  if (tmp != 'c')
    return;

  for (int i = 0; i < 2; i++) {
    QCString cmd = (i) ? "AUTH" : "CAPA";
    if (!command(cmd))
      continue;
    while (true) {
      myReadLine(buf, MAX_PACKET_LEN - 1);
      if (qstrcmp(buf, ".\r\n") == 0)
        break;
      result += " " + QString(buf).left(strlen(buf) - 2)
          .replace(" ", "-");
    }
  }
  if (supports_apop)
    result += " APOP";
  result = result.mid(1);
  infoMessage(result);
  finished();
}

void POP3Protocol::get(const KURL & url)
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

  bool ok = true;
  char buf[MAX_PACKET_LEN];
  char destbuf[MAX_PACKET_LEN];
  QByteArray array;
  QString cmd, path = url.path();
  int maxCommands = (metaData("pipelining") == "on") ? MAX_COMMANDS : 1;

  if (path.at(0) == '/')
    path.remove(0, 1);
  if (path.isEmpty()) {
    POP3_DEBUG << "We should be a dir!!" << endl;
    error(ERR_IS_DIRECTORY, url.url());
    m_cmd = CMD_NONE;
    return;
  }

  if (((path.find('/') == -1) && (path != "index") && (path != "uidl")
       && (path != "commit"))) {
    error(ERR_MALFORMED_URL, url.url());
    m_cmd = CMD_NONE;
    return;
  }

  cmd = path.left(path.find('/'));
  path.remove(0, path.find('/') + 1);

  if (!pop3_open()) {
    POP3_DEBUG << "pop3_open failed" << endl;
    closeConnection();
    error(ERR_COULD_NOT_CONNECT, m_sServer);
    return;
  }

  if ((cmd == "index") || (cmd == "uidl")) {
    unsigned long size = 0;
    bool result;

    if (cmd == "index") {
      result = command("LIST");
    } else {
      result = command("UIDL");
    }

    /*
       LIST
       +OK Mailbox scan listing follows
       1 2979
       2 1348
       .
     */
    if (result) {
      while (true /* !AtEOF() */ ) {
        memset(buf, 0, sizeof(buf));
        myReadLine(buf, sizeof(buf) - 1);

        // HACK: This assumes fread stops at the first \n and not \r
        if (strcmp(buf, ".\r\n") == 0) {
          break;                // End of data
        }
        // sanders, changed -2 to -1 below
        int bufStrLen = strlen(buf);
        buf[bufStrLen - 2] = '\0';
        size += bufStrLen;
        array.setRawData(buf, bufStrLen);
        data(array);
        array.resetRawData(buf, bufStrLen);
        totalSize(size);
      }
    }
    POP3_DEBUG << "Finishing up list" << endl;
    data(QByteArray());
    finished();
  } else if (cmd == "remove") {
    QStringList waitingCommands = QStringList::split(',', path);
    int activeCommands = 0;
    QStringList::Iterator it = waitingCommands.begin();
    while (it != waitingCommands.end() || activeCommands > 0) {
      while (activeCommands < maxCommands && it != waitingCommands.end()) {
        sendCommand(("DELE " + *it).latin1());
        activeCommands++;
        it++;
      }
      getResponse(buf, sizeof(buf) - 1, "");
      activeCommands--;
    }
    finished();
    m_cmd = CMD_NONE;
  } else if (cmd == "download" || cmd == "headers") {
    QStringList waitingCommands = QStringList::split(',', path);
    bool noProgress = (metaData("progress") == "off"
                       || waitingCommands.count() > 1);
    int p_size = 0;
    unsigned int msg_len = 0;
    QString list_cmd("LIST ");
    list_cmd += path;
    memset(buf, 0, sizeof(buf));
    if ( !noProgress ) {
      if (command(list_cmd.ascii(), buf, sizeof(buf) - 1)) {
        list_cmd = buf;
        // We need a space, otherwise we got an invalid reply
        if (!list_cmd.find(" ")) {
          POP3_DEBUG << "List command needs a space? " << list_cmd << endl;
          closeConnection();
          error(ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
          return;
        }
        list_cmd.remove(0, list_cmd.find(" ") + 1);
        msg_len = list_cmd.toUInt(&ok);
        if (!ok) {
          POP3_DEBUG << "LIST command needs to return a number? :" <<
              list_cmd << ":" << endl;
          closeConnection();
          error(ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
          return;
        }
      } else {
        closeConnection();
        error(ERR_COULD_NOT_READ, m_sError);
        return;
      }
    }

    int activeCommands = 0;
    QStringList::Iterator it = waitingCommands.begin();
    while (it != waitingCommands.end() || activeCommands > 0) {
      while (activeCommands < maxCommands && it != waitingCommands.end()) {
        sendCommand(((cmd ==
                      "headers") ? "TOP " + *it + " 0" : "RETR " +
                     *it).latin1());
        activeCommands++;
        it++;
      }
      if (getResponse(buf, sizeof(buf) - 1, "")) {
        activeCommands--;
        mimeType("message/rfc822");
        totalSize(msg_len);
        memset(buf, 0, sizeof(buf));
        char ending = '\n';
        bool endOfMail = false;
        bool eat = false;
        while (true /* !AtEOF() */ ) {
          ssize_t readlen = myRead(buf, sizeof(buf) - 1);
          if (readlen <= 0) {
            if (isConnectionValid())
              error(ERR_SERVER_TIMEOUT, m_sServer);
            else
              error(ERR_CONNECTION_BROKEN, m_sServer);
            closeConnection();
            return;
          }
          if (ending == '.' && readlen > 1 && buf[0] == '\r'
              && buf[1] == '\n') {
            readBufferLen = readlen - 2;
            memcpy(readBuffer, &buf[2], readBufferLen);
            break;
          }
          bool newline = (ending == '\n');

          if (buf[readlen - 1] == '\n')
            ending = '\n';
          else if (buf[readlen - 1] == '.'
                   && ((readlen > 1) ? buf[readlen - 2] == '\n' : ending ==
                       '\n'))
            ending = '.';
          else
            ending = ' ';

          char *buf1 = buf, *buf2 = destbuf;
          // ".." at start of a line means only "."
          // "." means end of data
          for (ssize_t i = 0; i < readlen; i++) {
            if (*buf1 == '\r' && eat) {
              endOfMail = true;
              if (i == readlen - 1 /* && !AtEOF() */ )
                myRead(buf, 1);
              else if (i < readlen - 2) {
                readBufferLen = readlen - i - 2;
                memcpy(readBuffer, &buf[i + 2], readBufferLen);
              }
              break;
            } else if (*buf1 == '\n') {
              newline = true;
              eat = false;
            } else if (*buf1 == '.' && newline) {
              newline = false;
              eat = true;
            } else {
              newline = false;
              eat = false;
            }
            if (!eat) {
              *buf2 = *buf1;
              buf2++;
            }
            buf1++;
          }

          if (buf2 > destbuf) {
            array.setRawData(destbuf, buf2 - destbuf);
            data(array);
            array.resetRawData(destbuf, buf2 - destbuf);
          }

          if (endOfMail)
            break;

          if (!noProgress) {
            p_size += readlen;
            processedSize(p_size);
          }
        }
        infoMessage("message complete");
      } else {
        POP3_DEBUG << "Couldn't login. Bad RETR Sorry" << endl;
        closeConnection();
        error(ERR_COULD_NOT_READ, m_sError);
        return;
      }
    }
    POP3_DEBUG << "Finishing up" << endl;
    data(QByteArray());
    finished();
  } else if ((cmd == "uid") || (cmd == "list")) {
    QString qbuf;
    (void) path.toInt(&ok);

    if (!ok) {
      return;                   //  We fscking need a number!
    }

    if (cmd == "uid") {
      path.prepend("UIDL ");
    } else {
      path.prepend("LIST ");
    }

    memset(buf, 0, sizeof(buf));
    if (command(path.ascii(), buf, sizeof(buf) - 1)) {
      const int len = strlen(buf);
      mimeType("text/plain");
      totalSize(len);
      array.setRawData(buf, len);
      data(array);
      array.resetRawData(buf, len);
      processedSize(len);
      POP3_DEBUG << buf << endl;
      POP3_DEBUG << "Finishing up uid" << endl;
      data(QByteArray());
      finished();
    } else {
      closeConnection();
      error(ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
      return;
    }
  } else if (cmd == "commit") {
    POP3_DEBUG << "Issued QUIT" << endl;
    closeConnection();
    finished();
    m_cmd = CMD_NONE;
    return;
  }
}

void POP3Protocol::listDir(const KURL &)
{
  bool isINT;
  int num_messages = 0;
  char buf[MAX_RESPONSE_LEN];
  QCString q_buf;

  // Try and open a connection
  if (!pop3_open()) {
    POP3_DEBUG << "pop3_open failed" << endl;
    error(ERR_COULD_NOT_CONNECT, m_sServer);
    closeConnection();
    return;
  }
  // Check how many messages we have. STAT is by law required to
  // at least return +OK num_messages total_size
  memset(buf, 0, MAX_RESPONSE_LEN);
  if (!command("STAT", buf, MAX_RESPONSE_LEN)) {
    error(ERR_INTERNAL, "??");
    return;
  }
  POP3_DEBUG << "The stat buf is :" << buf << ":" << endl;
  q_buf = buf;
  if (q_buf.find(" ") == -1) {
    error(ERR_INTERNAL,
          "Invalid POP3 response, we should have at least one space!");
    closeConnection();
    return;
  }
  q_buf.remove(q_buf.find(" "), q_buf.length());

  num_messages = q_buf.toUInt(&isINT);
  if (!isINT) {
    error(ERR_INTERNAL, "Invalid POP3 STAT response!");
    closeConnection();
    return;
  }
  UDSEntry entry;
  UDSAtom atom;
  QString fname;
  for (int i = 0; i < num_messages; i++) {
    fname = "Message %1";

    atom.m_uds = UDS_NAME;
    atom.m_long = 0;
    atom.m_str = fname.arg(i + 1);
    entry.append(atom);

    atom.m_uds = UDS_MIME_TYPE;
    atom.m_long = 0;
    atom.m_str = "text/plain";
    entry.append(atom);
    POP3_DEBUG << "Mimetype is " << atom.m_str.ascii() << endl;

    atom.m_uds = UDS_URL;
    KURL uds_url;
    if (m_bIsSSL) {
      uds_url.setProtocol("pop3s");
    } else {
      uds_url.setProtocol("pop3");
    }

    uds_url.setUser(m_sUser);
    uds_url.setPass(m_sPass);
    uds_url.setHost(m_sServer);
    uds_url.setPath(QString::fromLatin1("/download/%1").arg(i + 1));
    atom.m_str = uds_url.url();
    atom.m_long = 0;
    entry.append(atom);

    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    atom.m_long = S_IFREG;
    entry.append(atom);

    atom.m_uds = UDS_SIZE;
    atom.m_str = "";
    atom.m_long = realGetSize(i + 1);
    entry.append(atom);

    atom.m_uds = KIO::UDS_ACCESS;
    atom.m_long = S_IRUSR | S_IXUSR | S_IWUSR;
    entry.append (atom);

    listEntry(entry, false);
    entry.clear();
  }
  listEntry(entry, true);       // ready

  finished();
}

void POP3Protocol::stat(const KURL & url)
{
  QString _path = url.path();

  if (_path.at(0) == '/')
    _path.remove(0, 1);

  UDSEntry entry;
  UDSAtom atom;

  atom.m_uds = UDS_NAME;
  atom.m_str = _path;
  entry.append(atom);

  atom.m_uds = UDS_FILE_TYPE;
  atom.m_str = "";
  atom.m_long = S_IFREG;
  entry.append(atom);

  atom.m_uds = UDS_MIME_TYPE;
  atom.m_str = "message/rfc822";
  entry.append(atom);

  // TODO: maybe get the size of the message?
  statEntry(entry);

  finished();
}

void POP3Protocol::del(const KURL & url, bool /*isfile */ )
{
  QString invalidURI = QString::null;
  bool isInt;

  if (!pop3_open()) {
    POP3_DEBUG << "pop3_open failed" << endl;
    error(ERR_COULD_NOT_CONNECT, m_sServer);
    closeConnection();
    return;
  }

  QString _path = url.path();
  if (_path.at(0) == '/') {
    _path.remove(0, 1);
  }

  _path.toUInt(&isInt);
  if (!isInt) {
    invalidURI = _path;
  } else {
    _path.prepend("DELE ");
    if (!command(_path.ascii())) {
      invalidURI = _path;
    }
  }

  POP3_DEBUG << "POP3Protocol::del " << _path << endl;
  finished();
}
