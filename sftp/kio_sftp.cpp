/***************************************************************************
                          sftp.cpp  -  description
                             -------------------
    begin                : Fri Jun 29 23:45:40 CDT 2001
    copyright            : (C) 2001 by Lucas Fisher
    email                : ljfisher@purdue.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
DEBUGGING
We are pretty much left with kdDebug messages for debugging. We can't use a gdb
as described in the ioslave DEBUG.howto because kdeinit has to run in a terminal.
Ssh will detect this terminal and ask for a password there, but will just get garbage.
So we can't connect.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>

#include <qcstring.h>
#include <qstring.h>
#include <qobject.h>
#include <qstrlist.h>
#include <qfile.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <pwd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <kio/ioslave_defaults.h>
#include <kmimetype.h>
#include <kmimemagic.h>
#include <klargefile.h>

#include "sftp.h"
#include "kio_sftp.h"
#include "atomicio.h"
#include "sftpfileattr.h"
#include "ksshprocess.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define RETRIES 3


using namespace KIO;
extern "C"
{
  int kdemain( int argc, char **argv )
  {
    KInstance instance( "kio_sftp" );

    kdDebug(KIO_SFTP_DB) << "*** Starting kio_sftp " << endl;

    if (argc != 4) {
      kdDebug(KIO_SFTP_DB) << "Usage: kio_sftp  protocol domain-socket1 domain-socket2" << endl;
      exit(-1);
    }

    kio_sftpProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    kdDebug(KIO_SFTP_DB) << "*** kio_sftp Done" << endl;
    return 0;
  }
}

static void mymemcpy(const char* b, QByteArray& a, unsigned int offset, unsigned int len) {
    for(unsigned int i = 0; i < len; i++) {
        a[offset+i] = b[i];
    }
}

/*
 * This helper handles some special issues (blocking and interrupted
 * system call) when writing to a file handle.
 *
 * @return 0 on success or an error code on failure (ERR_COULD_NOT_WRITE,
 * ERR_DISK_FULL, ERR_CONNECTION_BROKEN).
 */
static int writeToFile (int fd, const char *buf, size_t len)
{
  while (len > 0)
  {  
    ssize_t written = ::write(fd, buf, len);
    if (written >= 0)
    {   
        buf += written;
        len -= written;
        continue;
    }
    
    switch(errno)
    {   
      case EINTR:
        continue;
      case EPIPE:
        return ERR_CONNECTION_BROKEN;
      case ENOSPC:
        return ERR_DISK_FULL;
      default:
        return ERR_COULD_NOT_WRITE;
    }
  }
  return 0;
}

kio_sftpProtocol::kio_sftpProtocol(const QCString &pool_socket, const QCString &app_socket)
                 :QObject(), SlaveBase("kio_sftp", pool_socket, app_socket),
                  mConnected(false), mPort(-1), mMsgId(0) {
  kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::kio_sftpProtocol(): pid = " << getpid() << endl;
}


kio_sftpProtocol::~kio_sftpProtocol() {
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::~kio_sftpProtocol(): pid = " << getpid() << endl;
    closeConnection();
}

QString kio_sftpProtocol::getCurrentUsername() {
  struct passwd *pw;
  
  pw = getpwuid(getuid());
  if (pw == NULL) {
    return QString::null;
  }
  
  kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::getCurrentUsername(): pw_name = "
    << pw->pw_name << endl;
  return QString(pw->pw_name);
}

bool kio_sftpProtocol::isSupportedOperation(int type) {
  switch (type) {
    case SSH2_FXP_VERSION:
    case SSH2_FXP_STATUS:
    case SSH2_FXP_HANDLE:
    case SSH2_FXP_DATA:
    case SSH2_FXP_NAME:
    case SSH2_FXP_ATTRS:
    case SSH2_FXP_INIT:
    case SSH2_FXP_OPEN:
    case SSH2_FXP_CLOSE:
    case SSH2_FXP_READ:
    case SSH2_FXP_WRITE:
    case SSH2_FXP_LSTAT:
    case SSH2_FXP_FSTAT:
    case SSH2_FXP_SETSTAT:
    case SSH2_FXP_FSETSTAT:
    case SSH2_FXP_OPENDIR:
    case SSH2_FXP_READDIR:
    case SSH2_FXP_REMOVE:
    case SSH2_FXP_MKDIR:
    case SSH2_FXP_RMDIR:
    case SSH2_FXP_REALPATH:
    case SSH2_FXP_STAT:
      return true;
    case SSH2_FXP_RENAME:
      return sftpVersion >= 2 ? true : false;
    case SSH2_FXP_EXTENDED:
    case SSH2_FXP_EXTENDED_REPLY:
    case SSH2_FXP_READLINK:
    case SSH2_FXP_SYMLINK:
      return sftpVersion >= 3 ? true : false;
    default:
      kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::isSupportedOperation(type:"
                            << type << "): unrecognized operation type" << endl;
      break;
  }
  
  return false;
}

void kio_sftpProtocol::copy(const KURL &src, const KURL &dest, int permissions, bool overwrite) {
    
    bool srcLocal = src.isLocalFile();
    bool destLocal = dest.isLocalFile();
        
    if ( srcLocal && !destLocal ) // Copy file -> sftp
      sftpCopyPut(src, dest, permissions, overwrite);
    else if ( destLocal && !srcLocal ) // Copy sftp -> file
      sftpCopyGet(dest, src, permissions, overwrite);
    else
      error(ERR_UNSUPPORTED_ACTION, QString::null);    
}

void kio_sftpProtocol::sftpCopyGet(const KURL& dest, const KURL& src, int mode, bool overwrite)
{
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpCopyGet: " << src << " -> " << dest << endl;
    
    // Attempt to establish a connection...
    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }
    
    KDE_struct_stat buff_orig;
    QCString dest_orig ( QFile::encodeName(dest.path()) );
    bool origExists = (KDE_lstat( dest_orig.data(), &buff_orig ) != -1);
        
    if (origExists)
    { 
        if (S_ISDIR(buff_orig.st_mode))
        {
          error(ERR_IS_DIRECTORY, dest.prettyURL());
          return;
        }
          
        if (!overwrite)
        {
          error(ERR_FILE_ALREADY_EXIST, dest.prettyURL());
          return;
        }
    }
    
    KIO::filesize_t offset = 0;
    QCString dest_part ( dest_orig + ".part" );
    
    int fd = -1;
    bool partExists = false;
    bool markPartial = config()->readBoolEntry("MarkPartial", true);
    
    if (markPartial)
    {
        KDE_struct_stat buff_part;
        partExists = (KDE_stat( dest_part.data(), &buff_part ) != -1);
        
        if (partExists && buff_part.st_size > 0 && S_ISREG(buff_part.st_mode))
        {
            if (canResume( buff_part.st_size ))
            {
                offset = buff_part.st_size;
                kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpCopyGet: Resuming @ " << offset << endl;
            }
        }
    
        if (offset > 0)
        {
            fd = KDE_open(dest_part.data(), O_RDWR);
            offset = KDE_lseek(fd, 0, SEEK_END);
            if (offset == 0)
            {
              error(ERR_CANNOT_RESUME, dest.prettyURL());
              return;
            }
        }
        else
        {   
            // Set up permissions properly, based on what is done in file io-slave
            int openFlags = (O_CREAT | O_TRUNC | O_WRONLY);
            int initialMode = (mode == -1) ? 0666 : (mode | S_IWUSR);
            fd = KDE_open(dest_part.data(), openFlags, initialMode);
        }
    }
    else
    {
        // Set up permissions properly, based on what is done in file io-slave
        int openFlags = (O_CREAT | O_TRUNC | O_WRONLY);
        int initialMode = (mode == -1) ? 0666 : (mode | S_IWUSR);
        fd = KDE_open(dest_orig.data(), openFlags, initialMode);    
    } 
    
    if(fd == -1) 
    {
      kdDebug(KIO_SFTP_DB) << "sftpCopyGet: Unable to open (" << fd << ") for writting." << endl;
      if (errno == EACCES)
        error (ERR_WRITE_ACCESS_DENIED, dest.prettyURL());
      else
        error (ERR_CANNOT_OPEN_FOR_WRITING, dest.prettyURL());
      return;
    }
    
    Status info = sftpGet(src, offset, fd);
    if ( info.code != 0 )
    {
      // Should we keep the partially downloaded file ??
      KIO::filesize_t size = config()->readNumEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
      if (info.size < size)
        ::remove(dest_part.data());
            
      error(info.code, info.text);
      return;
    }
    
    if (::close(fd) != 0)
    {
      error(ERR_COULD_NOT_WRITE, dest.prettyURL());
      return;
    }
    
    // 
    if (markPartial)
    {
      if (::rename(dest_part.data(), dest_orig.data()) != 0)
      {
        error (ERR_CANNOT_RENAME_PARTIAL, dest_part);
        return;
      }
    }
    
    data(QByteArray());
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpCopyGet(): emit finished()" << endl;
    finished();         
}

