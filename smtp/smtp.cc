// $Id$

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

#include <qstring.h>
#include <qstringlist.h>
#include <qcstring.h>

#include <kprotocolmanager.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <klocale.h>
#include <iostream.h>

#include "smtp.h"

using namespace KIO;

extern "C" { int kdemain (int argc, char **argv); }
void GetAddresses(const QString &str, const QString &delim, QStringList &list);
int GetVal(char *buf);

int kdemain( int argc, char **argv )
{
	KInstance instance( "kio_smtp" );

	if (argc != 4) {
		fprintf(stderr, "Usage: kio_smtp protocol domain-socket1 domain-socket2\n");
		exit(-1);
	}

	SMTPProtocol *slave;

	// Are we looking to use SSL?
	if (strcasecmp(argv[1], "smtps") == 0)
		slave = new SMTPProtocol(argv[2], argv[3], true);
	else
		slave = new SMTPProtocol(argv[2], argv[3], false);
	slave->dispatchLoop();
	delete slave;
	return 0;
}


SMTPProtocol::SMTPProtocol(const QCString &pool, const QCString &app, bool SSL) : TCPSlaveBase((SSL ? 465 : 25), (SSL ? "smtps" : "smtp"), pool, app)
{
	kdDebug() << "SMTPProtocol::SMTPProtocol" << endl;
	m_bIsSSL=SSL;
	opened=false;
	m_iSock=0;
	m_iOldPort = 0;
	m_sOldServer="";
	m_tTimeout.tv_sec=10;
	m_tTimeout.tv_usec=0;
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
	QString query = url.query().mid(1, url.query().length());
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

	if (!smtp_open())
	        error(ERR_SERVICE_NOT_AVAILABLE, url.path());

	if (! command("MAIL FROM: someuser@is.using.a.pre.release.kde.ioslave.compliments.of.kde.org")) {
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

	formatted_recip="To: %1";
	for ( QStringList::Iterator it = recip.begin(); it != recip.end(); ++it ) {
		subject=formatted_recip.arg(*it);
		Write(subject.latin1(), subject.length());
	}

	formatted_recip="CC: %1";
	for ( QStringList::Iterator it = cc.begin(); it != cc.end(); ++it ) {
		subject=formatted_recip.arg(*it);
		Write(subject.latin1(), subject.length());
	}

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

void SMTPProtocol::setHost( const QString& host, int port, const QString& /*user*/, const QString& /*pass*/)
{
	m_sServer = host;
	m_iPort = port;
}

int SMTPProtocol::getResponse(char *r_buf, unsigned int r_len)
{
	char *buf=0;
	unsigned int recv_len=0;
	fd_set FDs;

	// Give the buffer the appropiate size
	// a buffer of less than 5 bytes will *not* work
	if (r_len)
		buf=(char *)malloc(r_len);
	else {
		buf=(char *)malloc(512);
		r_len=512;
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
	memset(buf, 0, r_len);
	// And grab the data
	ReadLine(buf, r_len-1);

	// This is really a funky crash waiting to happen if something isn't
	// null terminated.
	recv_len=strlen(buf);

	char *origbuf=buf;
	if (buf[3] == '-') { // Multiline response
		while ( (buf[3] == '-') && (r_len-recv_len > 3) ) { // Three is quite arbitrary
			buf+=recv_len;
			r_len-=(recv_len+1);
			recv_len=ReadLine(buf, r_len-1);
		}
		buf=origbuf;
		fprintf(stderr,"kio_smtp: data diz: '%s'\n", buf);
		return GetVal(buf);
	} else {
		// Really crude, whee
		return GetVal(buf);
		if (r_len != 512) {
			r_len=recv_len-4;
			memcpy(r_buf, buf, r_len);
		}
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


bool SMTPProtocol::smtp_open()
{
	if ( (m_iOldPort == GetPort(m_iPort)) && (m_sOldServer == m_sServer) ) {
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

	char *ehlobuf=(char *)malloc(2048);
	memset(ehlobuf, 0, 2048);
	if (!command("EHLO kio_smtp", ehlobuf, 2048)) { // Yes, I *know* that this is not
							// the way it should be done, but
							// for now there's no real need
							// to complicate things by
							// determining our hostname
		free(ehlobuf);	// We should parse the ESMTP extensions here... pipelining would be really cool
		if (!command("HELO kio_smtp")) { // Let's just check to see if it speaks plain ol' SMTP
			smtp_close();
			return false;
		}
	}
	free(ehlobuf);
	

	m_iOldPort = m_iPort;
	m_sOldServer = m_sServer;

	return true;
}


void SMTPProtocol::smtp_close()
{
	if (!opened)
		return;
	command("QUIT");
	CloseDescriptor();
	m_sOldServer = "";
	opened = false;
}

void SMTPProtocol::stat( const KURL & url )
{
        error( KIO::ERR_DOES_NOT_EXIST, url.path() );
}

int GetVal(char *buf)
{
		int val;
		val=100*(((int)buf[0])-48);
		val+=(10*((((int)buf[1])-48)));
		val+=((((int)buf[2])-48));
		return val;
}

void GetAddresses(const QString &str, const QString &delim, QStringList &list)
{
	int curpos=0;
	while ( (curpos = str.find(delim, curpos) ) != -1) {
		curpos+=delim.length();
		if (str.find("&", curpos) != -1)
			list+=str.mid(curpos, str.find("&", curpos)-curpos);
		else
			list+=str.mid(curpos, str.length());
	}
}
