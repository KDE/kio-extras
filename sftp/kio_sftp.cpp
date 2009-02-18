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
We are pretty much left with kDebug messages for debugging. We can't use a gdb
as described in the ioslave DEBUG.howto because kdeinit has to run in a terminal.
Ssh will detect this terminal and ask for a password there, but will just get garbage.
So we can't connect.
*/

#include "kio_sftp.h"

#include <config-runtime.h>

#include <fcntl.h>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QString>

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
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <kio/ioslave_defaults.h>
#include <kmimetype.h>
#include <kde_file.h>
#include <kremoteencoding.h>
#include <kconfiggroup.h>

#include "sftp.h"
#include "atomicio.h"
#include "sftpfileattr.h"
#include "ksshprocess.h"


using namespace KIO;
extern "C"
{
  int KDE_EXPORT kdemain( int argc, char **argv )
  {
    KComponentData componentData( "kio_sftp" );

    kDebug(KIO_SFTP_DB) << "*** Starting kio_sftp ";

    if (argc != 4) {
      kDebug(KIO_SFTP_DB) << "Usage: kio_sftp  protocol domain-socket1 domain-socket2";
      exit(-1);
    }

    sftpProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    kDebug(KIO_SFTP_DB) << "*** kio_sftp Done";
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
#ifndef Q_WS_WIN
  kDebug(KIO_SFTP_DB) << "sftpProtocol(): pid = " << getpid();
#endif
}


sftpProtocol::~sftpProtocol() {
#ifndef Q_WS_WIN
    kDebug(KIO_SFTP_DB) << "~sftpProtocol(): pid = " << getpid();
#endif
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
      kDebug(KIO_SFTP_DB) << "isSupportedOperation(type:"
                            << type << "): unrecognized operation type" << endl;
      break;
  }

  return false;
}

void sftpProtocol::copy(const KUrl &src, const KUrl &dest, int permissions, KIO::JobFlags flags)
{
    kDebug(KIO_SFTP_DB) << "copy(): " << src << " -> " << dest;

    bool srcLocal = src.isLocalFile();
    bool destLocal = dest.isLocalFile();

    if ( srcLocal && !destLocal ) // Copy file -> sftp
      sftpCopyPut(src, dest, permissions, flags);
    else if ( destLocal && !srcLocal ) // Copy sftp -> file
      sftpCopyGet(dest, src, permissions, flags);
    else
      error(ERR_UNSUPPORTED_ACTION, QString());
}

void sftpProtocol::sftpCopyGet(const KUrl& dest, const KUrl& src, int mode, KIO::JobFlags flags)
{
    kDebug(KIO_SFTP_DB) << "sftpCopyGet(): " << src << " -> " << dest;

    // Attempt to establish a connection...
    openConnection();
    if( !mConnected )
        return;

    KDE_struct_stat buff_orig;
    const QString dest_orig = dest.path();
    bool origExists = (KDE::lstat( dest_orig, &buff_orig ) != -1);

    if (origExists)
    {
        if (S_ISDIR(buff_orig.st_mode))
        {
          error(ERR_IS_DIRECTORY, dest.prettyUrl());
          return;
        }

        if (!(flags & KIO::Overwrite))
        {
          error(ERR_FILE_ALREADY_EXIST, dest.prettyUrl());
          return;
        }
    }

    KIO::filesize_t offset = 0;
    QString dest_part(dest_orig + ".part");

    int fd = -1;
    bool partExists = false;
    bool markPartial = config()->readEntry("MarkPartial", true);

    if (markPartial)
    {
        KDE_struct_stat buff_part;
        partExists = (KDE::stat(dest_part, &buff_part ) != -1);

        if (partExists && buff_part.st_size > 0 && S_ISREG(buff_part.st_mode))
        {
            if (canResume( buff_part.st_size ))
            {
                offset = buff_part.st_size;
                kDebug(KIO_SFTP_DB) << "sftpCopyGet: Resuming @ " << offset;
            }
        }

        if (offset > 0)
        {
            fd = KDE::open(dest_part, O_RDWR);
            offset = KDE_lseek(fd, 0, SEEK_END);
            if (offset == 0)
            {
              error(ERR_CANNOT_RESUME, dest.prettyUrl());
              return;
            }
        }
        else
        {
            // Set up permissions properly, based on what is done in file io-slave
            int openFlags = (O_CREAT | O_TRUNC | O_WRONLY);
            int initialMode = (mode == -1) ? 0666 : (mode | S_IWUSR);
            fd = KDE::open(dest_part, openFlags, initialMode);
        }
    }
    else
    {
        // Set up permissions properly, based on what is done in file io-slave
        int openFlags = (O_CREAT | O_TRUNC | O_WRONLY);
        int initialMode = (mode == -1) ? 0666 : (mode | S_IWUSR);
        fd = KDE::open(dest_orig, openFlags, initialMode);
    }

    if(fd == -1)
    {
      kDebug(KIO_SFTP_DB) << "sftpCopyGet: Unable to open (" << fd << ") for writing.";
      if (errno == EACCES)
        error (ERR_WRITE_ACCESS_DENIED, dest.prettyUrl());
      else
        error (ERR_CANNOT_OPEN_FOR_WRITING, dest.prettyUrl());
      return;
    }

    Status info = sftpGet(src, offset, fd);
    if ( info.code != 0 )
    {
      // Should we keep the partially downloaded file ??
      KIO::filesize_t size = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
      if (info.size < size)
        QFile::remove(dest_part);

      error(info.code, info.text);
      return;
    }

    if (::close(fd) != 0)
    {
      error(ERR_COULD_NOT_WRITE, dest.prettyUrl());
      return;
    }

    //
    if (markPartial)
    {
      if (KDE::rename(dest_part, dest_orig) != 0)
      {
        error (ERR_CANNOT_RENAME_PARTIAL, dest_part);
        return;
      }
    }

    data(QByteArray());
    kDebug(KIO_SFTP_DB) << "sftpCopyGet(): emit finished()";
    finished();
}