kio_sftpProtocol::Status kio_sftpProtocol::sftpGet( const KURL& src, KIO::filesize_t offset, int fd )
{
    int code;
    sftpFileAttr attr;
    
    Status res;
    res.code = 0;
    res.size = 0;    
    
    // stat the file first to get its size
    if( (code = sftpStat(src, attr)) != SSH2_FX_OK ) {            
        return doProcessStatus(code, src.prettyURL());
    }

    // We cannot get file if it is a directory
    if( attr.fileType() == S_IFDIR ) {
        res.text = src.prettyURL();
        res.code = ERR_IS_DIRECTORY;
        return res;
    }

    KIO::filesize_t fileSize = attr.fileSize();
    Q_UINT32 pflags = SSH2_FXF_READ;
    attr.clear();
    
    QByteArray handle;
    if( (code = sftpOpen(src, pflags, attr, handle)) != SSH2_FX_OK ) {
        res.text = src.prettyURL();
        res.code = ERR_CANNOT_OPEN_FOR_READING;
        return res;
    }

    // needed for determining mimetype
    // note: have to emit mimetype before emitting totalsize.
    QByteArray buff;
    QByteArray mimeBuffer;

    unsigned int oldSize;
    bool foundMimetype = false;
    
    // How big should each data packet be? Definitely not bigger than 64kb or
    // you will overflow the 2 byte size variable in a sftp packet.
    Q_UINT32 len = 60*1024;
    code = SSH2_FX_OK;
    
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpGet(): offset = " << offset << endl;    
    while( code == SSH2_FX_OK ) {
        if( (code = sftpRead(handle, offset, len, buff)) == SSH2_FX_OK ) {
            offset += buff.size();
            
            // save data for mimetype. Pretty much follows what is in the ftp ioslave
            if( !foundMimetype ) {
                oldSize = mimeBuffer.size();
                mimeBuffer.resize(oldSize + buff.size());
                memcpy(mimeBuffer.data()+oldSize, buff.data(), buff.size());

                if( mimeBuffer.size() > 1024 ||  offset == fileSize ) {
                    // determine mimetype
                    KMimeMagicResult* result =
                        KMimeMagic::self()->findBufferFileType(mimeBuffer, src.filename());
                    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpGet(): mimetype is " <<
                                      result->mimeType() << endl;
                    mimeType(result->mimeType());
                    
                    // Always send the total size after emitting mime-type...
                    totalSize(fileSize);
                                        
                    if (fd == -1)
                        data(mimeBuffer);
                    else
                    {
                        if ( (res.code=writeToFile(fd, mimeBuffer.data(), mimeBuffer.size())) != 0 )
                            return res;
                    }
                    
                    processedSize(mimeBuffer.size());
                    mimeBuffer.resize(0);
                    foundMimetype = true;
                }
            }
            else {
                if (fd == -1)
                    data(buff);
                else
                {
                    if ( (res.code= writeToFile(fd, buff.data(), buff.size())) != 0 )
                        return res;
                }                
                processedSize(offset);
            }
        }

        /* 
          Check if slave was killed.  According to slavebase.h we need to leave
          the slave methods as soon as possible if the slave is killed. This 
          allows the slave to be cleaned up properly.
        */
        if( wasKilled() ) {
            res.text = i18n("An internal error occurred. Please retry the request again.");
            res.code = ERR_UNKNOWN;
            return res;
        }
    }

    if( code != SSH2_FX_EOF ) {
        res.text = src.prettyURL();
        res.code = ERR_COULD_NOT_READ; // return here or still send empty array to indicate end of read?
    }
    
    res.size = offset;
    sftpClose(handle);
    processedSize (offset);
    return res;
}

void kio_sftpProtocol::get(const KURL& url) {
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::get(" << url << ")" << endl ;
    
    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }

    // Get resume offset
    Q_UINT64 offset = config()->readUnsignedLongNumEntry("resume");   
    if( offset > 0 ) {        
        canResume();
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::get(): canResume(), offset = " << offset << endl;
    }
    
    Status info = sftpGet(url, offset);
    
    if (info.code != 0)
    {
      error(info.code, info.text);
      return;
    }
    
    data(QByteArray());
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::get(): emit finished()" << endl;
    finished();
}


void kio_sftpProtocol::setHost (const QString& h, int port, const QString& user, const QString& pass){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::setHost(): " << user << "@" 
                         << h << ":" << port << endl;

    if( mHost != h || mPort != port || user != mUsername || mPassword != pass )
        closeConnection();

    mHost = h;

    if( port > 0 )
        mPort = port;
    else {
        struct servent *pse;
        if( (pse = getservbyname("ssh", "tcp") ) == NULL )
            mPort = 22;
        else
            mPort = ntohs(pse->s_port);
    }

    if (user.isEmpty()) {
	    mUsername = getCurrentUsername();
    }
    else {
    	mUsername = user;
    }

    mPassword = pass;
}


