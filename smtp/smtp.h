#ifndef _SMTP_H
#define _SMTP_H

#include <qstring.h>
#include <kio/tcpslavebase.h>

#include <kconfig.h>

class SMTPProtocol
	: public KIO::TCPSlaveBase {
public:
	SMTPProtocol(const QCString &pool, const QCString &app);
	virtual ~SMTPProtocol();

	virtual void setHost(const QString &host, int port, const QString &user, const QString &pass);

	virtual void put( const KURL& url, int permissions, bool overwrite, bool resume);
	virtual void stat(const KURL&url);

protected:
	enum AUTH {
		AUTH_None	=0x000,
		AUTH_Plain	=0x002,
		AUTH_DIGEST	=0x004,
		AUTH_CRAM	=0x006
	};

	bool smtp_open();
	void smtp_close();
	bool command(const char *buf, char *r_buf = NULL, unsigned int r_len = 0);
	int getResponse(char *r_buf = NULL, unsigned int r_len = 0);
	void HandleSMTPWriteError(const KURL&url);
	void ParseFeatures (const char *buf);

	unsigned short m_iOldPort;
	bool opened, haveTLS;
	enum AUTH m_eAuthSupport;
	struct timeval m_tTimeout;
	QString m_sServer, m_sOldServer;
	QString m_sUser, m_sOldUser;
	QString m_sPass, m_sOldPass;
	QString m_sError;
};

#endif
