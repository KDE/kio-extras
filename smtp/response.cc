/*  -*- c++ -*-
    response.cc

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

#include <config.h>

#include "response.h"

#include <klocale.h>
#include <kio/global.h>

#include <QStringList>
#include <QByteArray>

namespace KioSMTP {

  void Response::parseLine( const char * line, int len ) {

    if ( !isWellFormed() ) return; // don't bother

    if ( isComplete() )
      // if the response is already complete, there can't be another line
      mValid = false;

    if ( len > 1 && line[len-1] == '\n' && line[len-2] == '\r' )
      len -= 2;

    if ( len < 3 ) {
      // can't be valid - too short
      mValid = false;
      mWellFormed = false;
      return;
    }

    bool ok = false;
    unsigned int code = QByteArray( line, 3+1 ).toUInt( &ok );
    if ( !ok || code < 100 || code > 559 ) {
      // not a number or number out of range
      mValid = false;
      if ( !ok || code < 100 )
	mWellFormed = false;
      return;
    }
    if ( mCode && code != mCode ) {
      // different codes in one response are not allowed.
      mValid = false;
      return;
    }
    mCode = code;

    if ( len == 3 || line[3] == ' ' )
      mSawLastLine = true;
    else if ( line[3] != '-' ) {
      // code must be followed by either SP or hyphen (len == 3 is
      // also accepted since broken servers exist); all else is
      // invalid
      mValid = false;
      mWellFormed = false;
      return;
    }

    mLines.push_back( len > 4 ? QByteArray( line+4, len-4+1 ).trimmed() : QByteArray() );
  }


  // hackishly fixing QCStringList flaws...
  static QByteArray join( char sep, const QCStringList & list ) {
    if ( list.empty() )
      return QByteArray();
    QByteArray result = list.front();
    for ( QCStringList::const_iterator it = ++list.begin() ; it != list.end() ; ++it )
      result += sep + *it;
    return result;
  }

  QString Response::errorMessage() const {
    QString msg;
    if ( lines().count() > 1 )
      msg = i18n("The server responded:\n%1",
	  QString(join( '\n', lines() )) );
    else
      msg = i18n("The server responded: \"%1\"",
	  QString(lines().front()) );
    if ( first() == 4 )
      msg += '\n' + i18n("This is a temporary failure. "
			 "You may try again later.");
    return msg;
  }

  int Response::errorCode() const {
    switch ( code() ) {
    case 421: // Service not available, closing transmission channel
    case 454: // TLS not available due to temporary reason
              // Temporary authentication failure
    case 554: // Transaction failed / No SMTP service here / No valid recipients
      return KIO::ERR_SERVICE_NOT_AVAILABLE;

    case 451: // Requested action aborted: local error in processing
      return KIO::ERR_INTERNAL_SERVER;

    case 452: // Requested action not taken: insufficient system storage
    case 552: // Requested mail action aborted: exceeded storage allocation
	return KIO::ERR_DISK_FULL;

    case 500: // Syntax error, command unrecognized
    case 501: // Syntax error in parameters or arguments
    case 502: // Command not implemented
    case 503: // Bad sequence of commands
    case 504: // Command parameter not implemented
      return KIO::ERR_INTERNAL;

    case 450: // Requested mail action not taken: mailbox unavailable
    case 550: // Requested action not taken: mailbox unavailable
    case 551: // User not local; please try <forward-path>
    case 553: // Requested action not taken: mailbox name not allowed
      return KIO::ERR_DOES_NOT_EXIST;

    case 530: // {STARTTLS,Authentication} required
    case 538: // Encryption required for requested authentication mechanism
    case 534: // Authentication mechanism is too weak
      return KIO::ERR_UPGRADE_REQUIRED;

    case 432: // A password transition is needed
      return KIO::ERR_COULD_NOT_AUTHENTICATE;

    default:
      if ( isPositive() )
	return 0;
      else
	return KIO::ERR_UNKNOWN;
    }
  }

} // namespace KioSMTP