void kio_sftpProtocol::openConnection() {
    if(mConnected)
      return;
    
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection() to " <<
        mUsername << "@" << mHost << ":" << mPort << endl;

    infoMessage( i18n("Opening SFTP connection to host <b>%1:%2</b>").arg(mHost).arg(mPort));

    if( mHost.isEmpty() ) {
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection(): "
            "Need hostname" << endl;
        error(ERR_UNKNOWN_HOST, i18n("No host name specified"));
        return;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Setup AuthInfo for use with password caching and the
    // password dialog box.
    AuthInfo info;
    info.url.setProtocol("sftp");
    info.url.setHost(mHost);
    info.url.setPort(mPort);
    info.url.setUser(mUsername);
    info.caption = i18n("SFTP Login");
    info.comment = "sftp://"+mHost+":"+mPort;
    info.commentLabel = i18n("site:");
    info.username = mUsername;
    info.keepPassword = true;

    ///////////////////////////////////////////////////////////////////////////
    // Check for cached authentication info if a username AND password were
    // not specified in setHost().
    bool gotCachedInfo;
    if( mUsername.isEmpty() && mPassword.isEmpty() ) {
      kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol(): checking cache " 
                           << "info.username = " << info.username 
                           << ", info.url = " << info.url.prettyURL() << endl;

    if( gotCachedInfo = checkCachedAuthentication(info) ) {
            mUsername = info.username;
            mPassword = info.password;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Now setup our ssh options. If we found a cached username
    // and password we set the SSH_PASSWORD and SSH_USERNAME
    // options right away.  Otherwise we wait. The other options are
    // necessary for running sftp over ssh.
    KSshProcess::SshOpt opt;   // a ssh option, this can be reused
    KSshProcess::SshOptList opts; // list of SshOpts
    KSshProcess::SshOptListIterator passwdIt; // points to the opt in opts that specifies the password
    KSshProcess::SshOptListIterator usernameIt;

//    opt.opt = KSshProcess::SSH_VERBOSE;
//    opts.append(opt);
//    opts.append(opt);

    if( mPort != -1 ) {
        opt.opt = KSshProcess::SSH_PORT;
        opt.num = mPort;
        opts.append(opt);
    }

    opt.opt = KSshProcess::SSH_SUBSYSTEM;
    opt.str = "sftp";
    opts.append(opt);

    opt.opt = KSshProcess::SSH_FORWARDX11;
    opt.boolean = false;
    opts.append(opt);

    opt.opt = KSshProcess::SSH_FORWARDAGENT;
    opt.boolean = false;
    opts.append(opt);

    opt.opt = KSshProcess::SSH_PROTOCOL;
    opt.num = 2;
    opts.append(opt);

    opt.opt = KSshProcess::SSH_HOST;
    opt.str = mHost;
    opts.append(opt);

    opt.opt = KSshProcess::SSH_ESCAPE_CHAR;
    opt.num = -1; // don't use any escape character
    opts.append(opt);

    // set the username and password if we have them
    if( !mUsername.isEmpty()  ) {
        opt.opt = KSshProcess::SSH_USERNAME;
        opt.str = mUsername;
        usernameIt = opts.append(opt);
    }

    if( !mPassword.isEmpty() ) {
        opt.opt = KSshProcess::SSH_PASSWD;
        opt.str = mPassword;
        passwdIt = opts.append(opt);
    }

    ssh.setOptions(opts);
    ssh.printArgs();

    ///////////////////////////////////////////////////////////////////////////
    // Start the ssh connection process.
    //

    int err;           // error code from KSshProcess
    QString msg;       // msg for dialog box
    QString caption;   // dialog box caption
    bool firstTime = true;
    bool dlgResult;

    while( !(mConnected = ssh.connect()) ) {
        err = ssh.error();
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection(): "
            "Got " << err << " from KSshProcess::connect()" << endl;

        switch(err) {
        case KSshProcess::ERR_NEED_PASSWD:
        case KSshProcess::ERR_NEED_PASSPHRASE:
            // At this point we know that either we didn't set
            // an username or password in the ssh options list,
            // or what we did pass did not work. Therefore we
            // must prompt the user.

            if( err == KSshProcess::ERR_NEED_PASSPHRASE ) {
                info.prompt =
                    i18n("Please enter your user name and key passphrase.");
            }
            else {
                info.prompt =

                    i18n("Please enter your user name and password.");
            }

            info.caption = i18n("SFTP Login");
            info.commentLabel = i18n("site:");
            info.comment = "sftp://"+mHost;
            info.keepPassword = true;
            kdDebug(KIO_SFTP_DB) << "Kio_sftpProtocol(): info.username = " << info.username <<
                ", info.url = " << info.url.prettyURL() << endl;

            if( firstTime ) {
                dlgResult = openPassDlg(info);
            }
            else {
                dlgResult = openPassDlg(info, i18n("Incorrect user name or password"));
            }

            if( dlgResult ) {
               if( info.username.isEmpty() || info.password.isEmpty() ) {
                    error(ERR_COULD_NOT_AUTHENTICATE,
                      i18n("Please enter a user name and password"));
                    continue;
                }
            }
            else {
                // user canceled or dialog failed to open
                error(ERR_USER_CANCELED, QString::null);
                kdDebug(KIO_SFTP_DB) << "Kio_sftpProtocol(): user canceled, dlgResult = " << dlgResult << endl;
                closeConnection();
                return;
            }

            firstTime = false;

            // Check if the username has changed. SSH only accepts
            // the username at startup. If the username has changed
            // we must disconnect ssh, change the SSH_USERNAME
            // option, and reset the option list. We will also set
            // the password option so the user is not prompted for
            // it again.
            if( mUsername != info.username ) {
                kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection(): "
                    "Username changed from " <<
                    mUsername << " to " << info.username << endl;

                ssh.disconnect();

                // if we haven't yet added the username
                // or password option to the ssh options list then
                // the iterators will be equal to the empty iterator.
                // Create the opts now and add them to the opt list.
                if( usernameIt == KSshProcess::SshOptListIterator() ) {
                    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection(): "
                        "Adding username to options list" << endl;
                    opt.opt = KSshProcess::SSH_USERNAME;
                    usernameIt = opts.append(opt);
                }

                if( passwdIt == KSshProcess::SshOptListIterator() ) {
                    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection(): "
                        "Adding password to options list" << endl;
                    opt.opt = KSshProcess::SSH_PASSWD;
                    passwdIt = opts.append(opt);
                }

                (*usernameIt).str = info.username;
                (*passwdIt).str = info.password;
                ssh.setOptions(opts);
                ssh.printArgs();
            }
            else { // just set the password
                ssh.setPassword(info.password);
            }

            mUsername = info.username;
            mPassword = info.password;

            break;

        case KSshProcess::ERR_NEW_HOST_KEY:
            caption = i18n("Warning: Cannot verify host's identity.");
            msg = ssh.errorMsg();
            if( KMessageBox::Yes != messageBox(WarningYesNo, msg, caption) ) {
                closeConnection();
                error(ERR_USER_CANCELED, QString::null);
                return;
            }
            ssh.acceptHostKey(true);
            break;

        case KSshProcess::ERR_DIFF_HOST_KEY:
            caption = i18n("Warning: Host's identity changed.");
            msg = ssh.errorMsg();
            if( KMessageBox::Yes != messageBox(WarningYesNo, msg, caption) ) {
                closeConnection();
                error(ERR_USER_CANCELED, QString::null);
                return;
            }
            ssh.acceptHostKey(true);
            break;

        case KSshProcess::ERR_AUTH_FAILED:
            infoMessage(i18n("Authentication failed."));
            error(ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
            return;

        case KSshProcess::ERR_AUTH_FAILED_NEW_KEY:
            msg = ssh.errorMsg();
            error(ERR_COULD_NOT_LOGIN, msg);
            return;

        case KSshProcess::ERR_AUTH_FAILED_DIFF_KEY:
            msg = ssh.errorMsg();
            error(ERR_COULD_NOT_LOGIN, msg);
            return;

        case KSshProcess::ERR_CLOSED_BY_REMOTE_HOST:
            infoMessage(i18n("Connection failed."));
            caption = i18n("Connection closed by remote host.");
            msg = ssh.errorMsg();
            messageBox(Information, msg, caption);
            closeConnection();
            error(ERR_COULD_NOT_LOGIN, msg);
            return;

        case KSshProcess::ERR_INTERACT:
        case KSshProcess::ERR_INTERNAL:
        case KSshProcess::ERR_UNKNOWN:
        case KSshProcess::ERR_INVALID_STATE:
        case KSshProcess::ERR_CANNOT_LAUNCH:
        case KSshProcess::ERR_HOST_KEY_REJECTED:
        default:
          /*
            infoMessage(i18n("Connection failed."));
            caption = i18n("Unexpected SFTP error: %1").arg(err);
            msg = ssh.errorMsg();
            messageBox(Information, msg, caption);
           */            
            return;
        }
    }

    // catch all in case we did something wrong above
    if( !mConnected ) {
        error(ERR_INTERNAL, QString::null);
        return;
    }

    // Now send init packet.
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection(): "
        "Sending SSH2_FXP_INIT packet." << endl;
    QByteArray p;
    QDataStream packet(p, IO_WriteOnly);
    packet << (Q_UINT32)5;                     // packet length
    packet << (Q_UINT8) SSH2_FXP_INIT;         // packet type
    packet << (Q_UINT32)SSH2_FILEXFER_VERSION; // client version

    putPacket(p);
    getPacket(p);

    QDataStream s(p, IO_ReadOnly);
    Q_UINT32 version;
    Q_UINT8  type;
    s >> type;
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection(): "
        "Got type " << type << endl;

    if( type == SSH2_FXP_VERSION ) {
        s >> version;
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::openConnection(): "
            "Got server version " << version << endl;

        // XXX Get extensions here      
        sftpVersion = version;
      
        /* Server should return lowest common version supported by
         * client and server, but double check just in case.
         */
        if( sftpVersion > SSH2_FILEXFER_VERSION ) {
            error(ERR_UNSUPPORTED_PROTOCOL,
                i18n("SFTP version %1").arg(version));
            closeConnection();
            return;
        }
    }
    else {
        error(ERR_UNKNOWN, i18n("Protocol error."));
        closeConnection();
        return;
    }

    // Login succeeded!
    infoMessage(i18n("Successfully connected to %1").arg(mHost));
    info.url.setProtocol("sftp");
    info.url.setHost(mHost);
    info.url.setPort(mPort);
    info.url.setUser(mUsername);
    info.username = mUsername;
    info.password = mPassword;
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol(): caching info.username = " << info.username <<
        ", info.url = " << info.url.prettyURL() << endl;
    cacheAuthentication(info);
    mConnected = true;
    connected();

    mPassword.fill('x');
    info.password.fill('x');

    return;
}

void kio_sftpProtocol::closeConnection() {
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::closeConnection()" << endl;
    ssh.disconnect();
    mConnected = false;
}

void kio_sftpProtocol::sftpCopyPut(const KURL& src, const KURL& dest, int permissions, bool overwrite) {    
      
    KDE_struct_stat buff;
    QCString file (QFile::encodeName(src.path()));
    
    if (KDE_lstat(file.data(), &buff) == -1) { 
        error (ERR_DOES_NOT_EXIST, src.prettyURL());
        return;
    }
    
    if (S_ISDIR (buff.st_mode)) {
        error (ERR_IS_DIRECTORY, src.prettyURL());
        return;
    }
    
    int fd = KDE_open (file.data(), O_RDONLY);
    if (fd == -1) {
        error (ERR_CANNOT_OPEN_FOR_READING, src.prettyURL());
        return;
    }
    
    totalSize (buff.st_size);    
    
    sftpPut (dest, permissions, false, overwrite, fd);
    
    // Close the file descriptor...
    ::close( fd );
}

void kio_sftpProtocol::sftpPut( const KURL& dest, int permissions, bool resume, bool overwrite, int fd ) {

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }
    
    bool markPartial = config()->readBoolEntry("MarkPartial", true);
    
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpPut: Mark partial = "
                         << markPartial << ", Resume = " << resume 
                         << ", Overwrite = " << overwrite << endl;

    KURL origUrl( dest );
    sftpFileAttr origAttr;
    bool origExists = false;
    
    // Stat original (without part ext)  to see if it already exists
    int code = sftpStat(origUrl, origAttr);
    
    if( code == SSH2_FX_OK ) {
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpPut(): <file> already exists" << endl;
        
        // Do not waste time/resources with more remote stat calls if the file exists 
        // and we weren't instructed to overwrite it...
        if( !overwrite ) {
            error(ERR_FILE_ALREADY_EXIST, origUrl.prettyURL());
            return;
        }
        // Delete remote file if its size is zero
        if( origAttr.fileSize() == 0 ) {
            if( sftpRemove(origUrl, true) != SSH2_FX_OK ) {
                error(ERR_CANNOT_DELETE_PARTIAL, origUrl.prettyURL());
                return;
            }
        }
        else {        
            // Find the real file if the file being copied to is a symlink...        
            if( S_ISLNK(origAttr.permissions()) && isSupportedOperation(SSH2_FXP_READLINK) ) {
                
                QString target;
                if ( (code=sftpReadLink( origUrl, target )) != SSH2_FX_OK ) {
                    doProcessStatus( code, i18n("Could not copy file to '%1'").arg(origUrl.host()) );
                    return;
                }
                
                if( target[0] == '/' )
                    origUrl.setPath( target );
                else
                    origUrl.addPath( target );
                 
                origUrl.cleanPath();
            }
        }
        origExists = true;
    }    
    else if( code != SSH2_FX_NO_SUCH_FILE ) {
        processStatus(code, origUrl.prettyURL());
        return;
    }
    
    // Stat file with part ext to see if it already exists...
    KURL partUrl( origUrl );
    partUrl.setFileName( partUrl.filename() + ".part" );
    
    bool partExists = false;
    Q_UINT64 offset = 0;
    
    if( markPartial ) {
                
        sftpFileAttr partAttr;
        code = sftpStat(partUrl, partAttr);
        
        if( code == SSH2_FX_OK ) {
            kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpPut(): <file.part> already exists" << endl;
            partExists = true;
            offset = partAttr.fileSize();
                        
            // If for some reason, both the original and partial files exist,
            // skip resumption just like we would if the size of the partial
            // file is zero...
            if( origExists || offset == 0 )
            {
                if( sftpRemove(partUrl, true) != SSH2_FX_OK ) {
                    error(ERR_CANNOT_DELETE_PARTIAL, partUrl.prettyURL());
                    return;
                }
                
                if( sftpRename(origUrl, partUrl) != SSH2_FX_OK ) {
                    error(ERR_CANNOT_RENAME_ORIGINAL, origUrl.prettyURL());
                    return;
                }
                
                offset = 0;
            }
            else if( !overwrite && !resume ) {            
                    if (fd != -1)
                        resume = (KDE_lseek(fd, offset, SEEK_SET) != -1);
                    else
                        resume = canResume( offset );
                    
                    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpPut: Can resume = " << resume
                                        << ", Resume offset = " << offset;
                    
                    if( !resume ) {
                      error(ERR_FILE_ALREADY_EXIST, partUrl.prettyURL());
                      return;
                    }
            }
            else {
                offset = 0;
            }
        }
        else if( code == SSH2_FX_NO_SUCH_FILE ) {
            if( origExists && sftpRename(origUrl, partUrl) != SSH2_FX_OK ) {
              error(ERR_CANNOT_RENAME_ORIGINAL, origUrl.prettyURL());
              return;
            }
        }
        else {
            processStatus(code, partUrl.prettyURL());
            return;
        }
    }

    // Determine the url we will actually write to...
    KURL writeUrl (markPartial ? partUrl:origUrl);

    Q_UINT32 pflags = 0;
    if( overwrite && !resume )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_TRUNC;
    else if( !overwrite && !resume )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_EXCL;
    else if( overwrite && resume )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT;
    else if( !overwrite && resume )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_APPEND;
    
    sftpFileAttr attr;
    QByteArray handle;
    
    // set the permissions of the file we write to if it didn't already exist
    if( !partExists && !origExists )
        attr.setPermissions(permissions);    
    
    code = sftpOpen( writeUrl, pflags, attr, handle );
    if( code != SSH2_FX_OK ) {
    
        // Rename the file back to its original name if a
        // put fails due to permissions problems...
        if( markPartial && overwrite ) {
            (void) sftpRename(partUrl, origUrl);
            writeUrl = origUrl;
        }
        
        if( code == SSH2_FX_FAILURE ) { // assume failure means file exists
            error(ERR_FILE_ALREADY_EXIST, writeUrl.prettyURL());
            return;
        }
        else {        
            processStatus(code, writeUrl.prettyURL());
            return;
        }
    }

    long nbytes;
    QByteArray buff;
       
    do {        
        if( fd != -1 ) {
            buff.resize( 16*1024 );
            if ( (nbytes = ::read(fd, buff.data(), buff.size())) > -1 )
                buff.resize( nbytes );
        }
        else {
            dataReq();
            nbytes = readData( buff );
        }
        
        if( nbytes > 0 ) {
            if( (code = sftpWrite(handle, offset, buff)) != SSH2_FX_OK ) {
                error(ERR_COULD_NOT_WRITE, dest.prettyURL());
                return;
            }
            
            offset += nbytes;
            processedSize(offset);
            
            /* Check if slave was killed.  According to slavebase.h we
              * need to leave the slave methods as soon as possible if
              * the slave is killed. This allows the slave to be cleaned
              * up properly.
              */
            if( wasKilled() ) {
                sftpClose(handle);
                closeConnection();
                error(ERR_UNKNOWN, i18n("An internal error occurred. Please try again."));
                return;
            }
        }
    } while( nbytes > 0 );

    if( nbytes != 0 ) {
        sftpClose(handle);

        if( markPartial ) {
            // Remove remote file if it smaller than our keep size
            uint minKeepSize = config()->readNumEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);

            if( sftpStat(writeUrl, attr) == SSH2_FX_OK ) {
                if( attr.fileSize() < minKeepSize ) {
                    sftpRemove(writeUrl, true);
                }
            }
        }
        
        error( ERR_UNKNOWN, i18n("Unknown error was encountered while copying the file "
                                 "to '%1'. Please try again.").arg(dest.host()) );
        return;
    }

    if( (code = sftpClose(handle)) != SSH2_FX_OK ) {
        error(ERR_COULD_NOT_WRITE, writeUrl.prettyURL());
        return;
    }

    // If wrote to a partial file, then remove the part ext
    if( markPartial ) {          
        if( sftpRename(partUrl, origUrl) != SSH2_FX_OK ) {
            error(ERR_CANNOT_RENAME_PARTIAL, origUrl.prettyURL());
            return;
        }
    }

    finished();
}

void kio_sftpProtocol::put ( const KURL& url, int permissions, bool overwrite, bool resume ){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::put(" << url << ")" << endl
                         << "kio_sftpProtocol::put(): overwrite = " << overwrite
                         << ", resume = " << resume << endl;
                            
    sftpPut( url, permissions, resume, overwrite );
}

void kio_sftpProtocol::stat ( const KURL& url ){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::stat( " << url << " )" << endl;

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }

    // If the stat URL has no path, do not attempt to determine the real
    // path and do a redirect. KRun will simply ignore such requests. 
    // Instead, simply return the mime-type as a directory...
    if( !url.hasPath() ) {
        UDSEntry entry;
        UDSAtom atom;
    
        atom.m_uds = KIO::UDS_NAME;
        atom.m_str = QString::null;
        entry.append( atom );
    
        atom.m_uds = KIO::UDS_FILE_TYPE;
        atom.m_long = S_IFDIR;
        entry.append( atom );
    
        atom.m_uds = KIO::UDS_ACCESS;
        atom.m_long = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
        entry.append( atom );
    
        atom.m_uds = KIO::UDS_USER;
        atom.m_str = mUsername;
        entry.append( atom );
        atom.m_uds = KIO::UDS_GROUP;
        entry.append( atom );
    
        // no size    
        statEntry( entry );
        finished();
        return;
    }

    int code;
    sftpFileAttr attr;
    if( (code = sftpStat(url, attr)) != SSH2_FX_OK ) {
        processStatus(code, url.prettyURL());
        return;
    }
    else {
        //kdDebug() << "We sent and received stat packet ok" << endl;
        attr.setFilename(url.filename());
        statEntry(attr.entry());
    }
    
    finished();

    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::stat: END" << endl;
    return;
}


void kio_sftpProtocol::mimetype ( const KURL& url ){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::mimetype( " << url << " )" << endl;

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }

    Q_UINT32 pflags = SSH2_FXF_READ;
    QByteArray handle, mydata;
    sftpFileAttr attr;
    int code;
    if( (code = sftpOpen(url, pflags, attr, handle)) != SSH2_FX_OK ) {
        error(ERR_CANNOT_OPEN_FOR_READING, url.prettyURL());
        return;
    }

    Q_UINT32 len = 1024; // Get first 1k for determining mimetype
    Q_UINT64 offset = 0;
    code = SSH2_FX_OK;
    while( offset < len && code == SSH2_FX_OK ) {
        if( (code = sftpRead(handle, offset, len, mydata)) == SSH2_FX_OK ) {
            data(mydata);
            offset += mydata.size();
            processedSize(offset);

            kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::mimetype(): offset = " << offset << endl;
        }
    }


    data(QByteArray());
    processedSize(offset);
    sftpClose(handle);
    finished();
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::mimetype(): END" << endl;
}


