#ifndef __kio_smb_h__
#define __kio_smb_h__

#include <qstring.h>
#include <kio/global.h>
#include <kio/slavebase.h>

#include <smb++.h>
class MyCallback;
class SmbProtocol : public KIO::SlaveBase
{
friend class MyCallback;
public:
	SmbProtocol( KIO::Connection *_conn = 0);
	virtual ~SmbProtocol();

	// Uses this function to get information in the url
	virtual void openConnection(const QString& host, int ip, const QString& user, const QString& pass);
	virtual void closeConnection();

	virtual void get( const QString& path, const QString& query, bool reload );
	virtual void put( const QString& path, int _mode,
				bool _overwrite, bool _resume );
	virtual void rename( const QString &src, const QString &dest,
						bool overwrite );

	virtual void stat( const QString& path );
	virtual void listDir( const QString& path );
	virtual void mkdir( const QString& path, int permissions );
	virtual void del( const QString& path, bool isfile);


protected:

	void createUDSEntry( const QString & filename, const QString & path, KIO::UDSEntry & entry );
// NB: That's because the smb servers can return statistics at the same time
//   => no need to re-stat after an opendir, SMBdirent inherits struct stat :-)
	void createUDSEntry( const SMBdirent* dent, const QString & path, KIO::UDSEntry & entry  );
	
	MyCallback *cb;
	SMB smb;
	int m_fPut;
	QString currentHost, currentIP, currentUser, currentPass;
	QString buildFullLibURL(const QString &pathArg);
};

#endif
