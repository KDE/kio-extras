/*
 * Copyright (c) 2000, 2001 Alex Zepeda <zipzippy@sonic.net>
 * Copyright (c) 2001 Michael Häckel <Michael@Haeckel.Net>
 * Copyright (c) 2002 Aaron J. Seigo <aseigo@olympusproject.org>
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

#include "smtp.h"
#include "request.h"
using KioSMTP::Request;

#include <kemailsettings.h>
#include <ksock.h>
#include <kdebug.h>
#include <kmdcodec.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <kio/kdesasl.h>
#include <klocale.h>
#include <kidna.h>

#include <qbuffer.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qcstring.h>
#include <qregexp.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


//using namespace KIO;

extern "C" {
  int kdemain(int argc, char **argv);
} 

int kdemain(int argc, char **argv)
{
  KInstance instance("kio_smtp");

  if (argc != 4) {
    fprintf(stderr,
            "Usage: kio_smtp protocol domain-socket1 domain-socket2\n");
    exit(-1);
  }

  SMTPProtocol slave( argv[2], argv[3], qstricmp( argv[1], "smtps" ) == 0 );
  slave.dispatchLoop();
  return 0;
}

SMTPProtocol::SMTPProtocol(const QCString & pool, const QCString & app,
                           bool useSSL)
:  TCPSlaveBase(useSSL ? 465 : 25, 
                useSSL ? "smtps" : "smtp", 
                pool, app, useSSL),
   m_iOldPort(0),
   m_opened(false),
   m_haveTLS(false),
   m_errorSent(false)
{
  //kdDebug() << "SMTPProtocol::SMTPProtocol" << endl;
}


SMTPProtocol::~SMTPProtocol()
{
  //kdDebug() << "SMTPProtocol::~SMTPProtocol" << endl;
  smtp_close();
}

void SMTPProtocol::openConnection()
{
  if (smtp_open()) {
    connected();
  } 
  else {
    closeConnection();
  }
}

void SMTPProtocol::closeConnection()
{
  smtp_close();
}

void SMTPProtocol::special(const QByteArray & /* aData */)
{
  QString result;
  if (m_haveTLS)
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
// headers=0 (turns off header generation)
// to=emailaddress
// cc=emailaddress
// bcc=emailaddress
// subject=text
// profile=text (this will override the "host" setting)
// hostname=text (used in the HELO)
void SMTPProtocol::put(const KURL & url, int /*permissions */ ,
                       bool /*overwrite */ , bool /*resume */ )
{
  Request request = Request::fromURL( url ); // parse settings from URL's query

  KEMailSettings mset;
  KURL open_url = url;
  if ( !request.hasProfile() ) {
    //kdDebug() << "kio_smtp: Profile is null" << endl;
    bool hasProfile = mset.profiles().contains( open_url.host() );
    if ( hasProfile ) {
      mset.setProfile(open_url.host());
      open_url.setHost(mset.getSetting(KEMailSettings::OutServer));
      m_sUser = mset.getSetting(KEMailSettings::OutServerLogin);
      m_sPass = mset.getSetting(KEMailSettings::OutServerPass);

      if (m_sUser.isEmpty())
        m_sUser = QString::null;
      if (m_sPass.isEmpty())
        m_sPass = QString::null;
      open_url.setUser(m_sUser);
      open_url.setPass(m_sPass);
      m_sServer = open_url.host();
      m_iPort = open_url.port();
    } 
    else {
      mset.setProfile(mset.defaultProfileName());
    }
  } 
  else {
    mset.setProfile( request.profileName() );
  }

  // Check KEMailSettings to see if we've specified an E-Mail address
  // if that worked, check to see if we've specified a real name
  // and then format accordingly (either: emailaddress@host.com or
  // Real Name <emailaddress@host.com>)
  if ( !request.hasFromAddress() ) {
    const QString from = mset.getSetting( KEMailSettings::EmailAddress );
    if ( !from.isNull() )
      request.setFromAddress( from );
    else {
      error(KIO::ERR_NO_CONTENT, i18n("The sender address is missing."));
      smtp_close(); // ### try resetting the dialogue instead
      return;
    }
  }

  if ( !smtp_open( request.heloHostname() ) )
  {
    error(KIO::ERR_SERVICE_NOT_AVAILABLE,
          i18n("SMTPProtocol::smtp_open failed (%1)") // ### better error message?
              .arg(open_url.path()));
    return;
  }

  if ( !command( "MAIL FROM: <" + request.fromAddress() + '>', false ) )
  {
    if (!m_errorSent)
    {
      error(KIO::ERR_NO_CONTENT, i18n("The server did not accept the "
                                 "sender address.\nThe server responded: \"%1\"").arg(m_lastError));
    }

    smtp_close(); // ### try resetting the dialogue instead.
    return;
  }

  // Loop through our To and CC recipients, and send the proper
  // SMTP commands, for the benefit of the server.
  if ( !putRecipients( request.recipients() ) )
    return;

  // Begin sending the actual message contents (most headers+body)
  if (!command("DATA", false)) 
  {
    if (!m_errorSent)
    {
      error(KIO::ERR_NO_CONTENT, 
            i18n("The attempt to start sending the "
                 "message content failed.\nThe server responded: \"%1\"")
                .arg(m_lastError));
    }

    smtp_close(); // ### try resetting the dialogue instead.
    return;
  }

  // Write slave-generated header fields (if any):
  const char * headerFields = request.headerFields();
  write( headerFields, qstrlen( headerFields ) );

  // Write client-provided data:
  int result;
  do {
    dataReq();                  // Request for data
    QByteArray buffer;
    result = readData(buffer);
    if (result > 0) {
      write(buffer.data(), buffer.size());
    } 
    else if (result < 0) {
      error(KIO::ERR_COULD_NOT_WRITE, open_url.path());
    }
  } while (result > 0);

  write("\r\n.\r\n", 5);
  if (getResponse(false) >= SMTP_MIN_NEGATIVE_REPLY) {
    if (!m_errorSent)
      error(KIO::ERR_NO_CONTENT, 
            i18n("The message content was not accepted.\nThe server responded: \"%1\"").arg(m_lastError));
    smtp_close(); // ### try resetting the dialogue instead
    return;
  }

  command("RSET");
  finished();
}

bool SMTPProtocol::putRecipients( const QStringList & list )
{
  const QString formatted_recip = QString::fromLatin1("RCPT TO: <%1>");
  for ( QStringList::const_iterator it = list.begin(); it != list.end(); ++it )
    if ( !command( formatted_recip.arg(*it), false ) ) {
      if (!m_errorSent)
      {
        error(KIO::ERR_NO_CONTENT,
              i18n("One of the recipients was not accepted.\nThe server responded: \"%1\"")
                  .arg(m_lastError));
      }

      smtp_close();
      return false;
    }
  return true;
}

void SMTPProtocol::setHost(const QString & host, int port,
                           const QString & user, const QString & pass)
{
  m_sServer = host;
  m_iPort = port;
  m_sUser = user;
  m_sPass = pass;
}

int SMTPProtocol::getResponse(bool handleErrors,
                              char *r_buf,
                              unsigned int r_len)
{
  char *buf = 0;
  ssize_t recv_len = 0, len;
  int retVal;
  m_lastError.truncate(0);
  m_errorSent = false;

  // Give the buffer the appropiate size
  // a buffer of less than 5 bytes will *not* work
  if (r_len) {
    buf = static_cast < char *>(calloc(r_len, sizeof(char)));
    len = r_len;
  } 
  else {
    buf = static_cast < char *>(calloc(DEFAULT_RESPONSE_BUFFER, sizeof(char)));
    len = DEFAULT_RESPONSE_BUFFER;
  }

  while (recv_len < 1)
  {
    if (!waitForResponse(60))
    {
      if (handleErrors)
      {
          error(KIO::ERR_SERVER_TIMEOUT, m_sServer + QString("<<") + buf + QString(">>"));
          m_errorSent = true;
      }
      free(buf);
      return 999;
    }

    // grab the data
    recv_len = readLine(buf, len - 1);
    if ((recv_len < 1) && !isConnectionValid())
    {
      error(KIO::ERR_CONNECTION_BROKEN, m_sServer);
      m_errorSent = true;
      free(buf);
      return 999;
    }
  }

  if (recv_len < 4) {
    error(KIO::ERR_NO_CONTENT, i18n("Invalid SMTP response received."));
    m_errorSent = true;
    free(buf);
    return 999;
  }

  if (buf[3] == '-') {          // Multiline response
    char *origbuf = buf;
    while ((buf[3] == '-') && (len - recv_len > 3)) {   // Three is quite arbitrary
      buf += recv_len;
      len -= (recv_len + 1);
      waitForResponse(60);
      recv_len = readLine(buf, len - 1);
      if (recv_len < 1) {
        if (isConnectionValid())
          error(KIO::ERR_SERVER_TIMEOUT, m_sServer + " strangeness!");
        else
          error(KIO::ERR_CONNECTION_BROKEN, m_sServer);
        m_errorSent = true;
        free(buf);
        return 999;
      }
      if (recv_len < 4) {
        error(KIO::ERR_NO_CONTENT, i18n("Invalid SMTP response received."));
        m_errorSent = true;
        free(buf);
        return 999;
      }
    }

    buf = origbuf;

    int bufLen = strlen(buf);
    if (r_len) 
    {
      memcpy(r_buf, buf, bufLen);
      r_buf[r_len - 1] = '\0';
    }
    m_lastError = QCString(buf, bufLen);
  }
  else
  {
    // single line response
    if (r_len && recv_len > 4) {
      memcpy(r_buf, buf + 4, recv_len - 4);
    }
    m_lastError = QCString(buf + 4, recv_len - 4);
  }

  retVal = GetVal(buf);
  
  if (retVal == -1) {
    if (!isConnectionValid())
      error(KIO::ERR_CONNECTION_BROKEN, m_sServer);
    else
      error(KIO::ERR_NO_CONTENT,
            i18n("Invalid SMTP response received: \"%1\"").arg(m_lastError));
    m_errorSent = true;
    free(buf);
    return 999;
  }

  free(buf);
  return retVal;
}

bool SMTPProtocol::command( const QString & cmd, bool handleError,
			    char * r_buf, unsigned int r_len ) {
  return command( QCString(cmd.latin1()), handleError, r_buf, r_len );
}

bool SMTPProtocol::command( const char *cmd, bool handleError,
			    char * r_buf, unsigned int r_len ) {
  return command( QCString(cmd), handleError, r_buf, r_len );
}

bool SMTPProtocol::command( QCString cmd, bool handleError,
			    char *recv_buf, unsigned int len ) {
  cmd += "\r\n";
  ssize_t cmd_len = cmd.length();

  // Write the command
  if ( write( cmd.data(), cmd_len ) != cmd_len )
    return false;

  return (getResponse(handleError, recv_buf, len) < SMTP_MIN_NEGATIVE_REPLY);
}

bool SMTPProtocol::smtp_open(const QString& fakeHostname)
{
  m_haveTLS = false;
  if (m_opened && 
      m_iOldPort == port(m_iPort) &&
      m_sOldServer == m_sServer && 
      m_sOldUser == m_sUser &&
      (fakeHostname.isNull() || m_hostname == fakeHostname)) 
  {
    return true;
  } 
  else 
  {
    smtp_close();
    if (!connectToHost(m_sServer, m_iPort))
      return false;             // connectToHost has already send an error message.
    m_opened = true;
  }

  if (getResponse(false) >= SMTP_MIN_NEGATIVE_REPLY) 
  {
    if (!m_errorSent)
    {
      error(KIO::ERR_COULD_NOT_LOGIN, 
            i18n("The server did not accept the connection: \"%1\"").arg(m_lastError));
    }
    smtp_close();
    return false;
  }

  QByteArray ehloByteArray(DEFAULT_EHLO_BUFFER);
  ehloByteArray.fill(0);

  if (!fakeHostname.isNull())
  {
    m_hostname = fakeHostname;
  }
  else
  { 
    QString tmpPort;
    KSocketAddress* addr = KExtendedSocket::localAddress(m_iSock);
    KExtendedSocket::resolve(addr, m_hostname, tmpPort);
    delete addr;

    if(m_hostname.isNull())
    {
      m_hostname = "localhost.invalid";
    }
  }

  if (!command( "EHLO " + KIDNA::toAsciiCString(m_hostname),
		false, ehloByteArray.data(), DEFAULT_EHLO_BUFFER - 1)) 
  {
    if (m_errorSent)
    {
      smtp_close();
      return false;
    }

    // Let's just check to see if it speaks plain ol' SMTP
    if (!command( "HELO " + KIDNA::toAsciiCString(m_hostname) )) {
      if (!m_errorSent)
        error(KIO::ERR_COULD_NOT_LOGIN,
              i18n("The server responded: \"%1\"").arg(m_lastError));
      smtp_close();
      return false;
    }
  }
  // We parse the ESMTP extensions here... pipelining would be really cool
  char ehlo_line[DEFAULT_EHLO_BUFFER];
  QBuffer ehlobuf(ehloByteArray);
  if (ehlobuf.open(IO_ReadWrite)) {
    while (ehlobuf.readLine(ehlo_line, DEFAULT_EHLO_BUFFER - 1) > 0)
      ParseFeatures(const_cast < const char *>(ehlo_line));
  }

  if ((m_haveTLS && canUseTLS() && metaData("tls") != "off")
      || metaData("tls") == "on") {
    // For now we're gonna force it on.

    if (command("STARTTLS")) {
      int tlsrc = startTLS();

      if (tlsrc != 1) {
        if (tlsrc != -3) {
          //kdDebug() << "TLS negotiation failed!" << endl;
          messageBox(Information,
                     i18n("Your SMTP server claims to "
                          "support TLS, but negotiation "
                          "was unsuccessful.\nYou can "
                          "disable TLS in KDE using the "
                          "crypto settings module."),
                     i18n("Connection Failed"));
        }
        smtp_close();
        return false;
      }

      /*
       * we now have TLS going
       * send our ehlo line
       * reset our features, and reparse them
       */
      //kdDebug() << "TLS has been enabled!" << endl;
      ehloByteArray.fill(0);

      if (!command( "EHLO " + KIDNA::toAsciiCString(m_hostname),
		    false, ehloByteArray.data(), DEFAULT_EHLO_BUFFER - 1))
      {
        if (!m_errorSent)
        {
            error(KIO::ERR_COULD_NOT_LOGIN,
                  i18n("The server responded: \"%1\"").arg(m_lastError));
        }
        smtp_close();
        return false;
      }

      m_sAuthConfig = QString::null;
      ehlobuf.close();
      ehlobuf.setBuffer(ehloByteArray);

      if (ehlobuf.open(IO_ReadWrite)) 
      {
        while (ehlobuf.readLine(ehlo_line, DEFAULT_EHLO_BUFFER - 1) > 0)
        {
           ParseFeatures(const_cast < const char *>(ehlo_line));
        }
      }
    } 
    else if (metaData("tls") == "on") {
      if (!m_errorSent)
        error(KIO::ERR_COULD_NOT_LOGIN,
              i18n("Your SMTP server does not support TLS. Disable "
                   "TLS, if you want to connect without encryption."));
      smtp_close();
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
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("When prompted, you ran away."));
        smtp_close();
        return false;
      } 
      else {
        m_sUser = authInfo.username;
        m_sPass = authInfo.password;
      }
    }
    if (!Authenticate()) {
      smtp_close();
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
  KDESasl SASL(m_sUser, m_sPass, (m_bIsSSL) ? "smtps" : "smtp");
  QString auth_method;

  // Choose available method from what the server has given us in  its greeting
  QStringList sl = QStringList::split(' ', m_sAuthConfig);
  // If the server doesn't support/require authentication, we ignore it
  if (sl.isEmpty())
    return true;
  QStrIList strList;
  if (!metaData("sasl").isEmpty())
    strList.append(metaData("sasl").latin1());
  else
    for (unsigned int i = 0; i < sl.count(); i++)
      strList.append(sl[i].latin1());

  auth_method = SASL.chooseMethod(strList);

  // If none are available, set it up so we can start over again
  if (auth_method.isNull()) 
  {
    //kdDebug() << "kio_smtp: no authentication available" << endl;
    error(KIO::ERR_COULD_NOT_LOGIN,
          i18n("No compatible authentication methods found."));
    return false;
  }
  else 
  {
    char challenge[2049];
    bool ret = false;
    QByteArray ba;
    QString cmd = QString::fromLatin1("AUTH ") + auth_method;

    if (auth_method == "PLAIN")
    {
      KCodecs::base64Encode(SASL.getBinaryResponse(ba, false), ba);
      cmd += ' ' + ba;
    }

    if (!command(cmd, false, challenge, 2049)) 
    {
      if (!m_errorSent)
      {
        error(KIO::ERR_COULD_NOT_LOGIN,
              i18n("Your SMTP server doesn't support %1.\n"
                   "Choose a different authentication method.")
                   .arg(auth_method));
      }
      return false;
    }

    if (auth_method == "PLAIN")
    {
      ret = true;
    }
    else
    {
      ba.duplicate(challenge, strlen(challenge));
      cmd = SASL.getResponse(ba);

      ret = command(cmd, true, challenge, 2049);
      if (auth_method == "DIGEST-MD5" ||
          auth_method == "LOGIN") 
      {
        ba.duplicate(challenge, strlen(challenge));
        cmd = SASL.getResponse(ba);
        ret = command(cmd);
      }
    }

    if (!ret && !m_errorSent)
    {
      error(KIO::ERR_COULD_NOT_LOGIN,
            i18n
            ("Authentication failed.\nMost likely the password is wrong.\nThe server responded: \"%1\"").
            arg(m_lastError));
    }

    return ret;
  }
  return false;
}

void SMTPProtocol::ParseFeatures(const char* buf)
{
  // We want it to be between 250 and 259 inclusive, and it needs to be "nnn-blah" or "nnn blah"
  // So sez the SMTP spec..
  if ((buf[0] != '2') || (buf[1] != '5') || (!isdigit(buf[2])) ||
      ((buf[3] != '-') && (buf[3] != ' ')))
    return;                     // We got an invalid line..

  if (qstrnicmp(&buf[4], "AUTH", strlen("AUTH")) == 0) {  // Look for auth stuff
    if (m_sAuthConfig.isNull())
      m_sAuthConfig = &buf[5 + strlen("AUTH")];
    m_sAuthConfig.replace(QRegExp("[\r\n]"), "");
  } 
  else if (qstrnicmp(&buf[4], "STARTTLS", strlen("STARTTLS")) == 0) {
    m_haveTLS = true;
  }
}

void SMTPProtocol::smtp_close()
{
  if (!m_opened)                  // We're already closed
    return;

  command("QUIT");
  closeDescriptor();
  m_sOldServer = QString::null;
  m_sOldUser = QString::null;
  m_sOldPass = QString::null;
  
  m_sAuthConfig = QString::null;
  m_opened = false;
}

void SMTPProtocol::stat(const KURL & url)
{
  QString path = url.path();
  error(KIO::ERR_DOES_NOT_EXIST, url.path());
}

int SMTPProtocol::GetVal(char *buf)
{
  bool ok;
  QCString st(buf, 4);
  int val = st.toInt(&ok);
  return (ok) ? val : -1;
}

