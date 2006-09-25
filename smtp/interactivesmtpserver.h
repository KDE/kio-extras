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

#include <QtGui/QWidget>
#include <QtNetwork/QTcpServer>

class QTcpServer;

class InteractiveSMTPServerWindow : public QWidget
{
  Q_OBJECT
  public:
    InteractiveSMTPServerWindow( QTcpSocket * socket, QWidget * parent=0);
    ~InteractiveSMTPServerWindow();

  public Q_SLOTS:
    void slotSendResponse();
    void slotDisplayClient( const QString & s );
    void slotDisplayServer( const QString & s );
    void slotDisplayMeta( const QString & s );
    void slotReadyRead();
    void slotError( QAbstractSocket::SocketError error );
    void slotConnectionClosed();
    void slotCloseConnection();

  private:
    QTcpSocket * mSocket;
    QTextEdit * mTextEdit;
    QLineEdit * mLineEdit;
    QLabel * mLabel;
};

class InteractiveSMTPServer : public QTcpServer
{
  Q_OBJECT

  public:
    InteractiveSMTPServer( QObject * parent=0 );
    ~InteractiveSMTPServer() {}

  private Q_SLOTS:
    void newConnectionAvailable();
};


#endif