sftpProtocol::Status sftpProtocol::sftpGet( const KUrl& src, KIO::filesize_t offset, int fd, bool abortAfterMimeType )
{
    int code;
    sftpFileAttr attr(remoteEncoding());

    Status res;
    res.code = 0;
    res.size = 0;

    kDebug(KIO_SFTP_DB) << "sftpGet(): " << src;

    // stat the file first to get its size
    if( (code = sftpStat(src, attr)) != SSH2_FX_OK ) {
        return doProcessStatus(code, src.prettyUrl());
    }

    // We cannot get file if it is a directory
    if( attr.fileType() == S_IFDIR ) {
        res.text = src.prettyUrl();
        res.code = ERR_IS_DIRECTORY;
        return res;
    }

    KIO::filesize_t fileSize = attr.fileSize();
    quint32 pflags = SSH2_FXF_READ;
    attr.clear();

    QByteArray handle;
    if( (code = sftpOpen(src, pflags, attr, handle)) != SSH2_FX_OK ) {
        res.text = src.prettyUrl();
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

    kDebug(KIO_SFTP_DB) << "sftpGet(): offset = " << offset;
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
                    KMimeType::Ptr mime = KMimeType::findByNameAndContent(src.fileName(), mimeBuffer);
                    kDebug(KIO_SFTP_DB) << "sftpGet(): mimetype is " <<
                                      mime->name() << endl;
                    mimeType(mime->name());

                    if (abortAfterMimeType)
                        break;

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

    if( code != SSH2_FX_EOF && !abortAfterMimeType ) {
        res.text = src.prettyUrl();
        res.code = ERR_COULD_NOT_READ; // return here or still send empty array to indicate end of read?
    }

    res.size = offset;
    sftpClose(handle);
    processedSize (offset);
    return res;
}

void sftpProtocol::get(const KUrl& url) {
    kDebug(KIO_SFTP_DB) << "get(): " << url;

    openConnection();
    if( !mConnected )
        return;

    // Get resume offset
    quint64 offset = config()->readEntry("resume",0);
    if( offset > 0 ) {
        canResume();
        kDebug(KIO_SFTP_DB) << "get(): canResume(), offset = " << offset;
    }

    Status info = sftpGet(url, offset);

    if (info.code != 0)
    {
      error(info.code, info.text);
      return;
    }

    data(QByteArray());
    kDebug(KIO_SFTP_DB) << "get(): emit finished()";
    finished();
}


void sftpProtocol::setHost (const QString& h, quint16 port, const QString& user, const QString& pass)
{
    kDebug(KIO_SFTP_DB) << "setHost(): " << user << "@" << h << ":" << port;

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

    kDebug(KIO_SFTP_DB) << "openConnection(): " << mUsername << "@"
                         << mHost << ":" << mPort << endl;

    infoMessage( i18n("Opening SFTP connection to host %1:%2", mHost, mPort));

    if( mHost.isEmpty() ) {
        kDebug(KIO_SFTP_DB) << "openConnection(): Need hostname...";
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
    info.comment = "sftp://" + mHost + ':' + QString::number(mPort);
    info.commentLabel = i18n("site:");
    info.username = mUsername;
    info.keepPassword = true;

    ///////////////////////////////////////////////////////////////////////////
    // Check for cached authentication info if a username AND password were
    // not specified in setHost().
    if( mUsername.isEmpty() && mPassword.isEmpty() ) {
        kDebug(KIO_SFTP_DB) << "openConnection(): checking cache "
                             << "info.username = " << info.username
                             << ", info.url = " << info.url.prettyUrl() << endl;

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

    // setHost will set a port in any case
    Q_ASSERT( mPort != -1 );

    opt.opt = KSshProcess::SSH_PORT;
    opt.num = mPort;
    opts.append(opt);

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
        kDebug(KIO_SFTP_DB) << "openConnection(): "
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

            kDebug(KIO_SFTP_DB) << "openConnection(): info.username = " << info.username
                                 << ", info.url = " << info.url.prettyUrl() << endl;

            if( firstTime )
                dlgResult = openPasswordDialog(info);
            else
                dlgResult = openPasswordDialog(info, i18n("Incorrect username or password"));

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
                kDebug(KIO_SFTP_DB) << "openConnection(): user canceled, dlgResult = " << dlgResult;
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
                kDebug(KIO_SFTP_DB) << "openConnection(): Username changed from "
                                     << mUsername << " to " << info.username << endl;

                ssh.disconnect();

                // if we haven't yet added the username
                // or password option to the ssh options list then
                // the iterators will be equal to the empty iterator.
                // Create the opts now and add them to the opt list.
                if( usernameIt == KSshProcess::SshOptListIterator() ) {
                    kDebug(KIO_SFTP_DB) << "openConnection(): "
                        "Adding username to options list" << endl;
                    opt.opt = KSshProcess::SSH_USERNAME;
					opts.append(opt);
                    usernameIt = opts.end()-1;
                }

                if( passwdIt == KSshProcess::SshOptListIterator() ) {
                    kDebug(KIO_SFTP_DB) << "openConnection(): "
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
            if (!msg.isEmpty()) {
                caption += '\n';
                caption += msg;
            }
            closeConnection();
            error(ERR_COULD_NOT_LOGIN, caption);
            return;

        case KSshProcess::ERR_INTERACT:
        case KSshProcess::ERR_INTERNAL:
        case KSshProcess::ERR_UNKNOWN:
        case KSshProcess::ERR_INVALID_STATE:
        case KSshProcess::ERR_CANNOT_LAUNCH:
        case KSshProcess::ERR_HOST_KEY_REJECTED:
        default:
            infoMessage(i18n("Connection failed."));
            // Don't call messageBox! Leave GUI handling to the apps (#108812)
            caption = i18n("Unexpected SFTP error: %1", err);
            msg = ssh.errorMsg();
            if (!msg.isEmpty()) {
                caption += '\n';
                caption += msg;
            }
            closeConnection();
            error(ERR_UNKNOWN, caption);
            return;
        }
    }

    // catch all in case we did something wrong above
    if( !mConnected ) {
        error(ERR_INTERNAL, QString());
        return;
    }

    // Now send init packet.
    kDebug(KIO_SFTP_DB) << "openConnection(): Sending SSH2_FXP_INIT packet.";
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
    kDebug(KIO_SFTP_DB) << "openConnection(): Got type " << type;

    if( type == SSH2_FXP_VERSION ) {
        s >> version;
        kDebug(KIO_SFTP_DB) << "openConnection(): Got server version " << version;

        // XXX Get extensions here
        sftpVersion = version;

        /* Server should return lowest common version supported by
         * client and server, but double check just in case.
         */
        if( sftpVersion > SSH2_FILEXFER_VERSION ) {
            error(ERR_UNSUPPORTED_PROTOCOL,
                i18n("SFTP version %1", version));
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
    infoMessage(i18n("Successfully connected to %1", mHost));
    info.url.setProtocol("sftp");
    info.url.setHost(mHost);
    info.url.setPort(mPort);
    info.url.setUser(mUsername);
    info.username = mUsername;
    info.password = mPassword;
    kDebug(KIO_SFTP_DB) << "sftpProtocol(): caching info.username = " << info.username <<
        ", info.url = " << info.url.prettyUrl() << endl;
    cacheAuthentication(info);
    mConnected = true;
    connected();

    mPassword.fill('x');
    info.password.fill('x');

    return;
}

#define _DEBUG kDebug(KIO_SFTP_DB)

void sftpProtocol::open(const KUrl &url, QIODevice::OpenMode mode)
{
    _DEBUG << url.url() << endl;
    openConnection();
    if (!mConnected) {
        error(KIO::ERR_CONNECTION_BROKEN, url.prettyUrl());
        return;
    }

    int code;
    sftpFileAttr attr(remoteEncoding());

    if ((code = sftpStat(url, attr)) != SSH2_FX_OK) {
        _DEBUG << "stat error" << endl;
        processStatus(code, url.prettyUrl());
        return;
    }

    // don't open a directory
    if (attr.fileType() == S_IFDIR) {
        _DEBUG << "a directory" << endl;
        error(KIO::ERR_IS_DIRECTORY, url.prettyUrl());
        return;
    }
    if (attr.fileType() != S_IFREG) {
        _DEBUG << "not a regular file" << endl;
        error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyUrl());
        return;
    }

    KIO::filesize_t fileSize = attr.fileSize();
    attr.clear();

    quint32 pflags = 0;

    if (mode & QIODevice::ReadOnly) {
        if (mode & QIODevice::WriteOnly) {
            pflags = SSH2_FXF_READ | SSH2_FXF_WRITE | SSH2_FXF_CREAT;
        } else {
            pflags = SSH2_FXF_READ;
        }
    } else if (mode & QIODevice::WriteOnly) {
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT;
    }

    if (mode & QIODevice::Append) {
        pflags |= SSH2_FXF_APPEND;
    } else if (mode & QIODevice::Truncate) {
        pflags |= SSH2_FXF_TRUNC;
    }

    code = sftpOpen(url, pflags, attr, openHandle);
    if (code != SSH2_FX_OK) {
        _DEBUG << "sftpOpen error" << endl;
        processStatus(code, url.prettyUrl());
        return;
    }

    // Determine the mimetype of the file to be retrieved, and emit it.
    // This is mandatory in all slaves (for KRun/BrowserRun to work).
    // If we're not opening the file ReadOnly or ReadWrite, don't attempt to
    // read the file and send the mimetype.
    if (mode & QIODevice::ReadOnly){
        QByteArray buffer;
        code = sftpRead(openHandle, 0, 1024, buffer);
        if ((code != SSH2_FX_OK) && (code != SSH2_FX_EOF)){
            _DEBUG << "error on mime type detection" << endl;
            processStatus(code, url.prettyUrl());
            close();
            return;
        }
        KMimeType::Ptr mime = KMimeType::findByNameAndContent(url.fileName(), buffer);
        mimeType(mime->name());
    }

    openUrl = url;
    openOffset = 0;
    totalSize(fileSize);
    position(0);
    opened();
}

void sftpProtocol::read(KIO::filesize_t bytes)
{
    _DEBUG << "read, offset = " << openOffset << ", bytes = " << bytes << endl;
    QByteArray buffer;
    int code = sftpRead(openHandle, openOffset, bytes, buffer);
    if ((code == SSH2_FX_OK) || (code == SSH2_FX_EOF)) {
        openOffset += buffer.size();
        data(buffer);
    } else {
        processStatus(code, openUrl.prettyUrl());
        close();
    }
}

void sftpProtocol::write(const QByteArray &data)
{
    _DEBUG << "write" << endl;
    int code = sftpWrite(openHandle, openOffset, data);
    if (code == SSH2_FX_OK) {
        openOffset += data.size();
        written(data.size());
    } else {
        processStatus(code, openUrl.prettyUrl());
        close();
    }
}

void sftpProtocol::seek(KIO::filesize_t offset)
{
    _DEBUG << "seek, offset = " << offset << endl;
    openOffset = offset;
    position(offset);
}

void sftpProtocol::close()
{
    sftpClose(openHandle);
    _DEBUG << "emitting finished" << endl;
    finished();
}
#undef _DEBUG

void sftpProtocol::closeConnection() {
    kDebug(KIO_SFTP_DB) << "closeConnection()";
    ssh.disconnect();
    mConnected = false;
}

void sftpProtocol::sftpCopyPut(const KUrl& src, const KUrl& dest, int permissions, KIO::JobFlags flags) {
    KDE_struct_stat buff;
    QByteArray file (QFile::encodeName(src.path()));

    if (KDE_lstat(file.data(), &buff) == -1) {
        error (ERR_DOES_NOT_EXIST, src.prettyUrl());
        return;
    }

    if (S_ISDIR (buff.st_mode)) {
        error (ERR_IS_DIRECTORY, src.prettyUrl());
        return;
    }

    int fd = KDE_open (file.data(), O_RDONLY);
    if (fd == -1) {
        error (ERR_CANNOT_OPEN_FOR_READING, src.prettyUrl());
        return;
    }

    totalSize (buff.st_size);

    sftpPut (dest, permissions, (flags & ~KIO::Resume), fd);

    // Close the file descriptor...
    ::close( fd );
}

void sftpProtocol::sftpPut( const KUrl& dest, int permissions, KIO::JobFlags flags, int fd ) {

    openConnection();
    if( !mConnected )
        return;

    kDebug(KIO_SFTP_DB) << "sftpPut(): " << dest
                         << ", resume=" << (flags & KIO::Resume)
                         << ", overwrite=" << (flags & KIO::Overwrite) << endl;

    KUrl origUrl( dest );
    sftpFileAttr origAttr(remoteEncoding());
    bool origExists = false;

    // Stat original (without part ext)  to see if it already exists
    int code = sftpStat(origUrl, origAttr);

    if( code == SSH2_FX_OK ) {
        kDebug(KIO_SFTP_DB) << "sftpPut(): <file> already exists";

        // Delete remote file if its size is zero
        if( origAttr.fileSize() == 0 ) {
            if( sftpRemove(origUrl, true) != SSH2_FX_OK ) {
                error(ERR_CANNOT_DELETE_ORIGINAL, origUrl.prettyUrl());
                return;
            }
        }
        else {
            origExists = true;
        }
    }
    else if( code != SSH2_FX_NO_SUCH_FILE ) {
        processStatus(code, origUrl.prettyUrl());
        return;
    }

    // Do not waste time/resources with more remote stat calls if the file exists
    // and we weren't instructed to overwrite it...
    if( origExists && !(flags & KIO::Overwrite) ) {
        error(ERR_FILE_ALREADY_EXIST, origUrl.prettyUrl());
        return;
    }

    // Stat file with part ext to see if it already exists...
    KUrl partUrl( origUrl );
    partUrl.setFileName( partUrl.fileName() + ".part" );

    quint64 offset = 0;
    bool partExists = false;
    bool markPartial = config()->readEntry("MarkPartial", true);

    if( markPartial ) {

        sftpFileAttr partAttr(remoteEncoding());
        code = sftpStat(partUrl, partAttr);

        if( code == SSH2_FX_OK ) {
            kDebug(KIO_SFTP_DB) << "sftpPut(): .part file already exists";
            partExists = true;
            offset = partAttr.fileSize();

            // If for some reason, both the original and partial files exist,
            // skip resumption just like we would if the size of the partial
            // file is zero...
            if( origExists || offset == 0 )
            {
                if( sftpRemove(partUrl, true) != SSH2_FX_OK ) {
                    error(ERR_CANNOT_DELETE_PARTIAL, partUrl.prettyUrl());
                    return;
                }

                if( sftpRename(origUrl, partUrl) != SSH2_FX_OK ) {
                    error(ERR_CANNOT_RENAME_ORIGINAL, origUrl.prettyUrl());
                    return;
                }

                offset = 0;
            }
            else if( !(flags & KIO::Overwrite) && !(flags & KIO::Resume) ) {
                    if (fd != -1)
                        flags |= (KDE_lseek(fd, offset, SEEK_SET) != -1) ? KIO::Resume : KIO::DefaultFlags;
                    else
                        flags |= canResume( offset ) ? KIO::Resume : KIO::DefaultFlags;

                    kDebug(KIO_SFTP_DB) << "sftpPut(): can resume = " << (flags & KIO::Resume)
                                         << ", offset = " << offset;

                    if( !(flags & KIO::Resume) ) {
                      error(ERR_FILE_ALREADY_EXIST, partUrl.prettyUrl());
                      return;
                    }
            }
            else {
                offset = 0;
            }
        }
        else if( code == SSH2_FX_NO_SUCH_FILE ) {
            if( origExists && sftpRename(origUrl, partUrl) != SSH2_FX_OK ) {
              error(ERR_CANNOT_RENAME_ORIGINAL, origUrl.prettyUrl());
              return;
            }
        }
        else {
            processStatus(code, partUrl.prettyUrl());
            return;
        }
    }

    // Determine the url we will actually write to...
    KUrl writeUrl (markPartial ? partUrl:origUrl);

    quint32 pflags = 0;
    if( (flags & KIO::Overwrite) && !(flags & KIO::Resume) )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_TRUNC;
    else if( !(flags & KIO::Overwrite) && !(flags & KIO::Resume) )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_EXCL;
    else if( (flags & KIO::Overwrite) && (flags & KIO::Resume) )
        pflags = SSH2_FXF_WRITE | SSH2_FXF_CREAT;
    else if( !(flags & KIO::Overwrite) && (flags & KIO::Resume) )
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
        if( markPartial && (flags & KIO::Overwrite) ) {
            (void) sftpRename(partUrl, origUrl);
            writeUrl = origUrl;
        }

        if( code == SSH2_FX_FAILURE ) { // assume failure means file exists
            error(ERR_FILE_ALREADY_EXIST, writeUrl.prettyUrl());
            return;
        }
        else {
            processStatus(code, writeUrl.prettyUrl());
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
                error(ERR_COULD_NOT_WRITE, dest.prettyUrl());
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
                                 "to '%1'. Please try again.", dest.host()) );
        return;
    }

    if( (code = sftpClose(handle)) != SSH2_FX_OK ) {
        error(ERR_COULD_NOT_WRITE, writeUrl.prettyUrl());
        return;
    }

    // If wrote to a partial file, then remove the part ext
    if( markPartial ) {
        if( sftpRename(partUrl, origUrl) != SSH2_FX_OK ) {
            error(ERR_CANNOT_RENAME_PARTIAL, origUrl.prettyUrl());
            return;
        }
    }

    finished();
}

void sftpProtocol::put ( const KUrl& url, int permissions, KIO::JobFlags flags ) {
    kDebug(KIO_SFTP_DB) << "put(): " << url << ", overwrite = " << (flags & KIO::Overwrite)
                         << ", resume = " << (flags & KIO::Resume) << endl;

    sftpPut( url, permissions, flags );
}

void sftpProtocol::stat ( const KUrl& url ){
    kDebug(KIO_SFTP_DB) << "stat(): " << url;

    openConnection();
    if( !mConnected )
        return;

    // If the stat URL has no path, do not attempt to determine the real
    // path and do a redirect. KRun will simply ignore such requests.
    // Instead, simply return the mime-type as a directory...
    if( !url.hasPath() ) {
        UDSEntry entry;

        entry.insert( KIO::UDSEntry::UDS_NAME, QString::fromLatin1(".") );
        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
        entry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
        entry.insert( KIO::UDSEntry::UDS_USER, mUsername );
        entry.insert( KIO::UDSEntry::UDS_GROUP, mUsername );

        // no size
        statEntry( entry );
        finished();
        return;
    }

    int code;
    sftpFileAttr attr(remoteEncoding());
    if( (code = sftpStat(url, attr)) != SSH2_FX_OK ) {
        processStatus(code, url.prettyUrl());
        return;
    }
    else {
        //kDebug() << "We sent and received stat packet ok";
        attr.setFilename(url.fileName());
        statEntry(attr.entry());
    }

    finished();

    kDebug(KIO_SFTP_DB) << "stat: END";
    return;
}


void sftpProtocol::mimetype ( const KUrl& url ){
    kDebug(KIO_SFTP_DB) << "mimetype(): " << url;

    openConnection();
    if( !mConnected )
        return;

    Status info = sftpGet(url, 0 /*offset*/, -1, true /*only emit mimetype*/);

    if (info.code != 0)
    {
      error(info.code, info.text);
      return;
    }

    finished();
}


void sftpProtocol::listDir(const KUrl& url) {
    kDebug(KIO_SFTP_DB) << "listDir(): " << url;

    openConnection();
    if( !mConnected )
        return;

    if( !url.hasPath() ) {
        KUrl newUrl ( url );
        if( sftpRealPath(url, newUrl) == SSH2_FX_OK ) {
            kDebug(KIO_SFTP_DB) << "listDir: Redirecting to " << newUrl;
            redirection(newUrl);
            finished();
            return;
        }
    }

    int code;
    QByteArray handle;

    if( (code = sftpOpenDirectory(url, handle)) != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "listDir(): open directory failed" << endl;
        processStatus(code, url.prettyUrl());
        return;
    }


    code = SSH2_FX_OK;
    while( code == SSH2_FX_OK ) {
        code = sftpReadDir(handle, url);
        if( code != SSH2_FX_OK && code != SSH2_FX_EOF )
            processStatus(code, url.prettyUrl());
        kDebug(KIO_SFTP_DB) << "listDir(): return code = " << code;
    }

    if( (code = sftpClose(handle)) != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "listdir(): closing of directory failed" << endl;
        processStatus(code, url.prettyUrl());
        return;
    }

    finished();
    kDebug(KIO_SFTP_DB) << "listDir(): END";
}

/** Make a directory.
    OpenSSH does not follow the internet draft for sftp in this case.
    The format of the mkdir request expected by OpenSSH sftp server is:
        uint32 id
        string path
        ATTR   attr
 */
void sftpProtocol::mkdir(const KUrl&url, int permissions){

    kDebug(KIO_SFTP_DB) << "mkdir() creating dir: " << url.path();

    openConnection();
    if( !mConnected )
        return;

    QByteArray path = remoteEncoding()->encode(url.path());
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

    kDebug(KIO_SFTP_DB) << "mkdir(): packet size is " << p.size();

    putPacket(p);
    getPacket(p);

    quint8 type;
    QDataStream r(p);

    r >> type >> id;
    if( id != expectedId ) {
        kError(KIO_SFTP_DB) << "mkdir: sftp packet id mismatch";
        error(ERR_COULD_NOT_MKDIR, url.path());
        finished();
        return;
    }

    if( type != SSH2_FXP_STATUS ) {
        kError(KIO_SFTP_DB) << "mkdir(): unexpected packet type of" << type;
        error(ERR_COULD_NOT_MKDIR, url.path());
        finished();
        return;
    }

    int code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "mkdir(): failed with code " << code;

        // Check if mkdir failed because the directory already exists so that
        // we can return the appropriate message...
        sftpFileAttr dirAttr(remoteEncoding());
        if ( sftpStat(url, dirAttr) == SSH2_FX_OK )
        {
          error( ERR_DIR_ALREADY_EXIST, url.prettyUrl() );
          return;
        }

        error(ERR_COULD_NOT_MKDIR, url.path());
    }

    finished();
}

