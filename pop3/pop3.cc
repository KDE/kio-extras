/*
 * Copyright (c) 1999-2001 Alex Zepeda
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
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
#include <kio/ksasl/saslcontext.h>
#include <kio/passdlg.h>
#include <klocale.h>

#include "pop3.h"

#define GREETING_BUF_LEN 1024
#define MAX_RESPONSE_LEN 512

#ifndef NAPOP
	#include <kmdcodec.h>
#endif

extern "C" {
	int kdemain(int argc, char **argv);
};

using namespace KIO;

int kdemain( int argc, char **argv )
{
	KInstance instance( "kio_pop3" );

	if (argc != 4) {
		kdDebug() << "Usage: kio_pop3 protocol domain-socket1 domain-socket2" << endl;
		exit(-1);
	}

	POP3Protocol *slave;

	// Are we looking to use SSL?
	if (strcasecmp(argv[1], "pop3s") == 0)
		slave = new POP3Protocol(argv[2], argv[3], true);
	else
		slave = new POP3Protocol(argv[2], argv[3], false);

	slave->dispatchLoop();
	delete slave;
	return 0;
}

POP3Protocol::POP3Protocol(const QCString &pool, const QCString &app, bool isSSL)
   : TCPSlaveBase((isSSL ? 995 : 110), (isSSL ? "pop3s" : "pop3"), pool, app, isSSL)
{
	kdDebug() << "POP3Protocol()" << endl;
	m_bIsSSL=isSSL;
	m_cmd = CMD_NONE;
	m_iOldPort = 0;
	m_tTimeout.tv_sec=10;
	m_tTimeout.tv_usec=0;
	m_try_apop = true;
	m_try_sasl = true;
	opened = false;
}

POP3Protocol::~POP3Protocol()
{
	kdDebug() << "~POP3Protocol()" << endl;
	closeConnection();
}

void POP3Protocol::setHost( const QString& _host, int _port, const QString& _user, const QString& _pass )
{
	m_sServer = _host;
	m_iPort = _port;
	m_sUser = _user;
	m_sPass = _pass;
}

bool POP3Protocol::getResponse (char *r_buf, unsigned int r_len, const char *cmd)
{
	char *buf=0;
	unsigned int recv_len=0;
	fd_set FDs;

	// Give the buffer the appropiate size
	r_len = r_len ? r_len : MAX_RESPONSE_LEN;

	buf=static_cast<char *>(malloc(r_len));

	// And keep waiting if it timed out
	unsigned int wait_time=600; // Wait 600sec. max.
	do {
		// Wait for something to come from the server
		FD_ZERO(&FDs);
		FD_SET(m_iSock, &FDs);
		// Yes, it's true, Linux sucks.
		// Erp, make that Linux has to be different because it's Linux.. stupid. stupid stupid.
		wait_time--;
		m_tTimeout.tv_sec=1;
		m_tTimeout.tv_usec=0;
	}
	while (wait_time && (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0));

	if (wait_time == 0) {
		kdDebug() << "No response from POP3 server in 600 secs." << endl;
		m_sError = i18n("No response from POP3 server in 600 secs.");
		if (r_buf)
			r_buf[0] = 0;
		return false;
	}

	// Clear out the buffer
	memset(buf, 0, r_len);
	ReadLine(buf, r_len-1);

	// This is really a funky crash waiting to happen if something isn't
	// null terminated.
	recv_len=strlen(buf);

	/*
	 *   From rfc1939:
	 *
	 *   Responses in the POP3 consist of a status indicator and a keyword
	 *   possibly followed by additional information.  All responses are
	 *   terminated by a CRLF pair.  Responses may be up to 512 characters
	 *   long, including the terminating CRLF.  There are currently two status
	 *   indicators: positive ("+OK") and negative ("-ERR").  Servers MUST
	 *   send the "+OK" and "-ERR" in upper case.
	 */

	if (strncmp(buf, "+OK", 3)==0) {
		if (r_buf && r_len) {
			memcpy(r_buf, (buf[3] == ' ' ? buf+4 : buf+3),
				QMIN(r_len, (buf[3] == ' ' ? recv_len-4 : recv_len-3)));
		}
		if (buf) free(buf);
		return true;
	} else if (strncmp(buf, "-ERR", 4)==0) {
		if (r_buf && r_len) {
			memcpy(r_buf, (buf[4] == ' ' ? buf+5 : buf+4),
				QMIN(r_len, (buf[4] == ' ' ? recv_len-5 : recv_len-4)));
		}
		QString command = QString::fromLatin1(cmd);
		QString serverMsg = QString::fromLatin1(buf).stripWhiteSpace();
		if (command.left(4) == "PASS") {
			command = i18n("PASS <your password>");
		}
		m_sError = i18n("I said:\n   \"%1\"\n\nAnd then the server said:\n   \"%2\"").arg(command).arg(serverMsg);
		if (buf) free(buf);
		return false;
	} else {
		kdDebug() << "Invalid POP3 response received!" << endl;
		if (r_buf && r_len) {
			memcpy(r_buf, buf, QMIN(r_len,recv_len));
		}
		if (!buf || !*buf) m_sError = i18n("The server terminated the connection.");
		else m_sError = i18n("Invalid response from server:\n   \"%1\"").arg(buf);
		if (buf) free(buf);
		return false;
	}
}

