/*
 * Copyright (c) 2000 Alex Zepeda <jazepeda@pacbell.net>
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
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <qbuffer.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qcstring.h>
#include <qglobal.h>

#include <kprotocolmanager.h>
#include <kemailsettings.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <kio/sasl/saslcontext.h>
#include <klocale.h>
#include <iostream.h>

#include "smtp.h"

using namespace KIO;

extern "C" { int kdemain (int argc, char **argv); }

void	GetAddresses(const QString &str, const QString &delim, QStringList &list);
int	GetVal(char *buf);

int kdemain (int argc, char **argv)
{
	KInstance instance ("kio_smtp");

	if (argc != 4) {
		fprintf(stderr, "Usage: kio_smtp protocol domain-socket1 domain-socket2\n");
		exit(-1);
	}

	// We might as well allocate it on the heap.  Since there's a heap o room there..
	SMTPProtocol *slave;
	slave = new SMTPProtocol(argv[2], argv[3]);
	slave->dispatchLoop();

	delete slave;
	return 0;
}


SMTPProtocol::SMTPProtocol(const QCString &pool, const QCString &app)
	: TCPSlaveBase(25, "smtp", pool, app)
{
	kdDebug() << "SMTPProtocol::SMTPProtocol" << endl;
	opened=false;
	haveTLS=false;
	m_iSock=0;
	m_iOldPort = 0;
	m_sOldServer=QString::null;
	m_tTimeout.tv_sec=10;
	m_tTimeout.tv_usec=0;

	// Auth stuff
	m_pSASL=0;
	m_sAuthConfig=QString::null;
}


SMTPProtocol::~SMTPProtocol()
{
	kdDebug() << "SMTPProtocol::~SMTPProtocol" << endl;
	smtp_close();
}

void SMTPProtocol::HandleSMTPWriteError(const KURL&url)
{
	if (!command("RSET")) // Attempt to save face
		error(ERR_SERVICE_NOT_AVAILABLE, url.path());
	else
		error(ERR_COULD_NOT_WRITE, url.path());
}

void SMTPProtocol::put( const KURL& url, int /*permissions*/, bool /*overwrite*/, bool /*resume*/)
{
/*
  smtp://smtphost:port/send?to=user@host.com&subject=blah
*/
	QString query = url.query();
	QString subject="missing subject";
	QStringList recip, cc;

	GetAddresses(query, "to=", recip);
	GetAddresses(query, "cc=", cc);

	// find the subject
	int curpos=0;
	if ( (curpos = query.find("subject=")) != -1) {
		curpos+=8;
                if (query.find("&", curpos) != -1)
                        subject=query.mid(curpos, query.find("&", curpos)-curpos);
                else
                        subject=query.mid(curpos, query.length());
	}

	if (!smtp_open(url))
	        error(ERR_SERVICE_NOT_AVAILABLE, url.path());

	KEMailSettings *mset = new KEMailSettings;
	if (mset->defaultProfileName() != QString::null) {
		mset->setProfile(mset->defaultProfileName());
	}
	QString from("MAIL FROM: ");
	if (mset->getSetting(KEMailSettings::EmailAddress) != QString::null)
		from+=mset->getSetting(KEMailSettings::EmailAddress);
	else
		from+="someuser@is.using.a.pre.release.kde.ioslave.compliments.of.kde.org";

	if (! command(from.latin1())) {
		HandleSMTPWriteError(url);
		return;
	}

	QString formatted_recip="RCPT TO: %1";
	for ( QStringList::Iterator it = recip.begin(); it != recip.end(); ++it ) {
		if (!command(formatted_recip.arg(*it).latin1()))
			HandleSMTPWriteError(url);
	}
	for ( QStringList::Iterator it = cc.begin(); it != cc.end(); ++it ) {
		if (!command(formatted_recip.arg(*it).latin1()))
			HandleSMTPWriteError(url);
	}

	if (!command("DATA")) {
		HandleSMTPWriteError(url);
	}

	formatted_recip="Subject: %1\r\n";
	subject=formatted_recip.arg(subject);
	Write(subject.latin1(), subject.length());

	formatted_recip="To: %1\r\n";
	for ( QStringList::Iterator it = recip.begin(); it != recip.end(); ++it ) {
		subject=formatted_recip.arg(*it);
		Write(subject.latin1(), subject.length());
	}

	formatted_recip="CC: %1\r\n";
	for ( QStringList::Iterator it = cc.begin(); it != cc.end(); ++it ) {
		subject=formatted_recip.arg(*it);
		Write(subject.latin1(), subject.length());
	}

	if (mset->getSetting(KEMailSettings::RealName) != QString::null) {
		if (mset->getSetting(KEMailSettings::EmailAddress) != QString::null) {
			from="From: ";
			from+=mset->getSetting(KEMailSettings::RealName);
			from+=" <";
			from+=mset->getSetting(KEMailSettings::EmailAddress);
			from+=">\r\n";
			Write(from.latin1(), from.length());
		}
	}
	delete mset;

	int result;
	// Loop until we got 0 (end of data)
	QByteArray buffer;
	do {
		dataReq(); // Request for data
		buffer.resize(0);
		result = readData( buffer );
		if (result > 0) {
			Write( buffer.data(), buffer.size());
		} else if (result < 0) {
			error(ERR_COULD_NOT_WRITE, url.path());
		}
	}
	while ( result > 0 );
	Write("\r\n.\r\n", 5);
	command("RSET");
	finished();
}

