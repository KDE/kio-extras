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
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#include <pop3-config.h>
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

#ifdef HAVE_LIBSASL2
extern "C" {
#include <sasl/sasl.h>
}
#endif

#include <qglobal.h>
#include <QByteArray>
#include <QRegExp>

#include <kdebug.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kcodecs.h>
#include <kprotocolmanager.h>

#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include "pop3.h"

#define GREETING_BUF_LEN 1024
#define MAX_RESPONSE_LEN 512
#define MAX_COMMANDS 10

#define POP3_DEBUG kDebug(7105)

extern "C" {
  int KDE_EXPORT kdemain(int argc, char **argv);
}

using namespace KIO;

#ifdef HAVE_LIBSASL2
static sasl_callback_t callbacks[] = {
    { SASL_CB_ECHOPROMPT, NULL, NULL },
    { SASL_CB_NOECHOPROMPT, NULL, NULL },
    { SASL_CB_GETREALM, NULL, NULL },
    { SASL_CB_USER, NULL, NULL },
    { SASL_CB_AUTHNAME, NULL, NULL },
    { SASL_CB_PASS, NULL, NULL },
    { SASL_CB_GETOPT, NULL, NULL },
    { SASL_CB_CANON_USER, NULL, NULL },
    { SASL_CB_LIST_END, NULL, NULL }
};
#endif

int kdemain(int argc, char **argv)
{

  if (argc != 4) {
    POP3_DEBUG << "Usage: kio_pop3 protocol domain-socket1 domain-socket2"
        << endl;
    return -1;
  }

#ifdef HAVE_LIBSASL2
  if ( sasl_client_init( callbacks ) != SASL_OK ) {
    fprintf(stderr, "SASL library initialization failed!\n");
    return -1;
  }
#endif

  KComponentData componentData("kio_pop3");
  POP3Protocol *slave;

  // Are we looking to use SSL?
  if (strcasecmp(argv[1], "pop3s") == 0) {
    slave = new POP3Protocol(argv[2], argv[3], true);
  } else {
    slave = new POP3Protocol(argv[2], argv[3], false);
  }

  slave->dispatchLoop();
  delete slave;

#ifdef HAVE_LIBSASL2
  sasl_done();
#endif

  return 0;
}

POP3Protocol::POP3Protocol(const QByteArray & pool, const QByteArray & app,
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
  return read((char*)data, len);
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

POP3Protocol::Resp POP3Protocol::getResponse(char *r_buf, unsigned int r_len)
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
             qMin(r_len, (buf[3] == ' ' ? recv_len - 4 : recv_len - 3)));
    }

    delete[]buf;

    return Ok;
  } else if (strncmp(buf, "-ERR", 4) == 0) {
    if (r_buf && r_len) {
      memcpy(r_buf, (buf[4] == ' ' ? buf + 5 : buf + 4),
             qMin(r_len, (buf[4] == ' ' ? recv_len - 5 : recv_len - 4)));
    }

    QString serverMsg = QString::fromLatin1(buf).mid(5).trimmed();

    m_sError = i18n("The server said: \"%1\"", serverMsg);

    delete[]buf;

    return Err;
  } else if (strncmp(buf, "+ ", 2) == 0) {
    if (r_buf && r_len) {
      memcpy(r_buf, buf + 2, qMin(r_len, recv_len - 4));
      r_buf[qMin(r_len - 1, recv_len - 4)] = '\0';
    }

    delete[]buf;

    return Cont;
  } else {
    POP3_DEBUG << "Invalid POP3 response received!" << endl;

    if (r_buf && r_len) {
      memcpy(r_buf, buf, qMin(r_len, recv_len));
    }

    if (!*buf) {
      m_sError = i18n("The server terminated the connection.");
    } else {
      m_sError = i18n("Invalid response from server:\n\"%1\"", buf);
    }

    delete[]buf;

    return Invalid;
  }
}

bool POP3Protocol::sendCommand(const QByteArray &cmd)
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
  QByteArray cmdrn = cmd + "\r\n";

  if (write(cmdrn.data(), cmdrn.size()) != static_cast < ssize_t >
      (cmdrn.size())) {
    m_sError = i18n("Could not send to server.\n");
    return false;
  }

  return true;
}