bool POP3Protocol::command (const char *cmd, char *recv_buf, unsigned int len)
{
	/*
	 *   From rfc1939:
	 *
	 *   Commands in the POP3 consist of a case-insensitive keyword, possibly
	 *   followed by one or more arguments.  All commands are terminated by a
	 *   CRLF pair.  Keywords and arguments consist of printable ASCII
	 *   characters.  Keywords and arguments are each separated by a single
	 *   SPACE character.  Keywords are three or four characters long. Each
	 *   argument may be up to 40 characters long.
	 */

	// Write the command
	char *cmdrn;
	cmdrn = static_cast<char *>(malloc(strlen(cmd) + 3));
	sprintf(cmdrn, "%s\r\n", cmd);
  
	if (Write(cmdrn, strlen(cmdrn)) != static_cast<ssize_t>(strlen(cmdrn))) {
		m_sError = i18n("Could not send to server.\n");
		free(cmdrn);
		return false;
	}
	free(cmdrn);
	return getResponse(recv_buf, len, cmd);
}

void POP3Protocol::openConnection()
{
	if (!pop3_open()) {
		kdDebug() << "pop3_open failed" << endl;
		closeConnection();
	} else connected();
}

void POP3Protocol::closeConnection()
{
	// If the file pointer exists, we can assume the socket is valid,
	// and to make sure that the server doesn't magically undo any of
	// our deletions and so-on, we should send a QUIT and wait for a
	// response.  We don't care if it's positive or negative.  Also
	// flush out any semblance of a persistant connection, i.e.: the
	// old username and password are now invalid.
	if (!opened)
		return;
	command("QUIT");
	CloseDescriptor();
	m_sOldUser = ""; m_sOldPass = ""; m_sOldServer = "";
	opened = false;
}

