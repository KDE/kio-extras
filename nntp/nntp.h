#ifndef _NNTP_H
#define _NNTP_H "$Id$"

#include <qstring.h>
#include <qcstring.h>
#include <kurl.h>
#include <stdio.h>
#include <sys/types.h>
#include <kio/global.h>
#include <kio/slavebase.h>



class NNTPProtocol : public KIO::SlaveBase
{

public:
	NNTPProtocol( KIO::Connection *_conn = 0);
	virtual ~NNTPProtocol();

	// Uses this function to get information in the url
	virtual void setHost(const QString& host, int ip, const QString& user, const QString& pass);
	virtual void special(const QByteArray &);


protected:

	QString currentHost, currentIP, currentUser, currentPass;
	QString buildFullLibURL(const QString &pathArg);
	QString host, pass, user;
};

#endif