void SMTPProtocol::setHost( const QString& host, int port, const QString &user, const QString &pass)
{
	m_sServer = host;
	m_iPort = port;
	m_sUser = user;
	m_sPass = pass;
}

int SMTPProtocol::getResponse(char *r_buf, unsigned int r_len)
{
	char *buf=0;
	unsigned int recv_len=0, len;
	fd_set FDs;

	// Give the buffer the appropiate size
	// a buffer of less than 5 bytes will *not* work
	if (r_len) {
		buf=static_cast<char *>(malloc(r_len));
		len=r_len;
	} else {
		buf=static_cast<char *>(malloc(512));
		len=512;
	}

	// And keep waiting if it timed out
	unsigned int wait_time=60; // Wait 60sec. max.
	do {
		// Wait for something to come from the server
		FD_ZERO(&FDs);
		FD_SET(m_iSock, &FDs);
		// Yes, it's true, Linux sucks.
		wait_time--;
		m_tTimeout.tv_sec=1;
		m_tTimeout.tv_usec=0;
	}
	while (wait_time && (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0));

	if (wait_time == 0) {
		kdDebug () << "kio_smtp: No response from SMTP server in 60 secs." << endl;
		return false;
	}

	// Clear out the buffer
	memset(buf, 0, len);
	// And grab the data
	ReadLine(buf, len-1);

	// This is really a funky crash waiting to happen if something isn't
	// null terminated.
	recv_len=strlen(buf);

	if (recv_len < 4)
		error(ERR_UNKNOWN, "Line too short");
	char *origbuf=buf;
	if (buf[3] == '-') { // Multiline response
		while ( (buf[3] == '-') && (len-recv_len > 3) ) { // Three is quite arbitrary
			buf+=recv_len;
			len-=(recv_len+1);
			recv_len=ReadLine(buf, len-1);
			if (recv_len == 0)
				buf[0]=buf[1]=buf[2]=buf[3]=' ';
		}
		buf=origbuf;
		memcpy(r_buf, buf, strlen(buf));
		r_buf[r_len-1]=0;
		return GetVal(buf);
	} else {
		// Really crude, whee
		//return GetVal(buf);
		if (r_len) {
			r_len=recv_len-4;
			memcpy(r_buf, buf+4, r_len);
		}
		return GetVal(buf);
	}

}


bool SMTPProtocol::command(const char *cmd, char *recv_buf, unsigned int len) {
	// Write the command
	if (Write(cmd, strlen(cmd)) != static_cast<ssize_t>(strlen(cmd))) {
		m_sError = i18n("Could not send to server.\n");
		return false;
	}

	if (Write("\r\n", 2) != 2) {
		m_sError = i18n("Could not send to server.\n");
		return false;
	}
	return ( getResponse(recv_buf, len) < 400 );
}


