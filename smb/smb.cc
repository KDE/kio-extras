#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <iostream.h>

#include <qvaluelist.h>

#include <kshred.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kprotocolmanager.h>
#include <kmimemagic.h>
#include <kinstance.h>

#include <kconfig.h>
#include <qlist.h>

#include "smb.h"

using namespace KIO;

class MyCallback : public SmbAnswerCallback
{
protected:
	SmbProtocol *proto;
	// we keep this info so that no password
	// is asked to user when accessing subdirs
	char *user, *pass, *service;
	QString message;
	bool havePass;
	bool haveServicePass;
public:
	MyCallback(SmbProtocol *p) : proto(p),
		user(0), pass(0), service(0),
        	havePass(false), haveServicePass(false) {}
	~MyCallback() {
		if (user) {delete user; user = 0;}
		if (pass) {delete pass; pass = 0;}
		if (service) {delete service; service = 0;}
	}
	
	char *getAnswer(int type, const char *optmessage) {
		proto->callbackUsed = true;
		bool res = true;
		QString myUser, myPass;
		switch (type) {
			case ANSWER_USER_NAME:
				// look if we have it already
				proto->loadBindings(); // reload each time, to keep in sync
				for (SmbProtocol::Binding* it = proto->bindings.first(); (it); it = proto->bindings.next()) {
					if (it->server.local8Bit().upper()==QCString(optmessage).upper()
					&& !it->login.isEmpty()) {
						if (user) delete user;
						user = new char[it->login.local8Bit().length()+1];
						strcpy(user,it->login.local8Bit().data());
						if (pass) delete pass;
						pass = new char[it->password.local8Bit().length()+1];
						strcpy(pass,it->password.local8Bit().data());
						kdDebug(7106) << "CallBack: use binding: user=" << user << ", pass=" << pass << endl;
						havePass=true;
						return user;
					}
				}
				message = i18n("Login for host %1").arg(optmessage);
				myUser = user?user:"";
				myPass = "";
				res = proto->openPassDlg(message, myUser, myPass, optmessage);
				kdDebug(7106) << "CallBack: res=" << (res?"true":"false") << endl;
				if (!res) {
					if (user) {delete user; user = 0;}
					if (pass) {delete pass; pass = 0;}
					havePass=false;
				} else {
					if (user) delete user;
					user=new char[myUser.local8Bit().length()+1];
					strcpy(user,myUser.local8Bit().data());
					if (pass) delete pass;
					pass=new char[myPass.local8Bit().length()+1];
					strcpy(pass,myPass.local8Bit().data());
					kdDebug(7106) << "CallBack: user=" << user << ", pass=" << pass << endl;
					havePass=true;
					// Store the new binding info in case of success
					proto->bserver = optmessage;
					proto->bshare = "";
					proto->blogin = user;
					proto->bpassword = pass;
				}
				return user;
			
			case ANSWER_USER_PASSWORD:
				if (havePass) return pass;
				message = i18n("Password for user %1").arg(optmessage);
				myUser = optmessage;
				myPass = "";
				res = proto->openPassDlg(message, myUser, myPass, proto->currentHost);
				kdDebug(7106) << "CallBack: res=" << (res?"true":"false") << endl;
				if (!res) {
					if (user) {delete user; user = 0;}
					if (pass) {delete pass; pass = 0;}
					havePass=false;
				} else {
					if (user) delete user;
					user=new char[myUser.local8Bit().length()+1];
					strcpy(user,myUser.local8Bit().data());
					if (pass) delete pass;
					pass=new char[myPass.local8Bit().length()+1];
					strcpy(pass,myPass.local8Bit().data());
					kdDebug(7106) << "CallBack: user=" << user << ", pass=" << pass << endl;
					havePass=true;
					// Store the new binding info in case of success
					proto->bserver = proto->currentHost;
					proto->bshare = "";
					proto->blogin = user;
					proto->bpassword = pass;
				}
				return pass;
			
			case ANSWER_SERVICE_PASSWORD:
/*				if (haveServicePass && !strcmp(service, optmessage))
					return pass; // we have it already*/
				// look if we have it in the bindings
				proto->loadBindings(); // reload each time, to keep in sync
				for (SmbProtocol::Binding* it = proto->bindings.first(); (it); it = proto->bindings.next()) {
					if (it->server.local8Bit().upper()==proto->currentHost.local8Bit().upper()
						&& it->share.local8Bit().upper()==QCString(optmessage).upper()) {
						if (service) delete service;
						service = new char[strlen(optmessage)+1];
						strcpy(service,optmessage);
						if (pass) delete pass;
						pass = new char[it->password.local8Bit().length()+1];
						strcpy(pass,it->password.local8Bit().data());
						kdDebug(7106) << "CallBack: use binding: service=" << service << ", pass=" << pass << endl;
						haveServicePass=true;
						return pass;
					}
				}
				message = i18n("Password for service %1 (user ignored)").arg(optmessage);
				myUser = user?user:"";
				myPass = "";
				res = proto->openPassDlg(message, myUser, myPass, proto->currentHost);
				kdDebug(7106) << "CallBack: res=" << (res?"true":"false") << endl;
				kdDebug(7106) << "CallBack: user=" << user << ", pass=" << pass << endl;
				if (!res) {
					if (service) {delete service; service = 0;}
					if (pass) {delete pass; pass = 0;}
					haveServicePass=false;
				} else {
					if (service) delete service;
					service=new char[strlen(optmessage)+1];
					strcpy(service,optmessage);
					if (pass) delete pass;
					pass=new char[myPass.local8Bit().length()+1];
					strcpy(pass,myPass.local8Bit().data());
					kdDebug(7106) << "CallBack: service=" << service << ", pass=" << pass << endl;
					haveServicePass=true;
					// Store the new binding info in case of success
					proto->bserver = proto->currentHost;
					proto->bshare = optmessage;
					proto->blogin = "";
					proto->bpassword = pass;
				}
				return pass;
		}
		return 0; //???
	}
};

