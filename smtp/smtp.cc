/*
 * Copyright (c) 2000, 2001 Alex Zepeda <jazepeda@pacbell.net>
 * Copyright (c) 2001 Michael Häckel <Michael@Haeckel.Net>
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

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <qbuffer.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qcstring.h>
#include <qglobal.h>

#include <kprotocolmanager.h>
#include <kemailsettings.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <kio/kdesasl.h>
#include <klocale.h>
#include <iostream.h>

#include "smtp.h"

#define ASCII(x) QString::fromLatin1(x)
#define WRITE_STRING(x) write (x.latin1(), strlen(x.latin1()))

#define DEFAULT_RESPONSE_BUFFER 512
#define DEFAULT_EHLO_BUFFER 5120

using namespace KIO;

extern "C" {
  int kdemain(int argc, char **argv);
} void GetAddresses(const QString & str, const QString & delim,
                    QStringList & list);
int GetVal(char *buf);

int kdemain(int argc, char **argv)
{
  KInstance instance("kio_smtp");

  if (argc != 4) {
    fprintf(stderr,
            "Usage: kio_smtp protocol domain-socket1 domain-socket2\n");
    exit(-1);
  }

  bool useSSL = (strcmp(argv[1], "smtps") == 0) ? true : false;
  // We might as well allocate it on the heap.  Since there's a heap o room there..
  SMTPProtocol *slave = new SMTPProtocol(argv[2], argv[3], useSSL);
  slave->dispatchLoop();

  delete slave;
  return 0;
}


SMTPProtocol::SMTPProtocol(const QCString & pool, const QCString & app,
                           bool useSSL)
:  TCPSlaveBase(useSSL ? 465 : 25, useSSL ? "smtps" : "smtp", pool, app,
             useSSL)
{
  kdDebug() << "SMTPProtocol::SMTPProtocol" << endl;
  opened = false;
  haveTLS = false;
  m_iSock = 0;
  m_iOldPort = 0;
  m_sOldServer = QString::null;
  m_tTimeout.tv_sec = 10;
  m_tTimeout.tv_usec = 0;

  // Auth stuff
  m_pSASL = 0;
  m_sAuthConfig = QString::null;
}


SMTPProtocol::~SMTPProtocol()
{
  kdDebug() << "SMTPProtocol::~SMTPProtocol" << endl;
  smtp_close();
}

void SMTPProtocol::openConnection()
{
  if (smtp_open()) {
    connected();
  } else {
    closeConnection();
  }
}

void SMTPProtocol::closeConnection()
{
  smtp_close();
}

void SMTPProtocol::special(const QByteArray & aData)
{
  QString result;
  if (haveTLS)
    result = " STARTTLS";
  if (!m_sAuthConfig.isEmpty())
    result += " " + m_sAuthConfig;
  infoMessage(result.mid(1));
  finished();
}


// Usage: smtp://smtphost:port/send?to=user@host.com&subject=blah
// If smtphost is the name of a profile, it'll use the information 
// provided by that profile.  If it's not a profile name, it'll use it as
// nature intended.
// One can also specify in the query:
// headers=0 (turns of header generation)
// to=emailaddress
// cc=emailaddress
// bcc=emailaddress
// subject=text
// profile=text (this will override the "host" setting
void SMTPProtocol::put(const KURL & url, int /*permissions */ ,
                       bool /*overwrite */ , bool /*resume */ )
{
  QString query = url.query();
  QString subject = ASCII("missing subject");
  QString profile = QString::null;
  QString from = QString::null;
  QStringList recip, bcc, cc, temp_list;
  bool headers = true;

  GetAddresses(query, ASCII("to="), recip);
  GetAddresses(query, ASCII("cc="), cc);
  GetAddresses(query, ASCII("bcc="), bcc);

  GetAddresses(query, ASCII("headers="), temp_list);
  if (temp_list.count() && temp_list.last() == "0") {
    headers = false;
  }
  // find the subject
  GetAddresses(query, ASCII("subject="), temp_list);
  if (temp_list.count()) {
    subject = temp_list.last();
    temp_list.clear();
  }

  GetAddresses(query, ASCII("from="), temp_list);
  if (temp_list.count()) {
    from = temp_list.last();
    temp_list.clear();
  }

  GetAddresses(query, ASCII("profile="), temp_list);
  if (temp_list.count()) {
    profile = temp_list.last();
    temp_list.clear();
  }

  KEMailSettings *mset = new KEMailSettings;
  KURL open_url = url;
  if (profile == QString::null) {
    kdDebug() << "kio_smtp: Profile is null" << endl;
    QStringList profiles = mset->profiles();
    bool hasProfile = false;
    for (QStringList::Iterator it = profiles.begin(); it != profiles.end();
         ++it) {
      if ((*it) == open_url.host()) {
        hasProfile = true;
        break;
      }
    }
    if (hasProfile) {
      mset->setProfile(open_url.host());
      open_url.setHost(mset->getSetting(KEMailSettings::OutServer));
      m_sUser = mset->getSetting(KEMailSettings::OutServerLogin);
      m_sPass = mset->getSetting(KEMailSettings::OutServerPass);

      if (m_sUser.isEmpty())
        m_sUser = QString::null;
      if (m_sPass.isEmpty())
        m_sPass = QString::null;
      open_url.setUser(m_sUser);
      open_url.setPass(m_sPass);
      m_sServer = open_url.host();
      m_iPort = open_url.port();
    } else {
      mset->setProfile(mset->defaultProfileName());
    }
  } else {
    mset->setProfile(profile);
  }

  // Check KEMailSettings to see if we've specified an E-Mail address
  // if that worked, check to see if we've specified a real name
  // and then format accordingly (either: emailaddress@host.com or
  // Real Name <emailaddress@host.com>)
  if (from.isEmpty()) {
    if (mset->getSetting(KEMailSettings::EmailAddress) != QString::null) {
      from = mset->getSetting(KEMailSettings::EmailAddress);
    } else {
      error(ERR_NO_CONTENT, i18n("The sender address is missing."));
      smtp_close();
      return;
    }
  }
  from.prepend(ASCII("MAIL FROM: <"));
  from.append(ASCII(">"));

  if (!smtp_open())
    error(ERR_SERVICE_NOT_AVAILABLE,
          i18n("SMTPProtocol::smtp_open failed (%1)").arg(open_url.
                                                          path()));

  if (!command(from)) {
    if (!errorSent)
      error(ERR_NO_CONTENT, i18n("The server didn't accept the "
                                 "sender address:\n%1").arg(lastError));
    smtp_close();
    return;
  }
  // Loop through our To and CC recipients, and send the proper
  // SMTP commands, for the benefit of the server.
  if (!PutRecipients(recip, open_url))
    return;                     // To
  if (!PutRecipients(cc, open_url))
    return;                     // Carbon Copy (CC)
  if (!PutRecipients(bcc, open_url))
    return;                     // Blind Carbon Copy (BCC)

  // Begin sending the actual message contents (most headers+body)
  if (!command(ASCII("DATA"))) {
    if (!errorSent)
      error(ERR_NO_CONTENT, i18n("The attempt to start sending the "
                                 "message content failed.\nThe server said:\n%1")
            .arg(lastError));
    smtp_close();
    return;
  }

  if (headers) {
    if (mset->getSetting(KEMailSettings::EmailAddress) != QString::null) {
      if (mset->getSetting(KEMailSettings::RealName) != QString::null) {
        from =
            ASCII("From: %1 <%2>\r\n").arg(mset->
                                           getSetting(KEMailSettings::
                                                      RealName))
            .arg(mset->getSetting(KEMailSettings::EmailAddress));
      } else {
        from =
            ASCII("From: %1\r\n").arg(mset->
                                      getSetting(KEMailSettings::
                                                 EmailAddress));
      }
    } else {
      error(ERR_NO_CONTENT, i18n("The sender address is missing."));
      smtp_close();
      return;
    }
    WRITE_STRING(from);

    subject = ASCII("Subject: %1\r\n").arg(subject);
    WRITE_STRING(subject);

    // Write out the To header for the benefit of the mail clients
    query = ASCII("To: %1\r\n");
    for (QStringList::Iterator it = recip.begin(); it != recip.end(); ++it) {
      query = query.arg(*it);
      WRITE_STRING(query);
    }

    // Write out the CC header for the benefit of the mail clients
    query = ASCII("CC: %1\r\n");
    for (QStringList::Iterator it = cc.begin(); it != cc.end(); ++it) {
      query = query.arg(*it);
      WRITE_STRING(query);
    }
  }

  delete mset;

  // Loop until we got 0 (end of data)
  int result;
  QByteArray buffer;

  do {
    dataReq();                  // Request for data
    buffer.resize(0);
    result = readData(buffer);
    if (result > 0) {
      write(buffer.data(), buffer.size());
    } else if (result < 0) {
      error(ERR_COULD_NOT_WRITE, open_url.path());
    }
  } while (result > 0);

  write("\r\n.\r\n", 5);
  if (getResponse() >= 400) {
    if (!errorSent)
      error(ERR_NO_CONTENT, i18n("The server didn't accept the "
                                 "message content:\n%1").arg(lastError));
    smtp_close();
    return;
  }
  command(ASCII("RSET"));
  finished();
}

