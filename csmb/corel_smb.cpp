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
#include <dlfcn.h>

#include <qvaluelist.h>
#include <qtextstream.h>

#include <kshred.h>
#include <kdebug.h>
#include <kurl.h>
#include <kprotocolmanager.h>
#include <kmimemagic.h>
#include <kinstance.h>
#include <kapp.h>

#include <kconfig.h>
#include <qlist.h>
#include <kdebug.h>

#include <fileUNC.h>
#include <smbutil.h>
#include <smbfile.h>
#include <smbshare.h>
#include <copymove.h>
#include <common.h>
#include <credentials.h>
#include <PasswordDlg.h>

#include "corel_smb.h"

using namespace KIO;


extern "C" { int kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv)
{
	KApplication app(argc, argv, "kio_corel_smb");

	KInstance instance("kio_corel_smb");

	if (argc != 4)
	{
		fprintf(stderr, "Usage: kio_smb protocol domain-socket1 domain-socket2\n");
		exit(-1);
	}

	CSmbProtocol slave(argv[2], argv[3]);
	slave.dispatchLoop();

	return 0;
}


CSmbProtocol::CSmbProtocol(const QCString& _pool, const QCString& _app):SlaveBase("smb", _pool, _app)
{
  nCredentialsIndex = 0;
  CCredentials cred("", "", "");
  gCredentials.Add(cred);
  currentHost=QString::null;
  currentIP=QString::null;
  currentUser=QString::null;
  currentPass=QString::null;

}


CSmbProtocol::~CSmbProtocol()
{

}

void CSmbProtocol::setHost(const QString& _host, int _ip, const QString& _user, const QString& _pass)
{
	//kDebugInfo( 7106, "in set host method");
	currentHost=_host;
	currentIP=_ip;
	currentUser=_user;
	currentPass=_pass;
}

QString CSmbProtocol::buildFullLibURL(const KURL &_pathArg)
{

	QString st;

	KURL url = KURL(_pathArg);
	if (url.url().left(3) == "smb")
	{
		st = url.url();
		st.remove(0,4);
	}
	else
		if (url.url().left(4) == "file")
		{
			st = url.url();
			st.remove(0,5);
		}
		else
			st = url.url();
	if (st[0] == '/' && st[1] != '/')
		st.prepend('/');
	URLDecode(st);
	return st;
}


void CSmbProtocol::mkdir(const KURL& _pathArg, int /*permissions*/)
{
	QString path = buildFullLibURL(_pathArg);

	struct stat buff;
	if (-1 == U_Stat1((LPCSTR)path
#ifdef QT_20
		.latin1()
#endif
						, &buff))
	{
		CSMBErrorCode err;
		if (keSuccess != (err = SMBMkdir((LPCSTR)path
#ifdef QT_20
			.latin1()
#endif
						, 0)))
		{
  		if (err == keErrorAccessDenied)
			{
				error(KIO::ERR_ACCESS_DENIED, path);
				return;
			}
			else
			{
				error(KIO::ERR_COULD_NOT_MKDIR, path);
				return;
			}
		}
	finished();
	}

	if (S_ISDIR(buff.st_mode))
	{
		error(KIO::ERR_DIR_ALREADY_EXIST, path);
		return;
	}
}