extern "C" { int kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
  KInstance instance( "kio_smb" );

  kdDebug(7106) << "Starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: kio_smb protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  SmbProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  kdDebug(7106) << "Done" << endl;
  return 0;
}


SmbProtocol::SmbProtocol( const QCString &pool, const QCString &app) : SlaveBase( "smb", pool, app )
{
  kdDebug( 7106 ) << "Constructor" << endl;
	currentHost=QString::null;
	currentIP=QString::null;
	currentUser=QString::null;
	currentPass=QString::null;
	cb = new MyCallback(this);
	smb.setPasswordCallback(cb);

	// Load config info
	KConfig *g_pConfig = new KConfig("kioslaverc");

	QString tmp;
	g_pConfig->setGroup( "Browser Settings/SMB" );
	tmp = g_pConfig->readEntry( "Browse server" );
	if (!tmp.isEmpty()) smb.setDefaultBrowseServer(tmp.latin1());
#if USE_SAMBA != 1
	tmp = g_pConfig->readEntry( "Broadcast address" );
	if (!tmp.isEmpty()) smb.setBroadcastAddress(tmp.latin1());
	tmp = g_pConfig->readEntry( "WINS address" );
	if (!tmp.isEmpty()) smb.setWINSAddress(tmp.latin1());
#endif
	delete g_pConfig;
	
	bindings.setAutoDelete(true);
	loadBindings(true); // load the bindings at least once
}

void SmbProtocol::loadBindings(bool force)
{
	KConfig *g_pConfig = new KConfig("kioslaverc");
	g_pConfig->setGroup( "Browser Settings/SMB" );
	QString tmp = g_pConfig->readEntry( "Password policy" );
	if ( tmp == "Don't store" ) storeBindings = false;
	else storeBindings = true;
	if (!storeBindings && !force) return; // don't destroy our copy in memory in that case
	bindings.clear();
	int count = g_pConfig->readNumEntry( "Bindings count");
	QString key, server, share, login, password;
	for (int index=0; index<count; index++) {
		key.sprintf("server%d",index);
		server = g_pConfig->readEntry( key );
		key.sprintf("share%d",index);
		share = g_pConfig->readEntry( key );
		key.sprintf("login%d",index);
		login = g_pConfig->readEntry( key );
		key.sprintf("password%d",index);
		// unscramble
		QString scrambled = g_pConfig->readEntry( key );
		password = "";
		for (unsigned int i=0; i<scrambled.length()/3; i++) {
			QChar qc1 = scrambled[i*3];
			QChar qc2 = scrambled[i*3+1];
			QChar qc3 = scrambled[i*3+2];
			unsigned int a1 = qc1.latin1() - '0';
			unsigned int a2 = qc2.latin1() - 'A';
			unsigned int a3 = qc3.latin1() - '0';
			unsigned int num = ((a1 & 0x3F) << 10) | ((a2& 0x1F) << 5) | (a3 & 0x1F);
			password[i] = QChar((uchar)((num - 17) ^ 173)); // restore
		}
		bindings.append(new Binding(server,share,login,password));
	}
	delete g_pConfig;
}

