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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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


#include <qstring.h>
#include <smtp-config.h>

#ifdef HAVE_LIBSASL2
extern "C" {
#include <sasl/sasl.h>
}
#endif

#include <kio/authinfo.h>

class SMTPProtocol;

namespace KioSMTP {

  class Response;
  class TransactionState;

  /**
   * @short Represents an SMTP command
   *
   * Semantics: A command consists of a series of "command lines"
   * (though that's stretching it a bit for @ref TransferJob and @ref
   * AuthCommand) and responses. There's typically one response for
   * one command line and the command is completed.
   *
   * However, some commands consist of a dialog (command line,
   * response, command line, response,...) where each successive
   * command line is dependant on the previously received response
   * (and thus those commands are not pipelinable). That's why each
   * command signals completion by having it's @ref #isComplete()
   * method return true @em after the last command line to be sent,
   * but @em before the last response to receive. @ref AuthCommand is
   * the principal representative of this kind of command. Because
   * @ref EHLOCommand automatically falls back to HELO in case EHLO
   * isn't supported, it is also of this kind. If completion is
   * signalled before the first command line is issued, it is not to
   * be executed at all.
   *
   * Other commands need to send multiple "command lines" before
   * receiving a single (final) response. @ref TransferCommand is the
   * only representative of this kind of "command". That's why each
   * command signals whether it now expects a response before being
   * able to issue the next command line (if any) by having it's @ref
   * #needsResponse() method return true.
   *
   * Commands whose @ref #nextCommandLine() does not support being
   * called multiple times in a row without changing command state,
   * must reimplement @ref #ungetCommandLine().
   **/
  class Command {
  public:
    enum Flags {
      OnlyLastInPipeline = 1,
      OnlyFirstInPipeline = 2,
      CloseConnectionOnError = 4
    };

    Command( SMTPProtocol * smtp, int flags=0 );
    virtual ~Command();

    enum Type {
      STARTTLS, DATA, NOOP, RSET, QUIT
    };

    static Command * createSimpleCommand( int which, SMTPProtocol * smtp );

    virtual QByteArray nextCommandLine( TransactionState * ts=0 ) = 0;
    /* Reimplement this if your @ref #nextCommandLine() implementation
       changes state other than @ref mComplete. The default
       implementation just resets @ref mComplete to false. */
    virtual void ungetCommandLine( const QByteArray & cmdLine, TransactionState * ts=0 );
    /* Reimplement this if your command need more sophisicated
       response processing than just checking for @ref
       Response::isOk(). The default implementation sets @ref
       mComplete to true, @ref mNeedResponse to false and returns
       whether the response isOk(). */
    virtual bool processResponse( const Response & response, TransactionState * ts=0 );
    
    virtual bool doNotExecute( const TransactionState * ) const { return false; }

    bool isComplete() const { return mComplete; }
    /** @return whether the command expects a response now. Some
	commands (most notably AUTH) may consist of a series of
	commands and associated responses until they are
	complete. Others (most notably @ref TransferCommand usually
	send multiple "command lines" before expecting a response. */
    bool needsResponse() const { return mNeedResponse; }
    /** @return whether an error in executing this command is so fatal
	that closing the connection is the only option */
    bool closeConnectionOnError() const {
      return mFlags & CloseConnectionOnError;
    }
    bool mustBeLastInPipeline() const {
      return mFlags & OnlyLastInPipeline;
    }
    bool mustBeFirstInPipeline() const {
      return mFlags & OnlyFirstInPipeline;
    }

  protected:
    SMTPProtocol * mSMTP;
    bool mComplete;
    bool mNeedResponse;
    const int mFlags;

  protected:
    // only relay methods to enable access to slave-protected methods
    // for subclasses of Command:
    void parseFeatures( const Response & r );
    int startTLS();
    bool usingSSL() const;
    bool usingTLS() const;
    bool haveCapability( const char * cap ) const;
  };

  class EHLOCommand : public Command {
  public:
    EHLOCommand( SMTPProtocol * smtp, const QString & hostname )
      : Command( smtp, CloseConnectionOnError|OnlyLastInPipeline ),
	mEHLONotSupported( false ),
	mHostname( hostname.trimmed() ) {}