bool POP3Protocol::pop3_open()
{
	char buf[512], *greeting_buf;
	if ( (m_iOldPort == GetPort(m_iPort)) && (m_sOldServer == m_sServer) &&
		(m_sOldUser == m_sUser) && (m_sOldPass == m_sPass)) {
		kdDebug() << "Reusing old connection" << endl;
		return true;
	} else {
		closeConnection();
		if( !ConnectToHost(m_sServer.ascii(), m_iPort))
		{
			error( ERR_COULD_NOT_CONNECT, m_sServer);
			return false; // ConnectToHost has already send an error message.
		}
		opened = true;

		greeting_buf=static_cast<char *>(malloc(GREETING_BUF_LEN));
		memset(greeting_buf, 0, GREETING_BUF_LEN);

		// If the server doesn't respond with a greeting
		if (!getResponse(greeting_buf, GREETING_BUF_LEN, "")) {
			m_sError = i18n("Could not login to %1.\n\n").arg(m_sServer) +
		((!greeting_buf || !*greeting_buf) ? i18n("The server terminated the connection immediately.") : i18n("Server does not respond properly:\n%1\n").arg(greeting_buf));
			error( ERR_COULD_NOT_LOGIN, m_sError );
			free(greeting_buf);
			return false;	// we've got major problems, and possibly the
					// wrong port
		}
		QCString greeting(greeting_buf);
		free(greeting_buf);

#ifndef NAPOP
		//
		// Does the server support APOP?
		//
		QString apop_cmd;
		QRegExp re("<[A-Za-z0-9\\.\\-_]+@[A-Za-z0-9\\.\\-_]+>$", false);
		if(greeting.length() > 0)
			greeting.truncate(greeting.length() - 2);
		int apop_pos = greeting.find(re);
		bool apop = (bool)(apop_pos != -1);
#endif

		m_iOldPort = m_iPort;
		m_sOldServer = m_sServer;

                // Try to go into TLS mode
                if ((metaData("tls") == "on" || (canUseTLS() &&
                  metaData("tls") != "off")) && command("STLS"))
                {
                   if (startTLS()) {
                      kdDebug() << "TLS mode has been enabled." << endl;
                   } else {
                      kdDebug() << "TLS mode setup has failed.  Aborting." << endl;
		      error( ERR_COULD_NOT_CONNECT,
                                 i18n("Your POP3 server claims to "
                                      "support TLS but negotiation "
                                      "was unsuccessful.  You can "
                                      "disable TLS in KDE using the "
                                      "crypto settings module."));
                      return false;
                   }
                }
                else if (metaData("tls") == "on")
                {
                  error( ERR_COULD_NOT_CONNECT,
                    i18n("Your POP3 server does not support TLS. Disable\n"
                         "TLS, if you want to connect without encryption."));
                  return false;
                }

		QString usr, pass, one_string="USER ";
#ifndef NAPOP
		QString apop_string = "APOP ";
#endif
		if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
			// Prompt for usernames
			QString head=i18n("Username and password for your POP3 account:");
			if (!openPassDlg(head, usr, pass)) {
				closeConnection();
				return false;
			} else {
#ifndef NAPOP
				apop_string.append(usr);
#endif
				one_string.append(usr);
				m_sOldUser=usr;
				m_sUser=usr; m_sPass=pass;
			}
		} else {
#ifndef NAPOP
	      		apop_string.append(m_sUser);
#endif
			one_string.append(m_sUser);
			m_sOldUser = m_sUser;
		}
		memset(buf, 0, sizeof(buf));
#ifndef NAPOP
		if(apop && m_try_apop) {
			char *c = greeting.data() + apop_pos;
			KMD5 ctx;

			if ( m_sPass.isEmpty())
				m_sOldPass = pass;
			else
				m_sOldPass = m_sPass;

			// Generate digest
			ctx.update((unsigned char *)c, (unsigned)strlen(c));
			ctx.update((unsigned char *)m_sOldPass.local8Bit().data(), (unsigned)m_sOldPass.local8Bit().length());
			ctx.finalize();

			// Genenerate APOP command
			apop_string.append(" ");
			apop_string.append(ctx.hexDigest());

			if(command(apop_string.local8Bit(), buf, sizeof(buf)))
				return true;

			kdDebug() << "Couldn't login via APOP. Falling back to USER/PASS" << endl;
			closeConnection();
			m_try_apop = false;
			return pop3_open();
		}
#endif
		// Let's try SASL stuff first.. it might be more secure
		QString sasl_auth, sasl_buffer="AUTH";

		// We need to check what methods the server supports...
		// This is based on RFC 1734's wisdom
		if (m_try_sasl && command(sasl_buffer.local8Bit())) {
			while (!AtEOF()) {
				memset(buf, 0, sizeof(buf));
				ReadLine(buf, sizeof(buf)-1);

				// HACK: This assumes fread stops at the first \n and not \r
				if (strcmp(buf, ".\r\n")==0) break; // End of data
				// sanders, changed -2 to -1 below
				buf[strlen(buf)-2]='\0';
			}

			sasl_auth=buf;
			sasl_auth.replace(QRegExp("."), "");
			sasl_auth.replace(QRegExp("\\r\\n"), " ");
			KURL url;
			url.setUser(m_sUser);
			url.setPass(m_sPass);
			KSASLContext *m_pSASL = new KSASLContext;
			m_pSASL->setURL(url);
			sasl_buffer = m_pSASL->chooseMethod(sasl_auth);
			sasl_auth=sasl_buffer;
			if (sasl_buffer == QString::null) {
				delete m_pSASL;
			} else {
				// Yich character arrays..
				char *challenge=static_cast<char *>(malloc(2049));
				sasl_buffer.prepend("AUTH ");
				if (!command(sasl_buffer.latin1(), challenge, 2049)) {
					free(challenge);
					delete m_pSASL; m_pSASL=0;
				} else {
					bool ret, b64=true;

					// See the SMTP ioslave
					if (sasl_auth == "PLAIN")
						b64=false;
					ret = command(m_pSASL->generateResponse(challenge, false).latin1());
					free(challenge);
					delete m_pSASL;
					if (ret)
						return true;
				}
			}
		}
		else if (m_try_sasl)
		{
			closeConnection();
			m_try_sasl = false;
			return pop3_open();
		}

		// Fall back to conventional USER/PASS scheme
		if (!command(one_string.local8Bit(), buf, sizeof(buf))) {
			kdDebug() << "Couldn't login. Bad username Sorry" << endl;
			m_sError = i18n("Could not login to %1.\n\n").arg(m_sServer) + m_sError;
			error( ERR_COULD_NOT_LOGIN, m_sError );
			closeConnection();
			return false;
		}

		one_string="PASS ";
		if (m_sPass.isEmpty()) {
			m_sOldPass = pass;
			one_string.append(pass);
		} else {
			m_sOldPass = m_sPass;
			one_string.append(m_sPass);
		}
		if (!command(one_string.local8Bit(), buf, sizeof(buf))) {
			kdDebug() << "Couldn't login. Bad password Sorry." << endl;
			m_sError = i18n("Could not login to %1.\n\n").arg(m_sServer) + m_sError;
			error( ERR_COULD_NOT_LOGIN, m_sError );
			closeConnection();
			return false;
		}
		return true;
	}
}

