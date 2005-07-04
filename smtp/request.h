/*  -*- c++ -*-
    request.h

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
    Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301  USA

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

#ifndef __KIOSMTP_REQUEST_H__
#define __KIOSMTP_REQUEST_H__

#include <qstring.h>
#include <qstringlist.h>

class KURL;

namespace KioSMTP {

  class Request {
  public:
    Request()
      : mSubject( "missing subject" ), mEmitHeaders( true ),
	m8Bit( false ), mSize( 0 ) {}

    static Request fromURL( const KURL & url );

    QString profileName() const { return mProfileName; }
    void setProfileName( const QString & profileName ) { mProfileName = profileName; }
    bool hasProfile() const { return !profileName().isNull(); }

    QString subject() const { return mSubject; }
    void setSubject( const QString & subject ) { mSubject = subject; }

    QString fromAddress() const { return mFromAddress; }
    void setFromAddress( const QString & fromAddress ) { mFromAddress = fromAddress; }
    bool hasFromAddress() const { return !mFromAddress.isEmpty(); }

    QStringList recipients() const { return to() + cc() + bcc() ; }
    bool hasRecipients() const { return !to().empty() || !cc().empty() || !bcc().empty() ; }

    QStringList to() const { return mTo; }
    QStringList cc() const { return mCc; }
    QStringList bcc() const { return mBcc; }
    void addTo( const QString & to ) { mTo.push_back( to ); }
    void addCc( const QString & cc ) { mCc.push_back( cc ); }
    void addBcc( const QString & bcc ) { mBcc.push_back( bcc ); }

    QString heloHostname() const { return mHeloHostname; }
    QCString heloHostnameCString() const;
    void setHeloHostname( const QString & hostname ) { mHeloHostname = hostname; }

    bool emitHeaders() const { return mEmitHeaders; }
    void setEmitHeaders( bool emitHeaders ) { mEmitHeaders = emitHeaders; }

    bool is8BitBody() const { return m8Bit; }
    void set8BitBody( bool a8Bit ) { m8Bit = a8Bit; }

    unsigned int size() const { return mSize; }
    void setSize( unsigned int size ) { mSize = size; }

    /** If @ref #emitHeaders() is true, returns the rfc2822
	serialization of the header fields "To", "Cc", "Subject" and
	"From", as determined by the respective settings. If @ref
	#emitHeaders() is false, returns a null string. */
    QCString headerFields( const QString & fromRealName=QString::null ) const;

  private:
    QStringList mTo, mCc, mBcc;
    QString mProfileName, mSubject, mFromAddress, mHeloHostname;
    bool mEmitHeaders;
    bool m8Bit;
    unsigned int mSize;
  };

} // namespace KioSMTP

#endif // __KIOSMTP_REQUEST_H__
