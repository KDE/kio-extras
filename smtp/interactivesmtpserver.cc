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

#include "interactivesmtpserver.h"

static const QHostAddress localhost( 0x7f000001 ); // 127.0.0.1

InteractiveSMTPServerWindow::~InteractiveSMTPServerWindow() {
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

void InteractiveSMTPServerWindow::slotSendResponse()
{
        const QString line = mLineEdit->text();
    mLineEdit->clear();
    QTextStream s( mSocket );
    s << line + "\r\n";
    slotDisplayServer( line );
}

InteractiveSMTPServer::InteractiveSMTPServer( QObject* parent )
    : QServerSocket( localhost, 2525, 1, parent )
{
}

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