bool SMTPProtocol::PutRecipients(QStringList & list, const KURL & url)
{
  QString formatted_recip = ASCII("RCPT TO: <%1>");
  for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
    if (!command(formatted_recip.arg(*it))) {
      if (!errorSent)
        error(ERR_NO_CONTENT,
              i18n
              ("The server didn't accept one of the recipients.\nIt said: ").
              arg(lastError));
      smtp_close();
      return FALSE;
    }
  }
  return TRUE;
}

void SMTPProtocol::setHost(const QString & host, int port,
                           const QString & user, const QString & pass)
{
  m_sServer = host;
  m_iPort = port;
  m_sUser = user;
  m_sPass = pass;
}

int SMTPProtocol::getResponse(char *r_buf, unsigned int r_len)
{
  char *buf = 0;
  unsigned int recv_len = 0, len;
  int retVal;
  lastError = QCString();
  errorSent = false;

  // Give the buffer the appropiate size
  // a buffer of less than 5 bytes will *not* work
  if (r_len) {
    buf = static_cast < char *>(malloc(r_len));
    len = r_len;
  } else {
    buf = static_cast < char *>(malloc(DEFAULT_RESPONSE_BUFFER));
    len = DEFAULT_RESPONSE_BUFFER;
  }

  waitForResponse(60);

  // Clear out the buffer
  memset(buf, 0, len);
  // And grab the data
  recv_len = readLine(buf, len - 1);

  if (recv_len <= 0) {
    if (isConnectionValid())
      error(ERR_SERVER_TIMEOUT, m_sServer);
    else
      error(ERR_CONNECTION_BROKEN, m_sServer);
    errorSent = true;
    return 999;
  }

  if (recv_len < 4) {
    error(ERR_NO_CONTENT, i18n("Invalid SMTP response received."));
    errorSent = true;
    return 999;
  }
  char *origbuf = buf;
  if (buf[3] == '-') {          // Multiline response
    while ((buf[3] == '-') && (len - recv_len > 3)) {   // Three is quite arbitrary
      buf += recv_len;
      len -= (recv_len + 1);
      waitForResponse(60);
      recv_len = readLine(buf, len - 1);
      if (recv_len <= 0) {
        if (isConnectionValid())
          error(ERR_SERVER_TIMEOUT, m_sServer);
        else
          error(ERR_CONNECTION_BROKEN, m_sServer);
        errorSent = true;
        return 999;
      }
      if (recv_len < 4) {
        error(ERR_NO_CONTENT, i18n("Invalid SMTP response received."));
        errorSent = true;
        return 999;
      }
    }
    buf = origbuf;
    if (r_len) {
      memcpy(r_buf, buf, strlen(buf));
      r_buf[r_len - 1] = 0;
    }
    lastError = QCString(buf + 4, recv_len - 4);
    retVal = GetVal(buf);
  } else {
    // Really crude, whee
    if (r_len) {
      r_len = recv_len - 4;
      memcpy(r_buf, buf + 4, r_len);
    }
    lastError = QCString(buf + 4, recv_len - 4);
    retVal = GetVal(buf);
  }
  if (retVal == -1) {
    if (!isConnectionValid())
      error(ERR_CONNECTION_BROKEN, m_sServer);
    else
      error(ERR_NO_CONTENT,
            i18n("Invalid SMTP response received: %1").arg(lastError));
    errorSent = true;
    return 999;
  }
  return retVal;
}