    QByteArray nextCommandLine( TransactionState * );
    bool processResponse( const Response & response, TransactionState * );
  private:
    bool mEHLONotSupported;
    QString mHostname;
  };

  class StartTLSCommand : public Command {
  public:
    StartTLSCommand( SMTPProtocol * smtp )
      : Command( smtp, CloseConnectionOnError|OnlyLastInPipeline ) {}

    QByteArray nextCommandLine( TransactionState * );
    bool processResponse( const Response & response, TransactionState * );
  };

  class AuthCommand : public Command {
  public:
    AuthCommand( SMTPProtocol * smtp, const char *mechanisms,
      const QString &aFQDN, KIO::AuthInfo &ai );
    ~AuthCommand();
    bool doNotExecute( const TransactionState * ts ) const;
    QByteArray nextCommandLine( TransactionState * );
    void ungetCommandLine( const QByteArray & cmdLine, TransactionState * );
    bool processResponse( const Response & response, TransactionState * );
  private:
    bool saslInteract( void *in );

#ifdef HAVE_LIBSASL2
    sasl_conn_t *conn;
    sasl_interact_t *client_interact;
#endif    
    const char *mOut, *mMechusing;
    uint mOutlen;
    bool mOneStep;

    KIO::AuthInfo *mAi;
    QByteArray mLastChallenge;
    QByteArray mUngetSASLResponse;
    bool mFirstTime;
  };

  class MailFromCommand : public Command {
  public:
    MailFromCommand( SMTPProtocol * smtp, const QByteArray & addr,
		     bool eightBit=false, unsigned int size=0  )
      : Command( smtp ), mAddr( addr ), m8Bit( eightBit ), mSize( size ) {}

    QByteArray nextCommandLine( TransactionState * );
    bool processResponse( const Response & response, TransactionState * );
  private:
    QByteArray mAddr;
    bool m8Bit;
    unsigned int mSize;
  };

  class RcptToCommand : public Command {
  public:
    RcptToCommand( SMTPProtocol * smtp, const QByteArray & addr )
      : Command( smtp ), mAddr( addr ) {}

    QByteArray nextCommandLine( TransactionState * );
    bool processResponse( const Response & response, TransactionState * );
  private:
    QByteArray mAddr;
  };

  /** Handles only the initial intermediate response and compltetes at
      the point where the mail contents need to be sent */
  class DataCommand : public Command {
  public:
    DataCommand( SMTPProtocol * smtp )
      : Command( smtp, OnlyLastInPipeline ) {}

    QByteArray nextCommandLine( TransactionState * );
    void ungetCommandLine( const QByteArray & cmd, TransactionState * ts );
    bool processResponse( const Response & response, TransactionState * );
  };

  /** Handles the data transfer following a successful DATA command */
  class TransferCommand : public Command {
  public:
    TransferCommand( SMTPProtocol * smtp, const QByteArray & initialBuffer )
      : Command( smtp, OnlyFirstInPipeline ),
	mUngetBuffer( initialBuffer ), mLastChar( '\n' ), mWasComplete( false ) {}

    bool doNotExecute( const TransactionState * ts ) const;
    QByteArray nextCommandLine( TransactionState * );
    void ungetCommandLine( const QByteArray & cmd, TransactionState * ts );
    bool processResponse( const Response & response, TransactionState * );
  private:
    QByteArray prepare( const QByteArray & ba );
    QByteArray mUngetBuffer;
    char mLastChar;
    bool mWasComplete; // ... before ungetting
  };

  class NoopCommand : public Command {
  public:
    NoopCommand( SMTPProtocol * smtp )
      : Command( smtp, OnlyLastInPipeline ) {}

    QByteArray nextCommandLine( TransactionState * );
  };

  class RsetCommand : public Command {
  public:
    RsetCommand( SMTPProtocol * smtp )
      : Command( smtp, CloseConnectionOnError ) {}

    QByteArray nextCommandLine( TransactionState * );
  };

  class QuitCommand : public Command {
  public:
    QuitCommand( SMTPProtocol * smtp )
      : Command( smtp, CloseConnectionOnError|OnlyLastInPipeline ) {}

    QByteArray nextCommandLine( TransactionState * );
  };

} // namespace KioSMTP

#endif // __KIOSMTP_COMMAND_H__
