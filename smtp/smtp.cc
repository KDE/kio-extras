/**
 * smtp.cpp
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <qcstring.h>
#include <qglobal.h>

#include <kprotocolmanager.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <klocale.h>

#include "smtp.h"


//
//
// Based partially on the pop3 ioslave.
//

using namespace KIO;

extern "C" { int kdemain (int argc, char **argv); }

int kdemain( int argc, char **argv )
{
KInstance instance( "kio_smtp" );

  if (argc != 4)
  {
    fprintf(stderr, "Usage: kio_smtp protocol domain-socket1 domain-socket2\n");
    exit(-1);
  }

  SMTPProtocol *slave;
  // Are we looking to use SSL?
  if (strcasecmp(argv[1], "pop3s") == 0)
        slave = new SMTPProtocol(argv[2], argv[3], true);
  else
        slave = new SMTPProtocol(argv[2], argv[3], false);
  slave->dispatchLoop();
  delete slave;
  return 0;
}



//
//   COMMANDS WE SUPPORT:
//
//
//   COMMANDS WE SHOULD SUPPORT:
//   QUIT,       EXPN,       HELO,       EHLO,       HELP,       NOOP,
//   VRFY,       RSET,       DATA,       RCPT TO:,   MESG FROM:, ETRN,
//   VERB,       DSN,        8BITMIME,   SIZE,       ONEX,       XUSR
//
//   We should separate ESMTP from SMTP and do the test by the banner and/or
//   EHLO vs HELO support.
//
//   COMMANDS THAT DON'T SEEM SUPPORTED ANYMORE:
//   SAML FROM:, SOML FROM:, SEND FROM:, TURN
//

SMTPProtocol::SMTPProtocol(const QCString &pool, const QCString &app, bool SSL) : TCPSlaveBase((SSL ? 995 : 110), (SSL ? "smtps" : "smtp"), pool, app)
{
  kdDebug() << "SMTPProtocol::SMTPProtocol" << endl;
  m_bIsSSL=SSL;
}


SMTPProtocol::~SMTPProtocol()
{
  kdDebug() << "SMTPProtocol::~SMTPProtocol" << endl;
  smtp_close();
}


void SMTPProtocol::put( const KURL& url, int permissions, bool overwrite, bool resume) {
//
// Commands
//
// smtp://host:port/done                 QUIT
// smtp://host:port/mail                 Send a mail
// Should these two be in ::get()?  ....
// smtp://host:port/verify               VRFY
// smtp://host:port/expand               EXPN
//
}

void SMTPProtocol::setHost( const QString& host, int port, const QString& /*user*/, const QString& /*pass*/) {
  m_sServer = host;
  m_iPort = port;
}

//
// REPLY CODES
//       (is this up to date?)
// 5xx - errors
// 500 Syntax error, command unrecognized, parameter required
// 501 Syntax error in parameters or arguments
// 502 Command not implemented
// 503 Bad sequence of commands
// 504 Command parameter not implemented
//
//     - status
// 211 System status or help reply
// 214 Help message
// 220 <domain> Service ready
// 221 <domain> Service closing transmission channel
// 421 <domain> Service not available, closing transmission channel
//
//     - success or error
// 250 Requested mail action okay, completed
// 251 User not local; will forward to <addr>
// 450 Requested mail action not taken: mailbox unavailable (busy)
// 550 Requested action not taken: mailbox unavailable (not found)
// 451 Requested action aborted: error in processing
// 551 User not local; please try <addr>
// 452 Requested action not taken: insufficient storage
// 553 Requested action not taken: mailbox name not allowed
// 354 Start mail input; end with <CRLF>.<CRLF>
// 554 Transaction failed
//
// Commands with their replies:
//
// CONNECTION: Success: 220; Fail: 421; Error:
// HELO:       Success: 250; Fail: ; Error: 500, 501, 504, 421
// MAIL FROM:: Success: 250; Fail: 552, 451, 452; Error: 500, 501, 421
// RCPT FROM:: Success: 250, 251; Fail: 550, 551, 552, 553, 450, 451, 452;
//             Error: 500, 501, 503, 421
// DATA:       Initial: 354;  data -> Success: 250; Fail: 552, 554, 451, 452
//             Fail: 451, 554; Error: 500, 501, 503, 421
// RSET:       Success: 250; Fail: ; Error: 500, 501, 504, 421
// VRFY:       Success: 250, 251; Fail: 550, 551, 553;
//             Error: 500, 501, 502, 504, 421
// EXPN:       Success: 250; Fail: 550; Error: 500, 501, 502, 504, 421
// HELP:       Success: ; Fail: ; Error: 500, 501, 502, 504, 421
// NOOP:       Success: 250; Fail: ; Error: 500, 421
// QUIT:       Success: 221; Fail: ; Error: 500
// EHLO:       ???
// VERB:       ???
// ETRN:       ???
// DSN:        ???
// 8BITMIME:   ???
// SIZE:       ???
// ONEX:       ???
// XUSR:       ???
//

