/*  -*- c++ -*-
    request.cc

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

#include "request.h"

#include <kurl.h>
#include <kidna.h>
#include <kcodecs.h>
#include <kdebug.h>

#include <assert.h>
//Added by qt3to4:
#include <QByteArray>

namespace KioSMTP {

  Request Request::fromURL( const KURL & url ) {
    Request request;

    const QStringList query = url.query().mid(1).split( '&');
#ifndef NDEBUG
    kdDebug(7112) << "Parsing request from query:\n" + query.join("\n" ) << endl;
#endif
    for ( QStringList::const_iterator it = query.begin() ; it != query.end() ; ++it ) {
      int equalsPos = (*it).find( '=' );
      if ( equalsPos <= 0 )
	continue;

      const QString key = (*it).left( equalsPos ).toLower();
      const QString value = KURL::decode_string( (*it).mid( equalsPos + 1 ) );

      if ( key == "to" )
	request.addTo( value );
      else if ( key == "cc" )
	request.addCc( value );
      else if ( key == "bcc" )
	request.addBcc( value );
      else if ( key == "headers" ) {
	request.setEmitHeaders( value == "0" );
	request.setEmitHeaders( false ); // ### ???
      }
      else if ( key == "subject" )
	request.setSubject( value );
      else if ( key == "from" )
	request.setFromAddress( value );
      else if ( key == "profile" )
	request.setProfileName( value );
      else if ( key == "hostname" )
	request.setHeloHostname( value );
      else if ( key == "body" )
	request.set8BitBody( value.toUpper() == "8BIT" );
      else if ( key == "size" )
	request.setSize( value.toUInt() );
      else
	kdWarning(7112) << "while parsing query: unknown query item \""
			<< key << "\" with value \"" << value << "\"" << endl;
    }

    return request;
  }

  QByteArray Request::heloHostnameCString() const {
    return KIDNA::toAsciiCString( heloHostname() );
  }

  static bool isUsAscii( const QString & s ) {
    for ( int i = 0 ; i < s.length() ; ++i )
      if ( s[i].unicode() > 127 ) return false;
    return true;
  }



  static inline bool isSpecial( char ch ) {
    static const QByteArray specials = "()<>[]:;@\\,.\"";
    return specials.find( ch ) >= 0;
  }



  static inline bool needsQuoting( char ch ) {
    return ch == '\\' || ch == '"' || ch == '\n' ;
  }



  static inline QByteArray rfc2047Encode( const QString & s ) {
    QByteArray r = KCodecs::base64Encode( s.trimmed().toUtf8(), false );
    return "=?utf-8?b?" + r + "?=" ; // use base64 since that always gives a valid encoded-word
  }



  static QByteArray quote( const QString & s ) {
    assert( isUsAscii( s ) );

    QByteArray r( s.length() * 2 );
    bool needsQuotes = false;

    unsigned int j = 0;
    for ( int i = 0 ; i < s.length() ; ++i ) {
      char ch = s[i].latin1();
      if ( isSpecial( ch ) ) {
	if ( needsQuoting( ch ) )
	  r[j++] = '\\';
	needsQuotes = true;
      }
      r[j++] = ch;
    }
    r.truncate( j );

    if ( needsQuotes )
      return '"' + r + '"';
    else
      return r;
  }



  static QByteArray formatFromAddress( const QString & fromRealName, const QString & fromAddress ) {
    if ( fromRealName.isEmpty() )
      return fromAddress.latin1(); // no real name: return "joe@user.org"

    // return "Joe User <joe@user.org>", "\"User, Joe\" <joe@user.org>"
    // or "=?utf-8?q?Joe_User?= <joe@user.org>", depending on real name's nature.
    QByteArray r = isUsAscii( fromRealName ) ? quote( fromRealName ) : rfc2047Encode( fromRealName );
    return r + " <" + fromAddress.latin1() + '>';
  }



  static QByteArray formatSubject( QString s ) {
    if ( isUsAscii( s ) )
      return s.remove( '\n' ).latin1(); // don't break header folding,
					// so remove any line break
					// that happen to be around
    else
      return rfc2047Encode( s );
  }



  QByteArray Request::headerFields( const QString & fromRealName ) const {
    if ( !emitHeaders() )
      return 0;

    assert( hasFromAddress() ); // should have been checked for by
				// caller (MAIL FROM comes before DATA)

    QByteArray result = "From: " + formatFromAddress( fromRealName, fromAddress() ) + "\r\n";

    if ( !subject().isEmpty() )
      result += "Subject: " + formatSubject( subject() ) + "\r\n";
    if ( !to().empty() )
      result += QByteArray( "To: " ) + to().join( ",\r\n\t" /* line folding */ ).latin1() + "\r\n";
    if ( !cc().empty() )
      result += QByteArray( "Cc: " ) + cc().join( ",\r\n\t" /* line folding */ ).latin1() + "\r\n";
    return result;
  }

} // namespace KioSMTP