void sftpProtocol::rename(const KUrl& src, const KUrl& dest, KIO::JobFlags flags) {
    kDebug(KIO_SFTP_DB) << "rename(" << src << " -> " << dest << ")";

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
      if (!(flags & KIO::Overwrite))
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
    kDebug(KIO_SFTP_DB) << "rename(): END";
}

void sftpProtocol::symlink(const QString& target, const KUrl& dest, KIO::JobFlags flags) {
    kDebug(KIO_SFTP_DB) << "symlink()";

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
        if( flags & KIO::Overwrite ) { // try to delete the destination
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
            error(ERR_FILE_ALREADY_EXIST, dest.prettyUrl());
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

void sftpProtocol::chmod(const KUrl& url, int permissions){
    QString perms;
    perms.setNum(permissions, 8);
    kDebug(KIO_SFTP_DB) << "chmod(" << url << ", " << perms << ")";

    openConnection();
    if( !mConnected )
        return;

    sftpFileAttr attr(remoteEncoding());

    if (permissions != -1)
      attr.setPermissions(permissions);

    int code;
    if( (code = sftpSetStat(url, attr)) != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "chmod(): sftpSetStat failed with error " << code << endl;
        if( code == SSH2_FX_FAILURE )
            error(ERR_CANNOT_CHMOD, QString());
        else
            processStatus(code, url.prettyUrl());
    }
    finished();
}


void sftpProtocol::del(const KUrl &url, bool isfile){
    kDebug(KIO_SFTP_DB) << "del(" << url << ", " << (isfile?"file":"dir") << ")";

    openConnection();
    if( !mConnected )
        return;

    int code;
    if( (code = sftpRemove(url, isfile)) != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "del(): sftpRemove failed with error code " << code << endl;
        processStatus(code, url.prettyUrl());
    }
    finished();
}

void sftpProtocol::slave_status() {
    kDebug(KIO_SFTP_DB) << "slave_status(): connected to "
                         <<  mHost << "? " << mConnected << endl;

    slaveStatus ((mConnected ? mHost : QString()), mConnected);
}

bool sftpProtocol::getPacket(QByteArray& msg) {
    QByteArray buf(4096, '\0');

#ifdef Q_WS_WIN
    ssize_t len;
    if(ssh.pty()->waitForReadyRead(2000)) {
        len = ssh.pty()->read(buf.data(), 4);
    }
#else
    // Get the message length...
    ssize_t len = atomicio(ssh.stdioFd(), buf.data(), 4, true /*read*/);
#endif
    if( len == 0 || len == -1 ) {
        kDebug(KIO_SFTP_DB) << "getPacket(): read of packet length failed, ret = "
                             << len << ", error =" << strerror(errno) << endl;
        closeConnection();
        error( ERR_CONNECTION_BROKEN, mHost);
        msg.resize(0);
        return false;
    }

    int msgLen;
    QDataStream s(buf);
    s >> msgLen;

    //kDebug(KIO_SFTP_DB) << "getPacket(): Message size = " << msgLen;

    msg.resize(0);

    QBuffer b( &msg );
    b.open( QIODevice::WriteOnly );

    while( msgLen ) {
#ifdef Q_WS_WIN
        len = ssh.pty()->read(buf.data(), qMin(buf.size(), msgLen));
#else
        len = atomicio(ssh.stdioFd(), buf.data(), qMin(buf.size(), msgLen), true /*read*/);
#endif

        if( len == 0 || len == -1) {
            QString errmsg;
            if (len == 0)
              errmsg = i18n("Connection closed");
            else
              errmsg = i18n("Could not read SFTP packet");
            kDebug(KIO_SFTP_DB) << "getPacket(): nothing to read, ret = " <<
                len << ", error =" << strerror(errno) << endl;
            closeConnection();
            error(ERR_CONNECTION_BROKEN, errmsg);
            b.close();
            return false;
        }

        b.write(buf.data(), len);

        //kDebug(KIO_SFTP_DB) << "getPacket(): Read Message size = " << len;
        //kDebug(KIO_SFTP_DB) << "getPacket(): Copy Message size = " << msg.size();

        msgLen -= len;
    }

    b.close();

    return true;
}

/** Send an sftp packet to stdin of the ssh process. */
bool sftpProtocol::putPacket(QByteArray& p){
//    kDebug(KIO_SFTP_DB) << "putPacket(): size == " << p.size();
    int ret;
#ifdef Q_WS_WIN
    ret = ssh.pty()->write(p.data(), p.size());
#else
    ret = atomicio(ssh.stdioFd(), p.data(), p.size(), false /*write*/);
#endif
    if( ret <= 0 ) {
        kDebug(KIO_SFTP_DB) << "putPacket(): write failed, ret =" << ret <<
            ", error = " << strerror(errno) << endl;
        return false;
    }

    return true;
}

/** Used to have the server canonicalize any given path name to an absolute path.
This is useful for converting path names containing ".." components or relative
pathnames without a leading slash into absolute paths.
Returns the canonicalized url. */
int sftpProtocol::sftpRealPath(const KUrl& url, KUrl& newUrl){

    kDebug(KIO_SFTP_DB) << "sftpRealPath(" << url << ", newUrl)";

    QByteArray path = remoteEncoding()->encode(url.path());
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
        kError(KIO_SFTP_DB) << "sftpRealPath: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 code;
        r >> code;
        return code;
    }

    if( type != SSH2_FXP_NAME ) {
        kError(KIO_SFTP_DB) << "sftpRealPath(): unexpected packet type of " << type << endl;
        return -1;
    }

    quint32 count;
    r >> count;
    if( count != 1 ) {
        kError(KIO_SFTP_DB) << "sftpRealPath(): Bad number of file attributes for realpath command" << endl;
        return -1;
    }

    QByteArray newPath;
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
          res.text = i18n("End of file.");
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
          res.text = i18n("Error code: %1", code);
          res.code = ERR_UNKNOWN;
    }

    return res;
}

