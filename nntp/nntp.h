#ifndef _NNTP_H
#define _NNTP_H "$Id$"
#ifndef __kio_smb_h__
#define __kio_smb_h__

#include <qstring.h>
#include <kio/global.h>
#include <kio/slavebase.h>



class NNTPProtocol : public KIO::SlaveBase
{

public:
	NNTPProtocol( KIO::Connection *_conn = 0);
	virtual ~NNTPProtocol();

	// Uses this function to get information in the url
	virtual void openConnection(const QString& host, int ip, const QString& user, const QString& pass);
	virtual void closeConnection();



protected:

	void createUDSEntry( const QString & filename, const QString & path, KIO::UDSEntry & entry)
	void createUDSEntry( const SMBdirent* dent, const QString & path, KIO::UDSEntry & entry) 	

	NNTP *m_pNNTP;
	
	QString currentHost, currentIP, currentUser, currentPass;
	QString buildFullLibURL(const QString &pathArg);
};

#endif

