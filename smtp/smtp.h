/* -*- c++ -*-
 * Copyright (c) 2000, 2001 Alex Zepeda <zipzippy@sonic.net>
 * Copyright (c) 2001 Michael Häckel <Michael@Haeckel.Net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id$
 */

#ifndef _SMTP_H
#define _SMTP_H

#include <kio/tcpslavebase.h>

class KDESasl;

class SMTPProtocol : public KIO::TCPSlaveBase {
public:
  SMTPProtocol(const QCString & pool, const QCString & app, bool useSSL);
  virtual ~ SMTPProtocol();

  virtual void setHost(const QString & host, int port,
                       const QString & user, const QString & pass);

  virtual void special(const QByteArray & aData);
  virtual void put(const KURL & url, int permissions, bool overwrite,
                   bool resume);
  virtual void stat(const KURL & url);
  virtual void openConnection();
  virtual void closeConnection();

protected:

  bool smtp_open(const QString& fakeHostname = QString::null);
  void smtp_close();
  /** Send command @p cmd. The line ending CRLF is added here. */
  bool command(QCString cmd, 
               bool handleErrors = true,
               char *r_buf = NULL, 
               unsigned int r_len = 0);
  /** This is an overloaded member function, provided for
      convenience. It behaves essentially like the above function. */
  bool command(const QString & cmd, 
               bool handleErrors = true,
               char *r_buf = NULL, 
               unsigned int r_len = 0);
  /** This is an overloaded member function, provided for
      convenience. It behaves essentially like the above function. */
  bool command(const char * cmd, 
               bool handleErrors = true,
               char *r_buf = NULL, 
               unsigned int r_len = 0);
  int getResponse(bool handleErrors = true,
                  char *real_buf = NULL, 
                  unsigned int real_len = 0);
  bool Authenticate();
  void ParseFeatures(const char *buf);
  bool putRecipients( const QStringList & list );
  int GetVal(char *buf);

  unsigned short m_iOldPort;
  bool m_opened;
  bool m_haveTLS;
  bool m_errorSent;
  QString m_sServer, m_sOldServer;
  QString m_sUser, m_sOldUser;
  QString m_sPass, m_sOldPass;
  QString m_hostname;

  QString  m_sAuthConfig;
  QCString m_lastError;

  static const int DEFAULT_RESPONSE_BUFFER = 512;
  static const int DEFAULT_EHLO_BUFFER = 5120;
  static const int SMTP_MIN_NEGATIVE_REPLY = 400;

  class Response;
  class Capabilities;
};

#endif