bool SMTPProtocol::command(const QString & cmd, char *recv_buf,
                           unsigned int len)
{
  QCString write_buf = cmd.latin1();
  write_buf += "\r\n";

  // Write the command
  if (write(static_cast < const char *>(write_buf), write_buf.length()) !=
      static_cast < ssize_t > (write_buf.length())) {
    m_sError = i18n("Could not send to server.\n");
    return false;
  }

  return (getResponse(recv_buf, len) < 400);
}

bool SMTPProtocol::smtp_open()
{
  if (opened && (m_iOldPort == port(m_iPort))
      && (m_sOldServer == m_sServer) && (m_sOldUser == m_sUser)) {
    return true;
  } else {
    smtp_close();
    if (!connectToHost(m_sServer.latin1(), m_iPort))
      return false;             // connectToHost has already send an error message.
    opened = true;
  }

  if (getResponse() >= 400) {
    if (!errorSent)
      error(ERR_COULD_NOT_LOGIN, i18n("The server didn't accept "
        "the connection: %1").arg(lastError));
    return false;
  }

  QBuffer ehlobuf(QByteArray(DEFAULT_EHLO_BUFFER));
  memset(ehlobuf.buffer().data(), 0, DEFAULT_EHLO_BUFFER);

  char hostname[100];
  gethostname(hostname, 100);

  if(hostname[0] == '\0')
  {
    strncpy(hostname, "there", strlen("there") + 1);
  }
  
  if (!command
      (ASCII("EHLO " + QCString(hostname, 100)), ehlobuf.buffer().data(),
       5119)) {
    if (errorSent)
      return false;
    // Let's just check to see if it speaks plain ol' SMTP
    if (!command(ASCII("HELO " + QCString(hostname, 100)))) {
      if (!errorSent)
        error(ERR_COULD_NOT_LOGIN,
              i18n("The server said: %1").arg(lastError));
      smtp_close();
      return false;
    }
  }
  // We parse the ESMTP extensions here... pipelining would be really cool
  char ehlo_line[DEFAULT_EHLO_BUFFER];
  if (ehlobuf.open(IO_ReadWrite)) {
    while (ehlobuf.readLine(ehlo_line, DEFAULT_EHLO_BUFFER) > 0)
      ParseFeatures(const_cast < const char *>(ehlo_line));
  }

  if ((haveTLS && canUseTLS() && metaData("tls") != "off")
      || metaData("tls") == "on") {
    // For now we're gonna force it on.
    if (command(ASCII("STARTTLS"))) {
      int tlsrc = startTLS();
      if (tlsrc == 1) {
        kdDebug() << "TLS has been enabled!" << endl;
      } else {
        if (tlsrc != -3) {
          kdDebug() << "TLS negotiation failed!" << endl;
          messageBox(Information,
                     i18n("Your SMTP server claims to "
                          "support TLS, but negotiation "
                          "was unsuccessful.\nYou can "
                          "disable TLS in KDE using the "
                          "crypto settings module."),
                     i18n("Connection Failed"));
        }
        return false;
      }
      if (!command(ASCII("EHLO " + QCString(hostname, 100)))) {
        error(ERR_COULD_NOT_LOGIN,
              i18n("The server said: %1").arg(lastError));
        smtp_close();
        return false;
      }
    } else if (metaData("tls") == "on") {
      if (!errorSent)
        error(ERR_COULD_NOT_LOGIN,
              i18n("Your SMTP server does not support TLS. Disable "
                   "TLS, if you want to connect without encryption."));
      return false;
    }
  }
  // Now we try and login
  if (!m_sUser.isNull()) {
    if (m_sPass.isNull()) {
      KIO::AuthInfo authInfo;
      authInfo.username = m_sUser;
      authInfo.password = m_sPass;
      authInfo.prompt = i18n("Username and password for your SMTP account:");
      if (!openPassDlg(authInfo)) {
        error(ERR_COULD_NOT_LOGIN, i18n("When prompted, you ran away."));
        return false;
      } else {
        m_sUser = authInfo.username;
        m_sPass = authInfo.password;
      }
    }
    if (!Authenticate()) {
      return false;
    }
  }

  m_iOldPort = m_iPort;
  m_sOldServer = m_sServer;
  m_sOldUser = m_sUser;
  m_sOldPass = m_sPass;

  return true;
}

