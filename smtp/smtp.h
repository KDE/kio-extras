#ifndef _SMTP_H
#define _SMTP_H

#include <qstring.h>
#include <kio/tcpslavebase.h>

class SMTPProtocol
	: public KIO::TCPSlaveBase {
public:
	SMTPProtocol( const QCString &pool, const QCString &app, bool SSL );
	virtual ~SMTPProtocol();

	virtual void setHost(const QString& host, int port, const QString& user, const QString& pass);

	virtual void put( const KURL& url, int permissions, bool overwrite, bool resume);
	virtual void stat(const KURL&url);

protected:

	bool smtp_open();
	void smtp_close();
	bool command(const char *buf, char *r_buf = NULL, unsigned int r_len = 0);
	int getResponse(char *r_buf = NULL, unsigned int r_len = 0);
	void HandleSMTPWriteError(const KURL&url);

	struct timeval m_tTimeout;
	bool opened;
	QString m_sServer, m_sOldServer;
	unsigned short m_iOldPort;
	QString m_sError;
};

#endif
