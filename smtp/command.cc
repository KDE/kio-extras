/*  -*- c++ -*-
    command.cc

    This file is part of kio_smtp, the KDE SMTP kioslave.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config.h>

#include "command.h"

#include "smtp.h"
#include "response.h"
#include "transactionstate.h"

#include <kidna.h>
#include <klocale.h>
#include <kdebug.h>
#include <kio/slavebase.h> // for test_commands, where SMTPProtocol is not derived from TCPSlaveBase

#include <assert.h>

namespace KioSMTP {

  //
  // Command (base class)
  //

  Command::Command( SMTPProtocol * smtp, int flags )
    : mSMTP( smtp ),
      mComplete( false ), mNeedResponse( false ), mFlags( flags )
  {
    assert( smtp );
  }

  Command::~Command() {}

  bool Command::processResponse( const Response & r, TransactionState * ) {
    mComplete = true;
    mNeedResponse = false;
    return r.isOk();
  }

  void Command::ungetCommandLine( const QCString &, TransactionState * ) {
    mComplete = false;
  }

  Command * Command::createSimpleCommand( int which, SMTPProtocol * smtp ) {
    switch ( which ) {
    case STARTTLS: return new StartTLSCommand( smtp );
    case DATA:     return new DataCommand( smtp );
    case NOOP:     return new NoopCommand( smtp );
    case RSET:     return new RsetCommand( smtp );
    case QUIT:     return new QuitCommand( smtp );
    default:       return 0;
    }
  }

  //
  // relay methods:
  //

  void Command::parseFeatures( const Response & r ) {
    mSMTP->parseFeatures( r );
  }

  int Command::startTLS() {
    return mSMTP->startTLS();
  }

  bool Command::usingSSL() const {
    return mSMTP->usingSSL();
  }

  bool Command::usingTLS() const {
    return mSMTP->usingTLS();
  }

  bool Command::haveCapability( const char * cap ) const {
    return mSMTP->haveCapability( cap );
  }

  //
  // EHLO / HELO
  //

  QCString EHLOCommand::nextCommandLine( TransactionState * ) {
    mNeedResponse = true;
    mComplete = mEHLONotSupported;
    const char * cmd = mEHLONotSupported ? "HELO " : "EHLO " ;
    return cmd + KIDNA::toAsciiCString( mHostname ) + "\r\n";
  }

  bool EHLOCommand::processResponse( const Response & r, TransactionState * ) {
    mNeedResponse = false;
    // "command not {recognized,implemented}" response:
    if ( r.code() == 500 || r.code() == 502 ) {
      if ( mEHLONotSupported ) { // HELO failed...
	mSMTP->error( KIO::ERR_INTERNAL_SERVER,
		      i18n("The server rejected both EHLO and HELO commands "
			   "as unknown or unimplemented.\n"
			   "Please contact the server's system administrator.") );
	return false;
      }
      mEHLONotSupported = true; // EHLO failed, but that's ok.
      return true;
    }
    mComplete = true;
    if ( r.code() / 10 ==  25 ) { // 25x: success
      parseFeatures( r );
      return true;
    }
    mSMTP->error( KIO::ERR_UNKNOWN,
		  i18n("Unexpected server response to %1 command.\n%2")
		  .arg( mEHLONotSupported ? "HELO" : "EHLO" )
		  .arg( r.errorMessage() ) );
    return false;
  }

  //
  // STARTTLS - rfc 3207
  //      

  QCString StartTLSCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "STARTTLS\r\n";
  }

  bool StartTLSCommand::processResponse( const Response & r, TransactionState * ) {
    mNeedResponse = false;
    if ( r.code() != 220 ) {
      mSMTP->error( r.errorCode(),
		    i18n("Your SMTP server does not support TLS. "
			 "Disable TLS, if you want to connect "
			 "without encryption.") );
      return false;
    }

    int tlsrc = startTLS();

    if ( tlsrc == 1 )
      return true;

    if ( tlsrc != -3 )
      //kdDebug(7112) << "TLS negotiation failed!" << endl;
      mSMTP->messageBox(KIO::SlaveBase::Information,
			i18n("Your SMTP server claims to "
			     "support TLS, but negotiation "
			     "was unsuccessful.\nYou can "
			     "disable TLS in KDE using the "
			     "crypto settings module."),
			i18n("Connection Failed"));
    return false;
  }

  //
  // AUTH - rfc 2554
  //
  AuthCommand::AuthCommand( SMTPProtocol * smtp,
			    const QStrIList & mechanisms,
			    const QString & user,
			    const QString & pass )
    : Command( smtp, CloseConnectionOnError|OnlyLastInPipeline ),
      mSASL( user, pass, usingSSL() ? "smtps" : "smtp" ),
      mNumResponses( 0 ),
      mFirstTime( true )
  {
    if ( !mSASL.chooseMethod( mechanisms ) ) {
      if ( smtp->metaData("sasl").isEmpty() )
	// user didn't force a particular method:
	smtp->error( KIO::ERR_COULD_NOT_LOGIN,
		     i18n("No compatible authentication methods found.") );
      else
	if ( mechanisms.isEmpty() )
	  // user forced a particular method, but server doesn't list AUTH cap
	  smtp->error( KIO::ERR_COULD_NOT_LOGIN,
		       i18n("You have requested to authenticate to the server, "
			    "but the server doesn't seem to support authentication.\n"
			    "Try disabling authentication entirely.") );
	else
	  // user forced a particular method, but server doesn't support it
	  smtp->error( KIO::ERR_COULD_NOT_LOGIN,
		       i18n("Your SMTP server doesn't support %1.\n"
			    "Choose a different authentication method.")
		       .arg( smtp->metaData("sasl") ) );
    }
  }

  bool AuthCommand::doNotExecute( const TransactionState * ) const {
    return !mSASL.method();
  }

  void AuthCommand::ungetCommandLine( const QCString & s, TransactionState * ) {
    mUngetSASLResponse = s;
    mComplete = false;
  }

  QCString AuthCommand::nextCommandLine( TransactionState * ) {
    mNeedResponse = true;
    QCString cmd;
    if ( !mUngetSASLResponse.isNull() ) {
      // implement un-ungetCommandLine
      cmd = mUngetSASLResponse;
      mUngetSASLResponse = 0;
    } else if ( mFirstTime ) {
      cmd = "AUTH " + mSASL.method();
      if ( sendInitialResponse() ) {
	QCString resp = mSASL.getResponse();
	if ( resp.isEmpty() )
	  resp = '='; // empty initial responses are represented by a
		      // single '=' in the SMTP SASL profile
	cmd += ' ' + resp;
	++mNumResponses;
      }
      cmd += "\r\n";
    } else {
      ++mNumResponses;
      cmd = mSASL.getResponse( mLastChallenge ) + "\r\n";
    }
    mComplete = mSASL.dialogComplete( mNumResponses );
    return cmd;
  }

  bool AuthCommand::sendInitialResponse() const {
    // don't send credentials if we're not under encryption
    // until we know that the server supports this SASL
    // mechanism. We can do this since the sending the
    // initial-response right away is optional:
    return mSASL.clientStarts() && ( usingSSL() || usingTLS() );
  }

  bool AuthCommand::processResponse( const Response & r, TransactionState * ) {
    if ( !r.isOk() ) {
      if ( mFirstTime && !sendInitialResponse() )
	if ( haveCapability( "AUTH" ) )
	  mSMTP->error( KIO::ERR_COULD_NOT_LOGIN,
			i18n("Your SMTP server doesn't support %1.\n"
			     "Choose a different authentication method.\n"
			     "%2").arg( mSASL.method() ).arg( r.errorMessage() ) );
	else
	  mSMTP->error( KIO::ERR_COULD_NOT_LOGIN,
			i18n("Your SMTP server doesn't support authentication.\n"
			     "%2").arg( r.errorMessage() ) );
      else
	mSMTP->error( KIO::ERR_COULD_NOT_LOGIN,
		      i18n("Authentication failed.\n"
			   "Most likely the password is wrong.\n"
			   "%1").arg( r.errorMessage() ) );
      return false;
    }
    mFirstTime = false;
    mLastChallenge = r.lines().front(); // ### better join all lines with \n?
    return true;
  }

  //
  // MAIL FROM:
  //

  QCString MailFromCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    QCString cmdLine = "MAIL FROM:<" + mAddr + '>';
    if ( m8Bit && haveCapability("8BITMIME") )
      cmdLine += " BODY=8BITMIME";
    if ( mSize && haveCapability("SIZE") )
      cmdLine += " SIZE=" + QCString().setNum( mSize );
    return cmdLine + "\r\n";
  }

  bool MailFromCommand::processResponse( const Response & r, TransactionState * ts ) {
    mNeedResponse = false;
    if ( r.code() == 250 )
      return true;
    // ### better error messages...
    if ( ts ) ts->setMailFromFailed( mAddr, r );
    else if ( mAddr.isEmpty() )
      mSMTP->error( KIO::ERR_NO_CONTENT,
		    i18n("The server did not accept the blank sender address.\n"
			 "%1").arg( r.errorMessage() ) );
    else
      mSMTP->error( KIO::ERR_NO_CONTENT,
		    i18n("The server did not accept the sender address \"%1\".\n"
			 "%2").arg( mAddr ).arg( r.errorMessage() ) );
    return false;
  }

  //
  // RCPT TO:
  //

  QCString RcptToCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "RCPT TO:<" + mAddr + ">\r\n";
  }

  bool RcptToCommand::processResponse( const Response & r, TransactionState * ts ) {
    mNeedResponse = false;
    if ( r.code() == 250 ) {
      if ( ts ) ts->setRecipientAccepted();
      return true;
    }
    if ( ts )
      // if we have transaction state, we return success, since
      // multiple RCPT TO: may fail without affecting the transaction
      // as such:
      ts->addRejectedRecipient( mAddr, r.errorMessage() );
    else
      mSMTP->error( KIO::ERR_NO_CONTENT,
		    i18n("The server did not accept the recipient \"%1\".\n"
			 "%2").arg( mAddr ).arg( r.errorMessage() ) );
    return false;
  }		  

  //
  // DATA (only initial processing!)
  //

  QCString DataCommand::nextCommandLine( TransactionState * ts ) {
    mComplete = true;
    mNeedResponse = true;
    if ( ts ) ts->setDataCommandIssued( true );
    return "DATA\r\n";
  }

  void DataCommand::ungetCommandLine( const QCString &, TransactionState * ts ) {
    mComplete = false;
    if ( ts ) ts->setDataCommandIssued( false );
  }

  bool DataCommand::processResponse( const Response & r, TransactionState * ts ) {
    mNeedResponse = false;
    if ( r.code() == 354 ) {
      if ( ts )
	ts->setDataCommandSucceeded( true, r );
      return true;
    }

    if ( ts )
      ts->setDataCommandSucceeded( false, r );
    else
      mSMTP->error( KIO::ERR_NO_CONTENT,
		    i18n("The attempt to start sending the message content failed.\n"
			 "%1").arg( r.errorMessage() ) );
    return false;
  }

  //
  // DATA (data transfer)
  //
  void TransferCommand::ungetCommandLine( const QCString & cmd, TransactionState * ) {
    if ( cmd.isEmpty() )
      return; // don't change state when we can't detect the unget in
	      // the next nextCommandLine !!
    kdDebug() << "uncomplete transfer again since ungetCommandLine was called" << endl;
    mWasComplete = mComplete;
    mComplete = false;
    mNeedResponse = false;
    mUngetBuffer = cmd;
  }

  bool TransferCommand::doNotExecute( const TransactionState * ts ) const {
    assert( ts );
    assert( !ts->failed() );
    return ts->failed();
  }

  QCString TransferCommand::nextCommandLine( TransactionState * ts ) {
    assert( ts ); // let's rely on it ( at least for the moment )
    assert( !isComplete() );
    assert( !ts->failed() );

    static const QCString dotCRLF = ".\r\n";
    static const QCString CRLFdotCRLF = "\r\n.\r\n";

    if ( !mUngetBuffer.isEmpty() ) {
      kdDebug() << "returning already requested data" << endl;
      const QCString ret = mUngetBuffer;
      mUngetBuffer = 0;
      if ( mWasComplete ) {
	mComplete = true;
	mNeedResponse = true;
      }
      return ret; // don't prepare(), it's slave-generated or already prepare()d
    }

    // normal processing:

    kdDebug(7112) << "requesting data" << endl;
    mSMTP->dataReq();
    QByteArray ba;
    int result = mSMTP->readData( ba );
    kdDebug(7112) << "got " << result << " bytes" << endl;
    if ( result > 0 )
      return prepare( ba );
    else if ( result < 0 ) {
      ts->setFailedFatally( KIO::ERR_INTERNAL,
			    i18n("Could not read data from application.") );
      mComplete = true;
      mNeedResponse = true;
      return 0;
    }
    kdDebug() << "transfer complete since result of readData() <= 0" << endl;
    mComplete = true;
    mNeedResponse = true;
    kdDebug() << "since mLastChar == " << mLastChar << " (" << '\n' << ") returning "
	      << ( mLastChar == '\n' ? ".\\r\\n" : "\\r\\n.\\r\\n" ) << endl;
    return mLastChar == '\n' ? dotCRLF : CRLFdotCRLF ;
  }

  bool TransferCommand::processResponse( const Response & r, TransactionState * ts ) {
    mNeedResponse = false;
    assert( ts );
    ts->setComplete();
    if ( !r.isOk() ) {
      ts->setFailed();
      mSMTP->error( r.errorCode(),
		    i18n("The message content was not accepted.\n"
			 "%1").arg( r.errorMessage() ) );
      return false;
    }
    return true;
  }

  static QCString dotstuff_lf2crlf( const QByteArray & ba, char & last ) {
    QCString result( ba.size() * 2 + 1 ); // worst case: repeated "[.]\n"
    const char * s = ba.data();
    const char * const send = ba.data() + ba.size();
    char * d = result.data();

    while ( s < send ) {
      const char ch = *s++;
      if ( ch == '\n' && last != '\r' )
	*d++ = '\r'; // lf2crlf
      else if ( ch == '.' && last == '\n' )
	*d++ = '.'; // dotstuff
      last = *d++ = ch;
    }

    result.truncate( d - result.data() );
    return result;
  }

  QCString TransferCommand::prepare( const QByteArray & ba ) {
    if ( ba.isEmpty() )
      return 0;
    if ( mSMTP->metaData("lf2crlf+dotstuff") == "slave" ) {
      kdDebug(7112) << "performing dotstuffing and LF->CRLF transformation" << endl;
      return dotstuff_lf2crlf( ba, mLastChar );
    } else {
      mLastChar = ba[ ba.size() - 1 ];
      return QCString( ba.data(), ba.size() + 1 );
    }
  }

  //
  // NOOP
  //

  QCString NoopCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "NOOP\r\n";
  }

  //
  // RSET
  //

  QCString RsetCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "RSET\r\n";
  }

  //
  // QUIT
  //

  QCString QuitCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "QUIT\r\n";
  }

}; // namespace KioSMTP