size_t POP3Protocol::realGetSize(unsigned int msg_num)
{
	char *buf;
	QCString cmd;
	size_t ret=0;

	buf=static_cast<char *>(malloc(MAX_RESPONSE_LEN));
	memset(buf, 0, MAX_RESPONSE_LEN);
	cmd.sprintf("LIST %u", msg_num);
	if (!command(cmd.data(), buf, MAX_RESPONSE_LEN)) {
		free(buf);
		return 0;
	} else {
		cmd=buf;
		cmd.remove(0, cmd.find(" "));
		ret=cmd.toLong();
	}
	free(buf);
	return ret;
}

void POP3Protocol::get( const KURL& url )
{
// List of supported commands
//
// URI                                 Command   Result
// pop3://user:pass@domain/index       LIST      List message sizes
// pop3://user:pass@domain/uidl        UIDL      List message UIDs
// pop3://user:pass@domain/remove/#1   DELE #1   Mark a message for deletion
// pop3://user:pass@domain/download/#1 RETR #1   Get message header and body
// pop3://user:pass@domain/list/#1     LIST #1   Get size of a message
// pop3://user:pass@domain/uid/#1      UIDL #1   Get UID of a message
// pop3://user:pass@domain/commit      QUIT      Delete marked messages
// pop3://user:pass@domain/headers/#1  TOP #1    Get header of message
//
// Notes:
// Sizes are in bytes.
// No support for the STAT command has been implemented.
// commit closes the connection to the server after issuing the QUIT command.

	bool ok=true;
	char buf[MAX_RESPONSE_LEN];
	QByteArray array;

	QString cmd, path = url.path();

	if (path.at(0)=='/') path.remove(0,1);
	if (path.isEmpty()) {
		kdDebug() << "We should be a dir!!" << endl;
		error(ERR_IS_DIRECTORY, url.url());
		m_cmd=CMD_NONE; return;
	}

	if (((path.find('/') == -1) && (path != "index") && (path != "uidl") && (path != "commit")) ) {
		error( ERR_MALFORMED_URL, url.url() );
		m_cmd = CMD_NONE;
		return;
	}

	cmd = path.left(path.find('/'));
	path.remove(0,path.find('/')+1);

	if (!pop3_open()) {
		kdDebug() << "pop3_open failed" << endl;
		closeConnection();
		error( ERR_COULD_NOT_CONNECT, m_sServer);
		return;
	}

	if ((cmd == "index") || (cmd == "uidl")) {
		unsigned long size=0;
		bool result;
		if (cmd == "index")
			result = command("LIST");
		else
			result = command("UIDL");
		if (result) {
/*
LIST
+OK Mailbox scan listing follows
1 2979
2 1348
3 1213
4 1286
5 2363
6 1410
7 2048
8 958
9 91684
10 3770
11 1547
12 649
.
*/
			while (!AtEOF()) {
				memset(buf, 0, sizeof(buf));
				ReadLine(buf, sizeof(buf)-1);

				// HACK: This assumes fread stops at the first \n and not \r
				if (strcmp(buf, ".\r\n")==0) break; // End of data
				// sanders, changed -2 to -1 below
				int bufStrLen = strlen(buf);
				buf[bufStrLen-2]='\0';
				size+=bufStrLen;
				array.setRawData(buf, bufStrLen);
				data( array );
				array.resetRawData(buf, bufStrLen);
				totalSize(size);
			}
		}
		kdDebug() << "Finishing up list" << endl;
		data( QByteArray() );
		speed(0); finished();
	} else if (cmd == "headers") {
		(void)path.toInt(&ok);
		if (!ok) {
			error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
			return; //  We fscking need a number!
		}
		path.prepend("TOP ");
		path.append(" 0");
		if (command(path.ascii())) {	// This should be checked, and a more hackish way of
						// getting at the headers by d/l the whole message
						// and stopping at the first blank line used if the
						// TOP cmd isn't supported
			mimeType("text/plain");
			memset(buf, 0, sizeof(buf));
			while (!AtEOF()) {
				memset(buf, 0, sizeof(buf));
				ReadLine(buf, sizeof(buf)-1);

				// HACK: This assumes fread stops at the first \n and not \r
				if (strcmp(buf, ".\r\n")==0) break; // End of data
				// sanders, changed -2 to -1 below
				buf[strlen(buf)-1]='\0';
				if (strcmp(buf, "..")==0) {
					buf[0] = '.';
					array.setRawData(buf, 1);
					data( array );
					array.resetRawData(buf, 1);
				} else {
					array.setRawData(buf, strlen(buf));
					data( array );
					array.resetRawData(buf, strlen(buf));
				}
			}
			kdDebug() << "Finishing up" << endl;
			data( QByteArray() );
			speed(0);finished();
		}
	} else if (cmd == "remove") {
		(void)path.toInt(&ok);
		if (!ok) {
			error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
			return; //  We fscking need a number!
		}
		path.prepend("DELE ");
		command(path.ascii());
		finished();
		m_cmd = CMD_NONE;
	} else if (cmd == "download") {
		int p_size=0;
		unsigned int msg_len=0;
		(void)path.toInt(&ok);
		QString list_cmd("LIST ");
		if (!ok) {
			error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
			return; //  We fscking need a number!
		}
		list_cmd+= path;
		path.prepend("RETR ");
		memset(buf, 0, sizeof(buf));
		if (command(list_cmd.ascii(), buf, sizeof(buf)-1)) {
			list_cmd=buf;
			// We need a space, otherwise we got an invalid reply
			if (!list_cmd.find(" ")) {
				kdDebug(7105) << "List command needs a space? " << list_cmd << endl;
				closeConnection();
				error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
				return;
			}
			list_cmd.remove(0, list_cmd.find(" ")+1);
			msg_len = list_cmd.toUInt(&ok);
			if (!ok) {
				kdDebug(7105) << "LIST command needs to return a number? :" << list_cmd << ":" << endl;
				closeConnection();
				error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
				return;
			}
		} else {
			closeConnection();
			error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
			return;
		}
		if (command(path.ascii())) {
			mimeType("message/rfc822");
			totalSize(msg_len);
			memset(buf, 0, sizeof(buf));
			while (!AtEOF()) {
				ReadLine(buf, sizeof(buf)-1);

				// HACK: This assumes fread stops at the first \n and not \r
				if (strcmp(buf, ".\r\n")==0) break; // End of data
				// sanders, changed -2 to -1 below
				buf[strlen(buf)-1]='\0';
				if (buf[0] == 46 && buf[1] == 46) { // .. at the start of a line means only .
					array.setRawData(&buf[1], strlen(buf) - 1);
					data( array );
					array.resetRawData(&buf[1], strlen(buf) - 1);
				} else {
					array.setRawData(buf, strlen(buf));
					data( array );
					array.resetRawData(buf, strlen(buf));
				}
				p_size+=strlen(buf);
				processedSize(p_size);
			}
			kdDebug() << "Finishing up" << endl;
			data(QByteArray());
			speed(0); finished();
		} else {
			kdDebug() << "Couldn't login. Bad RETR Sorry" << endl;
			closeConnection();
			error( ERR_INTERNAL, i18n("Couldn't login."));
			return;
		}
	} else if ((cmd == "uid") || (cmd == "list")) {
		QString qbuf;
		(void)path.toInt(&ok);
		if (!ok) return; //  We fscking need a number!
		if (cmd == "uid")
			path.prepend("UIDL ");
		else
			path.prepend("LIST ");
		memset(buf, 0, sizeof(buf));
		if (command(path.ascii(), buf, sizeof(buf)-1)) {
			const int len = strlen(buf);
			mimeType("text/plain");
			totalSize(len);
			array.setRawData(buf, len);
			data( array );
			array.resetRawData(buf, len);
			processedSize(len);
			kdDebug() << buf << endl;
			kdDebug() << "Finishing up uid" << endl;
			data(QByteArray());
			speed(0); finished();
		} else {
			closeConnection();
			error( ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
			return;
		}
	} else if (cmd == "commit") {
		kdDebug() << "Issued QUIT" << endl;
		closeConnection();
		finished();
		m_cmd = CMD_NONE;
		return;
	}
}

void POP3Protocol::listDir (const KURL &url)
{
	bool isINT; int num_messages=0;
	char buf[MAX_RESPONSE_LEN];
	QCString q_buf;

	// Try and open a connection
	if (!pop3_open()) {
		kdDebug() << "pop3_open failed" << endl;
		error( ERR_COULD_NOT_CONNECT, m_sServer);
		closeConnection();
		return;
	}
	// Check how many messages we have. STAT is by law required to
	// at least return +OK num_messages total_size
	memset(buf, 0, MAX_RESPONSE_LEN);
	if (!command("STAT", buf, MAX_RESPONSE_LEN)) {
		error(ERR_INTERNAL, "??");
		return;
	}
	kdDebug() << "The stat buf is :" << buf << ":" << endl;
	q_buf=buf;
	if (q_buf.find(" ")==-1) {
		error(ERR_INTERNAL, "Invalid POP3 response, we should have at least one space!");
		closeConnection();
		return;
	}
	q_buf.remove(q_buf.find(" "), q_buf.length());

	num_messages=q_buf.toUInt(&isINT);
	if (!isINT) {
		error(ERR_INTERNAL, "Invalid POP3 STAT response!");
		closeConnection();
		return;
	}
	UDSEntry entry;
	UDSAtom atom;
	QString fname;
	for (int i=0; i < num_messages; i++) {
		fname="Message %1";

		atom.m_uds = UDS_NAME;
		atom.m_long = 0;
		atom.m_str = fname.arg(i+1);
		entry.append(atom);

		atom.m_uds = UDS_MIME_TYPE;
		atom.m_long = 0;
		atom.m_str = "text/plain";
		entry.append(atom);
		kdDebug() << "Mimetype is " << atom.m_str.ascii() << endl;

		atom.m_uds = UDS_URL;
		KURL uds_url;
		if (m_bIsSSL)
			uds_url.setProtocol("pop3s");
		else
			uds_url.setProtocol("pop3");
		uds_url.setUser(m_sUser);
		uds_url.setPass(m_sPass);
		uds_url.setHost(m_sServer);
		uds_url.setPath(QString::fromLatin1("/download/%1").arg(i+1));
		atom.m_str = uds_url.url();
		atom.m_long = 0;
		entry.append(atom);

		atom.m_uds = UDS_FILE_TYPE;
		atom.m_str = "";
		atom.m_long = S_IFREG;
		entry.append(atom);

		atom.m_uds = UDS_SIZE;
		atom.m_str = "";
		atom.m_long = realGetSize(i+1);
		entry.append(atom);

		listEntry(entry, false);
		entry.clear();
	}
	listEntry( entry, true ); // ready

	finished();
}

void POP3Protocol::stat( const KURL & url )
{
	QString _path = url.path();

	if (_path.at(0) == '/')
		_path.remove(0,1);

	UDSEntry entry;
	UDSAtom atom;

	atom.m_uds = UDS_NAME;
	atom.m_str = _path;
	entry.append( atom );

	atom.m_uds = UDS_FILE_TYPE;
	atom.m_str = "";
	atom.m_long = S_IFREG;
	entry.append(atom);

	atom.m_uds = UDS_MIME_TYPE;
	atom.m_str = "message/rfc822";
	entry.append( atom );

	// TODO: maybe get the size of the message?
	statEntry( entry );

	finished();
}

void POP3Protocol::del( const KURL& url, bool /*isfile*/ )
{
	QString invalidURI=QString::null;
	bool isInt;

	if ( !pop3_open() ) {
		kdDebug() << "pop3_open failed" << endl;
		error( ERR_COULD_NOT_CONNECT, m_sServer );
		closeConnection();
		return;
	}

	QString _path = url.path();
	if (_path.at(0) == '/')
		_path.remove(0,1);
	_path.toUInt(&isInt);
	if (!isInt) {
		invalidURI=_path;
	} else {
		_path.prepend("DELE ");
		if (!command(_path.ascii())) {
			invalidURI=_path;
		}
	}

	kdDebug() << "POP3Protocol::del " << _path << endl;
	finished();
}