void kio_sftpProtocol::listDir(const KURL& url) {
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::listDir(" << url << ")" << endl;

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }

    if( !url.hasPath() ) {
        KURL newUrl ( url );
        if( sftpRealPath(url, newUrl) == SSH2_FX_OK ) {
            kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::listDir: Redirecting to " << newUrl << endl;
            redirection(newUrl);
            finished();
            return;
        }
    }
    
    int code;
    QByteArray handle;
    QString path = url.path();
    
    if( (code = sftpOpenDirectory(url, handle)) != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::listDir(): open directory failed" << endl;
        processStatus(code, url.prettyURL());
        return;
    }


    code = SSH2_FX_OK;
    while( code == SSH2_FX_OK ) {
        code = sftpReadDir(handle, url);
        if( code != SSH2_FX_OK && code != SSH2_FX_EOF )
            processStatus(code, url.prettyURL());
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::listDir(): return code = " << code << endl;
    }

    if( (code = sftpClose(handle)) != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::listdir(): closing of directory failed" << endl;
        processStatus(code, url.prettyURL());
        return;
    }

    finished();
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::listDir(): END" << endl;
}

/** Make a directory.
    OpenSSH does not follow the internet draft for sftp in this case.
    The format of the mkdir request expected by OpenSSH sftp server is:
        uint32 id
        string path
        ATTR   attr
 */