void SmbProtocol::saveBindings() // Will store on the disk if required
{
	if (!storeBindings) return;
	KConfig *g_pConfig = new KConfig("kioslaverc");
	g_pConfig->setGroup( "Browser Settings/SMB" );
	g_pConfig->writeEntry( "Bindings count", bindings.count());
	QString key; int index=0;
	for (Binding* it = bindings.first(); (it); it = bindings.next(), index++) {
		key.sprintf("server%d",index);
		g_pConfig->writeEntry( key, it->server);
		key.sprintf("share%d",index);
		g_pConfig->writeEntry( key, it->share);
		key.sprintf("login%d",index);
		g_pConfig->writeEntry( key, it->login);
		key.sprintf("password%d",index);
		// Weak code, but least it makes the string unreadable
		QString scrambled;
		for (unsigned int i=0; i<it->password.length(); i++) {
			QChar c = it->password[i];
			unsigned int num = (c.unicode() ^ 173) + 17;
			unsigned int a1 = (num & 0xFC00) >> 10;
			unsigned int a2 = (num & 0x3E0) >> 5;
			unsigned int a3 = (num & 0x1F);
			scrambled += (char)(a1+'0');
			scrambled += (char)(a2+'A');
			scrambled += (char)(a3+'0');
		}
		g_pConfig->writeEntry( key, scrambled);
	}
	delete g_pConfig;
}

SmbProtocol::~SmbProtocol()
{
	if (cb) delete cb;
	cb = 0; // NB: paranoia can help living longer :-)
	if (!bindings.isEmpty()) saveBindings();
	bindings.clear();
}

// Uses this function to get information in the url
void SmbProtocol::setHost(const QString& host, int ip, const QString& user, const QString& pass)
{
  kdDebug( 7106 ) << "in set host method" << endl;
	currentHost=host;
	currentIP=ip;
	currentUser=user;
	currentPass=pass;
}

QString SmbProtocol::buildFullLibURL(const QString &pathArg)
{
	Util util;
	kdDebug( 7106 ) << "currentUser: " << currentUser << endl;
	kdDebug( 7106 ) << "currentPass: " << currentPass << endl;
	kdDebug( 7106 ) << "currentHost: " << currentHost << endl;
	kdDebug( 7106 ) << "currentIP: " << currentIP << endl;
	QString path = pathArg;
	if (path[0]=='/') path.remove(0,1);
	// NB20000423: Hmmm, with smb:// => smb:/ conversion, now there is no host
/*	QString ret = util.buildURL(
		currentUser.isEmpty()?(const char*)0:(const char*)currentUser.local8Bit(),
		currentPass.isEmpty()?(const char*)0:(const char*)currentPass.local8Bit(),
		currentHost.isEmpty()?(const char*)0:(const char*)currentHost.local8Bit(),
		0,
		0,
		path.isEmpty()?(const char*)0:(const char*)path.local8Bit(),
		currentIP.isEmpty()?(const char*)0:(const char*)currentIP.local8Bit()
		);*/
	// NB20000423: but the path is correct => can retrieve the host for bindings
	QString ret = util.buildURL(
		currentUser.isEmpty()?(const char*)0:(const char*)currentUser.local8Bit(),
		currentPass.isEmpty()?(const char*)0:(const char*)currentPass.local8Bit(),
		0,
		0,
		0,
		path.isEmpty()?(const char*)0:(const char*)path.local8Bit(),
		currentIP.isEmpty()?(const char*)0:(const char*)currentIP.local8Bit()
		);
	util.parse(ret);
	currentHost = util.host(); // Make sure we have the host, not the workgroup
	kdDebug(7106) << "converting argument " << pathArg << " to " << ret << endl;
	return ret;
}

void SmbProtocol::mkdir( const KURL& url, int /*permissions*/ )
{
	QString path = buildFullLibURL(url.path());
	kdDebug(7106) << "entering mkdir " << path << endl;
	struct stat buff;
	if ( smb.stat( path, &buff ) == -1 ) {
		if ( smb.mkdir( path ) != 0 ) {
			if ( smb.error() == EACCES ) {
				error( KIO::ERR_ACCESS_DENIED, path );
				return;
			} else {
				error( KIO::ERR_COULD_NOT_MKDIR, path );
				return;
			}
		} else {
			finished();
		}
	}

	if ( S_ISDIR( buff.st_mode ) ) {
		debug("ERR_DIR_ALREADY_EXIST");
		error( KIO::ERR_DIR_ALREADY_EXIST, path );
		return;
	}
	error( KIO::ERR_FILE_ALREADY_EXIST, path );
	return;
}