void CSmbProtocol::get(const KURL& pathArg)
{
	QString path = buildFullLibURL(pathArg);

	struct stat buff;
	if (-1 == U_Stat1((LPCSTR)path
#ifdef QT_20
			.latin1()
#endif
														, &buff))
	{
		error(KIO::ERR_ACCESS_DENIED, path);
		return;
	}

	if (S_ISDIR(buff.st_mode))
	{
		error(KIO::ERR_IS_DIRECTORY, path);
		return;
	}

int fd = open((LPCSTR)path
#ifdef QT_20
			.latin1()
#endif
															, O_RDONLY);
	if (-1 == fd)
	{
		error(KIO::ERR_CANNOT_OPEN_FOR_READING, path);
		return;
	}



	totalSize(buff.st_size);
	int processed_size = 0;
	time_t t_start = time(0L);
	time_t t_last = t_start;

	char buffer[4090];
	QByteArray array;
  bool mimetypeEmitted = false;

	int n;
	while((n = read(fd, buffer, 2048)) > 0)
	{
		array.setRawData(buffer, n);
		if (!mimetypeEmitted)
		{
			KMimeMagicResult * result = KMimeMagic::self()->findBufferFileType(array, pathArg.fileName());
			mimeType(result->mimeType());
			mimetypeEmitted = true;
		}

		data(array);
		array.resetRawData(buffer, n);

		processed_size += n;
		time_t t = time(0L);
		if (t - t_last >= 1)
		{
			processedSize(processed_size);
			speed(processed_size / (t - t_start));
			t_last = t;
		}
	}

	if (-1 == n)
	{
		error(KIO::ERR_COULD_NOT_READ, path);
		close(fd);
		return;
	}

	data(QByteArray());
	close(fd);
	processedSize(buff.st_size);
	time_t t = time(0L);

	if (t - t_start >= 1)
		speed(processed_size / (t - t_start));

	finished();
}


void CSmbProtocol::put(const KURL& _dest_orig_arg, int _mode, bool _overwrite,bool _resume )
{

  QString dest_orig = buildFullLibURL(_dest_orig_arg.path());
	struct stat buff_orig;
	bool orig_exists = (-1 != U_Stat1( (LPCSTR)dest_orig
#ifdef QT_20
			.latin1()
#endif
																											, &buff_orig ));
	if (orig_exists &&  !_overwrite && !_resume)
	{
		if (S_ISDIR(buff_orig.st_mode))
			error(KIO::ERR_DIR_ALREADY_EXIST, dest_orig);
		else
			error(KIO::ERR_FILE_ALREADY_EXIST, dest_orig);
		return;
	}


  QString dest = dest_orig;
	if ( orig_exists && !_resume )
	{
	  kdDebug(7106) << "Deleting destination file " << dest_orig << endl;
		remove((LPCSTR)dest_orig
#ifdef QT_20
			.latin1()
#endif
														 );

	}

	if ( _resume ) {
		m_fPut = open((LPCSTR)dest
#ifdef QT_20
			.latin1()
#endif
																, O_WRONLY | O_APPEND);
 	}
  else
  {
		m_fPut = open((LPCSTR)dest
#ifdef QT_20
			.latin1()
#endif
																, O_CREAT | O_TRUNC | O_RDWR);
	}


	if ( m_fPut == -1 )
  {
	  kdDebug(7106) << "####################### COULD NOT WRITE " << dest << endl;
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
			write( m_fPut, buffer.data(), buffer.size());
		}
	}
	while ( result > 0 );


	if (result != 0)
	{
		close(m_fPut);
		kdDebug( 7106 ) << "Error during 'put'. Aborting." << endl;
		::exit(255);
	}

	if (close(m_fPut))
	{
		error( KIO::ERR_COULD_NOT_WRITE, dest_orig);
		return;
	}

	// We have done our job => finish
	finished();


}

void CSmbProtocol::rename(const KURL& _srcArg, const KURL& _destArg, bool _overwrite)
{
	QString src = buildFullLibURL(_srcArg);
	QString dest = buildFullLibURL(_destArg);

	struct stat buff_src;
	if (-1 == U_Stat1((LPCSTR)src
#ifdef QT_20
				.latin1()
#endif
								, &buff_src))
	{
		error(KIO::ERR_ACCESS_DENIED, src);
		return;
	}

	struct stat buff_dest;
	bool dest_exists = (-1 == U_Stat1((LPCSTR)dest
#ifdef QT_20
				.latin1()
#endif
							, &buff_dest));
	if (dest_exists)
	{
		if (S_ISDIR(buff_dest.st_mode))
		{
			error(KIO::ERR_DIR_ALREADY_EXIST, dest);
			return;
		}

		if (!_overwrite)
		{
			error(KIO::ERR_FILE_ALREADY_EXIST, dest);
			return;
		}
	}

	CSMBErrorCode err;
	if((err=SMBRename((LPCSTR)src
#ifdef QT_20
			.latin1()
#endif
						,(LPCSTR)dest
#ifdef QT_20
									.latin1()
#endif
												,0)) != keSuccess)
	{
		if (err == keErrorAccessDenied)
		{
			error(KIO::ERR_ACCESS_DENIED, dest);
			return;
		}
		else
		{
			error(KIO::ERR_CANNOT_RENAME, src);
			return;
		}
	}

	finished();

}


