/*  -*- c++ -*-
    response.h

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

#ifndef __KIOSMTP_RESPONSE_H__
#define __KIOSMTP_RESPONSE_H__

#include <kcmdlineargs.h> // for QCStringList :-(

namespace KioSMTP {

  class Response {
  public:
    Response()
      : mCode(0),
	mValid(true),
	mSawLastLine(false),
	mWellFormed(true) {}

    void parseLine( const char * line, int len );

    /** Return an internationalized error message according to the
	response's code. */
    QString errorMessage() const;

    enum Reply {
      UnknownReply = -1,
      PositivePreliminary = 1,
      PositiveCompletion = 2,
      PositiveIntermediate = 3,
      TransientNegative = 4,
      PermanentNegative = 5
    };

    enum Category {
      UnknownCategory = -1,
      SyntaxError = 0,
      Information = 1,
      Connections = 2,
      MailSystem = 5
    };

    unsigned int code() const { return mCode; }
    unsigned int first() const { return code() / 100 ; }
    unsigned int second() const { return ( code() % 100 ) / 10 ; }
    unsigned int third() const { return code() % 10 ; }

    bool isPositive() const { return first() <= 3 && first() >= 1 ; }
    bool isNegative() const { return first() == 4 || first() == 5 ; }
    bool isUnknown() const { return !isPositive() && !isNegative() ; }

    QCStringList lines() const { return mLines; }

    bool isValid() const { return mValid; }
    bool isComplete() const { return mSawLastLine; }

    /** Shortcut method.
	@return true iff the response is valid, complete and positive */
    bool isOk() const { return isValid() && isComplete() && isPositive() ; }
    /** Indicates whether the response was well-formed, meaning it
	obeyed the syntax of smtp responses. That the response
	nevertheless is not valid may be caused by e.g. different
	response codes in a multilie response. A non-well-formed
	response is never valid. */
    bool isWellFormed() const { return mWellFormed; }

    void clear() { *this = Response(); }

  private:
    unsigned int mCode;
    QCStringList mLines;
    bool mValid;
    bool mSawLastLine;
    bool mWellFormed;
  };

}; // namespace KioSMTP

#endif // __KIOSMTP_RESPONSE_H__
