// smb.cc adapted from file.cc by Nicolas Brodu <brodu@kde.org>
// $Id$

#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#ifdef rewinddir
#undef rewinddir
#endif

#include "kio_smb.h"
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <kio_rename_dlg.h>
#include <kio_skip_dlg.h>
#include <kurl.h>
#include <kprotocolmanager.h>
#include <qvaluelist.h>
#include <kinstance.h>

#include <sys/stat.h>

#include <iostream.h>

#include <qapplication.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qdialog.h>
#include <qaccel.h>

// translation & config
#include <klocale.h>
#include <kconfig.h>

#define BUF_SIZE 2048

// used to create a dialog
QApplication *QtApp;

// class adapted from passworddialog
// passworddialog isn't adapted because passwords might be associated with
// shares and not users, and the libsmb does ask for the appropriate entry
// We also need echo for user names, no echo for passwords.
// Next version of the lib will have a better mechanism
// Note : removing this from here would make this kio_slave no Qt app anymore
class CallbackDialog : public QDialog
{
protected:
    QLineEdit *theLineEdit;

public:
	// constructor
	CallbackDialog( const char *text, bool echo=false, QWidget* parent=0, const char* name=0, bool modal=true, WFlags f=0 );
	const char *answer(); // the user answer
};

CallbackDialog::CallbackDialog( const char *text, bool echo, QWidget* parent, const char* name, bool modal, WFlags f )
   : QDialog(parent, name, modal, f)
{
	QVBoxLayout *vLay = new QVBoxLayout(this, 10 /* border */, 5);

	QLabel *l = new QLabel(text, this);
	
	l->adjustSize();
	l->setMinimumSize(l->size());
	vLay->addWidget(l);

	// The horizontal layout for label + lineedit
//	QHBoxLayout *hLay = new QHBoxLayout(5);
	
	
	theLineEdit = new QLineEdit( this );
	if (!echo) theLineEdit->setEchoMode( QLineEdit::Password );
	theLineEdit->adjustSize();
	theLineEdit->setFixedHeight(theLineEdit->height());
//	hLay->addWidget(theLineEdit,10);
	vLay->addWidget(theLineEdit,10);
//	vLay->addLayout(hLay);

	QAccel *ac = new QAccel(this);
	ac->connectItem( ac->insertItem(Key_Escape), this, SLOT(reject()) );
	connect( theLineEdit, SIGNAL(returnPressed()), SLOT(accept()) );
}

const char *CallbackDialog::answer() // the user answer
{
	if ( theLineEdit )
		return theLineEdit->text();
	else
		return 0;
}


// used by the lib to get info
// should display its argument and return an answer allocated with new.
// A callback class with virtual function should be used in the lib in the
// future
char *getPasswordCallBack(const char * c, bool echo)
{
	if (!c) return 0;
	QString s;
	if (!strcmp(c,"User")) {
		s+=i18n("User");
	}
	else if (!strcmp(c,"Password")) s+=i18n("Password");
	else if (!strncmp(c,"Password for service ",21)) {
		s+=i18n("Password for service ");
		s+=(c+21);
	}
	CallbackDialog d(s, echo);
	d.show();
	return qstrdup(d.answer());
}


int check( KIOConnection *_con );
void sigchld_handler( int );
void sigsegv_handler( int );

int main( int argc, char **argv )
{
	signal(SIGCHLD, sigchld_handler);
	signal(SIGSEGV, sigsegv_handler);

	qDebug( "kio_smb : Starting");

	QtApp = new QApplication( argc, argv );
	qDebug( "kio_smb : main 1");

        KInstance instance( "kio_smb" );

	KIOConnection parent( 0, 1 );
		
	qDebug( "kio_smb : main 2");

	SmbProtocol smb( &parent );
	qDebug( "kio_smb : main 3");
	smb.dispatchLoop();

	qDebug( "kio_smb : Done" );
}


void sigsegv_handler( int )
{
  write(2, "kio_smb : SEGMENTATION FAULT\n", 29);
  exit(1);
}


void sigchld_handler( int )
{
  int pid, status;
    
  while( 1 ) {
    pid = waitpid( -1, &status, WNOHANG );
    if ( pid <= 0 ) {
      // Reinstall signal handler, since Linux resets to default after
      // the signal occured ( BSD handles it different, but it should do
      // no harm ).
      signal( SIGCHLD, sigchld_handler );
      return;
    }
  }
}


SmbProtocol::SmbProtocol( KIOConnection *_conn ) : KIOProtocol( _conn )
{
	smbio=new SMBIO(getPasswordCallBack);
	smbio->setPasswordCallback(getPasswordCallBack);
	
	QString tmp;
	KConfig *config = new KConfig( "kioslaverc" );
	config->setGroup( "Browser Settings/SMB" );
	tmp = config->readEntry( "Browse server" );
	if (!(tmp.isEmpty())) smbio->setDefaultBrowseServer(tmp);
	tmp = config->readEntry( "Broadcast address" );
	if (!(tmp.isEmpty())) smbio->setNetworkBroadcastAddress(tmp);
	tmp = config->readEntry( "Default user" );
	if (!(tmp.isEmpty())) smbio->setDefaultUser(tmp);
	qDebug( "kio_smb : config read" );
//	tmp = konqConfig->readEntry( "Remember password" );
	delete config;

}