POP3Protocol::Resp POP3Protocol::command(const QByteArray &cmd, char *recv_buf,
                           unsigned int len)
{
  sendCommand(cmd);
  return getResponse(recv_buf, len);
}

void POP3Protocol::openConnection()
{
  m_try_apop = !hasMetaData("auth") || metaData("auth") == "APOP";
  m_try_sasl = !hasMetaData("auth") || metaData("auth") == "SASL";

  if (!pop3_open()) {
    POP3_DEBUG << "pop3_open failed" << endl;
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

int POP3Protocol::loginAPOP( char *challenge, KIO::AuthInfo &ai )
{
  char buf[512];

  QString apop_string = QString::fromLatin1("APOP ");
  if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
    // Prompt for usernames
    if (!openPasswordDialog(ai)) {
      error(ERR_ABORTED, i18n("No authentication details supplied."));
      closeConnection();
      return -1;
    } else {
      m_sUser = ai.username;
      m_sPass = ai.password;
    }
  }
  m_sOldUser = m_sUser;
  m_sOldPass = m_sPass;

  apop_string.append(m_sUser);

  memset(buf, 0, sizeof(buf));

  KMD5 ctx;

  POP3_DEBUG << "APOP challenge: " << challenge << endl;

  // Generate digest
  ctx.update(challenge, strlen(challenge));
  ctx.update(m_sPass.toLatin1() );

  // Genenerate APOP command
  apop_string.append(" ");
  apop_string.append(ctx.hexDigest());

  if (command(apop_string.toLocal8Bit(), buf, sizeof(buf)) == Ok) {
    return 0;
  }

  POP3_DEBUG << "Couldn't login via APOP. Falling back to USER/PASS" <<
      endl;
  closeConnection();
  if (metaData("auth") == "APOP") {
    error(ERR_COULD_NOT_LOGIN,
          i18n
          ("Login via APOP failed. The server %1 may not support APOP, although it claims to support it, or the password may be wrong.\n\n%2", 
          m_sServer, 
          m_sError));
    return -1;
  }
  return 1;
}

bool POP3Protocol::saslInteract( void *in, AuthInfo &ai )
{
#ifdef HAVE_LIBSASL2
  POP3_DEBUG << "sasl_interact" << endl;
  sasl_interact_t *interact = ( sasl_interact_t * ) in;

  //some mechanisms do not require username && pass, so don't need a popup
  //window for getting this info
  for ( ; interact->id != SASL_CB_LIST_END; interact++ ) {
    if ( interact->id == SASL_CB_AUTHNAME ||
         interact->id == SASL_CB_PASS ) {

      if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
        if (!openPasswordDialog(ai)) {
          error(ERR_ABORTED, i18n("No authentication details supplied."));
          return false;
        }
        m_sUser = ai.username;
        m_sPass = ai.password;
      }
      break;
    }
  }

  interact = ( sasl_interact_t * ) in;
  while( interact->id != SASL_CB_LIST_END ) {
    POP3_DEBUG << "SASL_INTERACT id: " << interact->id << endl;
    switch( interact->id ) {
      case SASL_CB_USER:
      case SASL_CB_AUTHNAME:
        POP3_DEBUG << "SASL_CB_[USER|AUTHNAME]: " << m_sUser << endl;
        interact->result = strdup( m_sUser.toUtf8() );
        interact->len = strlen( (const char *) interact->result );
        break;
      case SASL_CB_PASS:
        POP3_DEBUG << "SASL_CB_PASS: [hidden] " << endl;
        interact->result = strdup( m_sPass.toUtf8() );
        interact->len = strlen( (const char *) interact->result );
        break;
      default:
        interact->result = NULL; interact->len = 0;
        break;
    }
    interact++;
  }
  return true;
#else
  return false;
#endif
}

#define SASLERROR  closeConnection(); \
error(ERR_COULD_NOT_AUTHENTICATE, i18n("An error occurred during authentication: %1",  \
 QString::fromUtf8( sasl_errdetail( conn ) ))); \

