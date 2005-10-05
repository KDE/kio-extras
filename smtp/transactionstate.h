/*  -*- c++ -*-
    transactionstate.h

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

#ifndef __KIOSMTP_TRANSACTIONSTATE_H__
#define __KIOSMTP_TRANSACTIONSTATE_H__

#include "response.h"

#include <qstring.h>
#include <q3valuelist.h>

namespace KioSMTP {

  /**
     @short A class modelling an SMTP transaction's state
     
     This class models SMTP transaction state, ie. the collective
     result of the MAIL FROM:, RCPT TO: and DATA commands. This is
     needed since e.g. a single failed RCPT TO: command does not
     neccessarily fail the whole transaction (servers are free to
     accept delivery for some recipients, but not for others).

     The class can operate in two modes, which differ in the way
     failed recipients are handled. If @p rcptToDenyIsFailure is true
     (the default), then any failing RCPT TO: will cause the
     transaction to fail. Since at the point of RCPT TO: failure
     detection, the DATA command may have already been sent
     (pipelining), the only way to cancel the transaction is to take
     down the connection hard (ie. without proper quit).

     Since that is not very nice behaviour, a second mode that is more
     to the spirit of SMTP is provided that can cope with partially
     failed RCPT TO: commands.
  */
  class TransactionState {
  public:
    struct RecipientRejection {
      RecipientRejection( const QString & who=QString::null,
			  const QString & why=QString::null )
	: recipient( who ), reason( why ) {}
      QString recipient;
      QString reason;
#ifdef KIOSMTP_COMPARATORS
      bool operator==( const RecipientRejection & other ) const {
	return recipient == other.recipient && reason == other.reason;
      }
#endif
    };
    typedef Q3ValueList<RecipientRejection> RejectedRecipientList;

    TransactionState( bool rcptToDenyIsFailure=true )
      : mErrorCode( 0 ),
	mRcptToDenyIsFailure( rcptToDenyIsFailure ),
	mAtLeastOneRecipientWasAccepted( false ),
	mDataCommandIssued( false ),
	mDataCommandSucceeded( false ),
	mFailed( false ),
	mFailedFatally( false ),
	mComplete( false ) {}

    /** @return whether the transaction failed (e.g. the server
	rejected all recipients. Graceful failure is handled after
	transaction ends. */
    bool failed() const { return mFailed || mFailedFatally; }
    void setFailed() { mFailed = true; }

    /** @return whether the failure was so grave that an immediate
	untidy connection shutdown is in order (ie. @ref
	smtp_close(false)). Fatal failure is handled immediately */
    bool failedFatally() const { return mFailedFatally; }
    void setFailedFatally( int code=0, const QString & msg=QString::null );

    /** @return whether the transaction was completed successfully */
    bool complete() const { return mComplete; }
    void setComplete() { mComplete = true; }

    /** @return an appropriate KIO error code in case the transaction
	failed, or 0 otherwise */
    int errorCode() const;
    /** @return an appropriate error message in case the transaction
	failed or QString::null otherwise */
    QString errorMessage() const;

    void setMailFromFailed( const QString & addr, const Response & r );

    bool dataCommandIssued() const { return mDataCommandIssued; }
    void setDataCommandIssued( bool issued ) { mDataCommandIssued = issued; }

    bool dataCommandSucceeded() const {
      return mDataCommandIssued && mDataCommandSucceeded;
    }
    void setDataCommandSucceeded( bool succeeded, const Response & r );

    Response dataResponse() const {
      return mDataResponse;
    }

    bool atLeastOneRecipientWasAccepted() const {
      return mAtLeastOneRecipientWasAccepted;
    }
    void setRecipientAccepted() {
      mAtLeastOneRecipientWasAccepted = true;
    }

    bool haveRejectedRecipients() const {
      return !mRejectedRecipients.empty();
    }
    RejectedRecipientList rejectedRecipients() const {
      return mRejectedRecipients;
    }
    void addRejectedRecipient( const RecipientRejection & r );
    void addRejectedRecipient( const QString & who, const QString & why ) {
      addRejectedRecipient( RecipientRejection( who, why ) );
    }

    void clear() {
      mRejectedRecipients.clear();
      mDataResponse.clear();
      mAtLeastOneRecipientWasAccepted
	= mDataCommandIssued
	= mDataCommandSucceeded
	= mFailed = mFailedFatally
	= mComplete = false;
    }

#ifdef KIOSMTP_COMPARATORS
    bool operator==( const TransactionState & other ) const {
      return
	mAtLeastOneRecipientWasAccepted == other.mAtLeastOneRecipientWasAccepted &&
	mDataCommandIssued == other.mDataCommandIssued &&
	mDataCommandSucceeded == other.mDataCommandSucceeded &&
	mFailed == other.mFailed &&
	mFailedFatally == other.mFailedFatally &&
	mComplete == other.mComplete &&
	mDataResponse.code() == other.mDataResponse.code() &&
	mRejectedRecipients == other.mRejectedRecipients;
    }
#endif


  private:
    RejectedRecipientList mRejectedRecipients;
    Response mDataResponse;
    QString mErrorMessage;
    int mErrorCode;
    bool mRcptToDenyIsFailure;
    bool mAtLeastOneRecipientWasAccepted;
    bool mDataCommandIssued;
    bool mDataCommandSucceeded;
    bool mFailed;
    bool mFailedFatally;
    bool mComplete;
  };

} // namespace KioSMTP

#endif // __KIOSMTP_TRANSACTIONSTATE_H__
