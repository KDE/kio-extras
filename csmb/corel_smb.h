#ifndef __kio_smb_h__
#define __kio_smb_h__

#include <qstring.h>
#include <kio/global.h>
#include <kio/slavebase.h>


class CSmbProtocol : public KIO::SlaveBase
{

public:
	CSmbProtocol(const QCString &_pool, const QCString &_app);
	virtual ~CSmbProtocol();
	virtual void setHost(const QString& _host, int _ip, const QString& _user, const QString& _pass);
	virtual void get(const KURL& _path);
	virtual void put(const KURL& _path, int _mode,bool _overwrite, bool _resume);
	virtual void rename(const KURL& _src, const KURL& _dest,bool _overwrite);
	virtual void stat(const KURL& _path);
	virtual void listDir(const KURL& _path);
	virtual void mkdir(const KURL& _path, int _permissions);
	virtual void del(const KURL& _path, bool _isfile);

protected:

	void createUDSEntry(const QString& _filename, const QString& _path, KIO::UDSEntry& _entry);
	void createUDSEntry(const QString& _filename, KIO::UDSEntry& _entry);
	void createUDSEntry(bool _isDir, const QString& _filename, const QString& _path, KIO::UDSEntry& _entry  );
 	QString buildFullLibURL(const KURL& _pathArg);
  QString currentHost, currentIP, currentUser, currentPass;
 	int m_fPut;
  int nCredentialsIndex;
};

#endif