bool SMTPProtocol::getResponse(char *r_buf, unsigned int r_len, const char *cmd) {
  char *buf=0;
  unsigned int recv_len=0;
  fd_set FDs;

  // Give the buffer the appropiate size
  if (r_len)
    buf=(char *)malloc(r_len);
  else {
    buf=(char *)malloc(512);
    r_len=512;
  }

  // And keep waiting if it timed out
  unsigned int wait_time=60; // Wait 60sec. max.
  do
  {
      // Wait for something to come from the server
    FD_ZERO(&FDs);
    FD_SET(m_iSock, &FDs);
    // Yes, it's true, Linux sucks. (And you can't program --Waba)
    wait_time--;
    m_tTimeout.tv_sec=1;
    m_tTimeout.tv_usec=0;
  }
  while (wait_time && (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0));

  if (wait_time == 0)
  {
    kdDebug () << "No response from SMTP server in 60 secs." << endl;
    return false;
  }

  // Clear out the buffer
  memset(buf, 0, r_len);
  // And grab the data
  ReadLine(buf, r_len-1);

  // This is really a funky crash waiting to happen if something isn't
  // null terminated.
  recv_len=strlen(buf);

  // [E]SMTP returns responses as follows:
  //
  // xxx-text
  // xxx-text
  // xxx final text line.
  //
  // The subtle identifier of EOT is that the last line has a space instead
  // of a dash after the number.  See appendix E in STD010
  // FIXME:
  // This has to be tested after every read....  The code does not do this
  // yet and hence will most certainly not work.
  //

  // Can we assume that all return messages will be in the same primary class?
  // (ie all 1xx, but no 1xx and 4xx together?)

  // HERE WE CHECK THE SUCCESSFUL RESPONSES FIRST 1xy,2xy,3xy
  // AND THE ERROR RESPONSES SECOND               4xy,5xy
  switch(buf[0]) {
  case '1':
  case '2':
  case '3':
    if (buf[3] == ' ') {         // final line

    } else if (buf[3] == '-') {  // another line follows

    } else {                     // error of some sort?

    }
    if (r_buf && r_len) {
      memcpy(r_buf, buf+4, QMIN(r_len,recv_len-4));
    }
    if (buf) free(buf);
    return true;
  case '4':
  case '5':
    if (buf[3] == ' ') {         // final line

    } else if (buf[3] == '-') {  // another line follows

    } else {                     // error of some sort?

    }
    if (r_buf && r_len) {
      memcpy(r_buf, buf+4, QMIN(r_len,recv_len-4));
    }
    if (buf) free(buf);
    return false;
  default:
    kdDebug () << "Invalid or unknown SMTP response received!" << endl;
    if (r_buf && r_len) {
      memcpy(r_buf, buf, QMIN(r_len,recv_len));
    }
    if (buf) free(buf);
    return false;
  }
}


bool SMTPProtocol::command(const char *cmd, char *recv_buf, unsigned int len) {
  // Write the command
  if (Write(cmd, strlen(cmd)) != static_cast<ssize_t>(strlen(cmd)))
  {
    m_sError = i18n("Could not send to server.\n");
    return false;
  }
  if (Write("\r\n", 2) != 2)
  {
    m_sError = i18n("Could not send to server.\n");
    return false;
  }
  return getResponse(recv_buf, len, cmd);
}


bool SMTPProtocol::smtp_open(KURL& url) {
  if ( (m_iOldPort == GetPort(m_iPort)) && (m_sOldServer == m_sServer) ) {
    kdDebug() << "Reusing old connection." << endl;
    return true;
  } else {
    smtp_close();
    if( !ConnectToHost(m_sServer.ascii(), m_iPort))
       return false; // ConnectToHost has already send an error message.
  }

  QCString greeting(1024);
  if (!getResponse(greeting.data(), greeting.size(), ""))
    return false;

  m_iOldPort = m_iPort;
  m_sOldServer = m_sServer;

return true;
}


void SMTPProtocol::smtp_close() {
  if (!opened)
      return;
  command("QUIT");
  CloseDescriptor();
  m_sOldServer = "";
  opened = false;
}


