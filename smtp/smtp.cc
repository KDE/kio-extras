/*
 * Copyright (c) 2000, 2001 Alex Zepeda <zipzippy@sonic.net>
 * Copyright (c) 2001 Michael Häckel <Michael@Haeckel.Net>
 * Copyright (c) 2002 Aaron J. Seigo <aseigo@olympusproject.org>
 * Copyright (c) 2003 Marc Mutz <mutz@kde.org>
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
using KioSMTP::Command;
using KioSMTP::EHLOCommand;
using KioSMTP::AuthCommand;
using KioSMTP::MailFromCommand;
using KioSMTP::RcptToCommand;
using KioSMTP::DataCommand;
using KioSMTP::Request;
using KioSMTP::Response;

#include <kemailsettings.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <klocale.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qcstring.h>

#include <memory>
using std::auto_ptr;

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#ifdef HAVE_SYSY_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

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
   m_opened(false)
{
  //kdDebug(7112) << "SMTPProtocol::SMTPProtocol" << endl;
  mPendingCommandQueue.setAutoDelete( true );
  mSentCommandQueue.setAutoDelete( true );
}

int SMTPProtocol::sendBufferSize() const {
  const int fd = ::fileno( fp );
  int value = -1;
  ksize_t len = sizeof(value);
  if ( fd < 0 || ::getsockopt( fd, SOL_SOCKET, SO_SNDBUF, (char*)&value, &len ) )
    value = 1024; // let's be conservative
  kdDebug(7112) << "send buffer size seems to be " << value << " octets." << endl;
  return value;
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

void SMTPProtocol::special( const QByteArray & aData ) {
  QDataStream s( aData, IO_ReadOnly );
  int what;
  s >> what;
  if ( what == 'c' ) {
    infoMessage( createSpecialResponse() );
#ifndef NDEBUG
    kdDebug(7112) << "special('c') returns \"" << createSpecialResponse() << "\"" << endl;
#endif
  } else if ( what == 'N' ) {
    if ( !execute( Command::NOOP ) )
      return;
  } else {
    error( KIO::ERR_SERVICE_NOT_AVAILABLE,
	   i18n("The application sent an invalid request.") );
    return;
  }
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
// body={7bit,8bit} (default: 7bit; 8bit activates the use of the 8BITMIME SMTP extension)
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
    else if ( request.emitHeaders() ) {
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

  if ( request.is8BitBody()
       && !haveCapability("8BITMIME") && metaData("8bitmime") != "on" ) {
    error( KIO::ERR_SERVICE_NOT_AVAILABLE,
	   i18n("Your server does not support sending of 8bit messages.\n"
		"Please use base64 or quoted-printable encoding.") );
    return;
  }

  queueCommand( new MailFromCommand( this, request.fromAddress().latin1(),
				     request.is8BitBody(), request.size() ) );

  // Loop through our To and CC recipients, and send the proper
  // SMTP commands, for the benefit of the server.
  QStringList recipients = request.recipients();
  for ( QStringList::const_iterator it = recipients.begin() ; it != recipients.end() ; ++it )
    queueCommand( new RcptToCommand( this, (*it).latin1() ) );

  queueCommand( Command::DATA );

  if ( !executeQueuedCommands() )
    return;

  // Write slave-generated header fields (if any):
  const char * headerFields = request.headerFields( mset.getSetting( KEMailSettings::RealName ) );
  if ( headerFields && *headerFields )
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
    error( KIO::ERR_NO_CONTENT, 
	   i18n("The message content was not accepted.\n"
		"%1").arg( response.errorMessage() ) );
    if ( !execute( Command::RSET ) )
      smtp_close();
    return;
  }

  finished();
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
      return response;
    }

    // ...read data...
    recv_len = readLine( buf, sizeof(buf) - 1 );
    if ( recv_len < 1 && !isConnectionValid() ) {
      error( KIO::ERR_CONNECTION_BROKEN, m_sServer );
      return response;
    }

    //kdDebug(7112) << "S: " << QCString( buf, recv_len + 1 ).data();
    // ...and parse lines...
    response.parseLine( buf, recv_len );

    // ...until the response is complete or the parser is so confused
    // that it doesn't think a RSET would help anymore:
  } while ( !response.isComplete() && response.isWellFormed() );

  if ( !response.isValid() ) {
    error( KIO::ERR_NO_CONTENT, i18n("Invalid SMTP response (%1) recveived.").arg(response.code()) );
    return response;
  }

  if ( ok )
    *ok = true;

  return response;
}

bool SMTPProtocol::executeQueuedCommands() {
  if ( canPipelineCommands() )
    return executeQueuedCommandsPipelined();

  for ( Command * cmd = mPendingCommandQueue.dequeue() ; cmd ; cmd = mPendingCommandQueue.dequeue() ) {
    if ( !execute( cmd ) ) {
      if ( cmd->closeConnectionOnError() || !execute( Command::RSET ) )
	smtp_close();
      delete cmd; cmd = 0;
      mPendingCommandQueue.clear();
      return false;
    }
    delete cmd; cmd = 0;
  }

  return true;
}

QCString SMTPProtocol::collectPipelineCommands() {
  QCString cmdLine;
  int cmdLine_len = 0;
  while ( mPendingCommandQueue.head() ) {
    Command * cmd = mPendingCommandQueue.head();
    if ( cmd->isComplete() ) { // this command doesn't want to be executed
      delete mPendingCommandQueue.dequeue();
      break;
    }
    QCString currentCmdLine = cmd->nextCommandLine();
    cmdLine_len += currentCmdLine.length();
    if ( cmdLine_len && cmdLine_len > sendBufferSize() )
      // must all fit into the send buffer, else connection deadlocks,
      // but we need to have at least _one_ command to send
      break;
    mSentCommandQueue.enqueue( mPendingCommandQueue.dequeue() );
    cmdLine += currentCmdLine;
    if ( cmd->mustBeLastInPipeline() )
      break;
  }
  return cmdLine;
}

bool SMTPProtocol::batchProcessResponses() {
  bool success = true;
  while ( !mSentCommandQueue.isEmpty() ) {
    Command * cmd = mSentCommandQueue.dequeue();
    bool ok = false;
    Response r = getResponse( &ok );
    if ( !ok || !cmd->processResponse( r ) )
      // we must still process all commands to empty the incoming pipe!
      // ### flush the recv buffer and return false on !ok instead?
      success = false;
    else
      assert( cmd->isComplete() );
    delete cmd; cmd = 0;
  }
  return success;
}

bool SMTPProtocol::executeQueuedCommandsPipelined() {
  kdDebug(7112) << "using pipelining" << endl;

  while( !mPendingCommandQueue.isEmpty() ) {
    QCString cmdline = collectPipelineCommands();
    if ( !cmdline.isEmpty() ) {
      if ( !sendCommandLine( cmdline ) || !batchProcessResponses() ) {
	mPendingCommandQueue.clear();
	mSentCommandQueue.clear();
	return false;
      }
    }
  }

  return true;
}

bool SMTPProtocol::execute( Command::Type type ) {
  auto_ptr<Command> cmd( Command::createSimpleCommand( type, this ) );
  kdFatal( !cmd.get(), 7112 ) << "Command::createSimpleCommand( " << (int)type << " ) returned null!" << endl;
  return execute( cmd.get() );
}

bool SMTPProtocol::sendCommandLine( const QCString & cmdline ) {
  //kdDebug(7112) << "C: " << cmdline.data();
  ssize_t cmdline_len = cmdline.length();
  if ( write( cmdline.data(), cmdline_len ) != cmdline_len )
    return false;
  return true;
}

bool SMTPProtocol::execute( Command * cmd ) {
  kdFatal( !cmd, 7112 ) << "SMTPProtocol::execute() called with no command to run!" << endl;
  while ( !cmd->isComplete() ) {
    if ( !sendCommandLine( cmd->nextCommandLine() ) )
      return false;
    // process response
    bool ok = false;
    Response r = getResponse( &ok );
    if ( !ok || !cmd->processResponse( r ) )
      return false;
  }
  return true;
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
    if ( ok )
      error( KIO::ERR_COULD_NOT_LOGIN, 
             i18n("The server did not accept the connection.\n"
		  "%1").arg( greeting.errorMessage() ) );
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

  EHLOCommand ehloCmdPreTLS( this, m_hostname );
  if ( !execute( &ehloCmdPreTLS ) ) {
    smtp_close();
    return false;
  }

  if ( ( haveCapability("STARTTLS") && canUseTLS() && metaData("tls") != "off" )
       || metaData("tls") == "on" ) {
    // For now we're gonna force it on.

    if ( execute( Command::STARTTLS ) ) {

      /*
       * we now have TLS going
       * send our ehlo line
       * reset our features, and reparse them
       */
      //kdDebug(7112) << "TLS has been enabled!" << endl;

      EHLOCommand ehloCmdPostTLS( this, m_hostname );
      if ( !execute( &ehloCmdPostTLS ) ) {
        smtp_close();
        return false;
      }
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
  // return with success if the server doesn't support SMTP-AUTH and
  // metadata doesn't tell us to force it.
  if ( !haveCapability( "AUTH" ) && metaData( "sasl" ).isEmpty() )
    return true;

  QStrIList strList( true ); // deep copies
  if (!metaData("sasl").isEmpty())
    strList.append(metaData("sasl").latin1());
  else
    strList = mCapabilities.saslMethods();

  AuthCommand authCmd( this, strList, m_sUser, m_sPass );
  return execute( &authCmd );
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

  execute( Command::QUIT );
  closeDescriptor();
  m_sOldServer = QString::null;
  m_sOldUser = QString::null;
  m_sOldPass = QString::null;
  
  mCapabilities.clear();
  mPendingCommandQueue.clear();

  m_opened = false;
}

void SMTPProtocol::stat(const KURL & url)
{
  QString path = url.path();
  error(KIO::ERR_DOES_NOT_EXIST, url.path());
}