int POP3Protocol::loginSASL( KIO::AuthInfo &ai )
{
#ifdef HAVE_LIBSASL2
  char buf[512];
  QString sasl_buffer = QString::fromLatin1("AUTH");

  int result;
  sasl_conn_t *conn = NULL;
  sasl_interact_t *client_interact = NULL;
  const char *out = NULL;
  uint outlen;
  const char *mechusing = NULL;
  Resp resp;

  result = sasl_client_new( "pop",
                       m_sServer.toLatin1(),
                       0, 0, NULL, 0, &conn );

  if ( result != SASL_OK ) {
    POP3_DEBUG << "sasl_client_new failed with: " << result << endl;
    SASLERROR
    return false;
  }

  // We need to check what methods the server supports...
  // This is based on RFC 1734's wisdom
  if ( hasMetaData("sasl") || command(sasl_buffer.toLocal8Bit()) == Ok  ) {

    QStringList sasl_list;
    if (hasMetaData("sasl")) {
      sasl_list.append(metaData("sasl").toLatin1());
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

    do {
      result = sasl_client_start(conn, sasl_list.join(" ").toLatin1(),
        &client_interact, &out, &outlen, &mechusing);

      if (result == SASL_INTERACT)
        if ( !saslInteract( client_interact, ai ) ) {
          closeConnection();
          sasl_dispose( &conn );
          return -1;
        };
    } while ( result == SASL_INTERACT );
    if ( result != SASL_CONTINUE && result != SASL_OK ) {
      POP3_DEBUG << "sasl_client_start failed with: " << result << endl;
      SASLERROR
      sasl_dispose( &conn );
      return -1;
    }

    POP3_DEBUG << "Preferred authentication method is " << mechusing << "." << endl;

    QByteArray msg,tmp;

    QString firstCommand = "AUTH " + QString::fromLatin1( mechusing );
    msg = QByteArray::fromRawData( out, outlen ).toBase64();
    if ( !msg.isEmpty() ) {
      firstCommand += ' ';
      firstCommand += QString::fromLatin1( msg.data(), msg.size() );
    }

    tmp.resize( 2049 );
    resp = command( firstCommand.toLatin1(), tmp.data(), 2049 );
    while( resp == Cont ) {
      tmp.resize(msg.indexOf((char)0));
//      POP3_DEBUG << "S: " << QCString(challenge.data(),challenge.size()+1) << endl;
      msg = QByteArray::fromBase64( tmp );
      do {
        result = sasl_client_step(conn, msg.isEmpty() ? 0 : msg.data(),
                                msg.size(),
                                &client_interact,
                                &out, &outlen);

        if (result == SASL_INTERACT)
          if ( !saslInteract( client_interact, ai ) ) {
            closeConnection();
            sasl_dispose( &conn );
            return -1;
          };
      } while ( result == SASL_INTERACT );
      if ( result != SASL_CONTINUE && result != SASL_OK ) {
        POP3_DEBUG << "sasl_client_step failed with: " << result << endl;
        SASLERROR
        sasl_dispose( &conn );
        return -1;
      }

      msg = QByteArray::fromRawData( out, outlen ).toBase64();
//        POP3_DEBUG << "C: " << QCString(tmp.data(),tmp.size()+1) << endl;
      tmp.resize(2049);
      resp = command( msg, tmp.data(), 2049 );
    }

    sasl_dispose( &conn );
    if ( resp == Ok ) {
      POP3_DEBUG << "SASL authenticated" << endl;
      m_sOldUser = m_sUser;
      m_sOldPass = m_sPass;
      return 0;
    }

    if (metaData("auth") == "SASL") {
      closeConnection();
      error(ERR_COULD_NOT_LOGIN,
            i18n
            ("Login via SASL (%1) failed. The server may not support %2, or the password may be wrong.\n\n%3", 
            mechusing, mechusing, m_sError));
      return -1;
    }
  }

  if (metaData("auth") == "SASL") {
    closeConnection();
    error(ERR_COULD_NOT_LOGIN,
          i18n("Your POP3 server does not support SASL.\n"
               "Choose a different authentication method."));
    return -1;
  }
  return 1;
#else
  if (metaData("auth") == "SASL") {
    closeConnection();
    error(ERR_COULD_NOT_LOGIN, i18n("SASL authentication is not compiled into kio_pop3."));
    return -1;
  }
  return 1; //if SASL not explicitly required, try another method (USER/PASS)
#endif
}

bool POP3Protocol::loginPASS( KIO::AuthInfo &ai )
{
  char buf[512];

  if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
    // Prompt for usernames
    if (!openPasswordDialog(ai)) {
      error(ERR_ABORTED, i18n("No authentication details supplied."));
      closeConnection();
      return false;
    } else {
      m_sUser = ai.username;
      m_sPass = ai.password;
    }
  }
  m_sOldUser = m_sUser;
  m_sOldPass = m_sPass;

  QString one_string = QString::fromLatin1("USER ");
  one_string.append( m_sUser );

  if ( command(one_string.toLocal8Bit(), buf, sizeof(buf)) != Ok ) {
    POP3_DEBUG << "Couldn't login. Bad username Sorry" << endl;

    m_sError =
        i18n("Could not login to %1.\n\n", m_sServer) + m_sError;
    error(ERR_COULD_NOT_LOGIN, m_sError);
    closeConnection();

    return false;
  }

  one_string = QString::fromLatin1("PASS ");
  one_string.append(m_sPass);

  if ( command(one_string.toLocal8Bit(), buf, sizeof(buf)) != Ok ) {
    POP3_DEBUG << "Couldn't login. Bad password Sorry." << endl;
    m_sError =
        i18n
        ("Could not login to %1. The password may be wrong.\n\n%2", 
        m_sServer, m_sError);
    error(ERR_COULD_NOT_LOGIN, m_sError);
    closeConnection();
    return false;
  }
  POP3_DEBUG << "USER/PASS login succeeded" << endl;
  return true;
}