void kio_sftpProtocol::mkdir(const KURL&url, int permissions){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::mkdir()" << endl;

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }

    QString path = url.path();
    Q_UINT32 id, expectedId;
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    sftpFileAttr attr;
    attr.setPermissions(permissions);

    id = expectedId = mMsgId++;
    kdDebug(KIO_SFTP_DB) << "creating dir " << path << endl;
    s << Q_UINT32(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + path.length() + attr.size());
    s << (Q_UINT8)SSH2_FXP_MKDIR;
    s << id;
    s.writeBytes(path.latin1(), path.length());
    s << attr;
    kdDebug(KIO_SFTP_DB) << "mkdir(): packet size is " << p.size() << endl;

    putPacket(p);
    getPacket(p);

    Q_UINT8 type;
    QDataStream r(p, IO_ReadOnly);

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::mkdir: sftp packet id mismatch" << endl;
        error(ERR_COULD_NOT_MKDIR, path);
        finished();
        return;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::mkdir(): unexpected packet type of " << type << endl;
        error(ERR_COULD_NOT_MKDIR, path);
        finished();
        return;
    }

    int code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::mkdir(): failed with code " << code << endl;
        
        // Check if mkdir failed because the directory already exists so that
        // we can return the appropriate message...
        sftpFileAttr dirAttr;
        if ( sftpStat(url, dirAttr) == SSH2_FX_OK )
        {
          error( ERR_DIR_ALREADY_EXIST, url.prettyURL() );
          return;
        }
                
        error(ERR_COULD_NOT_MKDIR, path);
    }

    finished();
}

