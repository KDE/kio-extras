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
#include <kurl.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include "smb.h"

using namespace KIO;

class MyCallback : public SmbAnswerCallback
{
protected:
	SmbProtocol *proto;
	QString user, pass, message, service;
	bool havePass;
	bool haveServicePass;
public:
	MyCallback(SmbProtocol *p) {
		proto = p;
		havePass = false;
		haveServicePass = false; // we keep the service so that no password
		service = "";            // is asked to user when accessing subdirs
	};
	
	char *getAnswer(int type, const char *optmessage) {
		bool res = true;
		switch (type) {
			case ANSWER_USER_NAME:
				message = "Login for host ";
				message += optmessage;
				res = proto->openPassDlg(message, user, pass);
				kDebugInfo( 7106, "CallBack: res=%s", res?debugString("true"):debugString("false"));
				kDebugInfo( 7106, "CallBack: user=%s, pass=%s", debugString(user), debugString(pass));
				if (!res) {pass=""; user=""; havePass=false;}
				else havePass=true;
				return user.local8Bit().data();
			
			case ANSWER_USER_PASSWORD:
				if (havePass) return pass.local8Bit().data();
				message = "Password for user ";
				message += optmessage;
				res = proto->openPassDlg(message, user, pass);
				kDebugInfo( 7106, "CallBack: res=%s", res?debugString("true"):debugString("false"));
				kDebugInfo( 7106, "CallBack: user=%s, pass=%s", debugString(user), debugString(pass));
				if (!res) {pass=""; user=""; havePass=false;}
				else havePass=true;
				return pass.local8Bit().data();
			
			case ANSWER_SERVICE_PASSWORD:
				if (haveServicePass && !strcmp(service, optmessage))
					return pass.local8Bit().data(); // we have it already
				message = "Password for service ";
				message += optmessage;
				message += " (user ignored)";
				res = proto->openPassDlg(message, user, pass);
				kDebugInfo( 7106, "CallBack: res=%s", res?debugString("true"):debugString("false"));
				kDebugInfo( 7106, "CallBack: user=%s, pass=%s", debugString(user), debugString(pass));
				if (!res) {pass=""; user=""; haveServicePass=false;}
				else {haveServicePass=true; service=optmessage;}
				return pass.local8Bit().data();
		}
		return 0; //???
	}
};

SmbProtocol::SmbProtocol( Connection *_conn ) : SlaveBase( "smb", _conn )
{
	kDebugInfo( 7106, "Constructor");
	currentHost=QString::null;
	currentIP=QString::null;
	currentUser=QString::null;
	currentPass=QString::null;
	cb = new MyCallback(this);
	smb.setPasswordCallback(cb);
}

SmbProtocol::~SmbProtocol()
{
	if (cb) delete cb;
	cb = 0; // NB: paranoia can help living longer :-)
}

// Uses this function to get information in the url
void SmbProtocol::setHost(const QString& host, int ip, const QString& user, const QString& pass)
{
	kDebugInfo( 7106, "in set host method");
	currentHost=host;
	currentIP=ip;
	currentUser=user;
	currentPass=pass;
}

QString SmbProtocol::buildFullLibURL(const QString &pathArg)
{
	Util util;
	kDebugInfo( 7106, "currentUser: %s", debugString(currentUser));
	kDebugInfo( 7106, "currentPass: %s", debugString(currentPass));
	kDebugInfo( 7106, "currentHost: %s", debugString(currentHost));
	kDebugInfo( 7106, "currentIP: %s", debugString(currentIP));
	QString path = pathArg;
	if (path[0]=='/') path.remove(0,1);
	QString ret = util.buildURL(
		currentUser.isEmpty()?(const char*)0:(const char*)currentUser.local8Bit(),
		currentPass.isEmpty()?(const char*)0:(const char*)currentPass.local8Bit(),
		currentHost.isEmpty()?(const char*)0:(const char*)currentHost.local8Bit(),
		0,
		0,
		path.isEmpty()?(const char*)0:(const char*)path.local8Bit(),
		currentIP.isEmpty()?(const char*)0:(const char*)currentIP.local8Bit()
		);
	kDebugInfo( 7106, "converting argument %s to %s", debugString(pathArg), debugString(ret));
	return ret;
}

