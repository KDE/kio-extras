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
 */

#ifndef _SMTP_H
#define _SMTP_H

#include <kio/tcpslavebase.h>

#include "capabilities.h"

#include <qstring.h>
#include <qcstring.h>
#include <qstringlist.h>
#include <qstrlist.h>

namespace KioSMTP {
  class Response;
};

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
  bool command( QCString cmd, KioSMTP::Response * resp=0 );
  /** This is an overloaded member function, provided for
      convenience. It behaves essentially like the above function. */
  bool command( const QString & cmd, KioSMTP::Response * resp=0 );
  /** This is an overloaded member function, provided for
      convenience. It behaves essentially like the above function. */
  bool command( const char * cmd, KioSMTP::Response * resp=0 );

  /** Parse a single response from the server. Single- vs. multiline
      responses are correctly detected.

      @param ok if not 0, returns whether response parsing was
                successful. Don't confuse this with negative responses
                (e.g. 5xx), which you can check for using
                @ref Response::isNegative()
      @return the @ref Response object representing the server response.
  **/
  KioSMTP::Response getResponse( bool * ok );

  bool authenticate();
  void parseFeatures( const KioSMTP::Response & ehloResponse );
  bool putRecipients( const QStringList & list );

  /** This is a pure convenience wrapper around
      @ref KioSMTP::Capabilities::have() */
  bool haveCapability( const char * cap ) const {
    return mCapabilities.have( cap );
  }
  /** This is a pure convenience wrapper around
      @ref KioSMTP::Capabilities::createSpecialResponse */
  QString createSpecialResponse() const {
    return mCapabilities.createSpecialResponse( usingTLS() || haveCapability( "STARTTLS" ) );
  }

  unsigned short m_iOldPort;
  bool m_opened;
  bool m_errorSent;
  QString m_sServer, m_sOldServer;
  QString m_sUser, m_sOldUser;
  QString m_sPass, m_sOldPass;
  QString m_hostname;

  QCString m_lastError;

  KioSMTP::Capabilities mCapabilities;
};

#endif