bool POP3Protocol::pop3_open()
{
  POP3_DEBUG << "pop3_open()" << endl;
  char  *greeting_buf;
  if ((m_iOldPort == port(QString::number(m_iPort)).toInt()) && (m_sOldServer == m_sServer) &&
      (m_sOldUser == m_sUser) && (m_sOldPass == m_sPass)) {
    POP3_DEBUG << "Reusing old connection" << endl;
    return true;
  }
  do {
    closeConnection();

    if (!connectToHost(m_sServer.toLatin1(), QString::number(m_iPort))) {
      // error(ERR_COULD_NOT_CONNECT, m_sServer);
      // ConnectToHost has already send an error message.
      return false;
    }
    opened = true;

    greeting_buf = new char[GREETING_BUF_LEN];
    memset(greeting_buf, 0, GREETING_BUF_LEN);

    // If the server doesn't respond with a greeting
    if (getResponse(greeting_buf, GREETING_BUF_LEN) != Ok) {
      m_sError =
          i18n("Could not login to %1.\n\n", m_sServer) +
          ((!greeting_buf
            || !*greeting_buf) ?
           i18n("The server terminated the connection immediately.") :
           i18n("Server does not respond properly:\n%1\n", 
           greeting_buf));
      error(ERR_COULD_NOT_LOGIN, m_sError);
      delete[]greeting_buf;
      closeConnection();
      return false;             // we've got major problems, and possibly the
      // wrong port
    }
    QString greeting(greeting_buf);
    delete[]greeting_buf;

    if (greeting.length() > 0) {
      greeting.truncate(greeting.length() - 2);
    }

    // Does the server support APOP?
    QString apop_cmd;
    QRegExp re("<[A-Za-z0-9\\.\\-_]+@[A-Za-z0-9\\.\\-_]+>$", Qt::CaseInsensitive);

    POP3_DEBUG << "greeting: " << greeting << endl;
    int apop_pos = greeting.indexOf(re);
    supports_apop = (bool) (apop_pos != -1);

    if (metaData("nologin") == "on")
      return true;

    if (metaData("auth") == "APOP" && !supports_apop) {
      error(ERR_COULD_NOT_LOGIN,
          i18n("Your POP3 server does not support APOP.\n"
               "Choose a different authentication method."));
      closeConnection();
      return false;
    }

    m_iOldPort = m_iPort;
    m_sOldServer = m_sServer;

    // Try to go into TLS mode
    if ((metaData("tls") == "on" || (canUseTLS() &&
                                     metaData("tls") != "off"))
        && command("STLS") == Ok ) {
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
        closeConnection();
        return false;
      }
    } else if (metaData("tls") == "on") {
      error(ERR_COULD_NOT_CONNECT,
            i18n("Your POP3 server does not support TLS. Disable "
                 "TLS, if you want to connect without encryption."));
      closeConnection();
      return false;
    }

    KIO::AuthInfo authInfo;
    authInfo.username = m_sUser;
    authInfo.password = m_sPass;
    authInfo.prompt = i18n("Username and password for your POP3 account:");

    if ( supports_apop && m_try_apop ) {
      POP3_DEBUG << "Trying APOP" << endl;
      int retval = loginAPOP( greeting.toLatin1().data() + apop_pos, authInfo );
      switch ( retval ) {
        case 0: return true;
        case -1: return false;
        default:
          m_try_apop = false;
      }
    } else if ( m_try_sasl ) {
      POP3_DEBUG << "Trying SASL" << endl;
      int retval = loginSASL( authInfo );
      switch ( retval ) {
        case 0: return true;
        case -1: return false;
        default:
          m_try_sasl = false;
      }
    } else {
      // Fall back to conventional USER/PASS scheme
      POP3_DEBUG << "Trying USER/PASS" << endl;
      return loginPASS( authInfo );
    }
  } while ( true );
}

