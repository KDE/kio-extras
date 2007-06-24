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

#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

#include "interactivesmtpserver.h"

static const QHostAddress localhost( 0x7f000001 ); // 127.0.0.1

static QString err2str( QAbstractSocket::SocketError error )
{
  switch ( error ) {
    case QAbstractSocket::ConnectionRefusedError: return "Connection refused";
    case QAbstractSocket::HostNotFoundError: return "Host not found";
    default: return "Unknown error";
  }
}


static QString escape( QString s )
{
  return s
    .replace( '&', "&amp;" )
    .replace( '>', "&gt;" )
    .replace( '<', "&lt;" )
    .replace( '"', "&quot;" )
    ;
}


static QString trim( const QString & s )
{
  if ( s.endsWith( "\r\n" ) )
    return s.left( s.length() - 2 );
  if ( s.endsWith( "\r" ) || s.endsWith( "\n" ) )
    return s.left( s.length() - 1 );
  return s;
}

InteractiveSMTPServerWindow::~InteractiveSMTPServerWindow()
{
    if ( mSocket ) {
        mSocket->close();
        if ( mSocket->state() == QAbstractSocket::ClosingState )
            connect( mSocket, SIGNAL(delayedCloseFinished()),
                     mSocket, SLOT(deleteLater()) );
        else
            mSocket->deleteLater();
        mSocket = 0;
    }
}

void InteractiveSMTPServerWindow::slotSendResponse()
{
        const QString line = mLineEdit->text();
    mLineEdit->clear();
    QTextStream s( mSocket );
    s << line + "\r\n";
    slotDisplayServer( line );
}

InteractiveSMTPServer::InteractiveSMTPServer( QObject* parent )
    : QTcpServer( parent )
{
  listen( localhost, 2525 );
  setMaxPendingConnections( 1 );

  connect( this, SIGNAL( newConnection() ), this, SLOT( newConnectionAvailable() ) );
}

void InteractiveSMTPServer::newConnectionAvailable()
{
  InteractiveSMTPServerWindow * w = new InteractiveSMTPServerWindow( nextPendingConnection() );
  w->show();
}

int main( int argc, char * argv[] ) {
  QApplication app( argc, argv );

  InteractiveSMTPServer server;

  qDebug( "Server should now listen on localhost:2525" );
  qDebug( "Hit CTRL-C to quit." );

  return app.exec();
}


InteractiveSMTPServerWindow::InteractiveSMTPServerWindow( QTcpSocket * socket, QWidget * parent )
  : QWidget( parent ), mSocket( socket )
{
  QPushButton * but;
  Q_ASSERT( socket );

  QVBoxLayout * vlay = new QVBoxLayout( this );
  vlay->setSpacing( 6 );

  mTextEdit = new QTextEdit( this );
  vlay->addWidget( mTextEdit, 1 );
  QWidget *mLayoutWidget = new QWidget;
  vlay->addWidget( mLayoutWidget );

  QHBoxLayout * hlay = new QHBoxLayout( mLayoutWidget );
    
  mLineEdit = new QLineEdit( this );
  mLabel = new QLabel( "&Response:", this );
  mLabel->setBuddy( mLineEdit );
  but = new QPushButton( "&Send", this );
  hlay->addWidget( mLabel );
  hlay->addWidget( mLineEdit, 1 );
  hlay->addWidget( but );

  connect( mLineEdit, SIGNAL(returnPressed()), SLOT(slotSendResponse()) );
  connect( but, SIGNAL(clicked()), SLOT(slotSendResponse()) );

  but = new QPushButton( "&Close Connection", this );
  vlay->addWidget( but );

  connect( but, SIGNAL(clicked()), SLOT(slotConnectionClosed()) );

  connect( socket, SIGNAL(disconnected()), SLOT(slotConnectionClosed()) );
  connect( socket, SIGNAL(error(QAbstractSocket::SocketError)),
           SLOT(slotError(QAbstractSocket::SocketError)) );
  connect( socket, SIGNAL(readyRead()), SLOT(slotReadyRead()) );

  mLineEdit->setText( "220 hi there" );
  mLineEdit->setFocus();
}

void InteractiveSMTPServerWindow::slotDisplayClient( const QString & s )
{
  mTextEdit->append( "C:" + escape(s) );
}

void InteractiveSMTPServerWindow::slotDisplayServer( const QString & s )
{
  mTextEdit->append( "S:" + escape(s) );
}

void InteractiveSMTPServerWindow::slotDisplayMeta( const QString & s )
{
  mTextEdit->append( "<font color=\"red\">" + escape(s) + "</font>" );
}

void InteractiveSMTPServerWindow::slotReadyRead()
{
  while ( mSocket->canReadLine() )
    slotDisplayClient( trim( mSocket->readLine() ) );
}

void InteractiveSMTPServerWindow::slotError( QAbstractSocket::SocketError error )
{
  slotDisplayMeta( QString( "E: %1" ).arg( err2str( error ) ) );
}

void InteractiveSMTPServerWindow::slotConnectionClosed()
{
  slotDisplayMeta( "Connection closed by peer" );
}

void InteractiveSMTPServerWindow::slotCloseConnection()
{
  mSocket->close();
}

#include "interactivesmtpserver.moc"