void CSmbProtocol::del(const KURL& _pathArg, bool _isfile)
{
	QString path = buildFullLibURL(_pathArg);
	struct stat buff_src;
	if ( U_Stat1((LPCSTR)path
	#ifdef QT_20
			.latin1()
	#endif
					, &buff_src ) == -1 )
		{
			error( KIO::ERR_ACCESS_DENIED, path );
			return;
		}

	/*****
	* Delete files
	*****/
	if (_isfile)
	{
		if (-1 == U_Remove((LPCSTR)path
#ifdef QT_20
			.latin1()
#endif
						))
		{
			error(KIO::ERR_CANNOT_DELETE, path);
			return;
		}

	}
	else
	{
	/*****
	* Delete empty directory
	*****/
		if (-1 == rmdir((LPCSTR)path
#ifdef QT_20
			.latin1()
#endif
					 ))
		{
			error(KIO::ERR_COULD_NOT_RMDIR, path);
			return;
		}
	}
	finished();
}



void CSmbProtocol::createUDSEntry(bool _isDir, const QString& _filename, const QString& _path, UDSEntry& _entry)
{
	assert(_entry.count() == 0); // by contract
	UDSAtom atom;
	atom.m_uds = KIO::UDS_NAME;
	atom.m_str = _filename;
	_entry.append(atom);

	mode_t type;
	mode_t access;

	struct stat buff;
	if (-1 == U_Stat1((LPCSTR)_path
#ifdef QT_20
		.latin1()
#endif
			, &buff))
	{
		//kdDebug( 7106 ) << "cannot stat in createUDSEntry!!!" << endl;
		return;
	}

	struct stat buff2;
	U_Stat1((LPCSTR)_filename
#ifdef QT_20
		.latin1()
#endif
			, &buff2);

	if (_isDir)
	{
		type = buff.st_mode & S_IFMT; // extract file type
		access = buff.st_mode & 0x1FF; // extract permissions
	}
	else
	{
		type = buff2.st_mode & S_IFMT; // extract file type
		access = buff2.st_mode & 0x1FF; // extract permissions
	}

	atom.m_uds = KIO::UDS_FILE_TYPE;
	atom.m_long = type;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_ACCESS;
	atom.m_long = access;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_SIZE;
	if (_isDir)
		atom.m_long = buff.st_size;
	else
		atom.m_long = buff2.st_size;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_MODIFICATION_TIME;
	if (_isDir)
		atom.m_long = buff.st_mtime;
	else
		atom.m_long = buff2.st_mtime;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_ACCESS_TIME;
	if (_isDir)
		atom.m_long = buff.st_atime;
	else
		atom.m_long = buff2.st_atime;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_CREATION_TIME;
	if (_isDir)
		atom.m_long = buff.st_ctime;
	else
		atom.m_long = buff2.st_ctime;
	_entry.append(atom);
}


