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

#include <config.h>

#include "smtp.h"
#include "request.h"
#include "response.h"
#include "transactionstate.h"
#include "command.h"
using KioSMTP::Capabilities;
using KioSMTP::Command;
using KioSMTP::EHLOCommand;
using KioSMTP::AuthCommand;
using KioSMTP::MailFromCommand;
using KioSMTP::RcptToCommand;
using KioSMTP::DataCommand;
using KioSMTP::TransferCommand;
using KioSMTP::Request;
using KioSMTP::Response;
using KioSMTP::TransactionState;

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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <netdb.h>

#ifndef NI_NAMEREQD
// FIXME for KDE 3.3: fake defintion
// API design flaw in KExtendedSocket::resolve
# define NI_NAMEREQD	0
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

unsigned int SMTPProtocol::sendBufferSize() const {
  // ### how much is eaten by SSL/TLS overhead?
  const int fd = fileno( fp );
  int value = -1;
  ksize_t len = sizeof(value);
  if ( fd < 0 || ::getsockopt( fd, SOL_SOCKET, SO_SNDBUF, (char*)&value, &len ) )
    value = 1024; // let's be conservative
  kdDebug(7112) << "send buffer size seems to be " << value << " octets." << endl;
  return value > 0 ? value : 1024 ;
}

SMTPProtocol::~SMTPProtocol() {
  //kdDebug(7112) << "SMTPProtocol::~SMTPProtocol" << endl;
  smtp_close();
}

void SMTPProtocol::openConnection() {
  if ( smtp_open() )
    connected();
  else
    closeConnection();
}