bool SMTPProtocol::Authenticate()
{
  bool ret;
  QString auth_method;

  if (m_pSASL)
    delete m_pSASL;
  m_pSASL = new KDESasl(m_sUser, m_sPass, (m_bIsSSL) ? "smtps" : "smtp");

  // Choose available method from what the server has given us in  its greeting
  QStringList sl = QStringList::split(' ', m_sAuthConfig);
  // If the server doesn't support/require authentication, we ignore it
  if (sl.isEmpty())
    return true;
  QStrIList strList;
  if (!metaData("sasl").isEmpty())
    strList.append(metaData("sasl").latin1());
  else
    for (int i = 0; i < sl.count(); i++)
      strList.append(sl[i].latin1());

  auth_method = m_pSASL->chooseMethod(strList);

  // If none are available, set it up so we can start over again
  if (auth_method == QString::null) {
    delete m_pSASL;
    m_pSASL = 0;
    kdDebug() << "kio_smtp: no authentication available" << endl;
    error(ERR_COULD_NOT_LOGIN,
          i18n("No compatible authentication methods found."));
    return false;
  } else {
    char *challenge = static_cast < char *>(malloc(2049));

    if (!command(ASCII("AUTH ") + auth_method, challenge, 2049)) {
      free(challenge);
      delete m_pSASL;
      m_pSASL = 0;
      if (!errorSent)
        error(ERR_COULD_NOT_LOGIN,
              i18n
              ("Your SMTP server doesn't support %1.\nChoose a different authentication method.").
              arg(auth_method));
      return false;
    }

    QString cmd;
    QByteArray ba;
    ba.duplicate(challenge, strlen(challenge));
    cmd = m_pSASL->getResponse(ba);
    ret = command(cmd, challenge, 2049);
    if (auth_method.upper() == "DIGEST-MD5"
        || auth_method.upper() == "LOGIN") {
      ba.duplicate(challenge, strlen(challenge));
      cmd = m_pSASL->getResponse(ba);
      ret = command(cmd);
    }
    free(challenge);

    if (!ret && !errorSent)
      error(ERR_COULD_NOT_LOGIN,
            i18n
            ("Authentication failed.\nMost likely the password is wrong.\nThe server said: %1").
            arg(lastError));
    return ret;
  }
  return false;
}