size_t POP3Protocol::realGetSize(unsigned int msg_num)
{
  char *buf;
  QByteArray cmd;
  size_t ret = 0;

  buf = new char[MAX_RESPONSE_LEN];
  memset(buf, 0, MAX_RESPONSE_LEN);
  cmd = "LIST " + QByteArray::number( msg_num );
  if ( command(cmd, buf, MAX_RESPONSE_LEN) != Ok ) {
    delete[]buf;
    return 0;
  } else {
    cmd = buf;
    cmd.remove(0, cmd.indexOf(" "));
    ret = cmd.toLong();
  }
  delete[]buf;
  return ret;
}

void POP3Protocol::special(const QByteArray & aData)
{
  QString result;
  char buf[MAX_PACKET_LEN];
  QDataStream stream(aData);
  int tmp;
  stream >> tmp;

  if (tmp != 'c')
    return;

  for (int i = 0; i < 2; i++) {
    QByteArray cmd = (i) ? "AUTH" : "CAPA";
    if ( command(cmd) != Ok )
      continue;
    while (true) {
      myReadLine(buf, MAX_PACKET_LEN - 1);
      if (qstrcmp(buf, ".\r\n") == 0)
        break;
      result += ' ' + QString(buf).left(strlen(buf) - 2)
          .replace(" ", "-");
    }
  }
  if (supports_apop)
    result += " APOP";
  result = result.mid(1);
  infoMessage(result);
  finished();
}

