#ifndef __kio_smb_h__
#define __kio_smb_h__

#include <qstring.h>
#include <kio/global.h>
#include <kio/slavebase.h>

#include "libsmb++/src/smb++.h"
class MyCallback;
class SmbProtocol : public KIO::SlaveBase
{
friend class MyCallback;
public:
	SmbProtocol( const QCString &pool, const QCString &app);
	virtual ~SmbProtocol();

	// Uses this function to get information in the url
	virtual void setHost(const QString& host, int ip, const QString& user, const QString& pass);

	virtual void get( const KURL& url );
	virtual void put( const KURL& url, int _mode,
				bool _overwrite, bool _resume );
	virtual void rename( const KURL& src, const KURL& dest,
						bool overwrite );

	virtual void stat( const KURL& url );
	virtual void listDir( const KURL& url );
	virtual void mkdir( const KURL& url, int permissions );
	virtual void del( const KURL& url, bool isfile);


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

	// For the bindings
	class Binding {
	public:
		QString server;
		QString share;
		QString login;
		QString password;
		Binding(const QString &e, const QString& h, const QString& l, const QString& p)
		: server(e), share(h), login(l), password(p) {}
	};
	QList<Binding> bindings;
	bool storeBindings; // policy for new bindings
	void loadBindings(bool force = false);
	void saveBindings(); // Will store on the disk if required
	// Add without repetition a new item in the list
	void addBinding(const QString &e, const QString& h, const QString& l, const QString& p);

	// The callback set those values, and on success the new binding is created
	bool callbackUsed;
	QString bserver, bshare, blogin, bpassword;

};

#endif