void kio_sftpProtocol::rename(const KURL& src, const KURL& dest, bool overwrite){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::rename(" << src << " -> " << dest << ")" << endl;

    if (!isSupportedOperation(SSH2_FXP_RENAME)) {
      error(ERR_UNSUPPORTED_ACTION,
          i18n("The remote host does not support renaming files."));
      return;
    }

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }

    // Always stat the destination before attempting to rename 
    // a file or a directory...
            sftpFileAttr attr;
    int code = sftpStat(dest, attr);    
    
    // If the destination directory, exists tell it to the job
    // so it the proper action can be presented to the user...
    if( code == SSH2_FX_OK ) 
    {    
      if (!overwrite)
      {
        if ( S_ISDIR(attr.permissions()) )
          error( KIO::ERR_DIR_ALREADY_EXIST, dest.url() );
        else
          error( KIO::ERR_FILE_ALREADY_EXIST, dest.url() );
        return;
        }
      
      // If overwrite is specified, then simply remove the existing file/dir first...
      if( (code = sftpRemove( dest, !S_ISDIR(attr.permissions()) )) != SSH2_FX_OK )
      {
        processStatus(code);
            return;
        }
    }
    
    // Do the renaming...
    if( (code = sftpRename(src, dest)) != SSH2_FX_OK ) {
        processStatus(code);
        return;
    }
    
    finished();
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::rename(): END" << endl;
}

void kio_sftpProtocol::symlink(const QString& target, const KURL& dest, bool overwrite){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::symlink()" << endl;

    if (!isSupportedOperation(SSH2_FXP_SYMLINK)) {
      error(ERR_UNSUPPORTED_ACTION,
          i18n("The remote host does not support creating symbolic links."));
      return;
    }

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }

    int code;
    bool failed = false;
    if( (code = sftpSymLink(target, dest)) != SSH2_FX_OK ) {
        if( overwrite ) { // try to delete the destination
            sftpFileAttr attr;
            if( (code = sftpStat(dest, attr)) != SSH2_FX_OK ) {
                failed = true;
            }
            else {
                if( (code = sftpRemove(dest, !S_ISDIR(attr.permissions())) ) != SSH2_FX_OK ) {
                    failed = true;
                }
                else {
                    // XXX what if rename fails again? We have lost the file.
                    // Maybe rename dest to a temporary name first? If rename is
                    // successful, then delete?
                    if( (code = sftpSymLink(target, dest)) != SSH2_FX_OK )
                        failed = true;
                }
            }
        }
        else if( code == SSH2_FX_FAILURE ) {
            error(ERR_FILE_ALREADY_EXIST, dest.prettyURL());
            return;
        }
        else
            failed = true;
    }

    // What error code do we return? Code for the original symlink command
    // or for the last command or for both? The second one is implemented here.
    if( failed )
        processStatus(code);

    finished();
}

void kio_sftpProtocol::chmod(const KURL& url, int permissions){
    QString perms;
    perms.setNum(permissions, 8);
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::chmod(" << url << ", " << perms << ")" << endl;

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        finished();
        return;
    }

    sftpFileAttr attr;
    attr.setPermissions(permissions);
    int code;
    if( (code = sftpSetStat(url, attr)) != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::chmod(): sftpSetStat failed with error " << code << endl;
        if( code == SSH2_FX_FAILURE )
            error(ERR_CANNOT_CHMOD, QString::null);
        else
            processStatus(code, url.prettyURL());
    }
    finished();
}


void kio_sftpProtocol::del(const KURL &url, bool isfile){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::del(" << url << ", " << (isfile?"file":"dir") << ")" << endl;

    openConnection();
    if( !mConnected ) {
        error(ERR_COULD_NOT_CONNECT, mHost);
        return;
    }

    int code;
    if( (code = sftpRemove(url, isfile)) != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::del(): sftpRemove failed with error code " << code << endl;
        processStatus(code, url.prettyURL());
    }
    finished();
}

void kio_sftpProtocol::slave_status() {
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::slave_status(): connected to " 
                         <<  mHost << "? " << mConnected << endl;
    
    slaveStatus ((mConnected ? mHost : QString::null), mConnected);
}

bool kio_sftpProtocol::getPacket(QByteArray& msg) {
//    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::getPacket()" << endl;
    int len;
    unsigned int msgLen;
    char buf[4096];

    // Get the message length and type
    len = atomicio(ssh.stdioFd(), buf, 4, true /*read*/);
    if( len == 0 || len == -1 ) {
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::getPacket(): read of packet length failed, ret = " <<
            len << ", error =" << strerror(errno) << endl;
        closeConnection();
        error( ERR_CONNECTION_BROKEN, mHost);
        return false;
    }

    QByteArray a;
    a.duplicate(buf, (unsigned int)4);
    QDataStream s(a, IO_ReadOnly);
    s >> msgLen;
//    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::getPacket(): Got msg length of " << msgLen << endl;
    if( !msg.resize(msgLen) ) {
        error( ERR_OUT_OF_MEMORY, i18n("Could not allocate memory for SFTP packet."));
        return false;
    }

    unsigned int offset = 0;
    while( msgLen ) {
        len = atomicio(ssh.stdioFd(), buf, MIN(msgLen, sizeof(buf)), true /*read*/);
        if( len == 0 ) {
            kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::getPacket(): nothing to read, ret = " <<
                len << ", error =" << strerror(errno) << endl;
            closeConnection();
            error(ERR_CONNECTION_BROKEN, i18n("Connection closed"));
            return false;
        }
        else if( len == -1 ) {
            kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::getPacket(): read error, ret = " <<
                len << ", error =" << strerror(errno) << endl;
            closeConnection();
            error(ERR_CONNECTION_BROKEN, i18n("Could not read SFTP packet"));
            return false;
        }
        msgLen -= len;
        mymemcpy(buf, msg, offset, len);
        offset += len;
    }
//    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::getPacket(): END size = " << msg.size() << endl;
    return true;
}