void SMTPProtocol::closeConnection() {
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
    error( KIO::ERR_INTERNAL,
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
	   i18n("Your server does not support sending of 8-bit messages.\n"
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
  queueCommand( new TransferCommand( this, request.headerFields( mset.getSetting( KEMailSettings::RealName ) ) ) );

  TransactionState ts;
  if ( !executeQueuedCommands( &ts ) ) {
    if ( ts.errorCode() )
      error( ts.errorCode(), ts.errorMessage() );
  } else
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

bool SMTPProtocol::sendCommandLine( const QCString & cmdline ) {
  //kdDebug( cmdline.length() < 4096, 7112) << "C: " << cmdline.data();
  //kdDebug( cmdline.length() >= 4096, 7112) << "C: <" << cmdline.length() << " bytes>" << endl;
  kdDebug( 7112) << "C: <" << cmdline.length() << " bytes>" << endl;
  ssize_t cmdline_len = cmdline.length();
  if ( write( cmdline.data(), cmdline_len ) != cmdline_len ) {
    error( KIO::ERR_COULD_NOT_WRITE, m_sServer );
    return false;
  }
  return true;
}

Response SMTPProtocol::getResponse( bool * ok ) {

  if ( ok )
    *ok = false;

  Response response;
  char buf[2048];

  int recv_len = 0;
  do {
    // wait for data...
    if ( !waitForResponse( 600 ) ) {
      error( KIO::ERR_SERVER_TIMEOUT, m_sServer );
      return response;
    }

    // ...read data...
    recv_len = readLine( buf, sizeof(buf) - 1 );
    if ( recv_len < 1 && !isConnectionValid() ) {
      error( KIO::ERR_CONNECTION_BROKEN, m_sServer );
      return response;
    }

    kdDebug(7112) << "S: " << QCString( buf, recv_len + 1 ).data();
    // ...and parse lines...
    response.parseLine( buf, recv_len );

    // ...until the response is complete or the parser is so confused
    // that it doesn't think a RSET would help anymore:
  } while ( !response.isComplete() && response.isWellFormed() );

  if ( !response.isValid() ) {
    error( KIO::ERR_NO_CONTENT, i18n("Invalid SMTP response (%1) received.").arg(response.code()) );
    return response;
  }

  if ( ok )
    *ok = true;

  return response;
}

bool SMTPProtocol::executeQueuedCommands( TransactionState * ts ) {
  assert( ts );

  kdDebug( canPipelineCommands(), 7112 ) << "using pipelining" << endl;

  while( !mPendingCommandQueue.isEmpty() ) {
    QCString cmdline = collectPipelineCommands( ts );
    if ( ts->failedFatally() ) {
      smtp_close( false ); // _hard_ shutdown
      return false;
    }
    if ( cmdline.isEmpty() )
      continue;
    if ( !sendCommandLine( cmdline ) ||
	 !batchProcessResponses( ts ) ||
	 ts->failedFatally() ) {
      smtp_close( false ); // _hard_ shutdown
      return false;
    }
  }

  if ( ts->failed() ) {
    if ( !execute( Command::RSET ) )
      smtp_close( false );
    return false;
  }
  return true;
}

QCString SMTPProtocol::collectPipelineCommands( TransactionState * ts ) {
  assert( ts );

  QCString cmdLine;
  unsigned int cmdLine_len = 0;

  while ( mPendingCommandQueue.head() ) {

    Command * cmd = mPendingCommandQueue.head();

    if ( cmd->doNotExecute( ts ) ) {
      delete mPendingCommandQueue.dequeue();
      if ( cmdLine_len )
	break;
      else
	continue;
    }

    if ( cmdLine_len && cmd->mustBeFirstInPipeline() )
      break;

    if ( cmdLine_len && !canPipelineCommands() )
      break;

    while ( !cmd->isComplete() && !cmd->needsResponse() ) {
      const QCString currentCmdLine = cmd->nextCommandLine( ts );
      if ( ts->failedFatally() )
	return cmdLine;
      const unsigned int currentCmdLine_len = currentCmdLine.length();

      if ( cmdLine_len && cmdLine_len + currentCmdLine_len > sendBufferSize() ) {
	// must all fit into the send buffer, else connection deadlocks,
	// but we need to have at least _one_ command to send
	cmd->ungetCommandLine( currentCmdLine, ts );
	return cmdLine;
      }
      cmdLine_len += currentCmdLine_len;
      cmdLine += currentCmdLine;
    }

    mSentCommandQueue.enqueue( mPendingCommandQueue.dequeue() );

    if ( cmd->mustBeLastInPipeline() )
      break;
  }

  return cmdLine;
}

bool SMTPProtocol::batchProcessResponses( TransactionState * ts ) {
  assert( ts );

  while ( !mSentCommandQueue.isEmpty() ) {

    Command * cmd = mSentCommandQueue.head();
    assert( cmd->isComplete() );

    bool ok = false;
    Response r = getResponse( &ok );
    if ( !ok )
      return false;
    cmd->processResponse( r, ts );
    if ( ts->failedFatally() )
      return false;

    mSentCommandQueue.remove();
  }

  return true;
}

void SMTPProtocol::queueCommand( int type ) {
  queueCommand( Command::createSimpleCommand( type, this ) );
}

bool SMTPProtocol::execute( int type, TransactionState * ts ) {
  auto_ptr<Command> cmd( Command::createSimpleCommand( type, this ) );
  kdFatal( !cmd.get(), 7112 ) << "Command::createSimpleCommand( " << type << " ) returned null!" << endl;
  return execute( cmd.get(), ts );
}

// ### fold into pipelining engine? How? (execute() is often called
// ### when command queues are _not_ empty!)
bool SMTPProtocol::execute( Command * cmd, TransactionState * ts ) {

  kdFatal( !cmd, 7112 ) << "SMTPProtocol::execute() called with no command to run!" << endl;

  if ( cmd->doNotExecute( ts ) )
    return true;

  do {
    while ( !cmd->isComplete() && !cmd->needsResponse() ) {
      const QCString cmdLine = cmd->nextCommandLine( ts );
      if ( ts && ts->failedFatally() ) {
	smtp_close( false );
	return false;
      }
      if ( cmdLine.isEmpty() )
	continue;
      if ( !sendCommandLine( cmdLine ) ) {
	smtp_close( false );
	return false;
      }
    }

    bool ok = false;
    Response r = getResponse( &ok );
    if ( !ok ) {
      smtp_close( false );
      return false;
    }
    if ( !cmd->processResponse( r, ts ) ) {
      if ( ts && ts->failedFatally() ||
	   cmd->closeConnectionOnError() ||
	   !execute( Command::RSET ) )
	smtp_close( false );
      return false;
    }
  } while ( !cmd->isComplete() );

  return true;
}

bool SMTPProtocol::smtp_open(const QString& fakeHostname)
{
  if (m_opened && 
      m_iOldPort == port(m_iPort) &&
      m_sOldServer == m_sServer && 
      m_sOldUser == m_sUser &&
      (fakeHostname.isNull() || m_hostname == fakeHostname)) 
    return true;

  smtp_close();
  if (!connectToHost(m_sServer, m_iPort))
    return false;             // connectToHost has already send an error message.
  m_opened = true;

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
    // perform name lookup. NI_NAMEREQD means: don't return a numeric
    // value (we need to know when we get have the IP address, so we
    // can enclose it in sqaure brackets (domain-literal). Failure to
    // do so is normally harmless with IPv4, but fails for IPv6:
    if (KExtendedSocket::resolve(addr, m_hostname, tmpPort, NI_NAMEREQD) != 0)
      // FQDN resolution failed
      // use the IP address as domain-literal
      m_hostname = '[' + addr->nodeName() + ']';
    delete addr;

    if(m_hostname.isEmpty())
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

      // re-issue EHLO to refresh the capability list (could be have
      // been faked before TLS was enabled):
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

void SMTPProtocol::smtp_close( bool nice ) {
  if (!m_opened)                  // We're already closed
    return;

  if ( nice )
    execute( Command::QUIT );
  kdDebug( 7112 ) << "closing connection" << endl;
  closeDescriptor();
  m_sOldServer = QString::null;
  m_sOldUser = QString::null;
  m_sOldPass = QString::null;
  
  mCapabilities.clear();
  mPendingCommandQueue.clear();
  mSentCommandQueue.clear();

  m_opened = false;
}

void SMTPProtocol::stat(const KURL & url)
{
  QString path = url.path();
  error(KIO::ERR_DOES_NOT_EXIST, url.path());
}

