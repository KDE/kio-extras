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

#include "response.h"

namespace KioSMTP {

  void Response::parseLine( const char * line, int len ) {

    if ( !isWellFormed() ) return; // don't bother

    if ( isComplete() )
      // if the response is already complete, there can't be another line
      mValid = false;

    if ( len < 4 ) {
      // can't be valid - too short
      mValid = false;
      mWellFormed = false;
      return;
    }

    bool ok = false;
    unsigned int code = QCString( line, 3+1 ).toUInt( &ok );
    if ( !ok || code < 100 || code > 559 ) {
      // not a number or number out of range
      mValid = false;
      if ( !ok )
	mWellFormed = false;
      return;
    }
    if ( mCode && code != mCode ) {
      // different codes in one response are not allowed.
      mValid = false;
      return;
    }
    mCode = code;

    if ( line[3] == ' ' )
      mSawLastLine = true;
    else if ( line[3] != '-' ) {
      // code must be followed by either SP or hyphen; all else is invalid
      mValid = false;
      mWellFormed = false;
      return;
    }

    mLines.push_back( QCString( line+4, len-4+1 ).stripWhiteSpace() );
  }

    

}; // namespace KioSMTP