void CSmbProtocol::createUDSEntry(const QString& _filename, const QString& _path, UDSEntry& _entry)
{
	assert(_entry.count() == 0); // by contract
	UDSAtom atom;
	atom.m_uds = KIO::UDS_NAME;
	atom.m_str = _filename;
	_entry.append(atom);

	mode_t type;
	mode_t access;

	struct stat buff;

	if (-1 == U_Stat1((LPCSTR)_path
#ifdef QT_20
		.latin1()
#endif
			, &buff))
	{
		//kdDebug( 7106 ) << "cannot stat in createUDSEntry!!!" << endl;
		return;
	}

	type = buff.st_mode & S_IFMT; // extract file type
	access = buff.st_mode & 0x1FF; // extract permissions

	atom.m_uds = KIO::UDS_FILE_TYPE;
	atom.m_long = type;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_ACCESS;
	atom.m_long = access;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_SIZE;
	atom.m_long = buff.st_size;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_MODIFICATION_TIME;
	atom.m_long = buff.st_mtime;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_ACCESS_TIME;
	atom.m_long = buff.st_atime;
	_entry.append(atom);

	atom.m_uds = KIO::UDS_CREATION_TIME;
	atom.m_long = buff.st_ctime;
	_entry.append(atom);
}

void CSmbProtocol::createUDSEntry(const QString& _filename, UDSEntry& _entry)
{
	assert(_entry.count() == 0); // by contract
	UDSAtom atom;
	atom.m_uds = KIO::UDS_NAME;
	atom.m_str = _filename;
	_entry.append(atom);

	mode_t type;
	mode_t access;

	struct stat buff;

	if (-1 == U_Stat1("/usr", &buff))
	{
		//kdDebug( 7106 ) << "cannot stat in createUDSEntry!!!" << endl;
		return;
	}

	type = buff.st_mode & S_IFMT; // extract file type
	access = buff.st_mode & 0x1FF; // extract permissions

	atom.m_uds = KIO::UDS_FILE_TYPE;
	atom.m_long = type;
	_entry.append(atom);

}


void CSmbProtocol::stat(const KURL & _pathArg)
{
	QString path = buildFullLibURL(_pathArg);

	//kDebugInfo( 7106, "entering stat %s", debugString(path));

	QString Server,Share,Path;
	ParseUNCPath((LPCSTR)path
#ifdef QT_20
			.latin1()
#endif
														,Server,Share,Path);
	UDSEntry entry ;

	if (Share.isEmpty())
	{
		createUDSEntry(path, entry);
	}
	else
	{
    struct stat buff;
		if (-1 == U_Stat1(path
#ifdef QT_20
				.latin1()
#endif
							, &buff))
		{
			error(KIO::ERR_DOES_NOT_EXIST, path);
			return;
		}

	// Extract filename out of path
		QString filename = KURL((LPCSTR)path
#ifdef QT_20
				.latin1()
#endif
							).filename();

		createUDSEntry(filename, path, entry);
	///////// debug code
	}

	KIO::UDSEntry::ConstIterator it = entry.begin();
	for(; it != entry.end(); it++)
	{
		switch ((*it).m_uds)
		{
			case KIO::UDS_FILE_TYPE:
			//	kDebugInfo("File Type : %d", (mode_t)((*it).m_long) );
				break;

			case KIO::UDS_ACCESS:
			//	kDebugInfo("Access permissions : %d", (mode_t)((*it).m_long) );
				break;

			case KIO::UDS_USER:
			//	kDebugInfo("User : %s", ((*it).m_str.ascii() ) );
				break;

			case KIO::UDS_GROUP:
			//	kDebugInfo("Group : %s", ((*it).m_str.ascii() ) );
				break;

			case KIO::UDS_NAME:
			//	kDebugInfo("Name : %s", ((*it).m_str.ascii() ) );
				break;

			case KIO::UDS_URL:
			//	kDebugInfo("URL : %s", ((*it).m_str.ascii() ) );
				break;

			case KIO::UDS_MIME_TYPE:
			//	kDebugInfo("MimeType : %s", ((*it).m_str.ascii() ) );
				break;

			case KIO::UDS_LINK_DEST:
			//	kDebugInfo("LinkDest : %s", ((*it).m_str.ascii() ) );
					break;
		}
	}

/////////
	statEntry(entry);

  	finished();
}