void SmbProtocol::get( const KURL& url, bool /* reload */)
{
	QString path = buildFullLibURL(url.path());
	kdDebug( 7106 ) << "entering get " << path << endl;

	struct stat buff;
	if ( smb.stat( path, &buff ) == -1 ) {
		if ( smb.error() == EACCES )
			error( KIO::ERR_ACCESS_DENIED, path );
		else
			error( KIO::ERR_DOES_NOT_EXIST, path );
		return;
	}

	if ( S_ISDIR( buff.st_mode ) ) {
		error( KIO::ERR_IS_DIRECTORY, path );
		return;
	}

	int fd = smb.open( path, O_RDONLY );
	if ( fd == -1 ) {
		error( KIO::ERR_CANNOT_OPEN_FOR_READING, path );
		return;
	}

	totalSize( buff.st_size );
	int processed_size = 0;
	time_t t_start = time( 0L );
	time_t t_last = t_start;

	char buffer[ 4090 ];
	QByteArray array;
        bool mimetypeEmitted = false;

	int n;
	while( (n = smb.read(fd, buffer, 2048)) > 0 )
	{
		array.setRawData(buffer, n);
                if ( !mimetypeEmitted )
                {
                  KMimeMagicResult * result = KMimeMagic::self()->findBufferFileType( array, url.fileName() );
                  mimeType( result->mimeType() );
                  mimetypeEmitted = true;
                }
		data( array );
                array.resetRawData(buffer, n);

		processed_size += n;
		time_t t = time( 0L );
		if ( t - t_last >= 1 )
		{
                  processedSize( processed_size );
                  speed( processed_size / ( t - t_start ) );
                  t_last = t;
		}
	}
	if (n == -1)
	{
		error( KIO::ERR_COULD_NOT_READ, path);
		smb.close(fd);
		return;
	}

	data( QByteArray() );

	smb.close(fd);

	processedSize( buff.st_size );
	time_t t = time( 0L );
	if ( t - t_start >= 1 )
	speed( processed_size / ( t - t_start ) );

	finished();
}


void SmbProtocol::put( const KURL& url, int /*_mode*/, bool _overwrite, bool _resume )
{
    QString dest_orig = buildFullLibURL(url.path());
    kdDebug(7106) << "entering put " << dest_orig << endl;

    struct stat buff_orig;
    bool orig_exists = ( smb.stat( dest_orig, &buff_orig ) != -1 );
    if ( orig_exists &&  !_overwrite && !_resume)
    {
        if (S_ISDIR(buff_orig.st_mode))
           error( KIO::ERR_DIR_ALREADY_EXIST, dest_orig );
        else
           error( KIO::ERR_FILE_ALREADY_EXIST, dest_orig );
        return;
    }

    QString dest = dest_orig;
	if ( orig_exists && !_resume )
	{
	  kdDebug(7106) << "Deleting destination file " << dest_orig << endl;
		remove( dest_orig );
		// Catch errors when we try to open the file.
	}

	if ( _resume ) {
		m_fPut = smb.open( dest, O_WRONLY | O_APPEND );  // append if resuming
	} else {
		m_fPut = smb.open( dest, O_CREAT | O_TRUNC | O_WRONLY);
	}

	if ( m_fPut == -1 ) {
	  kdDebug(7106) << "####################### COULD NOT WRITE " << dest << endl;
		if ( smb.error() == EACCES ) {
			error( KIO::ERR_WRITE_ACCESS_DENIED, dest );
		} else {
			error( KIO::ERR_CANNOT_OPEN_FOR_WRITING, dest );
		}
		return;
	}

	kdDebug( 7106 ) << "Put: Ready" << endl;

	int result;
	// Loop until we got 0 (end of data)
	do
	{
		QByteArray buffer;
		dataReq(); // Request for data
		result = readData( buffer );
		if (result > 0)
		{
			smb.write( m_fPut, buffer.data(), buffer.size());
		}
	}
	while ( result > 0 );


	if (result != 0)
	{
		smb.close(m_fPut);
		kdDebug( 7106 ) << "Error during 'put'. Aborting." << endl;
		::exit(255);
	}

	if (smb.close(m_fPut))
	{
		error( KIO::ERR_COULD_NOT_WRITE, dest_orig);
		return;
	}
	
	// We have done our job => finish
	finished();
}