/** Process SSH_FXP_STATUS packets. */
void sftpProtocol::processStatus(quint8 code, const QString& message){
    Status st = doProcessStatus( code, message );
    if( st.code != 0 ){
        error( st.code, st.text );
    }
}

/** Opens a directory handle for url.path. Returns true if succeeds. */
int sftpProtocol::sftpOpenDirectory(const KUrl& url, QByteArray& handle){

    kDebug(KIO_SFTP_DB) << "sftpOpenDirectory(" << url << ", handle)";

    QByteArray path = remoteEncoding()->encode(url.path());
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
        kError(KIO_SFTP_DB) << "sftpOpenDirectory: sftp packet id mismatch: " <<
            "expected " << expectedId << ", got " << id << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 errCode;
        r >> errCode;
        return errCode;
    }

    if( type != SSH2_FXP_HANDLE ) {
        kError(KIO_SFTP_DB) << "sftpOpenDirectory: unexpected message type of " << type << endl;
        return -1;
    }

    r >> handle;
    if( handle.size() > 256 ) {
        kError(KIO_SFTP_DB) << "sftpOpenDirectory: handle exceeds max length" << endl;
        return -1;
    }

    kDebug(KIO_SFTP_DB) << "sftpOpenDirectory: handle (" << handle.size() << "): [" << handle << "]";
    return SSH2_FX_OK;
}

