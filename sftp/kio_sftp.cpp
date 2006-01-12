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

#include <q3cstring.h>
#include <qstring.h>
#include <qobject.h>
#include <q3strlist.h>
#include <qfile.h>
#include <qbuffer.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <netdb.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <kapplication.h>
#include <kuser.h>
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
#include <kde_file.h>
#include <kremoteencoding.h>

#include "sftp.h"
#include "kio_sftp.h"
#include "atomicio.h"
#include "sftpfileattr.h"
#include "ksshprocess.h"


using namespace KIO;
extern "C"
{
  int KDE_EXPORT kdemain( int argc, char **argv )
  {
    KInstance instance( "kio_sftp" );

    kdDebug(KIO_SFTP_DB) << "*** Starting kio_sftp " << endl;

    if (argc != 4) {
      kdDebug(KIO_SFTP_DB) << "Usage: kio_sftp  protocol domain-socket1 domain-socket2" << endl;
      exit(-1);
    }

    sftpProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    kdDebug(KIO_SFTP_DB) << "*** kio_sftp Done" << endl;
    return 0;
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

sftpProtocol::sftpProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
             : SlaveBase("kio_sftp", pool_socket, app_socket),
                  mConnected(false), mPort(-1), mMsgId(0) {
  kdDebug(KIO_SFTP_DB) << "sftpProtocol(): pid = " << getpid() << endl;
}


sftpProtocol::~sftpProtocol() {
    kdDebug(KIO_SFTP_DB) << "~sftpProtocol(): pid = " << getpid() << endl;
    closeConnection();
}

/**
  * Type is a sftp packet type found in .sftp.h'.
  * Example: SSH2_FXP_READLINK, SSH2_FXP_RENAME, etc.
  *
  * Returns true if the type is supported by the sftp protocol
  * version negotiated by the client and server (sftpVersion).
  */
bool sftpProtocol::isSupportedOperation(int type) {
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
      kdDebug(KIO_SFTP_DB) << "isSupportedOperation(type:"
                            << type << "): unrecognized operation type" << endl;
      break;
  }

  return false;
}

void sftpProtocol::copy(const KURL &src, const KURL &dest, int permissions, bool overwrite)
{
    kdDebug(KIO_SFTP_DB) << "copy(): " << src << " -> " << dest << endl;

    bool srcLocal = src.isLocalFile();
    bool destLocal = dest.isLocalFile();

    if ( srcLocal && !destLocal ) // Copy file -> sftp
      sftpCopyPut(src, dest, permissions, overwrite);
    else if ( destLocal && !srcLocal ) // Copy sftp -> file
      sftpCopyGet(dest, src, permissions, overwrite);
    else
      error(ERR_UNSUPPORTED_ACTION, QString());
}

void sftpProtocol::sftpCopyGet(const KURL& dest, const KURL& src, int mode, bool overwrite)
{
    kdDebug(KIO_SFTP_DB) << "sftpCopyGet(): " << src << " -> " << dest << endl;

    // Attempt to establish a connection...
    openConnection();
    if( !mConnected )
        return;

    KDE_struct_stat buff_orig;
    Q3CString dest_orig ( QFile::encodeName(dest.path()) );
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
    Q3CString dest_part ( dest_orig + ".part" );

    int fd = -1;
    bool partExists = false;
    bool markPartial = config()->readEntry("MarkPartial", QVariant(true)).toBool();

    if (markPartial)
    {
        KDE_struct_stat buff_part;
        partExists = (KDE_stat( dest_part.data(), &buff_part ) != -1);

        if (partExists && buff_part.st_size > 0 && S_ISREG(buff_part.st_mode))
        {
            if (canResume( buff_part.st_size ))
            {
                offset = buff_part.st_size;
                kdDebug(KIO_SFTP_DB) << "sftpCopyGet: Resuming @ " << offset << endl;
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
      KIO::filesize_t size = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
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
    kdDebug(KIO_SFTP_DB) << "sftpCopyGet(): emit finished()" << endl;
    finished();
}

sftpProtocol::Status sftpProtocol::sftpGet( const KURL& src, KIO::filesize_t offset, int fd )
{
    int code;
    sftpFileAttr attr(remoteEncoding());

    Status res;
    res.code = 0;
    res.size = 0;

    kdDebug(KIO_SFTP_DB) << "sftpGet(): " << src << endl;

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
    quint32 pflags = SSH2_FXF_READ;
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
    quint32 len = 60*1024;
    code = SSH2_FX_OK;

    kdDebug(KIO_SFTP_DB) << "sftpGet(): offset = " << offset << endl;
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
                        KMimeMagic::self()->findBufferFileType(mimeBuffer, src.fileName());
                    kdDebug(KIO_SFTP_DB) << "sftpGet(): mimetype is " <<
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

void sftpProtocol::get(const KURL& url) {
    kdDebug(KIO_SFTP_DB) << "get(): " << url << endl ;

    openConnection();
    if( !mConnected )
        return;

    // Get resume offset
    quint64 offset = config()->readUnsignedLongNumEntry("resume");
    if( offset > 0 ) {
        canResume();
        kdDebug(KIO_SFTP_DB) << "get(): canResume(), offset = " << offset << endl;
    }

    Status info = sftpGet(url, offset);

    if (info.code != 0)
    {
      error(info.code, info.text);
      return;
    }

    data(QByteArray());
    kdDebug(KIO_SFTP_DB) << "get(): emit finished()" << endl;
    finished();
}


void sftpProtocol::setHost (const QString& h, int port, const QString& user, const QString& pass)
{
    kdDebug(KIO_SFTP_DB) << "setHost(): " << user << "@" << h << ":" << port << endl;

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

    mUsername = user;
    mPassword = pass;

    if (user.isEmpty())
    {
      KUser u;
      mUsername = u.loginName();
    }
}


void sftpProtocol::openConnection() {

    if(mConnected)
      return;

    kdDebug(KIO_SFTP_DB) << "openConnection(): " << mUsername << "@"
                         << mHost << ":" << mPort << endl;

    infoMessage( i18n("Opening SFTP connection to host <b>%1:%2</b>").arg(mHost).arg(mPort));

    if( mHost.isEmpty() ) {
        kdDebug(KIO_SFTP_DB) << "openConnection(): Need hostname..." << endl;
        error(ERR_UNKNOWN_HOST, i18n("No hostname specified"));
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
    info.comment = "sftp://" + mHost + ":" + QString::number(mPort);
    info.commentLabel = i18n("site:");
    info.username = mUsername;
    info.keepPassword = true;

    ///////////////////////////////////////////////////////////////////////////
    // Check for cached authentication info if a username AND password were
    // not specified in setHost().
    if( mUsername.isEmpty() && mPassword.isEmpty() ) {
        kdDebug(KIO_SFTP_DB) << "openConnection(): checking cache "
                             << "info.username = " << info.username
                             << ", info.url = " << info.url.prettyURL() << endl;

        if( checkCachedAuthentication(info) ) {
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
		opts.append(opt);
        usernameIt = opts.end()-1;
    }

    if( !mPassword.isEmpty() ) {
        opt.opt = KSshProcess::SSH_PASSWD;
        opt.str = mPassword;
		opts.append(opt);
        passwdIt = opts.end()-1;
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
        kdDebug(KIO_SFTP_DB) << "openConnection(): "
            "Got " << err << " from KSshProcess::connect()" << endl;

        switch(err) {
        case KSshProcess::ERR_NEED_PASSWD:
        case KSshProcess::ERR_NEED_PASSPHRASE:
            // At this point we know that either we didn't set
            // an username or password in the ssh options list,
            // or what we did pass did not work. Therefore we
            // must prompt the user.
            if( err == KSshProcess::ERR_NEED_PASSPHRASE )
                info.prompt = i18n("Please enter your username and key passphrase.");
            else
                info.prompt = i18n("Please enter your username and password.");

            kdDebug(KIO_SFTP_DB) << "openConnection(): info.username = " << info.username
                                 << ", info.url = " << info.url.prettyURL() << endl;

            if( firstTime )
                dlgResult = openPassDlg(info);
            else
                dlgResult = openPassDlg(info, i18n("Incorrect username or password"));

            if( dlgResult ) {
               if( info.username.isEmpty() || info.password.isEmpty() ) {
                    error(ERR_COULD_NOT_AUTHENTICATE,
                      i18n("Please enter a username and password"));
                    continue;
                }
            }
            else {
                // user canceled or dialog failed to open
                error(ERR_USER_CANCELED, QString());
                kdDebug(KIO_SFTP_DB) << "openConnection(): user canceled, dlgResult = " << dlgResult << endl;
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
                kdDebug(KIO_SFTP_DB) << "openConnection(): Username changed from "
                                     << mUsername << " to " << info.username << endl;

                ssh.disconnect();

                // if we haven't yet added the username
                // or password option to the ssh options list then
                // the iterators will be equal to the empty iterator.
                // Create the opts now and add them to the opt list.
                if( usernameIt == KSshProcess::SshOptListIterator() ) {
                    kdDebug(KIO_SFTP_DB) << "openConnection(): "
                        "Adding username to options list" << endl;
                    opt.opt = KSshProcess::SSH_USERNAME;
					opts.append(opt);
                    usernameIt = opts.end()-1;
                }

                if( passwdIt == KSshProcess::SshOptListIterator() ) {
                    kdDebug(KIO_SFTP_DB) << "openConnection(): "
                        "Adding password to options list" << endl;
                    opt.opt = KSshProcess::SSH_PASSWD;
					opts.append(opt);
                    passwdIt = opts.end()-1;
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
                error(ERR_USER_CANCELED, QString());
                return;
            }
            ssh.acceptHostKey(true);
            break;

        case KSshProcess::ERR_DIFF_HOST_KEY:
            caption = i18n("Warning: Host's identity changed.");
            msg = ssh.errorMsg();
            if( KMessageBox::Yes != messageBox(WarningYesNo, msg, caption) ) {
                closeConnection();
                error(ERR_USER_CANCELED, QString());
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
            infoMessage(i18n("Connection failed."));
            caption = i18n("Unexpected SFTP error: %1").arg(err);
            msg = ssh.errorMsg();
            messageBox(Information, msg, caption);
            closeConnection();
            error(ERR_UNKNOWN, msg);
            return;
        }
    }

    // catch all in case we did something wrong above
    if( !mConnected ) {
        error(ERR_INTERNAL, QString());
        return;
    }

    // Now send init packet.
    kdDebug(KIO_SFTP_DB) << "openConnection(): Sending SSH2_FXP_INIT packet." << endl;
    QByteArray p;
    QDataStream packet(&p, QIODevice::WriteOnly);
    packet << (quint32)5;                     // packet length
    packet << (quint8) SSH2_FXP_INIT;         // packet type
    packet << (quint32)SSH2_FILEXFER_VERSION; // client version

    putPacket(p);
    getPacket(p);

    QDataStream s(p);
    quint32 version;
    quint8  type;
    s >> type;
    kdDebug(KIO_SFTP_DB) << "openConnection(): Got type " << type << endl;

    if( type == SSH2_FXP_VERSION ) {
        s >> version;
        kdDebug(KIO_SFTP_DB) << "openConnection(): Got server version " << version << endl;

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
    kdDebug(KIO_SFTP_DB) << "sftpProtocol(): caching info.username = " << info.username <<
        ", info.url = " << info.url.prettyURL() << endl;
    cacheAuthentication(info);
    mConnected = true;
    connected();

    mPassword.fill('x');
    info.password.fill('x');

    return;
}

void sftpProtocol::closeConnection() {
    kdDebug(KIO_SFTP_DB) << "closeConnection()" << endl;
    ssh.disconnect();
    mConnected = false;
}

void sftpProtocol::sftpCopyPut(const KURL& src, const KURL& dest, int permissions, bool overwrite) {

    KDE_struct_stat buff;
    Q3CString file (QFile::encodeName(src.path()));

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

void sftpProtocol::sftpPut( const KURL& dest, int permissions, bool resume, bool overwrite, int fd ) {

    openConnection();
    if( !mConnected )
        return;

    kdDebug(KIO_SFTP_DB) << "sftpPut(): " << dest
                         << ", resume=" << resume
                         << ", overwrite=" << overwrite << endl;

    KURL origUrl( dest );
    sftpFileAttr origAttr(remoteEncoding());
    bool origExists = false;

    // Stat original (without part ext)  to see if it already exists
    int code = sftpStat(origUrl, origAttr);

    if( code == SSH2_FX_OK ) {
        kdDebug(KIO_SFTP_DB) << "sftpPut(): <file> already exists" << endl;

        // Delete remote file if its size is zero
        if( origAttr.fileSize() == 0 ) {
            if( sftpRemove(origUrl, true) != SSH2_FX_OK ) {
                error(ERR_CANNOT_DELETE_ORIGINAL, origUrl.prettyURL());
                return;
            }
        }
        else {
            origExists = true;
        }
    }
    else if( code != SSH2_FX_NO_SUCH_FILE ) {
        processStatus(code, origUrl.prettyURL());
        return;
    }

    // Do not waste time/resources with more remote stat calls if the file exists
    // and we weren't instructed to overwrite it...
    if( origExists && !overwrite ) {
        error(ERR_FILE_ALREADY_EXIST, origUrl.prettyURL());
        return;
    }

    // Stat file with part ext to see if it already exists...
    KURL partUrl( origUrl );
    partUrl.setFileName( partUrl.fileName() + ".part" );

    quint64 offset = 0;
    bool partExists = false;
    bool markPartial = config()->readEntry("MarkPartial", QVariant(true)).toBool();

    if( markPartial ) {

        sftpFileAttr partAttr(remoteEncoding());
        code = sftpStat(partUrl, partAttr);

        if( code == SSH2_FX_OK ) {
            kdDebug(KIO_SFTP_DB) << "sftpPut(): .part file already exists" << endl;
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

                    kdDebug(KIO_SFTP_DB) << "sftpPut(): can resume = " << resume
                                         << ", offset = " << offset;

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

    quint32 pflags = 0;
    if( overwrite && !resume )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_TRUNC;
    else if( !overwrite && !resume )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_EXCL;
    else if( overwrite && resume )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT;
    else if( !overwrite && resume )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_APPEND;

    sftpFileAttr attr(remoteEncoding());
    QByteArray handle;

    // Set the permissions of the file we write to if it didn't already exist
    // and the permission info is supplied, i.e it is not -1
    if( !partExists && !origExists && permissions != -1)
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

        if( nbytes >= 0 ) {
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

    if( nbytes < 0 ) {
        sftpClose(handle);

        if( markPartial ) {
            // Remove remote file if it smaller than our keep size
            uint minKeepSize = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);

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

void sftpProtocol::put ( const KURL& url, int permissions, bool overwrite, bool resume ){
    kdDebug(KIO_SFTP_DB) << "put(): " << url << ", overwrite = " << overwrite
                         << ", resume = " << resume << endl;

    sftpPut( url, permissions, resume, overwrite );
}

void sftpProtocol::stat ( const KURL& url ){
    kdDebug(KIO_SFTP_DB) << "stat(): " << url << endl;

    openConnection();
    if( !mConnected )
        return;

    // If the stat URL has no path, do not attempt to determine the real
    // path and do a redirect. KRun will simply ignore such requests.
    // Instead, simply return the mime-type as a directory...
    if( !url.hasPath() ) {
        UDSEntry entry;

        entry.insert( KIO::UDS_NAME, QString::fromLatin1(".") );
        entry.insert( KIO::UDS_FILE_TYPE, S_IFDIR );
        entry.insert( KIO::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
        entry.insert( KIO::UDS_USER, mUsername );
        entry.insert( KIO::UDS_GROUP, mUsername );

        // no size
        statEntry( entry );
        finished();
        return;
    }

    int code;
    sftpFileAttr attr(remoteEncoding());
    if( (code = sftpStat(url, attr)) != SSH2_FX_OK ) {
        processStatus(code, url.prettyURL());
        return;
    }
    else {
        //kdDebug() << "We sent and received stat packet ok" << endl;
        attr.setFilename(url.fileName());
        statEntry(attr.entry());
    }

    finished();

    kdDebug(KIO_SFTP_DB) << "stat: END" << endl;
    return;
}


void sftpProtocol::mimetype ( const KURL& url ){
    kdDebug(KIO_SFTP_DB) << "mimetype(): " << url << endl;

    openConnection();
    if( !mConnected )
        return;

    quint32 pflags = SSH2_FXF_READ;
    QByteArray handle, mydata;
    sftpFileAttr attr(remoteEncoding());
    int code;
    if( (code = sftpOpen(url, pflags, attr, handle)) != SSH2_FX_OK ) {
        error(ERR_CANNOT_OPEN_FOR_READING, url.prettyURL());
        return;
    }

    quint32 len = 1024; // Get first 1k for determining mimetype
    quint64 offset = 0;
    code = SSH2_FX_OK;
    while( offset < len && code == SSH2_FX_OK ) {
        if( (code = sftpRead(handle, offset, len, mydata)) == SSH2_FX_OK ) {
            data(mydata);
            offset += mydata.size();
            processedSize(offset);

            kdDebug(KIO_SFTP_DB) << "mimetype(): offset = " << offset << endl;
        }
    }


    data(QByteArray());
    processedSize(offset);
    sftpClose(handle);
    finished();
    kdDebug(KIO_SFTP_DB) << "mimetype(): END" << endl;
}


void sftpProtocol::listDir(const KURL& url) {
    kdDebug(KIO_SFTP_DB) << "listDir(): " << url << endl;

    openConnection();
    if( !mConnected )
        return;

    if( !url.hasPath() ) {
        KURL newUrl ( url );
        if( sftpRealPath(url, newUrl) == SSH2_FX_OK ) {
            kdDebug(KIO_SFTP_DB) << "listDir: Redirecting to " << newUrl << endl;
            redirection(newUrl);
            finished();
            return;
        }
    }

    int code;
    QByteArray handle;

    if( (code = sftpOpenDirectory(url, handle)) != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "listDir(): open directory failed" << endl;
        processStatus(code, url.prettyURL());
        return;
    }


    code = SSH2_FX_OK;
    while( code == SSH2_FX_OK ) {
        code = sftpReadDir(handle, url);
        if( code != SSH2_FX_OK && code != SSH2_FX_EOF )
            processStatus(code, url.prettyURL());
        kdDebug(KIO_SFTP_DB) << "listDir(): return code = " << code << endl;
    }

    if( (code = sftpClose(handle)) != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "listdir(): closing of directory failed" << endl;
        processStatus(code, url.prettyURL());
        return;
    }

    finished();
    kdDebug(KIO_SFTP_DB) << "listDir(): END" << endl;
}

/** Make a directory.
    OpenSSH does not follow the internet draft for sftp in this case.
    The format of the mkdir request expected by OpenSSH sftp server is:
        uint32 id
        string path
        ATTR   attr
 */
void sftpProtocol::mkdir(const KURL&url, int permissions){

    kdDebug(KIO_SFTP_DB) << "mkdir() creating dir: " << url.path() << endl;

    openConnection();
    if( !mConnected )
        return;

    Q3CString path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    sftpFileAttr attr(remoteEncoding());

    if (permissions != -1)
      attr.setPermissions(permissions);

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << quint32(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + len + attr.size());
    s << (quint8)SSH2_FXP_MKDIR;
    s << id;
    s.writeBytes(path.data(), len);
    s << attr;

    kdDebug(KIO_SFTP_DB) << "mkdir(): packet size is " << p.size() << endl;

    putPacket(p);
    getPacket(p);

    quint8 type;
    QDataStream r(p);

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "mkdir: sftp packet id mismatch" << endl;
        error(ERR_COULD_NOT_MKDIR, path);
        finished();
        return;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "mkdir(): unexpected packet type of " << type << endl;
        error(ERR_COULD_NOT_MKDIR, path);
        finished();
        return;
    }

    int code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "mkdir(): failed with code " << code << endl;

        // Check if mkdir failed because the directory already exists so that
        // we can return the appropriate message...
        sftpFileAttr dirAttr(remoteEncoding());
        if ( sftpStat(url, dirAttr) == SSH2_FX_OK )
        {
          error( ERR_DIR_ALREADY_EXIST, url.prettyURL() );
          return;
        }

        error(ERR_COULD_NOT_MKDIR, path);
    }

    finished();
}

void sftpProtocol::rename(const KURL& src, const KURL& dest, bool overwrite){
    kdDebug(KIO_SFTP_DB) << "rename(" << src << " -> " << dest << ")" << endl;

    if (!isSupportedOperation(SSH2_FXP_RENAME)) {
      error(ERR_UNSUPPORTED_ACTION,
          i18n("The remote host does not support renaming files."));
      return;
    }

    openConnection();
    if( !mConnected )
        return;

    // Always stat the destination before attempting to rename
    // a file or a directory...
    sftpFileAttr attr(remoteEncoding());
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
    kdDebug(KIO_SFTP_DB) << "rename(): END" << endl;
}

void sftpProtocol::symlink(const QString& target, const KURL& dest, bool overwrite){
    kdDebug(KIO_SFTP_DB) << "symlink()" << endl;

    if (!isSupportedOperation(SSH2_FXP_SYMLINK)) {
      error(ERR_UNSUPPORTED_ACTION,
          i18n("The remote host does not support creating symbolic links."));
      return;
    }

    openConnection();
    if( !mConnected )
        return;

    int code;
    bool failed = false;
    if( (code = sftpSymLink(target, dest)) != SSH2_FX_OK ) {
        if( overwrite ) { // try to delete the destination
            sftpFileAttr attr(remoteEncoding());
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

void sftpProtocol::chmod(const KURL& url, int permissions){
    QString perms;
    perms.setNum(permissions, 8);
    kdDebug(KIO_SFTP_DB) << "chmod(" << url << ", " << perms << ")" << endl;

    openConnection();
    if( !mConnected )
        return;

    sftpFileAttr attr(remoteEncoding());

    if (permissions != -1)
      attr.setPermissions(permissions);

    int code;
    if( (code = sftpSetStat(url, attr)) != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "chmod(): sftpSetStat failed with error " << code << endl;
        if( code == SSH2_FX_FAILURE )
            error(ERR_CANNOT_CHMOD, QString());
        else
            processStatus(code, url.prettyURL());
    }
    finished();
}


void sftpProtocol::del(const KURL &url, bool isfile){
    kdDebug(KIO_SFTP_DB) << "del(" << url << ", " << (isfile?"file":"dir") << ")" << endl;

    openConnection();
    if( !mConnected )
        return;

    int code;
    if( (code = sftpRemove(url, isfile)) != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "del(): sftpRemove failed with error code " << code << endl;
        processStatus(code, url.prettyURL());
    }
    finished();
}

void sftpProtocol::slave_status() {
    kdDebug(KIO_SFTP_DB) << "slave_status(): connected to "
                         <<  mHost << "? " << mConnected << endl;

    slaveStatus ((mConnected ? mHost : QString()), mConnected);
}

bool sftpProtocol::getPacket(QByteArray& msg) {
    QByteArray buf(4096);

    // Get the message length...
    ssize_t len = atomicio(ssh.stdioFd(), buf.data(), 4, true /*read*/);

    if( len == 0 || len == -1 ) {
        kdDebug(KIO_SFTP_DB) << "getPacket(): read of packet length failed, ret = "
                             << len << ", error =" << strerror(errno) << endl;
        closeConnection();
        error( ERR_CONNECTION_BROKEN, mHost);
        msg.resize(0);
        return false;
    }

    int msgLen;
    QDataStream s(buf);
    s >> msgLen;

    //kdDebug(KIO_SFTP_DB) << "getPacket(): Message size = " << msgLen << endl;

    msg.resize(0);

    QBuffer b( &msg );
    b.open( QIODevice::WriteOnly );

    while( msgLen ) {
        len = atomicio(ssh.stdioFd(), buf.data(), qMin(buf.size(), msgLen), true /*read*/);

        if( len == 0 || len == -1) {
            QString errmsg;
            if (len == 0)
              errmsg = i18n("Connection closed");
            else
              errmsg = i18n("Could not read SFTP packet");
            kdDebug(KIO_SFTP_DB) << "getPacket(): nothing to read, ret = " <<
                len << ", error =" << strerror(errno) << endl;
            closeConnection();
            error(ERR_CONNECTION_BROKEN, errmsg);
            b.close();
            return false;
        }

        b.writeBlock(buf.data(), len);

        //kdDebug(KIO_SFTP_DB) << "getPacket(): Read Message size = " << len << endl;
        //kdDebug(KIO_SFTP_DB) << "getPacket(): Copy Message size = " << msg.size() << endl;

        msgLen -= len;
    }

    b.close();

    return true;
}

/** Send an sftp packet to stdin of the ssh process. */
bool sftpProtocol::putPacket(QByteArray& p){
//    kdDebug(KIO_SFTP_DB) << "putPacket(): size == " << p.size() << endl;
    int ret;
    ret = atomicio(ssh.stdioFd(), p.data(), p.size(), false /*write*/);
    if( ret <= 0 ) {
        kdDebug(KIO_SFTP_DB) << "putPacket(): write failed, ret =" << ret <<
            ", error = " << strerror(errno) << endl;
        return false;
    }

    return true;
}

/** Used to have the server canonicalize any given path name to an absolute path.
This is useful for converting path names containing ".." components or relative
pathnames without a leading slash into absolute paths.
Returns the canonicalized url. */
int sftpProtocol::sftpRealPath(const KURL& url, KURL& newUrl){

    kdDebug(KIO_SFTP_DB) << "sftpRealPath(" << url << ", newUrl)" << endl;

    Q3CString path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << quint32(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + len);
    s << (quint8)SSH2_FXP_REALPATH;
    s << id;
    s.writeBytes(path.data(), len);

    putPacket(p);
    getPacket(p);

    quint8 type;
    QDataStream r(p);

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpRealPath: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 code;
        r >> code;
        return code;
    }

    if( type != SSH2_FXP_NAME ) {
        kdError(KIO_SFTP_DB) << "sftpRealPath(): unexpected packet type of " << type << endl;
        return -1;
    }

    quint32 count;
    r >> count;
    if( count != 1 ) {
        kdError(KIO_SFTP_DB) << "sftpRealPath(): Bad number of file attributes for realpath command" << endl;
        return -1;
    }

    Q3CString newPath;
    r >> newPath;

    newPath.truncate(newPath.size());
    if (newPath.isEmpty())
      newPath = "/";
    newUrl.setPath(newPath);

    return SSH2_FX_OK;
}

sftpProtocol::Status sftpProtocol::doProcessStatus(quint8 code, const QString& message)
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
void sftpProtocol::processStatus(quint8 code, const QString& message){
    Status st = doProcessStatus( code, message );
    if( st.code != 0 )
        error( st.code, st.text );
}

/** Opens a directory handle for url.path. Returns true if succeeds. */
int sftpProtocol::sftpOpenDirectory(const KURL& url, QByteArray& handle){

    kdDebug(KIO_SFTP_DB) << "sftpOpenDirectory(" << url << ", handle)" << endl;

    Q3CString path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + len);
    s << (quint8)SSH2_FXP_OPENDIR;
    s << (quint32)id;
    s.writeBytes(path.data(), len);

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpOpenDirectory: sftp packet id mismatch: " <<
            "expected " << expectedId << ", got " << id << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 errCode;
        r >> errCode;
        return errCode;
    }

    if( type != SSH2_FXP_HANDLE ) {
        kdError(KIO_SFTP_DB) << "sftpOpenDirectory: unexpected message type of " << type << endl;
        return -1;
    }

    r >> handle;
    if( handle.size() > 256 ) {
        kdError(KIO_SFTP_DB) << "sftpOpenDirectory: handle exceeds max length" << endl;
        return -1;
    }

    kdDebug(KIO_SFTP_DB) << "sftpOpenDirectory: handle (" << handle.size() << "): [" << handle << "]" << endl;
    return SSH2_FX_OK;
}

/** Closes a directory or file handle. */
int sftpProtocol::sftpClose(const QByteArray& handle){

    kdDebug(KIO_SFTP_DB) << "sftpClose()" << endl;

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + handle.size());
    s << (quint8)SSH2_FXP_CLOSE;
    s << (quint32)id;
    s << handle;

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpClose: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "sftpClose: unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "sftpClose: close failed with err code " << code << endl;
    }

    return code;
}

/** Set a files attributes. */
int sftpProtocol::sftpSetStat(const KURL& url, const sftpFileAttr& attr){

    kdDebug(KIO_SFTP_DB) << "sftpSetStat(" << url << ", attr)" << endl;

    Q3CString path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + len + attr.size());
    s << (quint8)SSH2_FXP_SETSTAT;
    s << (quint32)id;
    s.writeBytes(path.data(), len);
    s << attr;

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpSetStat(): sftp packet id mismatch" << endl;
        return -1;
        // XXX How do we do a fatal error?
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "sftpSetStat(): unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "sftpSetStat(): set stat failed with err code " << code << endl;
    }

    return code;
}

/** Sends a sftp command to remove a file or directory. */
int sftpProtocol::sftpRemove(const KURL& url, bool isfile){

    kdDebug(KIO_SFTP_DB) << "sftpRemove(): " << url << ", isFile ? " << isfile << endl;

    Q3CString path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + len);
    s << (quint8)(isfile ? SSH2_FXP_REMOVE : SSH2_FXP_RMDIR);
    s << (quint32)id;
    s.writeBytes(path.data(), len);

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "del(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "del(): unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "del(): del failed with err code " << code << endl;
    }

    return code;
}

/** Send a sftp command to rename a file or directoy. */
int sftpProtocol::sftpRename(const KURL& src, const KURL& dest){

    kdDebug(KIO_SFTP_DB) << "sftpRename(" << src << " -> " << dest << ")" << endl;

    Q3CString srcPath = remoteEncoding()->encode(src.path());
    Q3CString destPath = remoteEncoding()->encode(dest.path());

    uint slen = srcPath.length();
    uint dlen = destPath.length();

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + slen +
                    4 /*str length*/ + dlen);
    s << (quint8)SSH2_FXP_RENAME;
    s << (quint32)id;
    s.writeBytes(srcPath.data(), slen);
    s.writeBytes(destPath.data(), dlen);

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpRename(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "sftpRename(): unexpected message type of " << type << endl;
        return -1;
    }

    int code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "sftpRename(): rename failed with err code " << code << endl;
    }

    return code;
}
/** Get directory listings. */
int sftpProtocol::sftpReadDir(const QByteArray& handle, const KURL& url){
    // url is needed so we can lookup the link destination
    kdDebug(KIO_SFTP_DB) << "sftpReadDir(): " << url << endl;

    quint32 id, expectedId, count;
    quint8 type;

    sftpFileAttr attr (remoteEncoding());
    attr.setDirAttrsFlag(true);

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    id = expectedId = mMsgId++;
    s << (quint32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + handle.size());
    s << (quint8)SSH2_FXP_READDIR;
    s << (quint32)id;
    s << handle;

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    r >> type >> id;

    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpReadDir(): sftp packet id mismatch" << endl;
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
    kdDebug(KIO_SFTP_DB) << "sftpReadDir(): got " << count << " entries" << endl;

    while(count--) {
        r >> attr;

        if( S_ISLNK(attr.permissions()) ) {
             KURL myurl ( url );
             myurl.addPath(attr.filename());

             // Stat the symlink to find out its type...
             sftpFileAttr attr2 (remoteEncoding());
             (void) sftpStat(myurl, attr2);

             attr.setLinkType(attr2.linkType());
             attr.setLinkDestination(attr2.linkDestination());
        }

        listEntry(attr.entry(), false);
    }

    listEntry(attr.entry(), true);

    return SSH2_FX_OK;
}

int sftpProtocol::sftpReadLink(const KURL& url, QString& target){

    kdDebug(KIO_SFTP_DB) << "sftpReadLink(): " << url << endl;

    Q3CString path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    //kdDebug(KIO_SFTP_DB) << "sftpReadLink(): Encoded Path: " << path << endl;
    //kdDebug(KIO_SFTP_DB) << "sftpReadLink(): Encoded Size: " << len << endl;

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + len);
    s << (quint8)SSH2_FXP_READLINK;
    s << id;
    s.writeBytes(path.data(), len);


    putPacket(p);
    getPacket(p);

    quint8 type;
    QDataStream r(p);

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpReadLink(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 code;
        r >> code;
        kdDebug(KIO_SFTP_DB) << "sftpReadLink(): read link failed with code " << code << endl;
        return code;
    }

    if( type != SSH2_FXP_NAME ) {
        kdError(KIO_SFTP_DB) << "sftpReadLink(): unexpected packet type of " << type << endl;
        return -1;
    }

    quint32 count;
    r >> count;
    if( count != 1 ) {
        kdError(KIO_SFTP_DB) << "sftpReadLink(): Bad number of file attributes for realpath command" << endl;
        return -1;
    }

    Q3CString linkAddress;
    r >> linkAddress;

    linkAddress.truncate(linkAddress.size());
    kdDebug(KIO_SFTP_DB) << "sftpReadLink(): Link address: " << linkAddress << endl;

    target = remoteEncoding()->decode(linkAddress);

    return SSH2_FX_OK;
}

int sftpProtocol::sftpSymLink(const QString& _target, const KURL& dest){

    Q3CString destPath = remoteEncoding()->encode(dest.path());
    Q3CString target = remoteEncoding()->encode(_target);
    uint dlen = destPath.length();
    uint tlen = target.length();

    kdDebug(KIO_SFTP_DB) << "sftpSymLink(" << target << " -> " << destPath << ")" << endl;

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + tlen +
                    4 /*str length*/ + dlen);
    s << (quint8)SSH2_FXP_SYMLINK;
    s << (quint32)id;
    s.writeBytes(target.data(), tlen);
    s.writeBytes(destPath.data(), dlen);

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpSymLink(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "sftpSymLink(): unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kdError(KIO_SFTP_DB) << "sftpSymLink(): rename failed with err code " << code << endl;
    }

    return code;
}

/** Stats a file. */
int sftpProtocol::sftpStat(const KURL& url, sftpFileAttr& attr) {

    kdDebug(KIO_SFTP_DB) << "sftpStat(): " << url << endl;

    Q3CString path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ + 4 /*str length*/ + len);
    s << (quint8)SSH2_FXP_LSTAT;
    s << (quint32)id;
    s.writeBytes(path.data(), len);

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpStat(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 errCode;
        r >> errCode;
        kdError(KIO_SFTP_DB) << "sftpStat(): stat failed with code " << errCode << endl;
        return errCode;
    }

    if( type != SSH2_FXP_ATTRS ) {
        kdError(KIO_SFTP_DB) << "sftpStat(): unexpected message type of " << type << endl;
        return -1;
    }

    r >> attr;
    attr.setFilename(url.fileName());
    kdDebug(KIO_SFTP_DB) << "sftpStat(): " << attr << endl;

    // If the stat'ed resource is a symlink, perform a recursive stat
    // to determine the actual destination's type (file/dir).
    if( S_ISLNK(attr.permissions()) && isSupportedOperation(SSH2_FXP_READLINK) ) {

        QString target;
        int code = sftpReadLink( url, target );

        if ( code != SSH2_FX_OK ) {
            kdError(KIO_SFTP_DB) << "sftpStat(): Unable to stat symlink destination" << endl;
            return -1;
        }

        kdDebug(KIO_SFTP_DB) << "sftpStat(): Resource is a symlink to -> " << target << endl;

        KURL dest( url );
        if( target[0] == '/' )
            dest.setPath(target);
        else
            dest.setFileName(target);

        dest.cleanPath();

        // Ignore symlinks that point to themselves...
        if ( dest != url ) {

            sftpFileAttr attr2 (remoteEncoding());
            (void) sftpStat(dest, attr2);

            if (attr2.linkType() == 0)
                attr.setLinkType(attr2.fileType());
            else
                attr.setLinkType(attr2.linkType());

            attr.setLinkDestination(target);

            kdDebug(KIO_SFTP_DB) << "sftpStat(): File type: " << attr.fileType() << endl;
        }
    }

    return SSH2_FX_OK;
}


int sftpProtocol::sftpOpen(const KURL& url, const quint32 pflags,
                           const sftpFileAttr& attr, QByteArray& handle) {
    kdDebug(KIO_SFTP_DB) << "sftpOpen(" << url << ", handle" << endl;

    Q3CString path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    quint32 id, expectedId;
    id = expectedId = mMsgId++;

    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);
    s << (quint32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + len +
                    4 /*pflags*/ + attr.size());
    s << (quint8)SSH2_FXP_OPEN;
    s << (quint32)id;
    s.writeBytes(path.data(), len);
    s << pflags;
    s << attr;

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpOpen(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 errCode;
        r >> errCode;
        return errCode;
    }

    if( type != SSH2_FXP_HANDLE ) {
        kdError(KIO_SFTP_DB) << "sftpOpen(): unexpected message type of " << type << endl;
        return -1;
    }

    r >> handle;
    if( handle.size() > 256 ) {
        kdError(KIO_SFTP_DB) << "sftpOpen(): handle exceeds max length" << endl;
        return -1;
    }

    kdDebug(KIO_SFTP_DB) << "sftpOpen(): handle (" << handle.size() << "): [" << handle << "]" << endl;
    return SSH2_FX_OK;
}


int sftpProtocol::sftpRead(const QByteArray& handle, KIO::filesize_t offset, quint32 len, QByteArray& data)
{
 //   kdDebug(KIO_SFTP_DB) << "sftpRead( offset = " << offset << ", len = " << len << ")" << endl;
    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);

    quint32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (quint32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + handle.size() +
                    8 /*offset*/ + 4 /*length*/);
    s << (quint8)SSH2_FXP_READ;
    s << (quint32)id;
    s << handle;
    s << offset; // we don't have a convienient 64 bit int so set upper int to zero
    s << len;

    putPacket(p);
    getPacket(p);

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpRead: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 errCode;
        r >> errCode;
        kdError(KIO_SFTP_DB) << "sftpRead: read failed with code " << errCode << endl;
        return errCode;
    }

    if( type != SSH2_FXP_DATA ) {
        kdError(KIO_SFTP_DB) << "sftpRead: unexpected message type of " << type << endl;
        return -1;
    }

    r >> data;

    return SSH2_FX_OK;
}


int sftpProtocol::sftpWrite(const QByteArray& handle, KIO::filesize_t offset, const QByteArray& data){
//    kdDebug(KIO_SFTP_DB) << "sftpWrite( offset = " << offset <<
//        ", data sz = " << data.size() << ")" << endl;
    QByteArray p;
    QDataStream s(&p, QIODevice::WriteOnly);

    quint32 id, expectedId;
    id = expectedId = mMsgId++;
    s << (quint32)(1 /*type*/ + 4 /*id*/ +
                    4 /*str length*/ + handle.size() +
                    8 /*offset*/ +
                    4 /* data size */ + data.size());
    s << (quint8)SSH2_FXP_WRITE;
    s << (quint32)id;
    s << handle;
    s << offset; // we don't have a convienient 64 bit int so set upper int to zero
    s << data;

//    kdDebug(KIO_SFTP_DB) << "sftpWrite(): SSH2_FXP_WRITE, id:"
//        << id << ", handle:" << handle << ", offset:" << offset << ", some data" << endl;

//    kdDebug(KIO_SFTP_DB) << "sftpWrite(): send packet [" << p << "]" << endl;

    putPacket(p);
    getPacket(p);

//    kdDebug(KIO_SFTP_DB) << "sftpWrite(): received packet [" << p << "]" << endl;

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kdError(KIO_SFTP_DB) << "sftpWrite(): sftp packet id mismatch, got "
            << id << ", expected " << expectedId << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kdError(KIO_SFTP_DB) << "sftpWrite(): unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    return code;
}


