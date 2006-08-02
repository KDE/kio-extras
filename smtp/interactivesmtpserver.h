#ifndef INTERACTIVESMTPSERVER_H
#define INTERACTIVESMTPSERVER_H

/*  -*- c++ -*-
    interactivesmtpserver.h

    Code based on the serverSocket example by Jesper Pedersen.

    This file is part of the testsuite of kio_smtp, the KDE SMTP kioslave.
    Copyright (c) 2004 Marc Mutz <mutz@kde.org>

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

#include <QWidget>


static QString err2str( int err ) {
  switch ( err ) {
  case Q3Socket::ErrConnectionRefused: return "Connection refused";
  case Q3Socket::ErrHostNotFound: return "Host not found";
  case Q3Socket::ErrSocketRead: return "Failed to read from socket";
  default: return "Unknown error";
  }
}


static QString escape( QString s ) {
  return s
    .replace( '&', "&amp;" )
    .replace( '>', "&gt;" )
    .replace( '<', "&lt;" )
    .replace( '"', "&quot;" )
    ;
}


static QString trim( const QString & s ) {
  if ( s.endsWith( "\r\n" ) )
    return s.left( s.length() - 2 );
  if ( s.endsWith( "\r" ) || s.endsWith( "\n" ) )
    return s.left( s.length() - 1 );
  return s;
}


class InteractiveSMTPServerWindow : public QWidget {
  Q_OBJECT
public:
  InteractiveSMTPServerWindow( Q3Socket * socket, QWidget * parent=0);
  ~InteractiveSMTPServerWindow();

public Q_SLOTS:
  void slotSendResponse();
  void slotDisplayClient( const QString & s ) {
    mTextEdit->append( "C:" + escape(s) );
  }
  void slotDisplayServer( const QString & s ) {
    mTextEdit->append( "S:" + escape(s) );
  }
  void slotDisplayMeta( const QString & s ) {
    mTextEdit->append( "<font color=\"red\">" + escape(s) + "</font>" );
  }
  void slotReadyRead() {
    while ( mSocket->canReadLine() )
      slotDisplayClient( trim( mSocket->readLine() ) );
  }
  void slotError( int err ) {
    slotDisplayMeta( QString( "E: %1 (%2)" ).arg( err2str( err ) ).arg( err ) );
  }
  void slotConnectionClosed() {
    slotDisplayMeta( "Connection closed by peer" );
  }
  void slotCloseConnection() {
    mSocket->close();
  }
private:
  Q3Socket * mSocket;
  QTextEdit * mTextEdit;
  QLineEdit * mLineEdit;
  QLabel * mLabel;
};

class InteractiveSMTPServer : public Q3ServerSocket {
  Q_OBJECT
public:
  InteractiveSMTPServer( QObject * parent=0 );
  ~InteractiveSMTPServer() {}

  /*! \reimp */
  void newConnection( int fd ) {
    Q3Socket * socket = new Q3Socket();
    socket->setSocket( fd );
    InteractiveSMTPServerWindow * w = new InteractiveSMTPServerWindow( socket );
    w->show();
  }
};


#endif
