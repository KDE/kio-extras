/*  -*- c++ -*-
    interactivesmtpserver.cc

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

#include <config.h>

#include <qserversocket.h>
#include <qsocket.h>
#include <qwidget.h>
#include <qapplication.h>
#include <qhostaddress.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qstring.h>
#include <qlayout.h>
#include <qpushbutton.h>

#include <cassert>

static const QHostAddress localhost( 0x7f000001 ); // 127.0.0.1

static QString err2str( int err ) {
  switch ( err ) {
  case QSocket::ErrConnectionRefused: return "Connection refused";
  case QSocket::ErrHostNotFound: return "Host not found";
  case QSocket::ErrSocketRead: return "Failed to read from socket";
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
  InteractiveSMTPServerWindow( QSocket * socket, QWidget * parent=0, const char * name=0, WFlags f=0 );
  ~InteractiveSMTPServerWindow() {
    if ( mSocket ) {
      mSocket->close();
      if ( mSocket->state() == QSocket::Closing )
	connect( mSocket, SIGNAL(delayedCloseFinished()),
		 mSocket, SLOT(deleteLater()) );
      else
	mSocket->deleteLater();
      mSocket = 0;
    }
  }
  
public slots:
  void slotSendResponse() {
    const QString line = mLineEdit->text();
    mLineEdit->clear();
    QTextStream s( mSocket );
    s << line + "\r\n";
    slotDisplayServer( line );
  }
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
  QSocket * mSocket;
  QTextEdit * mTextEdit;
  QLineEdit * mLineEdit;
};

class InteractiveSMTPServer : public QServerSocket {
  Q_OBJECT
public:
  InteractiveSMTPServer( QObject * parent=0 )
    : QServerSocket( localhost, 2525, 1, parent ) {}
  ~InteractiveSMTPServer() {}

  /*! \reimp */
  void newConnection( int fd ) {
    QSocket * socket = new QSocket();
    socket->setSocket( fd );
    InteractiveSMTPServerWindow * w = new InteractiveSMTPServerWindow( socket );
    w->show();
  }
};

int main( int argc, char * argv[] ) {
  QApplication app( argc, argv );

  InteractiveSMTPServer server;

  qDebug( "Server should now listen on localhost:2525" );
  qDebug( "Hit CTRL-C to quit." );
  return app.exec();
};


InteractiveSMTPServerWindow::InteractiveSMTPServerWindow( QSocket * socket, QWidget * parent, const char * name, WFlags f )
  : QWidget( parent, name, f ), mSocket( socket )
{
  QPushButton * but;
  assert( socket );

  QVBoxLayout * vlay = new QVBoxLayout( this, 6 );

  mTextEdit = new QTextEdit( this );
  mTextEdit->setTextFormat( QTextEdit::LogText );
  vlay->addWidget( mTextEdit, 1 );

  QHBoxLayout * hlay = new QHBoxLayout( vlay );

  mLineEdit = new QLineEdit( this );
  but = new QPushButton( "&Send", this );
  hlay->addWidget( new QLabel( mLineEdit, "&Response:", this ) );
  hlay->addWidget( mLineEdit, 1 );
  hlay->addWidget( but );

  connect( mLineEdit, SIGNAL(returnPressed()), SLOT(slotSendResponse()) );
  connect( but, SIGNAL(clicked()), SLOT(slotSendResponse()) );

  but = new QPushButton( "&Close Connection", this );
  vlay->addWidget( but );

  connect( but, SIGNAL(clicked()), SLOT(slotConnectionClosed()) );

  connect( socket, SIGNAL(connectionClosed()), SLOT(slotConnectionClosed()) );
  connect( socket, SIGNAL(error(int)), SLOT(slotError(int)) );
  connect( socket, SIGNAL(readyRead()), SLOT(slotReadyRead()) );

  mLineEdit->setText( "220 hi there" );
  mLineEdit->setFocus();
}

#include "interactivesmtpserver.moc"