void SmbProtocol::mkdir( const QString& pathArg, int permissions )
{
	QString path = buildFullLibURL(pathArg);
	kDebugInfo( 7106, "entering mkdir %s", debugString(path));
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

void SmbProtocol::get( const QString& pathArg, const QString& /*query*/, bool /* reload */)
{
	QString path = buildFullLibURL(pathArg);
	kDebugInfo( 7106, "entering get %s", debugString(path));

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

	int n;
	while( (n = smb.read(fd, buffer, 2048)) > 0 )
	{
		array.setRawData(buffer, n);
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


void SmbProtocol::put( const QString& dest_orig_arg, int _mode, bool _overwrite, bool _resume )
{
	QString dest_orig = buildFullLibURL(dest_orig_arg);
	kDebugInfo( 7106, "entering put %s", debugString(dest_orig));
    bool bMarkPartial = false;

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
		kDebugInfo( 7106, "Deleting destination file %s", debugString(dest_orig) );
		remove( dest_orig );
		// Catch errors when we try to open the file.
	}

	if ( _resume ) {
		m_fPut = smb.open( dest, O_WRONLY | O_APPEND );  // append if resuming
	} else {
		m_fPut = smb.open( dest, O_CREAT | O_TRUNC | O_WRONLY);
	}

	if ( m_fPut == -1 ) {
	kDebugInfo( 7106, "####################### COULD NOT WRITE %s", debugString(dest));
		if ( smb.error() == EACCES ) {
			error( KIO::ERR_WRITE_ACCESS_DENIED, dest );
		} else {
			error( KIO::ERR_CANNOT_OPEN_FOR_WRITING, dest );
		}
		return;
	}

	kDebugInfo( 7106, "Put: Ready" );

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
		kDebugInfo( 7106, "Error during 'put'. Aborting.");
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

void SmbProtocol::rename( const QString &srcArg, const QString &destArg,
                           bool _overwrite )
{
	QString src = buildFullLibURL(srcArg);
	QString dest = buildFullLibURL(destArg);
	kDebugInfo( 7106, "entering rename %s -> %s", debugString(src), debugString(dest));

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


void SmbProtocol::del( const QString& path, bool isfile)
{
	kDebugInfo( 7106, "entering del %s", debugString(path));
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

		kDebugInfo( 7106, "Deleting directory %s", debugString(path) );

		// TODO deletingFile( source );


		/*****
		* Delete empty directory
		*****/

		kdDebug( 7106 ) << "Deleting directory " << debugString(path) << endl;

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

	notype:
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
void SmbProtocol::createUDSEntry( const SMBdirent *dent, const QString & path, UDSEntry & entry  )
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

	notype:
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

void SmbProtocol::stat( const QString & pathArg )
{
	QString path = buildFullLibURL(pathArg);
	kDebugInfo( 7106, "entering stat %s", debugString(path));
	struct stat buff;
	if ( smb.stat( path, &buff ) == -1 ) {
		error( KIO::ERR_DOES_NOT_EXIST, path );
		return;
	}

	// Extract filename out of path
	QString filename = KURL( path ).filename();

	UDSEntry entry;
	createUDSEntry( filename, path, entry );
///////// debug code

	KIO::UDSEntry::ConstIterator it = entry.begin();
	for( ; it != entry.end(); it++ ) {
		switch ((*it).m_uds) {
			case KIO::UDS_FILE_TYPE:
				kDebugInfo("File Type : %d", (mode_t)((*it).m_long) );
				break;
			case KIO::UDS_ACCESS:
				kDebugInfo("Access permissions : %d", (mode_t)((*it).m_long) );
				break;
			case KIO::UDS_USER:
				kDebugInfo("User : %s", ((*it).m_str.ascii() ) );
				break;
			case KIO::UDS_GROUP:
				kDebugInfo("Group : %s", ((*it).m_str.ascii() ) );
				break;
			case KIO::UDS_NAME:
				kDebugInfo("Name : %s", ((*it).m_str.ascii() ) );
				//m_strText = decodeFileName( (*it).m_str );
				break;
			case KIO::UDS_URL:
				kDebugInfo("URL : %s", ((*it).m_str.ascii() ) );
				break;
			case KIO::UDS_MIME_TYPE:
				kDebugInfo("MimeType : %s", ((*it).m_str.ascii() ) );
				break;
			case KIO::UDS_LINK_DEST:
				kDebugInfo("LinkDest : %s", ((*it).m_str.ascii() ) );
				break;
		}
	}

/////////
	statEntry( entry );

	finished();
}

void SmbProtocol::listDir( const QString& pathArg )
{
	QString path = buildFullLibURL(pathArg);
	kDebugInfo( 7106, "=============== LIST %s ===============", debugString(path) );

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
	int dp = smb.opendir( path );
	if ( dp == -1 ) {
		error( KIO::ERR_CANNOT_ENTER_DIRECTORY, path );
		return;
	}

	UDSEntry entry;
	while ( ( ep = smb.readdir( dp ) ) != 0L ) {
		entry.clear();
		createUDSEntry( ep, path, entry );
		listEntry( entry, false);
	}
	smb.closedir(dp);

	listEntry( entry, true ); // ready

	kDebugInfo(7106, "============= COMPLETED LIST ============" );

	finished();

	kDebugInfo(7106, "=============== BYE ===========" );
}

extern "C" {
	SlaveBase *init_smb() {
		return new SmbProtocol();
	}
}