/*SmbProtocol::~SmbProtocol()
{
	qDebug( "kio_destructor : try deleting SMBIO object" );
	delete smbio;
	qDebug( "kio_destructor : end" );
}
*/
void SmbProtocol::slotMkdir( const char *_url, int /*_mode*/ )
{
	KURL usrc( _url );
	if ( usrc.isMalformed() ) {
		error( ERR_MALFORMED_URL, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	if ( strcmp(usrc.protocol(), "smb") ) {
		error( ERR_INTERNAL, "kio_smb got non-smb url in mkdir command" );
		m_cmd = CMD_NONE;
		return;
	}

	struct stat buff;
	if ( smbio->stat( usrc.decodedURL().ascii(), &buff ) == -1 ) {
		if ( smbio->mkdir( usrc.decodedURL().ascii() ) == -1 ) {
			if ( smbio->getError() == EACCES ) {
				error( ERR_ACCESS_DENIED, strdup(_url) );
				m_cmd = CMD_NONE;
				return;
			} else {
				error( ERR_COULD_NOT_MKDIR, strdup(_url) );
				m_cmd = CMD_NONE;
				return;
			}
		} else {
			finished();
			return;
		}
	}

	if ( S_ISDIR( buff.st_mode ) ) {
		error( ERR_DOES_ALREADY_EXIST, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	error( ERR_COULD_NOT_MKDIR, strdup(_url) );
	m_cmd = CMD_NONE;
	return;
}

void SmbProtocol::slotCopy( QStringList& _source, const char *_dest )
{
	qDebug( "kio_smb : slotCopy <List> %s", _dest );
	doCopy( _source, _dest, false );
}


void SmbProtocol::slotCopy( const char* _source, const char *_dest )
{
	qDebug( "kio_smb : slotCopy %s %s", _source, _dest );
	QStringList lst;
	lst.append( _source );

	doCopy( lst, _dest, true );
}

void SmbProtocol::slotMove( QStringList& _source, const char *_dest )
{
	qDebug( "kio_smb : slotMove <List> %s", _dest );
	doCopy( _source, _dest, false, true );
}

void SmbProtocol::slotMove( const char* _source, const char *_dest )
{
	qDebug( "kio_smb : slotMove %s %s", _source, _dest );
	QStringList lst;
	lst.append( _source );

	doCopy( lst, _dest, true, true );
}


void SmbProtocol::doCopy( QStringList& _source, const char *_dest, bool _rename, bool _move )
{
	if ( _rename )
		assert( _source.count() == 1 );

	qDebug( "kio_smb : Making copy to %s", _dest );

	// Check whether the URLs are wellformed
	QStringList::Iterator source_files_it = _source.begin();
	while (source_files_it != _source.end()) {
		qDebug( "kio_smb : Checking %s", (*source_files_it).ascii() );
		KURL usrc(*source_files_it);
		if ( usrc.isMalformed() ) {
			error( ERR_MALFORMED_URL, *source_files_it );
			m_cmd = CMD_NONE;
			return;
		}
		if ( strcmp(usrc.protocol(), "smb") ) {
			error( ERR_INTERNAL, "kio_smb got non-smb url as source in copy command" );
			m_cmd = CMD_NONE;
			return;
		}
		++source_files_it;
	}
	
	qDebug( "kio_smb : All source URLs ok %s", _dest );

	// Make a copy of the parameter. if we do IPC calls from here, then we overwrite
	// our argument. This is tricky! ( but saves memory and speeds things up )
	QString dest = _dest;

	// Check wellformedness of the destination
	KURL udest( _dest );
	if ( udest.isMalformed() ) {
		error( ERR_MALFORMED_URL, dest );
		m_cmd = CMD_NONE;
		return;
	}

	qDebug( "kio_smb : Dest ok %s", dest.ascii() );

	// Find IO server for destination
	QString exec = KProtocolManager::self().executable( udest.protocol() );
	if ( exec.isEmpty() ) {
		error( ERR_UNSUPPORTED_PROTOCOL, udest.protocol() );
		m_cmd = CMD_NONE;
		return;
	}

	// Is the left most protocol a filesystem protocol ?
	if ( KProtocolManager::self().outputType( udest.protocol() ) != KProtocolManager::T_FILESYSTEM )
	{
		error( ERR_PROTOCOL_IS_NOT_A_FILESYSTEM, udest.protocol() );
		m_cmd = CMD_NONE;
		return;
	}
	qDebug( "kio_smb : IO server ok %s", dest.ascii() );


	// Check whether the URLs are wellformed
/*	QStringList::Iterator soit = _source.begin();
	for( ; soit != _source.end(); ++soit ) {
		qDebug( "kio_smb : Checking %s", soit->c_str() );
		char *workgroup=NULL, *host=NULL, *share=NULL, *file=NULL, *user=NULL;
		int result=smbio->parse(decode(soit->c_str()).ascii(), workgroup, host, share, file, user);
		if (workgroup) delete workgroup; workgroup=NULL;
		if (host) delete host; host=NULL;
		if (share) delete share; share=NULL;
		if (file) delete file; file=NULL;
		if (user) delete user; user=NULL;
		if (result==-1) {
			error( ERR_MALFORMED_URL, soit->c_str() );
			return;
		}
	}

	qDebug( "kio_smb : All source URLs ok." );
*/
	// Get a list of all source files and directories
	QValueList<Copy> files;
	QValueList<CopyDir> dirs;
	int size = 0;
	qDebug( "kio_smb : Iterating" );

	source_files_it = _source.begin();
	qDebug( "kio_smb : Looping" );
	while ( source_files_it != _source.end()) {
		qDebug( "kio_smb : Executing %s", (*source_files_it).ascii() );
		KURL usrc( (*source_files_it).ascii() );
		qDebug( "kio_smb : Parsed URL" );
		// Did an error occur ?
		int s;
// rename not used, dirChecked is argument
//		if ( ( s = listRecursive( usrc.url(), "", files, dirs, _rename ) ) == -1 ) {
		if ( ( s = listRecursive( usrc.url(), udest.url(), "", files, dirs, false ) ) == -1 ) {
			// Error message is already sent
			m_cmd = CMD_NONE;
			return;
		}
		// Sum up the total amount of bytes we have to copy
		size += s;
		++source_files_it;
	}

	qDebug( "kio_smb : Recursive 1 %s", dest.data() );

	// Check whether we do not copy a directory in itself or one of its subdirectories
	struct stat buff2;
	if ( (!strcmp(udest.protocol(),"smb") && (smbio->stat( udest.decodedURL().ascii(), &buff2 ))) != -1 ) {
		bool b_error = false, b_error_access = false;
		for ( source_files_it = _source.begin(); source_files_it != _source.end(); ++source_files_it ) {
			KURL usrc( (*source_files_it).ascii() );

			// case insensitive test of inclusion
			// for smb, url should always be complete, but workgroup might
			// be missing. The only way is to use smbio->parse(...) to get
			// the workgroups !
			char *workgroup1=0, *workgroup2=0;
			char *host1=0, *host2=0;
			char *share1=0, *share2=0;
			char *path1=0, *path2=0;
			char *user1=0, *user2=0;
			if (smbio->parse(usrc.decodedURL().ascii(),
					workgroup1, host1, share1, path1, user1)==-1) {
				b_error_access = true;
				break; // exit 'for' loop
			}
			if (smbio->parse(udest.decodedURL().ascii(),
					workgroup2, host2, share2, path2, user2)==-1) {
				b_error_access = true;
				break; // exit 'for' loop
			}
			
			// benefit from the fact that it's not possible to copy into a
			// workgroup url (would mean creating a new host!) or into a host
			// (would mean creating a new share... but even on localhost
			// we are no SMB server)
			// On the other hand, it should be possible to copy recursively
			// all the SMB network into a directory... Wow :-)
			if ( ((workgroup2) && (!host2))	|| ((host2) && (!path2)) )
				b_error_access = true;
			
			// Now is the comparison time
			if ((!workgroup1) || (!workgroup2) || (!strcasecmp(workgroup1,workgroup2))) {
				// workgroup might be the same, test hosts
				if ((host1) && (host2) && (!strcasecmp(host1,host2))) {
					// hmm, hosts are the same. test shares
					if ((share1) && (share2) && (!strcasecmp(share1,share2))) {
						// Argl, same share. now only an inclusion is enough
						if ((!path1) || (!path2)
								|| (!strncasecmp(path1,path2,strlen(path1)))
								|| (!strncasecmp(path1,path2,strlen(path2)))) {
							b_error=true;
						}
					}
				}
			}
			
			// let's be clean
			if (workgroup1) delete workgroup1;
			if (workgroup2) delete workgroup2;
			if (host1) delete host1;
			if (host2) delete host2;
			if (share1) delete share1;
			if (share2) delete share2;
			if (path1) delete path1;
			if (path2) delete path2;
			if (user1) delete user1;
			if (user2) delete user2;
			
			if ((b_error) || (b_error_access))
				break; // No need to continue
			
		}

		// Do we have a cyclic copy now ? => error
		if ( b_error ) {
			error( ERR_CYCLIC_COPY, *source_files_it );
			m_cmd = CMD_NONE;
			return;
		}
		
		// Do we have an access denied ? => error on dest url
		if ( b_error ) {
			error( ERR_ACCESS_DENIED, dest.data() );
			m_cmd = CMD_NONE;
			return;
		}
	}

	qDebug( "kio_smb : Recursive ok %s", dest.data() );

	m_cmd = CMD_GET;

	// Start a server for the destination protocol
	KIOSlave slave( exec );
	if ( slave.pid() == -1 ) {
		error( ERR_CANNOT_LAUNCH_PROCESS, exec );
		m_cmd = CMD_NONE;
		return;
	}

	// Put a protocol on top of the job
	SmbIOJob job( &slave, this );

	qDebug( "kio_smb : Job started ok %s", dest.ascii() );

	// Tell our client what we 'r' gonna do
	totalSize( size );
	totalFiles( files.count() );
	totalDirs( dirs.count() );

	int processed_files = 0;
	int processed_dirs = 0;
	int processed_size = 0;

	// Replace the relative destinations with absolute destinations
	// by prepending the destinations path
	QString tmp1 = udest.path( 1 );
	// Strip '/'
	QString tmp1_stripped = udest.path( -1 );

	QValueList<CopyDir>::Iterator dir_it = dirs.begin();
	while (dir_it != dirs.end()) {
		QString tmp2 = (*dir_it).m_strRelDest;
		if ( _rename )
			(*dir_it).m_strRelDest = tmp1_stripped;
		else
			(*dir_it).m_strRelDest = tmp1;
		(*dir_it).m_strRelDest += tmp2;
		dir_it++;
	}
	QValueList<Copy>::Iterator fit = files.begin();
	for( ; fit != files.end(); fit++ ) {
		QString tmp2 = (*fit).m_strRelDest;
		if ( _rename ) // !!! && fit->m_strRelDest == "" )
			(*fit).m_strRelDest = tmp1_stripped;
		else
			(*fit).m_strRelDest = tmp1;
		(*fit).m_strRelDest += tmp2;
	}

	qDebug( "kio_smb : Destinations ok %s", dest.data() );

	/*****
	* Make directories
	*****/

	m_bIgnoreJobErrors = true;
	bool overwrite_all = false;
	bool auto_skip = false;
	QStringList skip_list;
	QStringList overwrite_list;
	// Create all directories
	dir_it = dirs.begin();
	while (dir_it != dirs.end()) {
		// Repeat until we got no error
		do {
			job.clearError();

			KURL ud( dest );
			ud.setPath( (*dir_it).m_strRelDest );

			QString d = ud.url();

			// Is this URL on the skip list ?
			bool skip = false;
			QStringList::Iterator sit = skip_list.begin();
			for( ; sit != skip_list.end() && !skip; sit++ )
				// Is d a subdirectory of *sit ?
				if ( strncmp( *sit, d, (*sit).length() ) == 0 )
					skip = true;
			if ( skip ) continue;

			// Is this URL on the overwrite list ?
			bool overwrite = false;
			QStringList::Iterator oit = overwrite_list.begin();
			for( ; oit != overwrite_list.end() && !overwrite; oit++ )
				if ( strncmp( *oit, d, (*oit).length() ) == 0 )
					overwrite = true;
			if ( overwrite ) continue;
			
			// Tell what we are doing
			makingDir( d );

			qDebug( "kio_smb : Making remote dir %s", d.ascii() );
			// Create the directory
			job.mkdir( d, (*dir_it).m_mode );
			while( !job.hasFinished() )
				job.dispatch();

			// Did we have an error ?
			if ( job.hasError() ) {
				// Can we prompt the user and ask for a solution ?
				if ( job.errorId() == ERR_DOES_ALREADY_EXIST ) {
					QString old_path = ud.path( 1 );
					QString old_url = ud.url( 1 );
					// Should we skip automatically ?
					if ( auto_skip ) {
						job.clearError();
						// We dont want to copy files in this directory, so we
						// put it on the skip list.
						skip_list.append( old_url );
						continue;
					} else if ( overwrite_all ) {
						job.clearError();
						continue;
					}

					RenameDlg_Mode m = (RenameDlg_Mode)( M_MULTI | M_SKIP | M_OVERWRITE );
					QString tmp2 = ud.url();
					QString n;
					RenameDlg_Result r = open_RenameDlg( (*dir_it).m_strAbsSource, tmp2, m, n );
					if ( r == R_CANCEL ) {
						error( ERR_USER_CANCELED, "" );
						m_cmd = CMD_NONE;
						return;
					} else if ( r == R_RENAME ) {
						KURL u( n );
						// The Dialog should have checked this.
						if ( u.isMalformed() ) assert( 0 );
						// The new path with trailing '/'
						QString tmp3 = u.path( 1 );
						renamed( tmp3 );
	
						///////
						// Replace old path with tmp3
						///////
						QValueList<CopyDir>::Iterator dir_it2 = dir_it;
						// Change the current one and strip the trailing '/'
						(*dir_it2).m_strRelDest = u.path( -1 );
						// Change the name of all subdirectories
						dir_it2++;
						for( ; dir_it2 != dirs.end(); dir_it2++ )
						if ( strncmp( (*dir_it2).m_strRelDest, old_path, old_path.length() ) == 0 )
						(*dir_it2).m_strRelDest.replace( 0, old_path.length(), tmp3 );
						// Change all filenames
						QValueList<Copy>::Iterator fit2 = files.begin();
						for( ; fit2 != files.end(); fit2++ )
						if ( strncmp( (*fit2).m_strRelDest, old_path, old_path.length() ) == 0 )
						(*fit2).m_strRelDest.replace( 0, old_path.length(), tmp3 );
						// Dont clear error => we will repeat the current command
					} else if ( r == R_SKIP ) {
						// Skip all files and directories that start with 'old_url'
						skip_list.append( old_url );
						// Clear the error => The current command is not repeated => skipped
						job.clearError();
					} else if ( r == R_AUTO_SKIP ) {
						// Skip all files and directories that start with 'old_url'
						skip_list.append( old_url );
						// Clear the error => The current command is not repeated => skipped
						job.clearError();
						auto_skip = true;
					} else if ( r == R_OVERWRITE ) {
						// Dont bother for subdirectories
						overwrite_list.append( old_url );
						// Clear the error => The current command is not repeated => we will
						// overwrite every file in this directory or any of its subdirectories
						job.clearError();
					} else if ( r == R_OVERWRITE_ALL ) {
						job.clearError();
						overwrite_all = true;
					} else
						assert( 0 );
				}
				// No need to ask the user, so raise an error
				else {
					error( job.errorId(), job.errorText() );
					m_cmd = CMD_NONE;
					return;
				}
			}
		}
    	while( job.hasError() );
      
		processedDirs( ++processed_dirs );
		++dir_it;
	}

	qDebug( "kio_smb : Created directories %s", dest.data() );

	/*****
	* Copy files
	*****/

	time_t t_start = time( 0L );
	time_t t_last = t_start;

	fit = files.begin();
	for( ; fit != files.end(); fit++ ) {

		bool overwrite = false;
		bool skip_copying = false;

		// Repeat until we got no error
		do {
			job.clearError();

			KURL ud( dest );
			ud.setPath( (*fit).m_strRelDest );
			QString d = ud.url();

			// Is this URL on the skip list ?
			bool skip = false;
			QStringList::Iterator sit = skip_list.begin();
			for( ; sit != skip_list.end() && !skip; sit++ )
				// Is 'd' a file in directory '*sit' or one of its subdirectories ?
				if ( strncmp( *sit, d, (*sit).length() ) == 0 )
					skip = true;

			if ( skip ) continue;
    
			// What we are doing
			QString realpath = (*fit).m_strAbsSource;
			copyingFile( realpath, d );

			qDebug( "kio_smb : Writing to %s", d.ascii() );

			// Is this URL on the overwrite list ?
			QStringList::Iterator oit = overwrite_list.begin();
			for( ; oit != overwrite_list.end() && !overwrite; oit++ )
				if ( strncmp( *oit, d, (*oit).length() ) == 0 )
					overwrite = true;

			job.put( d, (*fit).m_mode, overwrite_all || overwrite,
				false, (*fit).m_size );

			while( !job.isReady() && !job.hasFinished() )
				job.dispatch();

			// Did we have an error ?
			if ( job.hasError() ) {
				int currentError = job.errorId();

				qDebug("################# COULD NOT PUT %d", currentError);
				if ( currentError != ERR_DOES_ALREADY_EXIST &&
							currentError != ERR_DOES_ALREADY_EXIST_FULL )
				{
					// Should we skip automatically ?
					if ( auto_skip ) {
						job.clearError();
						skip_copying = true;
						continue;
					}
					QString tmp2 = ud.url();
					SkipDlg_Result r;
					r = open_SkipDlg( tmp2, ( files.count() > 1 ) );
					if ( r == S_CANCEL ) {
						error( ERR_USER_CANCELED, "" );
						m_cmd = CMD_NONE;
						return;
					} else if ( r == S_SKIP ) {
						// Clear the error => The current command is not repeated => skipped
						job.clearError();
						skip_copying = true;
						continue;
					} else if ( r == S_AUTO_SKIP ) {
						// Clear the error => The current command is not repeated => skipped
						job.clearError();
						skip_copying = true;
						continue;
					} else
						assert( 0 );
				}
				// Can we prompt the user and ask for a solution ?
				else if ( /* m_bGUI && */ currentError == ERR_DOES_ALREADY_EXIST ||
							currentError == ERR_DOES_ALREADY_EXIST_FULL )
				{
					// Should we skip automatically ?
					if ( auto_skip ) {
						job.clearError();
						continue;
					}

					RenameDlg_Mode m = (RenameDlg_Mode)( M_SINGLE | M_OVERWRITE );
					if ( files.count() > 1 )
						m = (RenameDlg_Mode)( M_MULTI | M_SKIP | M_OVERWRITE );

					QString tmp2 = ud.url().data();
					QString n;
					RenameDlg_Result r = open_RenameDlg((*fit).m_strAbsSource, tmp2, m, n );

					if ( r == R_CANCEL )
					{
						error( ERR_USER_CANCELED, "" );
						m_cmd = CMD_NONE;
						return;
					}
					else if ( r == R_RENAME )
					{
						KURL u( n );
						// The Dialog should have checked this.
						if ( u.isMalformed() )
						assert( 0 );
						renamed( u.path( -1 ) );
						// Change the destination name of the current file
						(*fit).m_strRelDest = u.path( -1 );
						// Dont clear error => we will repeat the current command
					} else if ( r == R_SKIP ) {
						// Clear the error => The current command is not repeated => skipped
						job.clearError();
					}
					else if ( r == R_AUTO_SKIP )
					{
						// Clear the error => The current command is not repeated => skipped
						job.clearError();
						auto_skip = true;
					}
					else if ( r == R_OVERWRITE )
					{
						overwrite = true;
						// Dont clear error => we will repeat the current command
					}
					else if ( r == R_OVERWRITE_ALL )
					{
						overwrite_all = true;
						// Dont clear error => we will repeat the current command
					}
					else
						assert( 0 );
				}
				// No need to ask the user, so raise an error
				else {
					error( currentError, job.errorText() );
					return;
				}
			}
		}
		while( job.hasError() );

		if ( skip_copying ) continue;

		qDebug( "kio_smb : Opening %s", (*fit).m_strAbsSource.ascii() );
	
		QString sCode=(*fit).m_strAbsSource;
		KURL::decode(sCode);
		int fd = smbio->open( sCode.ascii(), O_RDONLY );
		if ( fd == -1 ) {
			error( ERR_CANNOT_OPEN_FOR_READING, (*fit).m_strAbsSource );
			m_cmd = CMD_NONE;
			return;
		}

		// You can use any buffer size for the lib, but small chuncks only here
		char buffer[ BUF_SIZE ];
		int count;
		do {
			count=smbio->read(fd, buffer, sizeof(buffer));
			if (count==-1) {
				error( ERR_COULD_NOT_READ, (*fit).m_strAbsSource );
				m_cmd = CMD_NONE;
				return;
			}
			if (count==0) break;
			job.data( buffer, count );
			processed_size += count;

			time_t t = time( 0L );
			if ( t - t_last >= 1 ) {
				processedSize( processed_size );
				speed( processed_size / ( t - t_start ) );
				t_last = t;
			}

			// Check parent
			while ( check( connection() ) )
				dispatch();
			// Check for error messages from slave
			while ( check( &slave ) )
				job.dispatch();

			// An error ?
			if ( job.hasFinished() ) {
				smbio->close( fd );
				m_cmd = CMD_NONE;
				finished();
				return;
			}

		} while (count>0);

		job.dataEnd();
		
		smbio->close( fd );

		while( !job.hasFinished() )
			job.dispatch();

		time_t t = time( 0L );

		processedSize( processed_size );
		if ( t - t_start >= 1 ) {
			speed( processed_size / ( t - t_start ) );
			t_last = t;
		}
		processedFiles( ++processed_files );
	}

	qDebug( "kio_smb : Copied files %s", dest.data() );

	if ( _move ) {
		slotDel( _source );
	}

	finished();
	m_cmd = CMD_NONE;
}

  
void SmbProtocol::slotGet( const char *_url )
{
	qDebug( "kio_smb : slotGet %s", _url );

	KURL usrc( _url );
	if ( usrc.isMalformed() ) {
		error( ERR_MALFORMED_URL, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}
/*	char *workgroup=NULL, *host=NULL, *share=NULL, *file=NULL, *user=NULL;
	int result=smbio->parse(decode(_url).ascii(), workgroup, host, share, file, user);
	if (workgroup) delete workgroup; workgroup=NULL;
	if (host) delete host; host=NULL;
	if (share) delete share; share=NULL;
	if (file) delete file; file=NULL;
	if (user) delete user; user=NULL;
	if (result==-1) {
		error( ERR_MALFORMED_URL, url.c_str() );
		return;
	}*/
	if ( strcmp(usrc.protocol(), "smb") ) {
		error( ERR_INTERNAL, "kio_smb got non-smb url in get command" );
		m_cmd = CMD_NONE;
		return;
	}

	struct stat buff;
	if ( smbio->stat( usrc.decodedURL().ascii(), &buff ) == -1 ) {
		error( ERR_DOES_NOT_EXIST, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	if ( S_ISDIR( buff.st_mode ) ) {
		error( ERR_IS_DIRECTORY, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	m_cmd = CMD_GET;
	
	qDebug( "kio_smb : Get, checkpoint 3" );

	int fd = smbio->open( usrc.decodedURL().ascii() , O_RDONLY);
	if ( fd == -1 ) {
		error( ERR_CANNOT_OPEN_FOR_READING, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	ready();

	gettingFile( _url );

	totalSize( buff.st_size );
	int processed_size = 0;
	time_t t_start = time( 0L );
	time_t t_last = t_start;

	char buffer[BUF_SIZE];
	int count;
	do {
		count=smbio->read(fd, buffer, sizeof(buffer));
		if (count==-1) {
			error( ERR_COULD_NOT_READ, strdup(_url) );
			m_cmd = CMD_NONE;
			return;
		}
		if (count==0) break;
		data( buffer, count );
		processed_size += count;

		time_t t = time( 0L );
		if ( t - t_last >= 1 ) {
			processedSize( processed_size );
			speed( processed_size / ( t - t_start ) );
			t_last = t;
		}

	} while (count>0);
	qDebug( "kio_smb : Get, checkpoint 4" );

	dataEnd();
  
	smbio->close( fd );

	processedSize( buff.st_size );
	time_t t = time( 0L );
	if ( t - t_start >= 1 )
		speed( processed_size / ( t - t_start ) );

	finished();
	m_cmd = CMD_NONE;
}


void SmbProtocol::slotGetSize( const char *_url )
{
	qDebug( "kio_smb : Getting size" );
	m_cmd = CMD_GET_SIZE;
	
	KURL usrc( _url );
	if ( usrc.isMalformed() ) {
		error( ERR_MALFORMED_URL, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}
/*	char *workgroup=NULL, *host=NULL, *share=NULL, *file=NULL, *user=NULL;
	int result=smbio->parse(decode(_url).ascii(), workgroup, host, share, file, user);
	if (workgroup) delete workgroup; workgroup=NULL;
	if (host) delete host; host=NULL;
	if (share) delete share; share=NULL;
	if (file) delete file; file=NULL;
	if (user) delete user; user=NULL;
	if (result==-1) {
		error( ERR_MALFORMED_URL, strdup(_url) );
		return;
	}*/
	if ( strcmp(usrc.protocol(), "smb") ) {
		error( ERR_INTERNAL, "kio_smb got non-smb url in get size command" );
		m_cmd = CMD_NONE;
		return;
	}

	qDebug( "kio_smb : Getting size, url OK" );
	struct stat buff;
	if ( smbio->stat( usrc.decodedURL().ascii(), &buff ) == -1 ) {
		error( ERR_DOES_NOT_EXIST, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	if ( S_ISDIR( buff.st_mode ) )  { // !!! needed ?
		error( ERR_IS_DIRECTORY, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	totalSize( buff.st_size );

	finished();
	m_cmd = CMD_NONE;
	qDebug( "kio_smb : Getting size, end" );
}


void SmbProtocol::slotPut( const char *_url, int /*_mode*/, 
			   bool _overwrite, bool _resume, int _size )
{
	QString url_orig = _url;
//	QString url_part = url_orig + ".part";

	KURL udest_orig( url_orig );
//	KURL udest_part( url_part );

//	bool m_bMarkPartial = KProtocolManager::self().markPartial();

	if ( udest_orig.isMalformed() ) {
		error( ERR_MALFORMED_URL, url_orig );
		finished();
		m_cmd = CMD_NONE;
		return;
	}

	if ( strcmp(udest_orig.protocol(), "smb") ) {
		error( ERR_INTERNAL, "kio_smb got non-smb url as destination in put command" );
	    finished();
		m_cmd = CMD_NONE;
		return;
	}

	m_cmd = CMD_PUT;

	struct stat buff;
	if ( smbio->stat( udest_orig.decodedURL().ascii(), &buff ) != -1 ) {
		// if original file exists but we are using mark partial -> rename it to XXX.part
//		if ( m_bMarkPartial )
//			rename ( udest_orig.url(), udest_part.url() );

		if ( !_overwrite && !_resume ) {
			if ( buff.st_size == _size )
				error( ERR_DOES_ALREADY_EXIST_FULL, udest_orig.decodedURL() );
			else
				error( ERR_DOES_ALREADY_EXIST, udest_orig.decodedURL() );

			finished();
			m_cmd = CMD_NONE;
			return;
		}
	}
/*	else if ( smbio->stat( udest_part.decodedURL().ascii(), &buff ) != -1 ) {
		// if file with extension .part exists but we are not using mark partial
		// -> rename XXX.part to original name
		if ( ! m_bMarkPartial )
			rename ( udest_part.url(), udest_orig.url() );

		if ( !_overwrite && !_resume ) {
			if ( buff.st_size == _size )
				error( ERR_DOES_ALREADY_EXIST_FULL, udest_part.decodedURL() );
			else
				error( ERR_DOES_ALREADY_EXIST, udest_part.decodedURL() );

			finished();
			m_cmd = CMD_NONE;
			return;
		}
	}
*/
	KURL udest;

	// if we are using marking of partial downloads -> add .part extension
/*	if ( m_bMarkPartial ) {
		qDebug( "kio_smb : Adding .part extension to %s", udest_orig.decodedURL().ascii() );
		udest = udest_part;
	} else*/
		udest = udest_orig;

	if ( _resume )
		// append if resuming
		m_fPut = smbio->open( udest.decodedURL().ascii(), O_WRONLY | O_CREAT | O_APPEND );
	else
		m_fPut = smbio->open( udest.decodedURL().ascii(), O_RDWR | O_CREAT | O_TRUNC);

	if ( m_fPut == -1 ) {
		qDebug( "kio_smb : ####################### COULD NOT WRITE %s", udest.decodedURL().ascii() );
		if ( smbio->getError() == EACCES )
			error( ERR_WRITE_ACCESS_DENIED, udest.decodedURL() );
		else
			error( ERR_CANNOT_OPEN_FOR_WRITING, udest.decodedURL() );
		m_cmd = CMD_NONE;
		finished();
		return;
	}

	// We are ready for receiving data
	ready();

	// Loop until we got 'dataEnd'
	while ( m_cmd == CMD_PUT && dispatch() );

	smbio->close( m_fPut );

	if ( smbio->stat( udest.decodedURL().ascii(), &buff ) != -1 ) {
		
		if ( buff.st_size == _size ) {
			// after full download rename the file back to original name
/*			if ( m_bMarkPartial ) {
				if ( rename( udest.url(), udest_orig.url() ) ) {
					error( ERR_CANNOT_RENAME, udest_orig.decodedURL() );
					m_cmd = CMD_NONE;
					finished();
					return;
				}
			}
*/
    } // if the size is less than minimum -> delete the file
	else if ( buff.st_size < KProtocolManager::self().minimumKeepSize() ) {
		remove( udest.url() );
	}
  }

  // We have done our job => finish
  finished();

  m_cmd = CMD_NONE;
}


void SmbProtocol::slotDel( QStringList& _source )
{
	// Check whether the URLs are wellformed
	QStringList::Iterator source_it = _source.begin();
	for( ; source_it != _source.end(); ++source_it ) {
		qDebug( "kio_smb : Checking %s", (*source_it).ascii() );
		KURL usrc( *source_it );
		if ( usrc.isMalformed() ) {
			error( ERR_MALFORMED_URL, *source_it );
			m_cmd = CMD_NONE;
			return;
		}
		if ( strcmp(usrc.protocol(), "smb") ) {
			error( ERR_INTERNAL, "kio_smb got non-smb url in delete command" );
			finished();
			m_cmd = CMD_NONE;
			return;
		}
	}

	qDebug( "kio_smb : All URLs ok" );

	// Get a list of all source files and directories
	QValueList<Copy> fs;
	QValueList<CopyDir> ds;
	int size = 0;
	qDebug( "kio_smb : Iterating" );

	source_it = _source.begin();
	qDebug( "kio_smb : Looping" );
	for( ; source_it != _source.end(); ++source_it ) {
      	        // struct stat stat_buf;
		qDebug( "kio_smb : Checking %s", (*source_it).ascii() );
		KURL victim( (*source_it) );
		int s;
		// rename not used, dirChecked is argument
		if ( ( s = listRecursive( victim.url(), "", "", fs, ds, false ) ) == -1 ) {
			// Error message is already sent
			m_cmd = CMD_NONE;
			return;
		}
		// Sum up the total amount of bytes we have to copy
		size += s;
		qDebug( "kio_smb : Parsed URL OK and added to appropiate list" );
	}

	qDebug( "kio_smb : Recursive ok" );

	if ( fs.count() == 1 )
		m_cmd = CMD_DEL;
	else
		m_cmd = CMD_MDEL;

	// Tell our client what we 'r' gonna do
	totalSize( size );
	totalFiles( fs.count() );
	totalDirs( ds.count() );

	/*****
	* Delete files
	*****/

	QValueList<Copy>::Iterator fit = fs.begin();
	for( ; fit != fs.end(); fit++ ) {
		// build full valid smb url
		KURL filename = (*fit).m_strAbsSource;
		qDebug( "kio_smb : Deleting file %s", filename.decodedURL().ascii() );

		deletingFile( filename.decodedURL() );
		qDebug( "kio_smb : Deleting file 2" );

		if ( smbio->unlink( filename.decodedURL().ascii() ) == -1 ) {
		qDebug( "kio_smb : Deleting file 3" );
			error( ERR_CANNOT_DELETE, filename.decodedURL() );
			m_cmd = CMD_NONE;
			return;
		}
		qDebug( "kio_smb : Deleting file 4" );
	}
	qDebug( "kio_smb : Deleting file 5" );

	/*****
	* Delete empty directories
	*****/                       		

	QValueList<CopyDir>::Iterator dit = ds.end();
	qDebug( "kio_smb : Deleting dir 1" );
	if (!ds.isEmpty()) do {
		dit--;
		qDebug( "kio_smb : Deleting dir 2" );
		// build full valid smb url
		KURL dirname = (*dit).m_strAbsSource;
		qDebug( "kio_smb : Deleting directory %s", dirname.decodedURL().ascii() );

		deletingFile( dirname.decodedURL() );
		qDebug( "kio_smb : Deleting dir 3" );

		if ( smbio->rmdir( dirname.decodedURL().ascii() ) == -1 ) {
			qDebug( "kio_smb : Deleting dir 4" );
			error( ERR_COULD_NOT_RMDIR, dirname.decodedURL() );
			m_cmd = CMD_NONE;
			return;
		}
		qDebug( "kio_smb : Deleting dir 5" );
	} while (dit != ds.begin());
	qDebug( "kio_smb : Deleting dir 6" );

	finished();

	m_cmd = CMD_NONE;
}


void SmbProtocol::slotListDir( const char *_url )
{
	qDebug( "kio_smb : listDir 1 %s", _url);
	
	KURL usrc( _url );
	if ( usrc.isMalformed() ) {
		error( ERR_MALFORMED_URL, strdup(_url) );
		return;
	}
	qDebug( "kio_smb : listDir 2 %s", _url);
/*	char *workgroup=NULL, *host=NULL, *share=NULL, *file=NULL, *user=NULL;
	int result=smbio->parse(decode(_url).ascii(), workgroup, host, share, file, user);
	if (workgroup) delete workgroup; workgroup=NULL;
	if (host) delete host; host=NULL;
	if (share) delete share; share=NULL;
	if (file) delete file; file=NULL;
	if (user) delete user; user=NULL;
	if (result==-1) {
		error( ERR_MALFORMED_URL, strdup(_url) );
		return;
	}*/
	if ( strcmp(usrc.protocol(), "smb") ) {
		error( ERR_INTERNAL, "kio_smb got non-smb url in list command" );
		finished();
		m_cmd = CMD_NONE;
		return;
	}
	qDebug( "kio_smb : listDir 3 %s", _url);

	struct stat buff;
	if ( smbio->stat( usrc.decodedURL().ascii(), &buff ) == -1 ) {
		error( ERR_DOES_NOT_EXIST, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	if ( !S_ISDIR( buff.st_mode ) ) {
		error( ERR_IS_FILE, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	m_cmd = CMD_LIST;

	struct SMBdirent *ep;
	int dp = smbio->opendir( usrc.decodedURL().ascii() );
	if ( dp == -1 ) {
		error( ERR_CANNOT_ENTER_DIRECTORY, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	while ( ( ep = smbio->readdir( dp ) ) != 0L ) {
		if ( strcmp( ep->d_name, "." ) == 0 || strcmp( ep->d_name, ".." ) == 0 )
			continue;

		qDebug( "kio_smb : Listing %s", ep->d_name );

		KUDSEntry entry;
		KUDSAtom atom;
		atom.m_uds = UDS_NAME;
		atom.m_str = ep->d_name;
		entry.append( atom );

		atom.m_uds = UDS_FILE_TYPE;
		atom.m_long = ep->st_mode;
		entry.append( atom );
		atom.m_uds = UDS_SIZE;
		atom.m_long = ep->st_size;
		entry.append( atom );
		atom.m_uds = UDS_MODIFICATION_TIME;
		atom.m_long = ep->st_mtime;
		entry.append( atom );
		atom.m_uds = UDS_ACCESS;
		atom.m_long = ep->st_mode;
		entry.append( atom );
		atom.m_uds = UDS_ACCESS_TIME;
		atom.m_long = ep->st_atime;
		entry.append( atom );
		atom.m_uds = UDS_CREATION_TIME;
		atom.m_long = ep->st_ctime;
		entry.append( atom );

		listEntry( entry );
	}

	smbio->closedir( dp );

	finished();
	m_cmd = CMD_NONE;
}


void SmbProtocol::slotTestDir( const char *_url )
{
	qDebug( "kio_smb : testing %s", _url );
	
	KURL usrc( _url );
	if ( usrc.isMalformed() ) {
		error( ERR_MALFORMED_URL, strdup(_url) );
		return;
	}
/*	qDebug( "kio_smb : testing %s, kurl OK", decode(_url).ascii() );
	char *workgroup=NULL, *host=NULL, *share=NULL, *file=NULL, *user=NULL;
	int result=smbio->parse(decode(_url).ascii(), workgroup, host, share, file, user);
	if (workgroup) delete workgroup; workgroup=NULL;
	if (host) delete host; host=NULL;
	if (share) delete share; share=NULL;
	if (file) delete file; file=NULL;
	if (user) delete user; user=NULL;
	if (result==-1) {
		error( ERR_MALFORMED_URL, url.c_str() );
		return;
	}*/
	if ( strcmp(usrc.protocol(), "smb") ) {
		error( ERR_INTERNAL, "kio_smb got non-smb url in testdir command" );
		finished();
		m_cmd = CMD_NONE;
		return;
	}
	qDebug( "kio_smb : testing %s, smburl OK", usrc.decodedURL().ascii() );

	struct stat buff;
	if ( smbio->stat( usrc.decodedURL().ascii(), &buff ) == -1 ) {
		error( ERR_DOES_NOT_EXIST, strdup(_url) );
		m_cmd = CMD_NONE;
		return;
	}

	if ( S_ISDIR( buff.st_mode ) )
		isDirectory();
	else
		isFile();

	finished();
	qDebug( "kio_smb : testing %s, end", _url );
}

void SmbProtocol::slotData( void *_p, int _len )
{
	switch( m_cmd ) {
		case CMD_PUT:
			smbio->write( m_fPut, _p, _len);
			break;
	}
}

void SmbProtocol::slotDataEnd()
{
	switch( m_cmd ) {
		case CMD_PUT:
			m_cmd = CMD_NONE;
			break;
	}
}

// both src and dest arguments should be full encoded urls
long SmbProtocol::listRecursive( const char *smbURL, const char *dest, QString relDestAdd,
	QValueList<Copy>& _files, QValueList<CopyDir>& _dirs, bool dirChecked )
{
	KURL udest(dest);
	KURL usrc(smbURL);
	qDebug( "kio_smb : dest argument : %s", dest);
	if (!dirChecked) {
		struct stat statBuf;
		if (smbio->stat(usrc.decodedURL().ascii(),&statBuf)==-1) {
			error( ERR_DOES_NOT_EXIST, strdup(smbURL) );
			return -1;
		}

		if ( !S_ISDIR(statBuf.st_mode) ) {
			Copy c;
			c.m_strAbsSource = smbURL;
			c.m_strRelDest = relDestAdd + usrc.filename();
			qDebug( "kio_smb : dest file name : %s", c.m_strRelDest.ascii());
			c.m_mode = statBuf.st_mode;
			c.m_size = statBuf.st_size;
			_files.append( c );
			return statBuf.st_size;
		}
	}

	int dd=smbio->opendir(usrc.decodedURL().ascii());
	if (dd==-1) {
		qDebug( "kio_smb : %s should have been a valid directory!", smbURL);
		return -1;
	}
	
	relDestAdd += usrc.filename(); // directory() ? I don't think so

	CopyDir c;
	c.m_strAbsSource = smbURL;
	c.m_strRelDest = relDestAdd;
	c.m_mode = 04755; //statBuf.st_mode;
	_dirs.append( c );

	SMBdirent *dent;
	long totalSize=0;
	QStringList newDirs;
	
	// First add all files in this dir. We'll do directories later, so as to
	// minimise SMB 'cd' commands and benefit from libsmb internal cache
	while ((dent=smbio->readdir(dd))) {
		// Guess what happened the first time I tried to download a directory ?
		if ((dent->d_name) && strcmp(dent->d_name,".") && strcmp(dent->d_name,"..")) {
			QString newName(dent->d_name);
			if ( !S_ISDIR(dent->st_mode) ) {
				Copy c;
				KURL newSrc = usrc;
				newSrc.addPath(newName); // add decoded name
				c.m_strAbsSource = newSrc.url();
				// trick : get the encoded filename and discards the src...
				c.m_strRelDest = relDestAdd + "/" + newSrc.filename();
				qDebug( "kio_smb : dest file name : %s", c.m_strRelDest.ascii());
				if ( c.m_strRelDest.isEmpty() ) return -1;
				c.m_mode = dent->st_mode;
				c.m_size = dent->st_size;
				_files.append( c );
				totalSize+=dent->st_size;
			} else newDirs.append(dent->d_name);
		}
	}
	smbio->closedir(dd);

	// Now we can go into each directory found recursively
	QStringList::Iterator ndit = newDirs.begin();
	for( ; ndit != newDirs.end(); ++ndit ) {
		QString ndName = (*ndit);
		KURL newSrcURL = usrc;
		KURL newDestURL = udest;
		newSrcURL.addPath(ndName);
		newDestURL.addPath(ndName);
		QString newSrc = newSrcURL.url();
		QString newDest = newDestURL.url();
		int plus=listRecursive(newSrc, newDest, relDestAdd + "/"+ (ndName+"/"), _files, _dirs, true);
		if (plus == -1) {
			qDebug( "kio_smb : recursive copy error, aborting..." );
			return -1;
		}
		totalSize+=plus;
	}
	
	return totalSize;
}


void SmbProtocol::jobError( int _errid, const char *_txt )
{
	if ( !m_bIgnoreJobErrors )
	    error( _errid, _txt );
}

/*************************************
 *
 * SmbIOJob
 *
 *************************************/

SmbIOJob::SmbIOJob( KIOConnection *_conn, SmbProtocol *_Smb ) : KIOJobBase( _conn )
{
  m_pSmb = _Smb;
}
  
void SmbIOJob::slotError( int _errid, const char *_txt )
{
  KIOJobBase::slotError( _errid, _txt );
  m_pSmb->jobError( _errid, _txt );
}

// utility

int check( KIOConnection *_con )
{
  int err;
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  fd_set rfds;
  FD_ZERO( &rfds );
  FD_SET( _con->inFD(), &rfds );

again:
  if ( ( err = ::select( _con->inFD(), &rfds, 0L, 0L, &tv ) ) == -1 && errno == EINTR )
    goto again;

  // No error and something to read ?
  if ( err != -1 && err != 0 )
    return 1;

  return 0;
}