void SmbProtocol::rename( const KURL &srcArg, const KURL &destArg,
                           bool _overwrite )
{
    QString src = buildFullLibURL(srcArg.path());
    QString dest = buildFullLibURL(destArg.path());
    kdDebug(7106) << "entering rename " << src << " -> " << dest << endl;

    struct stat buff_src;
    if ( smb.stat( src, &buff_src ) == -1 ) {
        if ( smb.error() == EACCES )
           error( KIO::ERR_ACCESS_DENIED, src );
        else
           error( KIO::ERR_DOES_NOT_EXIST, src );
		return;
    }

    struct stat buff_dest;
    bool dest_exists = ( smb.stat( dest, &buff_dest ) != -1 );
    if ( dest_exists )
    {
        if (S_ISDIR(buff_dest.st_mode))
        {
           error( KIO::ERR_DIR_ALREADY_EXIST, dest );
           return;
        }

        if (!_overwrite)
        {
           error( KIO::ERR_FILE_ALREADY_EXIST, dest );
           return;
        }
    }

    if ( smb.rename( src, dest))
    {
        if (( smb.error() == EACCES ) || (smb.error() == EPERM)) {
            error( KIO::ERR_ACCESS_DENIED, dest );
        }
        else {
           error( KIO::ERR_CANNOT_RENAME, src );
        }
        return;
    }

    finished();
}


void SmbProtocol::del( const KURL& url, bool isfile)
{
    QString path = url.path(); // Not URL??
    kdDebug(7106) << "entering del " << path << endl;
	/*****
	* Delete files
	*****/

	if (isfile) {
		kdDebug( 7106 ) <<  "Deleting file "<< path << endl;

		// TODO deletingFile( source );

		if ( smb.unlink( path ) == -1 ) {
			if ((smb.error() == EACCES) || (smb.error() == EPERM))
				error( KIO::ERR_ACCESS_DENIED, path);
			else if (smb.error() == EISDIR)
				error( KIO::ERR_IS_DIRECTORY, path);
			else
				error( KIO::ERR_CANNOT_DELETE, path );
			return;
		}
	} else {

		/*****
		* Delete empty directory
		*****/

       	  kdDebug(7106) << "Deleting directory " << path << endl;

		// TODO deletingFile( source );


		/*****
		* Delete empty directory
		*****/

		kdDebug(7106) << "Deleting directory " << path << endl;

		if ( smb.rmdir( path ) == -1 ) {
			if ((smb.error() == EACCES) || (smb.error() == EPERM))
				error( KIO::ERR_ACCESS_DENIED, path);
			else {
				kdDebug( 7106 ) << "could not rmdir " << endl;
				error( KIO::ERR_COULD_NOT_RMDIR, path );
				return;
			}
		}
	}

	finished();
}

void SmbProtocol::createUDSEntry( const QString & filename, const QString & path, UDSEntry & entry  )
{
	assert(entry.count() == 0); // by contract :-)
	UDSAtom atom;
	atom.m_uds = KIO::UDS_NAME;
	atom.m_str = filename;
	entry.append( atom );

	mode_t type;
	mode_t access;
	struct stat buff;

	if ( smb.stat( path, &buff ) == -1 )  {
		kdDebug( 7106 ) << "cannot stat in createUDSEntry!!!" << endl;
		return;
	}

	type = buff.st_mode & S_IFMT; // extract file type
	access = buff.st_mode & 0x1FF; // extract permissions

	atom.m_uds = KIO::UDS_FILE_TYPE;
	atom.m_long = type;
	entry.append( atom );

	atom.m_uds = KIO::UDS_ACCESS;
	atom.m_long = access;
	entry.append( atom );

	atom.m_uds = KIO::UDS_SIZE;
	atom.m_long = buff.st_size;
	entry.append( atom );

	atom.m_uds = KIO::UDS_MODIFICATION_TIME;
	atom.m_long = buff.st_mtime;
	entry.append( atom );

	atom.m_uds = KIO::UDS_ACCESS_TIME;
	atom.m_long = buff.st_atime;
	entry.append( atom );

	atom.m_uds = KIO::UDS_CREATION_TIME;
	atom.m_long = buff.st_ctime;
	entry.append( atom );
}