bool SMTPProtocol::smtp_open(const KURL &url)
{
	if ( (m_iOldPort == GetPort(m_iPort)) && (m_sOldServer == m_sServer) && (m_sOldUser == m_sUser) ) {
		return true;
	} else {
		smtp_close();
		if( !ConnectToHost(m_sServer.latin1(), m_iPort))
			return false; // ConnectToHost has already send an error message.
		opened=true;
	}

	if (getResponse() >= 400) {
		return false;
	}

	QBuffer ehlobuf(QByteArray(5120));
	memset(ehlobuf.buffer().data(), 0, 5120);
	if (!command("EHLO kio_smtp", ehlobuf.buffer().data(), 5119)) { // Yes, I *know* that this is not
							// the way it should be done, but
							// for now there's no real need
							// to complicate things by
							// determining our hostname
		if (!command("HELO kio_smtp")) { // Let's just check to see if it speaks plain ol' SMTP
			smtp_close();
			return false;
		}
	}
	// We should parse the ESMTP extensions here... pipelining would be really cool
	char ehlo_line[2048];
	if (ehlobuf.open(IO_ReadWrite)) {
		while ( ehlobuf.readLine(ehlo_line, 2048) > 0)
			ParseFeatures(const_cast<const char *>(ehlo_line));
	}

	if (haveTLS) { // we should also check kemailsettings as well..
		if (command("STARTTLS")) {
			//m_pSSL=new KSSL(true);
			//m_pSSL->connect(m_iSock);
		} else
			haveTLS=false;
	}

	// Now we try and login
	if (!m_sUser.isNull()) {
		if (!m_sPass.isNull()) {
			Authenticate(url);
		}
	}

	m_iOldPort = m_iPort;
	m_sOldServer = m_sServer;
	m_sOldUser = m_sUser;
	m_sOldPass = m_sPass;

	return true;
}

bool SMTPProtocol::Authenticate(const KURL &url)
{
	bool ret;
	QString auth_method;

	// Generate a new context.. now this isn't the most efficient way of doing things, but for now this suffices
	if (m_pSASL) delete m_pSASL;
	m_pSASL = new KSASLContext;
	m_pSASL->setURL(url);

	// Choose available method from what the server has given us in  its greeting
	auth_method = m_pSASL->chooseMethod(m_sAuthConfig);

	// If none are available, set it up so we can start over again
	if (auth_method == QString::null) {
		delete m_pSASL; m_pSASL=0;
		return false;
	} else {
		char *challenge=static_cast<char *>(malloc(2049));
 		// I've probably made some troll seek shelter somewhere else.. yay gov'nr and his matrix.. yes that one.. that looked like a  bird.. no not harvey milk
		if (!command(QString(QString("AUTH ")+auth_method).latin1(), challenge, 2049)) {
			free(challenge);
			delete m_pSASL; m_pSASL=0;
			return false;
		}

		// Now for the stupid part
		// PLAIN auth has embedded null characters.  Ew.
		// so it's encoded in HEX as part of its spec IIRC.
		// so.. let that auth module do the b64 encoding itself..
		// it's easier than generating a byte array simply to pass 
		// around null characters.  Stupid stupid stupid.
		if (auth_method == "PLAIN") {
			ret = command(m_pSASL->generateResponse(challenge, false).latin1());
		} else {
			// Since SMTP does indeed needs its auth responses base64 encoded... for some reason
			ret = command(m_pSASL->generateResponse(challenge, true).latin1());
		}
		free(challenge);
		return ret;
	}
	return false;
}


void SMTPProtocol::ParseFeatures(const char *_buf)
{
	QString buf(_buf);


	// We want it to be between 250 and 259 inclusive, and it needs to be "nnn-blah" or "nnn blah"
	// So sez the SMTP spec..
	if ( (buf.left(2) != "25") || (!isdigit((buf.latin1())[2])) || (!(buf.at(3) == '-') && !(buf.at(3) == ' ')) )
		return; // We got an invalid line..
	buf=buf.mid(4, buf.length()); // Clop off the beginning, no need for it really

	if (buf.left(4) == "AUTH") { // Look for auth stuff
		// keep this for later use ^^^^
		m_sAuthConfig=buf.mid(5, buf.length());
	} else if (buf.left(8) == "STARTTLS") {
		haveTLS=true;
	}

}

void SMTPProtocol::smtp_close()
{
	if (!opened) // We're already closed
		return;
	command("QUIT");
	CloseDescriptor();
	m_sOldServer = "";
	opened = false;
}

void SMTPProtocol::stat( const KURL & url )
{
	QString path = url.path();
        error( KIO::ERR_DOES_NOT_EXIST, url.path() );
}

int GetVal(char *buf)
{
		int val;
		val=(100*(static_cast<int>(buf[0])-48));
		val+=(10*(static_cast<int>(buf[1])-48));
		val+=static_cast<int>(buf[2])-48;
		return val;
}

void GetAddresses(const QString &str, const QString &delim, QStringList &list)
{
	int curpos=0;
	while ( (curpos = str.find(delim, curpos) ) != -1) {
		if ( (str.at(curpos-1) == "?") || (str.at(curpos-1) == "&") ) {
			curpos+=delim.length();
			if (str.find("&", curpos) != -1)
				list+=str.mid(curpos, str.find("&", curpos)-curpos);
			else
				list+=str.mid(curpos, str.length());
		} else
			curpos+=delim.length();
	}
}