/** Closes a directory or file handle. */
int sftpProtocol::sftpClose(const QByteArray& handle){

    kDebug(KIO_SFTP_DB) << "sftpClose()";

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
        kError(KIO_SFTP_DB) << "sftpClose: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kError(KIO_SFTP_DB) << "sftpClose: unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "sftpClose: close failed with err code " << code << endl;
    }

    return code;
}

/** Set a files attributes. */
int sftpProtocol::sftpSetStat(const KUrl& url, const sftpFileAttr& attr){

    kDebug(KIO_SFTP_DB) << "sftpSetStat(" << url << ", attr)";

    QByteArray path = remoteEncoding()->encode(url.path());
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
        kError(KIO_SFTP_DB) << "sftpSetStat(): sftp packet id mismatch" << endl;
        return -1;
        // XXX How do we do a fatal error?
    }

    if( type != SSH2_FXP_STATUS ) {
        kError(KIO_SFTP_DB) << "sftpSetStat(): unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "sftpSetStat(): set stat failed with err code " << code << endl;
    }

    return code;
}

/** Sends a sftp command to remove a file or directory. */
int sftpProtocol::sftpRemove(const KUrl& url, bool isfile){

    kDebug(KIO_SFTP_DB) << "sftpRemove(): " << url << ", isFile ? " << isfile;

    QByteArray path = remoteEncoding()->encode(url.path());
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
        kError(KIO_SFTP_DB) << "del(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kError(KIO_SFTP_DB) << "del(): unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "del(): del failed with err code " << code << endl;
    }

    return code;
}

