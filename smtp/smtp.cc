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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "smtp.h"
#include "request.h"
#include "response.h"
using KioSMTP::Capabilities;
using KioSMTP::Request;
using KioSMTP::Response;

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


// hackishly fixing QCStringList flaws...
static QCString join( char sep, const QCStringList & list ) {
  if ( list.empty() )
    return QCString();
  QCString result = list.front();
  for ( QCStringList::const_iterator it = ++list.begin() ; it != list.end() ; ++it )
    result += sep + *it;
  return result;
}

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
   m_errorSent(false)
{
  //kdDebug(7112) << "SMTPProtocol::SMTPProtocol" << endl;
}


SMTPProtocol::~SMTPProtocol()
{
  //kdDebug(7112) << "SMTPProtocol::~SMTPProtocol" << endl;
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
  infoMessage( createSpecialResponse() );
#ifndef NDEBUG
  kdDebug(7112) << "special() returns \"" << createSpecialResponse() << "\"" << endl;
#endif
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
    //kdDebug(7112) << "kio_smtp: Profile is null" << endl;
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

  if ( !command( "MAIL FROM: <" + request.fromAddress() + '>' ) )
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
  if (!command("DATA")) 
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
  bool ok = false;
  Response response = getResponse( &ok );
  if ( !ok || !response.isOk() ) {
    if (!m_errorSent)
      error(KIO::ERR_NO_CONTENT, 
            i18n("The message content was not accepted.\n"
		 "The server responded: \"%1\"").arg( join('\n',response.lines())) );
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
    if ( !command( formatted_recip.arg(*it) ) ) {
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

Response SMTPProtocol::getResponse( bool * ok ) {
  if ( ok )
    *ok = false;

  Response response;
  char buf[2048];

  int recv_len = 0;
  do {
    // wait for data...
    if ( !waitForResponse( 60 ) ) {
      error( KIO::ERR_SERVER_TIMEOUT, m_sServer );
      m_errorSent = true;
      return response;
    }

    // ...read data...
    recv_len = readLine( buf, sizeof(buf) - 1 );
    if ( recv_len < 1 && !isConnectionValid() ) {
      error( KIO::ERR_CONNECTION_BROKEN, m_sServer );
      m_errorSent = true;
      return response;
    }

    //kdDebug(7112) << "S: " << QCString( buf, recv_len + 1 ).data() << endl;
    // ...and parse lines...
    response.parseLine( buf, recv_len );

    // ...until the response is complete or the parser is so confused
    // that it doesn't think a RSET would help anymore:
  } while ( !response.isComplete() && response.isWellFormed() );

  if ( !response.isValid() ) {
    error( KIO::ERR_NO_CONTENT, i18n("Invalid SMTP response (%1) recveived.").arg(response.code()) );
    m_errorSent = true;
    return response;
  }

  if ( ok )
    *ok = true;

  return response;
}


bool SMTPProtocol::command( const QString & cmd, Response * response ) {
  return command( QCString(cmd.latin1()), response );
}

bool SMTPProtocol::command( const char *cmd, Response * response ) {
  return command( QCString(cmd), response );
}

bool SMTPProtocol::command( QCString cmd, Response * response ) {
  //kdDebug(7112) << "C: " << cmd.data() << endl;

  cmd += "\r\n";
  ssize_t cmd_len = cmd.length();

  // Write the command
  if ( write( cmd.data(), cmd_len ) != cmd_len )
    return false;

  bool ok = false;
  if ( response ) {
    *response = getResponse( &ok );
    ok = ok && response->isOk();
    if ( !ok )
      m_lastError = join( '\n', response->lines() );
  } else {
    Response r = getResponse( &ok );
    ok = ok && r.isOk();
    if ( !ok )
      m_lastError = join( '\n', r.lines() );
  }
  return ok;
}

bool SMTPProtocol::smtp_open(const QString& fakeHostname)
{
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

  bool ok = false;
  Response greeting = getResponse( &ok );
  if ( !ok || !greeting.isOk() )
  {
    if (!m_errorSent)
    {
      error(KIO::ERR_COULD_NOT_LOGIN, 
            i18n("The server did not accept the connection: \"%1\"").arg( join('\n', greeting.lines())));
    }
    smtp_close();
    return false;
  }

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

  Response ehloResponse;
  if ( !command( "EHLO " + KIDNA::toAsciiCString(m_hostname), &ehloResponse ) )
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

  parseFeatures( ehloResponse );

  if ( ( haveCapability("STARTTLS") && canUseTLS() && metaData("tls") != "off" )
       || metaData("tls") == "on" ) {
    // For now we're gonna force it on.

    if (command("STARTTLS")) {
      int tlsrc = startTLS();

      if (tlsrc != 1) {
        if (tlsrc != -3) {
          //kdDebug(7112) << "TLS negotiation failed!" << endl;
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
      //kdDebug(7112) << "TLS has been enabled!" << endl;

      ehloResponse.clear();
      if ( !command( "EHLO " + KIDNA::toAsciiCString(m_hostname), &ehloResponse ) )
      {
        if (!m_errorSent)
        {
            error(KIO::ERR_COULD_NOT_LOGIN,
                  i18n("The server responded: \"%1\"").arg(m_lastError));
        }
        smtp_close();
        return false;
      }

      parseFeatures( ehloResponse ); // reparse capabilities after starttls

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
    if (!authenticate()) {
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

bool SMTPProtocol::authenticate()
{
  KDESasl SASL(m_sUser, m_sPass, usingSSL() ? "smtps" : "smtp");

  // return with success if the server doesn't support SMTP-AUTH and
  // metadata doesn't tell us to force it.
  if ( !haveCapability( "AUTH" ) && metaData( "sasl" ).isEmpty() )
    return true;

  QStrIList strList( true ); // deep copies
  if (!metaData("sasl").isEmpty())
    strList.append(metaData("sasl").latin1());
  else
    strList = mCapabilities.saslMethods();

  // If none are available, set it up so we can start over again
  if ( !SASL.chooseMethod( strList ) )
  {
    //kdDebug(7112) << "kio_smtp: no authentication available" << endl;
    if ( metaData("sasl").isEmpty() )
      error(KIO::ERR_COULD_NOT_LOGIN,
	    i18n("No compatible authentication methods found."));
    else
      error(KIO::ERR_COULD_NOT_LOGIN,
              i18n("Your SMTP server doesn't support %1.\n"
                   "Choose a different authentication method.")
                   .arg( metaData("sasl") ));
    return false;
  }

  bool ret = false;
  int numResponses = 0;
  QCString cmd = "AUTH " + SASL.method();

  if ( SASL.clientStarts() ) {
    ++numResponses;
    cmd += ' ' + SASL.getResponse();
  }

  Response saslResponse;
  if ( !command( cmd, &saslResponse ) )
  {
    if (!m_errorSent)
    {
      error(KIO::ERR_COULD_NOT_LOGIN,
	    i18n("Your SMTP server doesn't support %1.\n"
		 "Choose a different authentication method.")
	    .arg(SASL.method()));
    }
    return false;
  }

  if ( SASL.dialogComplete( numResponses ) )
    return true;

  QByteArray ba;
  if ( !saslResponse.lines().empty() )
    ba.duplicate( saslResponse.lines().front().data(),
		  saslResponse.lines().front().length() );
  ++numResponses;
  cmd = SASL.getResponse(ba);

  saslResponse.clear();
  ret = command( cmd, &saslResponse );
  if ( !SASL.dialogComplete( numResponses ) )
  {
    if ( !saslResponse.lines().empty() )
      ba.duplicate( saslResponse.lines().front().data(),
		    saslResponse.lines().front().length() );
    else
      ba.resize( 0 );
    cmd = SASL.getResponse(ba);
    ++numResponses;
    ret = command(cmd);
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

void SMTPProtocol::parseFeatures( const Response & ehloResponse ) {
  mCapabilities = Capabilities::fromResponse( ehloResponse );

  QString category = usingTLS() ? "TLS" : usingSSL() ? "SSL" : "PLAIN" ;
  setMetaData( category + " AUTH METHODS", mCapabilities.authMethodMetaData() );
  setMetaData( category + " CAPABILITIES", mCapabilities.asMetaDataString() );
#ifndef NDEBUG
  kdDebug(7112) << "parseFeatures() " << category << " AUTH METHODS:"
		<< '\n' + mCapabilities.authMethodMetaData() << endl
		<< "parseFeatures() " << category << " CAPABILITIES:"
		<< '\n' + mCapabilities.asMetaDataString() << endl;
#endif
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
  
  mCapabilities.clear();

  m_opened = false;
}

void SMTPProtocol::stat(const KURL & url)
{
  QString path = url.path();
  error(KIO::ERR_DOES_NOT_EXIST, url.path());
}