void SMTPProtocol::ParseFeatures(const char *_buf)
{
  QCString buf(_buf);

  // We want it to be between 250 and 259 inclusive, and it needs to be "nnn-blah" or "nnn blah"
  // So sez the SMTP spec..
  if ((buf.left(2) != "25") || (!isdigit(buf[2]))
      || (!(buf.at(3) == '-') && !(buf.at(3) == ' ')))
    return;                     // We got an invalid line..
  // Clop off the beginning, no need for it really
  buf = buf.mid(4, buf.length());

  if (buf.left(4) == "AUTH") {  // Look for auth stuff
    if (m_sAuthConfig == QString::null)
      m_sAuthConfig = buf.mid(5, buf.length() - 7);
  } else if (buf.left(8) == "STARTTLS") {
    haveTLS = true;
  }
}

void SMTPProtocol::smtp_close()
{
  if (!opened)                  // We're already closed
    return;
  command(ASCII("QUIT"));
  closeDescriptor();
  m_sOldServer = QString::null;
  m_pSASL = 0;
  m_sAuthConfig = QString::null;
  opened = false;
}

void SMTPProtocol::stat(const KURL & url)
{
  QString path = url.path();
  error(KIO::ERR_DOES_NOT_EXIST, url.path());
}

int GetVal(char *buf)
{
  bool ok;
  QCString st(buf, 4);
  int val = st.toInt(&ok);
  return (ok) ? val : -1;
}

void GetAddresses(const QString & str, const QString & delim,
                  QStringList & list)
{
  int curpos = 0;

  while ((curpos = str.find(delim, curpos)) != -1) {
    if ((str.at(curpos - 1) == "?") || (str.at(curpos - 1) == "&")) {
      curpos += delim.length();
      if (str.find("&", curpos) != -1)
        list += KURL::decode_string(str.mid(curpos, str.find("&", curpos) - curpos));
      else
        list += KURL::decode_string(str.mid(curpos, str.length()));
    } else
      curpos += delim.length();
  }
}