/** Send an sftp packet to stdin of the ssh process. */
bool kio_sftpProtocol::putPacket(QByteArray& p){
//    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::putPacket(): size == " << p.size() << endl;
    int ret;
    ret = atomicio(ssh.stdioFd(), p.data(), p.size(), false /*write*/);
    if( ret <= 0 ) {
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::putPacket(): write failed, ret =" << ret <<
            ", error = " << strerror(errno) << endl;
        return false;
    }

    return true;
}

/** Used to have the server canonicalize any given path name to an absolute path.
This is useful for converting path names containing ".." components or relative
pathnames without a leading slash into absolute paths.
Returns the canonicalized url. */
int kio_sftpProtocol::sftpRealPath(const KURL& url, KURL& newUrl){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRealPath(" << url << ", newUrl)" << endl;
    QString path = url.path();
    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    s << Q_UINT32(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + path.length());
    s << (Q_UINT8)SSH2_FXP_REALPATH;
    s << id;
    s.writeBytes(path.latin1(), path.length());

    putPacket(p);
    getPacket(p);

    Q_UINT8 type;
    QDataStream r(p, IO_ReadOnly);

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRealPath: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        Q_UINT32 code;
        r >> code;
        return code;
    }

    if( type != SSH2_FXP_NAME ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRealPath(): unexpected packet type of " << type << endl;
        return -1;
    }

    Q_UINT32 count;
    r >> count;
    if( count != 1 ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRealPath(): Bad number of file attributes for realpath command" << endl;
        return -1;
    }

    QCString newPath;
    r >> newPath;
    // Make sure there is a terminating null character. QCString gets the string size
    // but I don't think a null character is appended. += doesn't always seem to work.
    int len = newPath.size();
    newPath.resize(newPath.size()+1);
    newPath[len] = '\0';
    if (newPath.isEmpty())
      newPath = "/";
    newUrl.setPath(newPath);
    return SSH2_FX_OK;
}

kio_sftpProtocol::Status kio_sftpProtocol::doProcessStatus(Q_UINT8 code, const QString& message)
{
    Status res;
    res.code = 0;
    res.size = 0;
    res.text = message;
    
    switch(code) 
    {
      case SSH2_FX_OK:          
      case SSH2_FX_EOF:
          break;
      case SSH2_FX_NO_SUCH_FILE:
          res.code = ERR_DOES_NOT_EXIST;
          break;
      case SSH2_FX_PERMISSION_DENIED:
          res.code = ERR_ACCESS_DENIED;
          break;
      case SSH2_FX_FAILURE:
          res.text = i18n("SFTP command failed for an unknown reason.");
          res.code = ERR_UNKNOWN;
          break;
      case SSH2_FX_BAD_MESSAGE:
          res.text = i18n("The SFTP server received a bad message.");
          res.code = ERR_UNKNOWN;
          break;
      case SSH2_FX_OP_UNSUPPORTED:
          res.text = i18n("You attempted an operation unsupported by the SFTP server.");
          res.code = ERR_UNKNOWN;
          break;
      default:
          res.text = i18n("Error code: %1").arg(code);
          res.code = ERR_UNKNOWN;
    }
    
    return res;
}

/** Process SSH_FXP_STATUS packets. */
void kio_sftpProtocol::processStatus(Q_UINT8 code, const QString& message){
    Status st = doProcessStatus( code, message );
    if( st.code != 0 )
        error( st.code, st.text );
}

/** Opens a directory handle for url.path. Returns true if succeeds. */
int kio_sftpProtocol::sftpOpenDirectory(const KURL& url, QByteArray& handle){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpenDirectory(" << url << ", handle)" << endl;
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);
    QString path = url.path();

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + path.length());
    s << (Q_UINT8)SSH2_FXP_OPENDIR;
    s << (Q_UINT32)id;
    s.writeBytes(path.latin1(), path.length());

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpenDirectory: sftp packet id mismatch: " <<
            "expected " << expectedId << ", got " << id << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        Q_UINT32 errCode;
        r >> errCode;
        return errCode;
    }

    if( type != SSH2_FXP_HANDLE ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpenDirectory: unexpected message type of " << type << endl;
        return -1;
    }

    r >> handle;
    if( handle.size() > 256 ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpenDirectory: handle exceeds max length" << endl;
        return -1;
    }

    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpenDirectory: handle (" << handle.size() << "): [" << handle << "]" << endl;
    return SSH2_FX_OK;
}

/** Closes a directory or file handle. */
int kio_sftpProtocol::sftpClose(const QByteArray& handle){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpClose()" << endl;
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + handle.size());
    s << (Q_UINT8)SSH2_FXP_CLOSE;
    s << (Q_UINT32)id;
    s << handle;

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpClose: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpClose: unexpected message type of " << type << endl;
        return -1;
    }

    Q_UINT32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpClose: close failed with err code " << code << endl;
    }

    return code;
}

/** Set a files attributes. */
int kio_sftpProtocol::sftpSetStat(const KURL& url, const sftpFileAttr& attr){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpSetStat(" << url << ", attr)" << endl;
    QString path = url.path();
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + path.length() + attr.size());
    s << (Q_UINT8)SSH2_FXP_SETSTAT;
    s << (Q_UINT32)id;
    s.writeBytes(path.latin1(), path.length());
    s << attr;

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpSetStat(): sftp packet id mismatch" << endl;
        return -1;
        // XXX How do we do a fatal error?
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpSetStat(): unexpected message type of " << type << endl;
        return -1;
    }

    Q_UINT32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpSetStat(): set stat failed with err code " << code << endl;
    }

    return code;
}