void CSmbProtocol::listDir(const KURL& _pathArg)
{

	QString path = buildFullLibURL(_pathArg);

  //kDebugInfo( 7106, "=============== LIST %s ===============", debugString(path) );

	QString Server, Share, Path, UserName, Domain;
  UserName = getenv("CURRENTUSER");
	Domain = getenv("CURRENTWORKGROUP");
	ParseUNCPath((LPCSTR)path
#ifdef QT_20
			.latin1()
#endif
														,Server, Share, Path);

	if (Share.isEmpty())
	{
		CShareArray list;
		CSMBErrorCode err;

DoShareAgain:;
		if((err = GetShareList((LPCSTR)path
#ifdef QT_20
				.latin1()
#endif
							,&list, nCredentialsIndex)) != keSuccess)
		{
			if (err == keErrorAccessDenied)
			{
				CPasswordDlg dlg((LPCSTR)path
#ifdef QT_20
					.latin1()
#endif
																			, (LPCSTR)Domain
#ifdef QT_20
					.latin1()
#endif
                                                        , (LPCSTR)UserName
#ifdef QT_20
					.latin1()
#endif
                                                                          );

						switch (dlg.exec())
						{
							case 1:
							{
								CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);

								nCredentialsIndex = gCredentials.Find(cred);


								if (nCredentialsIndex == -1)
									nCredentialsIndex = gCredentials.Add(cred);
								else
									if (gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
										gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
							}
							goto DoShareAgain;

							default: // Quit or Escape
								return ;
						}

				//return;
			}
			else if (err == keStoppedByUser)
			{
				return;
			}
		}

		UDSEntry entry;
		for (int i=0; i < list.count(); i++)
		{
			QString zzz = (LPCSTR)list[i].m_ShareName
#ifdef QT_20
				.latin1()
#endif
								;
			URLDecode(zzz);

			entry.clear();
			createUDSEntry(zzz, entry);
			listEntry(entry, false);
		}
		listEntry( entry, true );
	}
	else
	{
    CFileArray list;
		CSMBErrorCode err;

		  struct stat buff;
      if (-1 == U_Stat1((LPCSTR)path
#ifdef QT_20
			  .latin1()
#endif
				  													, &buff))

		  {
			  error(KIO::ERR_DOES_NOT_EXIST, path);
			  return;
		  }

		  if (!S_ISDIR(buff.st_mode))
		  {
			  error(KIO::ERR_IS_FILE, path);
			  return;
		  }


DoFileAgain:;

		if((err = GetFileList((LPCSTR)path
#ifdef QT_20
					.latin1()
#endif
																			,&list, nCredentialsIndex, TRUE))!=keSuccess)
		{
			if (err == keErrorAccessDenied)
			{
				CPasswordDlg dlg((LPCSTR)path
#ifdef QT_20
					.latin1()
#endif
																			, (LPCSTR)Domain
#ifdef QT_20
					.latin1()
#endif
                                                      , (LPCSTR)UserName
#ifdef QT_20
					.latin1()
#endif
                                                                        );

						switch (dlg.exec())
						{
							case 1:
							{
								CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);

								nCredentialsIndex = gCredentials.Find(cred);

								if (nCredentialsIndex == -1)
									nCredentialsIndex = gCredentials.Add(cred);
								else
									if (gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
										gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
							}
							goto DoFileAgain;

							default: // Quit or Escape
								return ;
						}

				//return;

			}
			else if (err == keStoppedByUser)
			{
				return;
			}
		}
		UDSEntry entry;
		for (int i=0; i < list.count(); i++)
		{
			QString zzz = (LPCSTR)list[i].m_FileName
#ifdef QT_20
					.latin1()
#endif
																								;
			URLDecode(zzz);
      entry.clear();
			createUDSEntry((int)list[i].IsFolder(), zzz, path, entry);
			listEntry(entry, false);

		}

//	UDSEntry entry;
		listEntry( entry, true );
	}
	finished();

}

