/*  -*- c++ -*-
    command.h

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

#ifndef __KIOSMTP_COMMAND_H__
#define __KIOSMTP_COMMAND_H__


#include <kio/kdesasl.h>

#include <qstring.h>

class SMTPProtocol;
class QCString;

namespace KioSMTP {

  class Response;

  class Command {
  public:
    Command( SMTPProtocol * smtp, bool fatalErrors=false );
    virtual ~Command();

    enum Type {
      STARTTLS, DATA, NOOP, RSET, QUIT
    };

    static Command * createSimpleCommand( Type which, SMTPProtocol * smtp );

    virtual QCString nextCommandLine() = 0;
    virtual bool processResponse( const Response & response );

    bool isComplete() const { return mComplete; }
    /** @return whether an error in executing this command is so fatal
	that closing the connection is the only option */
    bool isErrorFatal() const { return mFatalErrors; }

  protected:
    SMTPProtocol * mSMTP;
    bool mComplete;
    bool mFatalErrors;

  protected:
    // only relay methods to enable access to slave-protected methods
    // for subclasses of Command:
    void parseFeatures( const Response & r );
    int startTLS();
    bool usingSSL() const;
  };

  class EHLOCommand : public Command {
  public:
    EHLOCommand( SMTPProtocol * smtp, const QString & hostname )
      : Command( smtp, true ),
	mEHLONotSupported( false ),
	mHostname( hostname ) {}

    QCString nextCommandLine();
    bool processResponse( const Response & response );
  private:
    bool mEHLONotSupported;
    QString mHostname;
  };

  class StartTLSCommand : public Command {
  public:
    StartTLSCommand( SMTPProtocol * smtp )
      : Command( smtp, true ) {}

    QCString nextCommandLine();
    bool processResponse( const Response & response );
  };

  class AuthCommand : public Command {
  public:
    AuthCommand( SMTPProtocol * smtp, const QStrIList & mechanisms,
		 const QString & user, const QString & pass );

    QCString nextCommandLine();
    bool processResponse( const Response & response );
  private:
    KDESasl mSASL;
    int mNumResponses;
    QCString mLastChallenge;
    bool mFirstTime;
  };

  class MailFromCommand : public Command {
  public:
    MailFromCommand( SMTPProtocol * smtp, const QCString & addr )
      : Command( smtp ), mAddr( addr ) {}

    QCString nextCommandLine();
    bool processResponse( const Response & response );
  private:
    QCString mAddr;
  };

  class RcptToCommand : public Command {
  public:
    RcptToCommand( SMTPProtocol * smtp, const QCString & addr )
      : Command( smtp ), mAddr( addr ) {}

    QCString nextCommandLine();
    bool processResponse( const Response & response );
  private:
    QCString mAddr;
  };

  /** Handles only the initial intermediate response and compltetes at
      the point where the mail contents need to be sent */
  class DataCommand : public Command {
  public:
    DataCommand( SMTPProtocol * smtp )
      : Command( smtp ) {}

    QCString nextCommandLine();
    bool processResponse( const Response & response );
  };

  class NoopCommand : public Command {
  public:
    NoopCommand( SMTPProtocol * smtp )
      : Command( smtp ) {}

    QCString nextCommandLine();
    //bool processResponse( const Response & response );
  };

  class RsetCommand : public Command {
  public:
    RsetCommand( SMTPProtocol * smtp )
      : Command( smtp, true ) {}

    QCString nextCommandLine();
    //bool processResponse( const Response & response );
  };

  class QuitCommand : public Command {
  public:
    QuitCommand( SMTPProtocol * smtp )
      : Command( smtp, true ) {}

    QCString nextCommandLine();
    //bool processResponse( const Response & response );
  };

}; // namespace KioSMTP

#endif // __KIOSMTP_COMMAND_H__