/** Send a sftp command to rename a file or directory. */
int sftpProtocol::sftpRename(const KUrl& src, const KUrl& dest){

    kDebug(KIO_SFTP_DB) << "sftpRename(" << src << " -> " << dest << ")";

    QByteArray srcPath = remoteEncoding()->encode(src.path());
    QByteArray destPath = remoteEncoding()->encode(dest.path());

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
        kError(KIO_SFTP_DB) << "sftpRename(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kError(KIO_SFTP_DB) << "sftpRename(): unexpected message type of " << type << endl;
        return -1;
    }

    int code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "sftpRename(): rename failed with err code " << code << endl;
    }

    return code;
}
/** Get directory listings. */
int sftpProtocol::sftpReadDir(const QByteArray& handle, const KUrl& url){
    // url is needed so we can lookup the link destination
    kDebug(KIO_SFTP_DB) << "sftpReadDir(): " << url;

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
        kError(KIO_SFTP_DB) << "sftpReadDir(): sftp packet id mismatch" << endl;
        return -1;
    }

    int code;
    if( type == SSH2_FXP_STATUS ) {
        r >> code;
        return code;
    }

    if( type != SSH2_FXP_NAME ) {
        kError(KIO_SFTP_DB) << "kio_sftpProtocol::sftpReadDir(): Unexpected message" << endl;
        return -1;
    }

    r >> count;
    kDebug(KIO_SFTP_DB) << "sftpReadDir(): got " << count << " entries";

    while(count--) {
        r >> attr;

        if( S_ISLNK(attr.permissions()) ) {
             KUrl myurl ( url );
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

int sftpProtocol::sftpReadLink(const KUrl& url, QString& target){

    kDebug(KIO_SFTP_DB) << "sftpReadLink(): " << url;

    QByteArray path = remoteEncoding()->encode(url.path());
    uint len = path.length();

    //kDebug(KIO_SFTP_DB) << "sftpReadLink(): Encoded Path: " << path;
    //kDebug(KIO_SFTP_DB) << "sftpReadLink(): Encoded Size: " << len;

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
        kError(KIO_SFTP_DB) << "sftpReadLink(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 code;
        r >> code;
        kDebug(KIO_SFTP_DB) << "sftpReadLink(): read link failed with code " << code;
        return code;
    }

    if( type != SSH2_FXP_NAME ) {
        kError(KIO_SFTP_DB) << "sftpReadLink(): unexpected packet type of " << type << endl;
        return -1;
    }

    quint32 count;
    r >> count;
    if( count != 1 ) {
        kError(KIO_SFTP_DB) << "sftpReadLink(): Bad number of file attributes for realpath command" << endl;
        return -1;
    }

    QByteArray linkAddress;
    r >> linkAddress;

    linkAddress.truncate(linkAddress.size());
    kDebug(KIO_SFTP_DB) << "sftpReadLink(): Link address: " << linkAddress;

    target = remoteEncoding()->decode(linkAddress);

    return SSH2_FX_OK;
}

int sftpProtocol::sftpSymLink(const QString& _target, const KUrl& dest){

    QByteArray destPath = remoteEncoding()->encode(dest.path());
    QByteArray target = remoteEncoding()->encode(_target);
    uint dlen = destPath.length();
    uint tlen = target.length();

    kDebug(KIO_SFTP_DB) << "sftpSymLink(" << target << " -> " << destPath << ")";

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
        kError(KIO_SFTP_DB) << "sftpSymLink(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kError(KIO_SFTP_DB) << "sftpSymLink(): unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    if( code != SSH2_FX_OK ) {
        kError(KIO_SFTP_DB) << "sftpSymLink(): rename failed with err code " << code << endl;
    }

    return code;
}

/** Stats a file. */
int sftpProtocol::sftpStat(const KUrl& url, sftpFileAttr& attr) {

    kDebug(KIO_SFTP_DB) << "sftpStat(): " << url;

    QByteArray path = remoteEncoding()->encode(url.path());
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
        kError(KIO_SFTP_DB) << "sftpStat(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 errCode;
        r >> errCode;
        kError(KIO_SFTP_DB) << "sftpStat(): stat failed with code " << errCode << endl;
        return errCode;
    }

    if( type != SSH2_FXP_ATTRS ) {
        kError(KIO_SFTP_DB) << "sftpStat(): unexpected message type of " << type << endl;
        return -1;
    }

    r >> attr;
    attr.setFilename(url.fileName());
    kDebug(KIO_SFTP_DB) << "sftpStat(): " << attr;

    // If the stat'ed resource is a symlink, perform a recursive stat
    // to determine the actual destination's type (file/dir).
    if( S_ISLNK(attr.permissions()) && isSupportedOperation(SSH2_FXP_READLINK) ) {

        QString target;
        int code = sftpReadLink( url, target );

        if ( code != SSH2_FX_OK ) {
            kError(KIO_SFTP_DB) << "sftpStat(): Unable to stat symlink destination" << endl;
            return -1;
        }

        kDebug(KIO_SFTP_DB) << "sftpStat(): Resource is a symlink to -> " << target;

        KUrl dest( url );
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

            kDebug(KIO_SFTP_DB) << "sftpStat(): File type: " << attr.fileType();
        }
    }

    return SSH2_FX_OK;
}


