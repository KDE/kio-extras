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

#include "command.h"

#include "smtp.h"
#include "response.h"

#include <kidna.h>
#include <klocale.h>
#include <kio/kdesasl.h>

#include <qcstring.h>

#include <assert.h>

namespace KioSMTP {

  //
  // Command (base class)
  //

  Command::Command( SMTPProtocol * smtp, int flags )
    : mSMTP( smtp ), mComplete( false ), mFlags( flags )
  {
    assert( smtp );
  }

  Command::~Command() {}

  bool Command::processResponse( const Response & r ) {
    mComplete = true;
    return r.isOk();
  }

  Command * Command::createSimpleCommand( Type which, SMTPProtocol * smtp ) {
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

  bool Command::haveCapability( const char * cap ) const {
    return mSMTP->haveCapability( cap );
  }

  //
  // EHLO / HELO
  //

  QCString EHLOCommand::nextCommandLine() {
    assert( !isComplete() );
    const char * cmd = mEHLONotSupported ? "HELO " : "EHLO " ;
    return cmd + KIDNA::toAsciiCString( mHostname ) + "\r\n";
  }

  bool EHLOCommand::processResponse( const Response & r ) {
    // "command not {recognized,implemented}" response:
    if ( r.code() == 500 || r.code() == 502 ) {
      if ( mEHLONotSupported ) { // HELO failed...
	mComplete = true;
	mSMTP->error( KIO::ERR_COULD_NOT_LOGIN, r.errorMessage() );
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
    mSMTP->error( KIO::ERR_COULD_NOT_LOGIN, r.errorMessage() );
    return false;
  }

  //
  // STARTTLS - rfc 3207
  //      

  QCString StartTLSCommand::nextCommandLine() {
    return "STARTTLS\r\n";
  }

  bool StartTLSCommand::processResponse( const Response & r ) {
    mComplete = true;

    if ( r.code() != 220 ) {
      mSMTP->error( KIO::ERR_COULD_NOT_LOGIN,
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
      mComplete = true;
      if ( smtp->metaData("sasl").isEmpty() )
	smtp->error( KIO::ERR_COULD_NOT_LOGIN,
		     i18n("No compatible authentication methods found.") );
      else
	smtp->error( KIO::ERR_COULD_NOT_LOGIN,
		     i18n("Your SMTP server doesn't support %1.\n"
			  "Choose a different authentication method.")
		     .arg( smtp->metaData("sasl") ) );
    }
  }

  QCString AuthCommand::nextCommandLine() {
    assert( !isComplete() );

    if ( mFirstTime ) {
      QCString cmd = "AUTH " + mSASL.method();
      if ( mSASL.clientStarts() ) {
	QCString resp = mSASL.getResponse();
	if ( resp.isEmpty() )
	  resp = '='; // empty initial responses are represented by a
		      // single '=' in the SMTP SASL profile
	cmd += ' ' + resp;
	++mNumResponses;
      }
      return cmd + "\r\n";
    } else {
      ++mNumResponses;
      return mSASL.getResponse( mLastChallenge ) + "\r\n";
    }
  }

  bool AuthCommand::processResponse( const Response & r ) {
    if ( !r.isOk() ) {
      if ( mFirstTime && !mSASL.clientStarts() )
	mSMTP->error( KIO::ERR_COULD_NOT_LOGIN,
		      i18n("Your SMTP server doesn't support %1.\n"
			   "Choose a different authentication method.\n"
			   "%2").arg( mSASL.method() ).arg( r.errorMessage() ) );
      else
	mSMTP->error( KIO::ERR_COULD_NOT_LOGIN,
		      i18n("Authentication failed.\n"
			   "Most likely the password is wrong.\n"
			   "%1").arg( r.errorMessage() ) );
      mComplete = true;
      return false;
    }
    mFirstTime = false;
    if ( mSASL.dialogComplete( mNumResponses ) )
      mComplete = true;
    else
      mLastChallenge = r.lines().front(); // ### better join all lines with \n?
    return true;
  }

  //
  // MAIL FROM:
  //

  QCString MailFromCommand::nextCommandLine() {
    QCString cmdLine = "MAIL FROM:<" + mAddr + '>';
    if ( m8Bit )
      cmdLine += " BODY=8BITMIME";
    if ( mSize && haveCapability("SIZE") )
      cmdLine += " SIZE=" + QCString().setNum( mSize );
    return cmdLine + "\r\n";
  }

  bool MailFromCommand::processResponse( const Response & r ) {
    mComplete = true;
    if ( r.code() == 250 )
      return true;
    // ### better error messages...
    if ( mAddr.isEmpty() )
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

  QCString RcptToCommand::nextCommandLine() {
    return "RCPT TO:<" + mAddr + ">\r\n";
  }

  bool RcptToCommand::processResponse( const Response & r ) {
    mComplete = true;
    if ( r.code() == 250 )
      return true;

    mSMTP->error( KIO::ERR_NO_CONTENT,
		  i18n("The server did not accept the recipient \"%1\".\n"
		       "%2").arg( mAddr ).arg( r.errorMessage() ) );
    return false;
  }		  

  //
  // DATA (only initial processing!)
  //

  QCString DataCommand::nextCommandLine() {
    return "DATA\r\n";
  }

  bool DataCommand::processResponse( const Response & r ) {
    mComplete = true;
    if ( r.code() == 354 )
      return true;

    mSMTP->error( KIO::ERR_NO_CONTENT,
		  i18n("The attempt to start sending the message content failed.\n"
		       "%1").arg( r.errorMessage() ) );
    return false;
  }

  //
  // NOOP
  //

  QCString NoopCommand::nextCommandLine() {
    return "NOOP\r\n";
  }

  //
  // RSET
  //

  QCString RsetCommand::nextCommandLine() {
    return "RSET\r\n";
  }

  //
  // QUIT
  //

  QCString QuitCommand::nextCommandLine() {
    return "QUIT\r\n";
  }

}; // namespace KioSMTP