/** Sends a sftp command to remove a file or directory. */
int kio_sftpProtocol::sftpRemove(const KURL& url, bool isfile){
    QString path = url.path();
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + path.length());
    s << (Q_UINT8)(isfile ? SSH2_FXP_REMOVE : SSH2_FXP_RMDIR);
    s << (Q_UINT32)id;
    s.writeBytes(path.latin1(), path.length());

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::del(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::del(): unexpected message type of " << type << endl;
        return -1;
    }

    Q_UINT32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::del(): del failed with err code " << code << endl;
    }

    return code;
}
/** Send a sftp command to rename a file or directoy. */
int kio_sftpProtocol::sftpRename(const KURL& src, const KURL& dest){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRename(" << src << " -> " << dest << ")" << endl;

    QString srcPath = src.path();
    QString destPath = dest.path();
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + srcPath.length() +
                    4 /*str length*/ + destPath.length());
    s << (Q_UINT8)SSH2_FXP_RENAME;
    s << (Q_UINT32)id;
    s.writeBytes(srcPath.latin1(), srcPath.length());
    s.writeBytes(destPath.latin1(), destPath.length());

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRename(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRename(): unexpected message type of " << type << endl;
        return -1;
    }

    int code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRename(): rename failed with err code " << code << endl;
    }

    return code;
}
/** Get directory listings. */
int kio_sftpProtocol::sftpReadDir(const QByteArray& handle, const KURL& url){
    // url is needed so we can lookup the link destination
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadDir()" << endl;

    sftpFileAttr attr; 
    QByteArray p;
    Q_UINT32 id, expectedId, count;
    Q_UINT8 type;
    
    attr.setDirAttrsFlag(true);
    QDataStream s(p, IO_WriteOnly);
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + handle.size());
    s << (Q_UINT8)SSH2_FXP_READDIR;
    s << (Q_UINT32)id;
    s << handle;

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);

    r >> type >> id;

    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadDir(): sftp packet id mismatch" << endl;
        return -1;
    }

    int code;
    if( type == SSH2_FXP_STATUS ) {
        r >> code;
        return code;
    }

    if( type != SSH2_FXP_NAME ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocl::sftpReadDir(): Unexpected message" << endl;
        return -1;
    }

    r >> count;
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadDir(): got " << count << " entries" << endl;

    while(count--) {
        r >> attr;
                
        if( S_ISLNK(attr.permissions()) && isSupportedOperation(SSH2_FXP_READLINK) ) {
            KURL myurl ( url );
            myurl.addPath(attr.filename());
            QString target;
            if( (code = sftpReadLink(myurl, target)) == SSH2_FX_OK ) {
                kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadDir(): Got link dest " << target << endl;
                               
                myurl = url;
                if( target[0] == '/' )
                    myurl.setPath(target);                    
                else
                    myurl.addPath(target);
                    
                myurl.cleanPath();
                
                sftpFileAttr attr2;                                        
                (void) sftpStat(myurl, attr2);                                
                if (attr2.fileType() != 0)
                    attr.setLinkType(attr2.fileType());
                
                attr.setLinkDestination(target);
            }
        }
        
        listEntry(attr.entry(), false);
    }
    
    listEntry(attr.entry(), true);
    return SSH2_FX_OK;
}

int kio_sftpProtocol::sftpReadLink(const KURL& url, QString& target){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadLink(" << url << ")" << endl;
    QString path = url.path();
    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + path.length());
    s << (Q_UINT8)SSH2_FXP_READLINK;
    s << id;
    s.writeBytes(path.latin1(), path.length());

    putPacket(p);
    getPacket(p);

    Q_UINT8 type;
    QDataStream r(p, IO_ReadOnly);

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadLink: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        Q_UINT32 code;
        r >> code;
        kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadLink(): read link failed with code " << code << endl;
        return code;
    }

    if( type != SSH2_FXP_NAME ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadLink(): unexpected packet type of " << type << endl;
        return -1;
    }

    Q_UINT32 count;
    r >> count;
    if( count != 1 ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadLink(): Bad number of file attributes for realpath command" << endl;
        return -1;
    }

    QByteArray x;
    r >> x;
    target = QString(x);
    return SSH2_FX_OK;
}

int kio_sftpProtocol::sftpSymLink(const QString& target, const KURL& dest){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpSymLink(" << target << " -> " << dest << ")" << endl;
    QString destPath = dest.path();
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + target.length() +
                    4 /*str length*/ + destPath.length());
    s << (Q_UINT8)SSH2_FXP_SYMLINK;
    s << (Q_UINT32)id;
    s.writeBytes(target.latin1(), target.length());
    s.writeBytes(destPath.latin1(), destPath.length());

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpSymLink(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpSymLink(): unexpected message type of " << type << endl;
        return -1;
    }

    Q_UINT32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpSymLink(): rename failed with err code " << code << endl;
    }

    return code;
}
/** Stats a file. */
int kio_sftpProtocol::sftpStat(const KURL& url, sftpFileAttr& attr){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpStat(): " << url << endl;
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);
    QString path = url.path();

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + path.length());
    s << (Q_UINT8)SSH2_FXP_LSTAT;
    s << (Q_UINT32)id;
    s.writeBytes(path.latin1(), path.length());

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpStat(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        Q_UINT32 errCode;
        r >> errCode;
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpStat(): stat failed with code " << errCode << endl;
        return errCode;
    }

    if( type != SSH2_FXP_ATTRS ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpStat(): unexpected message type of " << type << endl;
        return -1;
    }

    r >> attr;
    attr.setFilename(url.filename());
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpStat(): " << attr << endl;
    return SSH2_FX_OK;
}


int kio_sftpProtocol::sftpOpen(const KURL& url, const Q_UINT32 pflags, const sftpFileAttr& attr, QByteArray& handle){
    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpen(" << url << ", handle)" << endl;

    QByteArray p;
    QDataStream s(p, IO_WriteOnly);
    QString path = url.path();

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + path.length() +
                    4 /*pflags*/ + attr.size());
    s << (Q_UINT8)SSH2_FXP_OPEN;
    s << (Q_UINT32)id;
    s.writeBytes(path.latin1(), path.length());
    s << pflags;
    s << attr;

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpen(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        Q_UINT32 errCode;
        r >> errCode;
        return errCode;
    }

    if( type != SSH2_FXP_HANDLE ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpen(): unexpected message type of " << type << endl;
        return -1;
    }

    r >> handle;
    if( handle.size() > 256 ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpen(): handle exceeds max length" << endl;
        return -1;
    }

    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpOpen(): handle (" << handle.size() << "): [" << handle << "]" << endl;
    return SSH2_FX_OK;
}


int kio_sftpProtocol::sftpRead(const QByteArray& handle, Q_UINT64 offset, Q_UINT32 len, QByteArray& data){
 //   kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRead( offset = " << offset << ", len = " << len << ")" << endl;
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + handle.size() +
                    8 /*offset*/ + 4 /*length*/);
    s << (Q_UINT8)SSH2_FXP_READ;
    s << (Q_UINT32)id;
    s << handle;
    s << offset; // we don't have a convienient 64 bit int so set upper int to zero
    s << len;

    putPacket(p);
    getPacket(p);

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRead: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        Q_UINT32 errCode;
        r >> errCode;
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRead: read failed with code " << errCode << endl;
        return errCode;
    }

    if( type != SSH2_FXP_DATA ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpRead: unexpected message type of " << type << endl;
        return -1;
    }

    r >> data;

    return SSH2_FX_OK;
}


int kio_sftpProtocol::sftpWrite(const QByteArray& handle, Q_UINT64 offset, const QByteArray& data){
//    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpWrite( offset = " << offset <<
//        ", data sz = " << data.size() << ")" << endl;
    QByteArray p;
    QDataStream s(p, IO_WriteOnly);

    Q_UINT32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (Q_UINT32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + handle.size() +
                    8 /*offset*/ +
                    4 /* data size */ + data.size());
    s << (Q_UINT8)SSH2_FXP_WRITE;
    s << (Q_UINT32)id;
    s << handle;
    s << offset; // we don't have a convienient 64 bit int so set upper int to zero
    s << data;

//    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpWrite(): SSH2_FXP_WRITE, id:"
//        << id << ", handle:" << handle << ", offset:" << offset << ", some data" << endl;

//    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpWrite(): send packet [" << p << "]" << endl;

    putPacket(p);
    getPacket(p);

//    kdDebug(KIO_SFTP_DB) << "kio_sftpProtocol::sftpWrite(): received packet [" << p << "]" << endl;

    QDataStream r(p, IO_ReadOnly);
    Q_UINT8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpWrite(): sftp packet id mismatch, got "
            << id << ", expected " << expectedId << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpWrite(): unexpected message type of " << type << endl;
        return -1;
    }

    Q_UINT32 code;
    r >> code;
    return code;
}


