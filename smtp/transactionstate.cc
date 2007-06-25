/*  -*- c++ -*-
    transactionstate.cc

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

#include <config-runtime.h>

#include "transactionstate.h"

#include <kio/global.h>
#include <klocale.h>


namespace KioSMTP {

  void TransactionState::setFailedFatally( int code, const QString & msg ) {
    mFailed = mFailedFatally = true;
    mErrorCode = code;
    mErrorMessage = msg;
  }

  void TransactionState::setMailFromFailed( const QString & addr, const Response & r ) {
    setFailed();
    mErrorCode = KIO::ERR_NO_CONTENT;
    if ( addr.isEmpty() )
      mErrorMessage = i18n("The server did not accept a blank sender address.\n"
			   "%1",  r.errorMessage() );
    else
      mErrorMessage = i18n("The server did not accept the sender address \"%1\".\n"
			   "%2",  addr ,  r.errorMessage() );
  }

  void TransactionState::addRejectedRecipient( const RecipientRejection & r ) {
    mRejectedRecipients.push_back( r );
    if ( mRcptToDenyIsFailure )
      setFailed();
  }

  void TransactionState::setDataCommandSucceeded( bool succeeded, const Response & r ) {
    mDataCommandSucceeded = succeeded;
    mDataResponse = r;
    if ( !succeeded )
      setFailed();
    else if ( failed() )
      // can happen with pipelining: the server accepts the DATA, but
      // we don't want to send the data, so force a connection
      // shutdown:
      setFailedFatally();
  }

  int TransactionState::errorCode() const {
    if ( !failed() )
      return 0;
    if ( mErrorCode )
      return mErrorCode;
    if ( haveRejectedRecipients() || !dataCommandSucceeded() )
      return KIO::ERR_NO_CONTENT;
    // ### what else?
    return KIO::ERR_INTERNAL;
  }

  QString TransactionState::errorMessage() const {
    if ( !failed() )
      return QString();

    if ( !mErrorMessage.isEmpty() )
      return mErrorMessage;

    if ( haveRejectedRecipients() ) {
      QStringList recip;
      for ( RejectedRecipientList::const_iterator it = mRejectedRecipients.begin() ;
	    it != mRejectedRecipients.end() ; ++it )
	recip.push_back( (*it).recipient + " (" + (*it).reason + ')' );
      return i18n("Message sending failed since the following recipients were rejected by the server:\n"
		  "%1", recip.join("\n"));
    }

    if ( !dataCommandSucceeded() )
      return i18n("The attempt to start sending the message content failed.\n"
		  "%1", mDataResponse.errorMessage() );

    // ### what else?
    return i18n("Unhandled error condition. Please send a bug report.");
  }

}
