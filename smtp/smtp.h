/**
 * smtp.h
 *
 * Copyright (c) 2000 George Staikos <staikos@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _SMTP_H
#define _SMTP_H

#include <qstring.h>
#include <kio/tcpslavebase.h>

class SMTPProtocol : public KIO::TCPSlaveBase {
 public:
  SMTPProtocol( const QCString &pool, const QCString &app, bool SSL );
  virtual ~SMTPProtocol();

  virtual void setHost(const QString& host, int port, const QString& user, const QString& pass);

  virtual void put( const KURL& url, int permissions, bool overwrite, bool resume);

 private:

  bool smtp_open(KURL &url);
  void smtp_close();
  bool command(const char *buf, char *r_buf = NULL, unsigned int r_len = 0);
  bool getResponse(char *r_buf = NULL, unsigned int r_len = 0, const char *cmd = NULL);

  int m_iSock;
  struct timeval m_tTimeout;
  FILE *fp;
  bool opened;
  QString m_sServer, m_sOldServer;
  unsigned short int m_iPort, m_iOldPort;
  QString m_sError;
};

#endif