int sftpProtocol::sftpOpen(const KUrl& url, const quint32 pflags,
                           const sftpFileAttr& attr, QByteArray& handle) {
    kDebug(KIO_SFTP_DB) << "sftpOpen(" << url << ", handle";

    QByteArray path = remoteEncoding()->encode(url.path());
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
        kError(KIO_SFTP_DB) << "sftpOpen(): sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 errCode;
        r >> errCode;
        return errCode;
    }

    if( type != SSH2_FXP_HANDLE ) {
        kError(KIO_SFTP_DB) << "sftpOpen(): unexpected message type of " << type << endl;
        return -1;
    }

    r >> handle;
    if( handle.size() > 256 ) {
        kError(KIO_SFTP_DB) << "sftpOpen(): handle exceeds max length" << endl;
        return -1;
    }

    kDebug(KIO_SFTP_DB) << "sftpOpen(): handle (" << handle.size() << "): [" << handle << "]";
    return SSH2_FX_OK;
}


int sftpProtocol::sftpRead(const QByteArray& handle, KIO::filesize_t offset, quint32 len, QByteArray& data)
{
 //   kDebug(KIO_SFTP_DB) << "sftpRead( offset = " << offset << ", len = " << len << ")";
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
        kError(KIO_SFTP_DB) << "sftpRead: sftp packet id mismatch" << endl;
        return -1;
    }

    if( type == SSH2_FXP_STATUS ) {
        quint32 errCode;
        r >> errCode;
        kError(KIO_SFTP_DB) << "sftpRead: read failed with code " << errCode << endl;
        return errCode;
    }

    if( type != SSH2_FXP_DATA ) {
        kError(KIO_SFTP_DB) << "sftpRead: unexpected message type of " << type << endl;
        return -1;
    }

    r >> data;

    return SSH2_FX_OK;
}


int sftpProtocol::sftpWrite(const QByteArray& handle, KIO::filesize_t offset, const QByteArray& data){
//    kDebug(KIO_SFTP_DB) << "sftpWrite( offset = " << offset <<
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

//    kDebug(KIO_SFTP_DB) << "sftpWrite(): SSH2_FXP_WRITE, id:"
//        << id << ", handle:" << handle << ", offset:" << offset << ", some data" << endl;

//    kDebug(KIO_SFTP_DB) << "sftpWrite(): send packet [" << p << "]";

    putPacket(p);
    getPacket(p);

//    kDebug(KIO_SFTP_DB) << "sftpWrite(): received packet [" << p << "]";

    QDataStream r(p);
    quint8 type;

    r >> type >> id;
    if( id != expectedId ) {
        kError(KIO_SFTP_DB) << "sftpWrite(): sftp packet id mismatch, got "
            << id << ", expected " << expectedId << endl;
        return -1;
    }

    if( type != SSH2_FXP_STATUS ) {
        kError(KIO_SFTP_DB) << "sftpWrite(): unexpected message type of " << type << endl;
        return -1;
    }

    quint32 code;
    r >> code;
    return code;
}