void POP3Protocol::get(const KUrl & url)
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

  if (((path.indexOf('/') == -1) && (path != "index") && (path != "uidl")
       && (path != "commit"))) {
    error(ERR_MALFORMED_URL, url.url());
    m_cmd = CMD_NONE;
    return;
  }

  cmd = path.left(path.indexOf('/'));
  path.remove(0, path.indexOf('/') + 1);

  if (!pop3_open()) {
    POP3_DEBUG << "pop3_open failed" << endl;
    error(ERR_COULD_NOT_CONNECT, m_sServer);
    return;
  }

  if ((cmd == "index") || (cmd == "uidl")) {
    unsigned long size = 0;
    bool result;

    if (cmd == "index") {
      result = ( command("LIST") == Ok );
    } else {
      result = ( command("UIDL") == Ok );
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
        data(QByteArray::fromRawData(buf, bufStrLen));
        totalSize(size);
      }
    }
    POP3_DEBUG << "Finishing up list" << endl;
    data(QByteArray());
    finished();
  } else if (cmd == "remove") {
    QStringList waitingCommands = path.split(',');
    int activeCommands = 0;
    QStringList::Iterator it = waitingCommands.begin();
    while (it != waitingCommands.end() || activeCommands > 0) {
      while (activeCommands < maxCommands && it != waitingCommands.end()) {
        sendCommand(("DELE " + *it).toLatin1());
        activeCommands++;
        it++;
      }
      getResponse(buf, sizeof(buf) - 1);
      activeCommands--;
    }
    finished();
    m_cmd = CMD_NONE;
  } else if (cmd == "download" || cmd == "headers") {
    QStringList waitingCommands = path.split(',');
    bool noProgress = (metaData("progress") == "off"
                       || waitingCommands.count() > 1);
    int p_size = 0;
    unsigned int msg_len = 0;
    QString list_cmd("LIST ");
    list_cmd += path;
    memset(buf, 0, sizeof(buf));
    if ( !noProgress ) {
      if ( command(list_cmd.toLatin1(), buf, sizeof(buf) - 1) == Ok ) {
        list_cmd = buf;
        // We need a space, otherwise we got an invalid reply
        if (!list_cmd.indexOf(" ")) {
          POP3_DEBUG << "List command needs a space? " << list_cmd << endl;
          closeConnection();
          error(ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
          return;
        }
        list_cmd.remove(0, list_cmd.indexOf(" ") + 1);
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
                     *it).toLatin1());
        activeCommands++;
        it++;
      }
      if ( getResponse(buf, sizeof(buf) - 1) == Ok ) {
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
            data(QByteArray::fromRawData(destbuf, buf2-destbuf));            
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
    if ( command(path.toAscii(), buf, sizeof(buf) - 1) == Ok ) {
      const int len = strlen(buf);
      mimeType("text/plain");
      totalSize(len);
      data(QByteArray::fromRawData(buf, len));
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

void POP3Protocol::listDir(const KUrl &)
{
  bool isINT;
  int num_messages = 0;
  QByteArray q_buf(MAX_RESPONSE_LEN, 0);

  // Try and open a connection
  if (!pop3_open()) {
    POP3_DEBUG << "pop3_open failed" << endl;
    error(ERR_COULD_NOT_CONNECT, m_sServer);
    return;
  }
  // Check how many messages we have. STAT is by law required to
  // at least return +OK num_messages total_size
  if ( command("STAT", q_buf.data(), MAX_RESPONSE_LEN) != Ok ) {
    error(ERR_INTERNAL, "??");
    return;
  }
  POP3_DEBUG << "The stat buf is :" << q_buf << ":" << endl;
  if (q_buf.indexOf(" ") == -1) {
    error(ERR_INTERNAL,
          "Invalid POP3 response, we should have at least one space!");
    closeConnection();
    return;
  }
  q_buf.remove(q_buf.indexOf(" "), q_buf.length());

  num_messages = q_buf.toUInt(&isINT);
  if (!isINT) {
    error(ERR_INTERNAL, "Invalid POP3 STAT response!");
    closeConnection();
    return;
  }
  UDSEntry entry;
  QString fname;
  for (int i = 0; i < num_messages; i++) {
    fname = "Message %1";

    entry.insert(UDS_NAME, fname.arg(i + 1));
    entry.insert(UDS_MIME_TYPE, QString::fromLatin1("text/plain"));

    KUrl uds_url;
    if (m_bIsSSL) {
      uds_url.setProtocol("pop3s");
    } else {
      uds_url.setProtocol("pop3");
    }

    uds_url.setUser(m_sUser);
    uds_url.setPass(m_sPass);
    uds_url.setHost(m_sServer);
    uds_url.setPath(QString::fromLatin1("/download/%1").arg(i + 1));
    entry.insert(UDS_URL, uds_url.url());

    entry.insert(UDS_FILE_TYPE, S_IFREG);
    entry.insert(UDS_SIZE, realGetSize(i + 1));
    entry.insert(UDS_ACCESS, S_IRUSR | S_IXUSR | S_IWUSR);

    listEntry(entry, false);
    entry.clear();
  }
  listEntry(entry, true);       // ready

  finished();
}

void POP3Protocol::stat(const KUrl & url)
{
  QString _path = url.path();

  if (_path.at(0) == '/')
    _path.remove(0, 1);

  UDSEntry entry;
  entry.insert(UDS_NAME, _path);
  entry.insert(UDS_FILE_TYPE, S_IFREG);
  entry.insert(UDS_MIME_TYPE, QString::fromLatin1("message/rfc822"));

  // TODO: maybe get the size of the message?
  statEntry(entry);

  finished();
}

void POP3Protocol::del(const KUrl & url, bool /*isfile */ )
{
  QString invalidURI = QString();
  bool isInt;

  if (!pop3_open()) {
    POP3_DEBUG << "pop3_open failed" << endl;
    error(ERR_COULD_NOT_CONNECT, m_sServer);
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
    if ( command(_path.toAscii()) != Ok ) {
      invalidURI = _path;
    }
  }

  POP3_DEBUG << "POP3Protocol::del " << _path << endl;
  finished();
}