// NB: That's because the smb servers can return statistics at the same time
//     => no need to re-stat after an opendir
void SmbProtocol::createUDSEntry( const SMBdirent *dent, const QString & /*path*/, UDSEntry & entry  )
{
	assert(entry.count() == 0); // by contract :-)
	UDSAtom atom;
	atom.m_uds = KIO::UDS_NAME;
	atom.m_str = dent->d_name;
	entry.append( atom );

	mode_t type;
	mode_t access;

	type = dent->st_mode & S_IFMT; // extract file type
	access = dent->st_mode & 0x1FF; // extract permissions

	atom.m_uds = KIO::UDS_FILE_TYPE;
	atom.m_long = type;
	entry.append( atom );

	atom.m_uds = KIO::UDS_ACCESS;
	atom.m_long = access;
	entry.append( atom );

	atom.m_uds = KIO::UDS_SIZE;
	atom.m_long = dent->st_size;
	entry.append( atom );

	atom.m_uds = KIO::UDS_MODIFICATION_TIME;
	atom.m_long = dent->st_mtime;
	entry.append( atom );

	atom.m_uds = KIO::UDS_ACCESS_TIME;
	atom.m_long = dent->st_atime;
	entry.append( atom );

	atom.m_uds = KIO::UDS_CREATION_TIME;
	atom.m_long = dent->st_ctime;
	entry.append( atom );
}

void SmbProtocol::stat( const KURL &url)
{
	QString path = buildFullLibURL(url.path());
	kdDebug( 7106 ) << "entering stat " << path << endl;
	struct stat buff;
	if ( smb.stat( path, &buff ) == -1 ) {
		error( KIO::ERR_DOES_NOT_EXIST, path );
		return;
	}

	UDSEntry entry;
	createUDSEntry( url.filename(), path, entry );
///////// debug code

	KIO::UDSEntry::ConstIterator it = entry.begin();
	for( ; it != entry.end(); it++ ) {
		switch ((*it).m_uds) {
			case KIO::UDS_FILE_TYPE:
			  kdDebug(7106) << "File Type : " << (mode_t)((*it).m_long) << endl;
				break;
			case KIO::UDS_ACCESS:
			  kdDebug(7106) << "Access permissions : " << (int)(mode_t)(*it).m_long << endl;
				break;
			case KIO::UDS_USER:
			  kdDebug(7106) << "User : " << (*it).m_str << endl;
				break;
			case KIO::UDS_GROUP:
			  kdDebug(7106) << "Group : " << (*it).m_str << endl;
				break;
			case KIO::UDS_NAME:
			kdDebug(7106) << "Name : " << (*it).m_str << endl;
				//m_strText = decodeFileName( (*it).m_str );
				break;
			case KIO::UDS_URL:
			kdDebug(7106) << "URL : " << (*it).m_str << endl;
				break;
			case KIO::UDS_MIME_TYPE:
			kdDebug(7106) << "MimeType : " << (*it).m_str << endl;
				break;
			case KIO::UDS_LINK_DEST:
			kdDebug(7106) << "LinkDest : " << (*it).m_str << endl;
				break;
		}
	}

/////////
	statEntry( entry );

	finished();
}

void SmbProtocol::listDir( const KURL& url )
{
	QString path = buildFullLibURL(url.path());
	kdDebug( 7106 ) << "=============== LIST " << path << " ===============" << endl;

	struct stat buff;
	if ( smb.stat( path, &buff ) == -1 ) {
		error( KIO::ERR_DOES_NOT_EXIST, path );
		return;
	}

	if ( !S_ISDIR( buff.st_mode ) ) {
		error( KIO::ERR_IS_FILE, path );
		return;
	}

	struct SMBdirent *ep;
	callbackUsed = false;
	int dp = smb.opendir( path );
	if ( dp == -1 ) {
		if (callbackUsed) error( KIO::ERR_ACCESS_DENIED, path);
		else error( KIO::ERR_CANNOT_ENTER_DIRECTORY, path );
		return;
	}
	// used callback successfully => new binding!
	if (callbackUsed) {
		bindings.append(new Binding(bserver, bshare, blogin, bpassword));
		saveBindings(); // Will store on the disk if required
	}

	UDSEntry entry;
	while ( ( ep = smb.readdir( dp ) ) != 0L ) {
		entry.clear();
		createUDSEntry( ep, path, entry );
		listEntry( entry, false);
	}
	smb.closedir(dp);

	listEntry( entry, true ); // ready

	kdDebug(7106) << "============= COMPLETED LIST ============" << endl;

	finished();

	kdDebug(7106) << "=============== BYE ===========" << endl;
}
