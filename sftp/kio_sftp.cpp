/*
 * Copyright (c) 2001      Lucas Fisher <ljfisher@purdue.edu>
 * Copyright (c) 2009      Andreas Schneider <mail@cynapses.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License (LGPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kio_sftp.h"

#include <config-runtime.h>
#include "kio_sftp_debug.h"
#include "kio_sftp_trace_debug.h"
#include <cerrno>
#include <cstring>
#include <utime.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QVarLengthArray>
#include <QMimeType>
#include <QMimeDatabase>

#include <kuser.h>
#include <kmessagebox.h>

#include <klocalizedstring.h>
#include <kconfiggroup.h>
#include <kio/ioslave_defaults.h>

#define KIO_SFTP_SPECIAL_TIMEOUT 30

// How big should each data packet be? Definitely not bigger than 64kb or
// you will overflow the 2 byte size variable in a sftp packet.
#define MAX_XFER_BUF_SIZE (60 * 1024)

#define KSFTP_ISDIR(sb) (sb->type == SSH_FILEXFER_TYPE_DIRECTORY)

using namespace KIO;
extern "C"
{
int Q_DECL_EXPORT kdemain( int argc, char **argv )
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("kio_sftp");

    qCDebug(KIO_SFTP_LOG) << "*** Starting kio_sftp ";

    if (argc != 4) {
        qCDebug(KIO_SFTP_LOG) << "Usage: kio_sftp protocol domain-socket1 domain-socket2";
        exit(-1);
    }

    sftpProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    qCDebug(KIO_SFTP_LOG) << "*** kio_sftp Done";
    return 0;
}
}

// Converts SSH error into KIO error. Only must be called for error handling
// as this will always return an error state and never NoError.
static int toKIOError (const int err)
{
    switch (err) {
    case SSH_FX_NO_SUCH_FILE:
    case SSH_FX_NO_SUCH_PATH:
        return KIO::ERR_DOES_NOT_EXIST;
    case SSH_FX_PERMISSION_DENIED:
        return KIO::ERR_ACCESS_DENIED;
    case SSH_FX_FILE_ALREADY_EXISTS:
        return KIO::ERR_FILE_ALREADY_EXIST;
    case SSH_FX_INVALID_HANDLE:
        return KIO::ERR_MALFORMED_URL;
    case SSH_FX_OP_UNSUPPORTED:
        return KIO::ERR_UNSUPPORTED_ACTION;
    case SSH_FX_BAD_MESSAGE:
        return KIO::ERR_UNKNOWN;
    default:
        return KIO::ERR_INTERNAL;
    }
    // We should not get here. When this function gets called we've
    // encountered an error on the libssh side, this needs to be mapped to *any*
    // KIO error. Not mapping is not an option at this point, even if the ssh err
    // is wrong or 'ok'.
    Q_UNREACHABLE();
    return KIO::ERR_UNKNOWN;
}

// Writes 'len' bytes from 'buf' to the file handle 'fd'.
static int writeToFile(int fd, const char *buf, size_t len)
{
    while (len > 0)  {
        ssize_t written = write(fd, buf, len);

        if (written >= 0) {
            buf += written;
            len -= written;
            continue;
        }

        switch(errno) {
        case EINTR:
        case EAGAIN:
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

static int seekPos(int fd, KIO::fileoffset_t pos, int mode)
{
    KIO::fileoffset_t offset = -1;
    while ((offset = QT_LSEEK(fd, pos, mode)) == EAGAIN);
    return offset;
}

static bool wasUsernameChanged(const QString& username, const KIO::AuthInfo& info)
{
    QString loginName (username);
    // If username is empty, assume the current logged in username. Why ?
    // Because libssh's SSH_OPTIONS_USER will default to that when it is not
    // set and it won't be set unless the user explicitly typed a user user
    // name as part of the request URL.
    if (loginName.isEmpty()) {
        KUser u;
        loginName = u.loginName();
    }
    return (loginName != info.username);
}

// The callback function for libssh
static int auth_callback(const char *prompt, char *buf, size_t len,
                         int echo, int verify, void *userdata)
{
    if (userdata == nullptr) {
        return -1;
    }

    sftpProtocol *slave = (sftpProtocol *) userdata;

    if (slave->auth_callback(prompt, buf, len, echo, verify, userdata) < 0) {
        return -1;
    }

    return 0;
}

static void log_callback(int priority, const char *function, const char *buffer,
                         void *userdata)
{
    if (userdata == nullptr) {
        return;
    }

    sftpProtocol *slave = (sftpProtocol *) userdata;

    slave->log_callback(priority, function, buffer, userdata);
}

int sftpProtocol::auth_callback(const char *prompt, char *buf, size_t len,
                                int echo, int verify, void *userdata) {

    // unused variables
    (void) echo;
    (void) verify;
    (void) userdata;

    QString errMsg;
    if (!mPublicKeyAuthInfo) {
        mPublicKeyAuthInfo = new KIO::AuthInfo;
    } else {
        errMsg = i18n("Incorrect or invalid passphrase");
    }

    mPublicKeyAuthInfo->url.setScheme(QLatin1String("sftp"));
    mPublicKeyAuthInfo->url.setHost(mHost);
    if (mPort > 0 && mPort != DEFAULT_SFTP_PORT) {
        mPublicKeyAuthInfo->url.setPort(mPort);
    }
    mPublicKeyAuthInfo->url.setUserName(mUsername);

    QUrl u (mPublicKeyAuthInfo->url);
    u.setPath(QString());
    mPublicKeyAuthInfo->comment = u.url();
    mPublicKeyAuthInfo->readOnly = true;
    mPublicKeyAuthInfo->prompt = QString::fromUtf8(prompt);
    mPublicKeyAuthInfo->keepPassword = false; // don't save passwords for public key,
    // that's the task of ssh-agent.
    mPublicKeyAuthInfo->setExtraField(QLatin1String("hide-username-line"), true);
    mPublicKeyAuthInfo->setModified(false);

    qCDebug(KIO_SFTP_LOG) << "Entering authentication callback, prompt=" << mPublicKeyAuthInfo->prompt;

    if (openPasswordDialogV2(*mPublicKeyAuthInfo, errMsg) != 0) {
        qCDebug(KIO_SFTP_LOG) << "User canceled public key passpharse dialog";
        return -1;
    }

    strncpy(buf, mPublicKeyAuthInfo->password.toUtf8().constData(), len - 1);

    mPublicKeyAuthInfo->password.fill('x');
    mPublicKeyAuthInfo->password.clear();

    return 0;
}

void sftpProtocol::log_callback(int priority, const char *function, const char *buffer,
                                void *userdata)
{
    (void) userdata;

    qCDebug(KIO_SFTP_LOG) << "[" << function << "] (" << priority << ") " << buffer;
}

void sftpProtocol::virtual_hook(int id, void *data)
{
    switch(id) {
    case SlaveBase::GetFileSystemFreeSpace: {
        QUrl *url = static_cast<QUrl *>(data);
        fileSystemFreeSpace(*url);
    } break;
    default:
        SlaveBase::virtual_hook(id, data);
    }
}

int sftpProtocol::authenticateKeyboardInteractive(AuthInfo &info) {

    int err = ssh_userauth_kbdint(mSession, nullptr, nullptr);

    while (err == SSH_AUTH_INFO) {
        const QString name = QString::fromUtf8(ssh_userauth_kbdint_getname(mSession));
        const QString instruction = QString::fromUtf8(ssh_userauth_kbdint_getinstruction(mSession));
        const int n = ssh_userauth_kbdint_getnprompts(mSession);

        qCDebug(KIO_SFTP_LOG) << "name=" << name << " instruction=" << instruction << " prompts=" << n;

        for (int i = 0; i < n; ++i) {
            char echo;
            const char *answer = "";

            const QString prompt = QString::fromUtf8(ssh_userauth_kbdint_getprompt(mSession, i, &echo));
            qCDebug(KIO_SFTP_LOG) << "prompt=" << prompt << " echo=" << QString::number(echo);
            if (echo) {
                // See RFC4256 Section 3.3 User Interface
                KIO::AuthInfo infoKbdInt;

                infoKbdInt.url.setScheme("sftp");
                infoKbdInt.url.setHost(mHost);
                if (mPort > 0 && mPort != DEFAULT_SFTP_PORT) {
                    infoKbdInt.url.setPort(mPort);
                }

                if (!name.isEmpty()) {
                    infoKbdInt.caption = QString(i18n("SFTP Login") + " - " + name);
                } else {
                    infoKbdInt.caption = i18n("SFTP Login");
                }

                infoKbdInt.comment = "sftp://" + mUsername + "@"  + mHost;

                QString newPrompt;
                if (!instruction.isEmpty()) {
                    newPrompt = instruction + "<br /><br />";
                }
                newPrompt.append(prompt);
                infoKbdInt.prompt = newPrompt;

                infoKbdInt.readOnly = false;
                infoKbdInt.keepPassword = false;

                if (openPasswordDialogV2(infoKbdInt, i18n("Use the username input field to answer this question.")) == 0) {
                    qCDebug(KIO_SFTP_LOG) << "Got the answer from the password dialog";
                    answer = info.username.toUtf8().constData();
                }

                if (ssh_userauth_kbdint_setanswer(mSession, i, answer) < 0) {
                    qCDebug(KIO_SFTP_LOG) << "An error occurred setting the answer: "
                                          << ssh_get_error(mSession);
                    return SSH_AUTH_ERROR;
                }
                break;
            } else {
                if (prompt.startsWith(QLatin1String("password:"), Qt::CaseInsensitive)) {
                    info.prompt = i18n("Please enter your password.");
                } else {
                    info.prompt = prompt;
                }
                info.comment = info.url.url();
                info.commentLabel = i18n("Site:");
                info.setExtraField(QLatin1String("hide-username-line"), true);

                if (openPasswordDialogV2(info) == 0) {
                    qCDebug(KIO_SFTP_LOG) << "Got the answer from the password dialog";
                    answer = info.password.toUtf8().constData();
                }

                if (ssh_userauth_kbdint_setanswer(mSession, i, answer) < 0) {
                    qCDebug(KIO_SFTP_LOG) << "An error occurred setting the answer: "
                                          << ssh_get_error(mSession);
                    return SSH_AUTH_ERROR;
                }
            }
        }
        err = ssh_userauth_kbdint(mSession, nullptr, nullptr);
    }

    return err;
}

void sftpProtocol::reportError(const QUrl &url, const int err) {
    qCDebug(KIO_SFTP_LOG) << "url = " << url << " - err=" << err;

    const int kioError = toKIOError(err);
    Q_ASSERT(kioError != 0);

    error(kioError, url.toDisplayString());
}

bool sftpProtocol::createUDSEntry(const QString &filename, const QByteArray &path,
                                  UDSEntry &entry, short int details) {
    mode_t access;
    char *link;
    bool isBrokenLink = false;
    long long fileType = S_IFREG;
    long long size = 0LL;

    Q_ASSERT(entry.count() == 0);

    sftp_attributes sb = sftp_lstat(mSftp, path.constData());
    if (sb == nullptr) {
        return false;
    }

    entry.fastInsert(KIO::UDSEntry::UDS_NAME, filename);

    if (sb->type == SSH_FILEXFER_TYPE_SYMLINK) {
        link = sftp_readlink(mSftp, path.constData());
        if (link == nullptr) {
            sftp_attributes_free(sb);
            return false;
        }
        entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, QFile::decodeName(link));
        free(link);
        // A symlink -> follow it only if details > 1
        if (details > 1) {
            sftp_attributes sb2 = sftp_stat(mSftp, path.constData());
            if (sb2 == nullptr) {
                isBrokenLink = true;
            } else {
                sftp_attributes_free(sb);
                sb = sb2;
            }
        }
    }

    if (isBrokenLink) {
        // It is a link pointing to nowhere
        fileType = S_IFMT - 1;
        access = S_IRWXU | S_IRWXG | S_IRWXO;
        size = 0LL;
    } else {
        switch (sb->type) {
        case SSH_FILEXFER_TYPE_REGULAR:
            fileType = S_IFREG;
            break;
        case SSH_FILEXFER_TYPE_DIRECTORY:
            fileType = S_IFDIR;
            break;
        case SSH_FILEXFER_TYPE_SYMLINK:
            fileType = S_IFLNK;
            break;
        case SSH_FILEXFER_TYPE_SPECIAL:
        case SSH_FILEXFER_TYPE_UNKNOWN:
            fileType = S_IFMT - 1;
            break;
        }
        access = sb->permissions & 07777;
        size = sb->size;
    }
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, fileType);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, access);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, size);

    if (details > 0) {
        if (sb->owner) {
            entry.fastInsert(KIO::UDSEntry::UDS_USER, QString::fromUtf8(sb->owner));
        } else {
            entry.fastInsert(KIO::UDSEntry::UDS_USER, QString::number(sb->uid));
        }

        if (sb->group) {
            entry.fastInsert(KIO::UDSEntry::UDS_GROUP, QString::fromUtf8(sb->group));
        } else {
            entry.fastInsert(KIO::UDSEntry::UDS_GROUP, QString::number(sb->gid));
        }

        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS_TIME, sb->atime);
        entry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, sb->mtime);
        entry.fastInsert(KIO::UDSEntry::UDS_CREATION_TIME, sb->createtime);
    }

    sftp_attributes_free(sb);

    return true;
}

QString sftpProtocol::canonicalizePath(const QString &path) {
    qCDebug(KIO_SFTP_LOG) << "Path to canonicalize: " << path;
    QString cPath;
    char *sPath = nullptr;

    if (path.isEmpty()) {
        return cPath;
    }

    sPath = sftp_canonicalize_path(mSftp, path.toUtf8().constData());
    if (sPath == nullptr) {
        qCDebug(KIO_SFTP_LOG) << "Could not canonicalize path: " << path;
        return cPath;
    }

    cPath = QFile::decodeName(sPath);
    delete sPath;

    qCDebug(KIO_SFTP_LOG) << "Canonicalized path: " << cPath;

    return cPath;
}

sftpProtocol::sftpProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase("kio_sftp", pool_socket, app_socket),
      mConnected(false), mPort(-1), mSession(nullptr), mSftp(nullptr), mPublicKeyAuthInfo(nullptr) {
#ifndef Q_OS_WIN
    qCDebug(KIO_SFTP_LOG) << "pid = " << getpid();

    qCDebug(KIO_SFTP_LOG) << "debug = " << getenv("KIO_SFTP_LOG_VERBOSITY");
#endif

    // Members are 'value initialized' to zero because of non-user defined ()!
    mCallbacks = new struct ssh_callbacks_struct();
    if (mCallbacks == nullptr) {
        error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not allocate callbacks"));
        return;
    }

    mCallbacks->userdata = this;
    mCallbacks->auth_function = ::auth_callback;

    ssh_callbacks_init(mCallbacks);

    bool ok;
    int level = qEnvironmentVariableIntValue("KIO_SFTP_LOG_VERBOSITY", &ok);
    if (ok) {
        int rc = ssh_set_log_level(level);
        if (rc != SSH_OK) {
            error(KIO::ERR_INTERNAL, i18n("Could not set log verbosity."));
            return;
        }

        rc = ssh_set_log_userdata(this);
        if (rc != SSH_OK) {
            error(KIO::ERR_INTERNAL, i18n("Could not set log userdata."));
            return;
        }

        rc = ssh_set_log_callback(::log_callback);
        if (rc != SSH_OK) {
            error(KIO::ERR_INTERNAL, i18n("Could not set log callback."));
            return;
        }
    }
}

sftpProtocol::~sftpProtocol() {
#ifndef Q_OS_WIN
    qCDebug(KIO_SFTP_LOG) << "pid = " << getpid();
#endif
    closeConnection();

    delete mCallbacks;
    delete mPublicKeyAuthInfo; // for precaution

    /* cleanup and shut down cryto stuff */
    ssh_finalize();
}

void sftpProtocol::setHost(const QString& host, quint16 port, const QString& user, const QString& pass) {
    qCDebug(KIO_SFTP_LOG) << user << "@" << host << ":" << port;

    // Close connection if the request is to another server...
    if (host != mHost || port != mPort ||
            user != mUsername || pass != mPassword) {
        closeConnection();
    }

    mHost = host;
    mPort = port;
    mUsername = user;
    mPassword = pass;
}

bool sftpProtocol::sftpOpenConnection (const AuthInfo& info)
{
    mSession = ssh_new();
    if (mSession == nullptr) {
        error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not create a new SSH session."));
        return false;
    }

    long timeout_sec = 30, timeout_usec = 0;

    qCDebug(KIO_SFTP_LOG) << "Creating the SSH session and setting options";

    // Set timeout
    int rc = ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT, &timeout_sec);
    if (rc < 0) {
        error(KIO::ERR_INTERNAL, i18n("Could not set a timeout."));
        return false;
    }
    rc = ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT_USEC, &timeout_usec);
    if (rc < 0) {
        error(KIO::ERR_INTERNAL, i18n("Could not set a timeout."));
        return false;
    }

#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0)
    // Disable Nagle's Algorithm (TCP_NODELAY). Usually faster for sftp.
    bool nodelay = true;
    rc = ssh_options_set(mSession, SSH_OPTIONS_NODELAY, &nodelay);
    if (rc < 0) {
        error(KIO::ERR_INTERNAL, i18n("Could not disable Nagle's Algorithm."));
        return false;
    }
#endif // 0.8.0

    // Don't use any compression
    rc = ssh_options_set(mSession, SSH_OPTIONS_COMPRESSION_C_S, "none");
    if (rc < 0) {
        error(KIO::ERR_INTERNAL, i18n("Could not set compression."));
        return false;
    }

    rc = ssh_options_set(mSession, SSH_OPTIONS_COMPRESSION_S_C, "none");
    if (rc < 0) {
        error(KIO::ERR_INTERNAL, i18n("Could not set compression."));
        return false;
    }

    // Set host and port
    rc = ssh_options_set(mSession, SSH_OPTIONS_HOST, mHost.toUtf8().constData());
    if (rc < 0) {
        error(KIO::ERR_INTERNAL, i18n("Could not set host."));
        return false;
    }

    if (mPort > 0) {
        rc = ssh_options_set(mSession, SSH_OPTIONS_PORT, &mPort);
        if (rc < 0) {
            error(KIO::ERR_INTERNAL, i18n("Could not set port."));
            return false;
        }
    }

    // Set the username
    if (!info.username.isEmpty()) {
        rc = ssh_options_set(mSession, SSH_OPTIONS_USER, info.username.toUtf8().constData());
        if (rc < 0) {
            error(KIO::ERR_INTERNAL, i18n("Could not set username."));
            return false;
        }
    }

    // Read ~/.ssh/config
    rc = ssh_options_parse_config(mSession, nullptr);
    if (rc < 0) {
        error(KIO::ERR_INTERNAL, i18n("Could not parse the config file."));
        return false;
    }

    ssh_set_callbacks(mSession, mCallbacks);

    qCDebug(KIO_SFTP_LOG) << "Trying to connect to the SSH server";

    unsigned int effectivePort;
    if (mPort > 0) {
        effectivePort = mPort;
    } else {
        effectivePort = DEFAULT_SFTP_PORT;
        ssh_options_get_port(mSession, &effectivePort);
    }

    qCDebug(KIO_SFTP_LOG) << "username=" << mUsername << ", host=" << mHost << ", port=" << effectivePort;

    infoMessage(xi18n("Opening SFTP connection to host %1:%2", mHost, QString::number(effectivePort)));

    /* try to connect */
    rc = ssh_connect(mSession);
    if (rc < 0) {
        error(KIO::ERR_SLAVE_DEFINED, QString::fromUtf8(ssh_get_error(mSession)));
        closeConnection();
        return false;
    }

    return true;
}


void sftpProtocol::openConnection() {

    if (mConnected) {
        return;
    }

    if (mHost.isEmpty()) {
        qCDebug(KIO_SFTP_LOG) << "openConnection(): Need hostname...";
        error(KIO::ERR_UNKNOWN_HOST, QString());
        return;
    }

    AuthInfo info;
    info.url.setScheme("sftp");
    info.url.setHost(mHost);
    if ( mPort > 0 && mPort != DEFAULT_SFTP_PORT ) {
        info.url.setPort(mPort);
    }
    info.url.setUserName(mUsername);
    info.username = mUsername;

    // Check for cached authentication info if no password is specified...
    if (mPassword.isEmpty()) {
        qCDebug(KIO_SFTP_LOG) << "checking cache: info.username =" << info.username
                              << ", info.url =" << info.url.toDisplayString();
        checkCachedAuthentication(info);
    } else {
        info.password = mPassword;
    }

    // Start the ssh connection.
    QString msg;     // msg for dialog box
    QString caption; // dialog box caption
    unsigned char *hash = nullptr; // the server hash
    ssh_key srv_pubkey;
    char *hexa;
    size_t hlen;
    int rc, state;

    // Attempt to start a ssh session and establish a connection with the server.
    if (!sftpOpenConnection(info)) {
        return;
    }

    qCDebug(KIO_SFTP_LOG) << "Getting the SSH server hash";

    /* get the hash */
    rc = ssh_get_publickey(mSession, &srv_pubkey);
    if (rc < 0) {
        error(KIO::ERR_SLAVE_DEFINED, QString::fromUtf8(ssh_get_error(mSession)));
        closeConnection();
        return;
    }

    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA1,
                                &hash,
                                &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0) {
        error(KIO::ERR_SLAVE_DEFINED,
              i18n("Could not create hash from server public key"));
        closeConnection();
        return;
    }

    qCDebug(KIO_SFTP_LOG) << "Checking if the SSH server is known";

    /* check the server public key hash */
    state = ssh_is_server_known(mSession);
    switch (state) {
    case SSH_SERVER_KNOWN_OK:
        break;
    case SSH_SERVER_FOUND_OTHER:
        ssh_string_free_char((char *)hash);
        error(KIO::ERR_SLAVE_DEFINED, i18n("The host key for this server was "
                                           "not found, but another type of key exists.\n"
                                           "An attacker might change the default server key to confuse your "
                                           "client into thinking the key does not exist.\n"
                                           "Please contact your system administrator.\n%1", QString::fromUtf8(ssh_get_error(mSession))));
        closeConnection();
        return;
    case SSH_SERVER_KNOWN_CHANGED:
        hexa = ssh_get_hexa(hash, hlen);
        ssh_string_free_char((char *)hash);
        /* TODO print known_hosts file, port? */
        error(KIO::ERR_SLAVE_DEFINED, i18n("The host key for the server %1 has changed.\n"
                                           "This could either mean that DNS SPOOFING is happening or the IP "
                                           "address for the host and its host key have changed at the same time.\n"
                                           "The fingerprint for the key sent by the remote host is:\n %2\n"
                                           "Please contact your system administrator.\n%3",
                                           mHost, QString::fromUtf8(hexa), QString::fromUtf8(ssh_get_error(mSession))));
        ssh_string_free_char(hexa);
        closeConnection();
        return;
    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN:
        hexa = ssh_get_hexa(hash, hlen);
        ssh_string_free_char((char *)hash);
        caption = i18n("Warning: Cannot verify host's identity.");
        msg = i18n("The authenticity of host %1 cannot be established.\n"
                   "The key fingerprint is: %2\n"
                   "Are you sure you want to continue connecting?", mHost, hexa);
        ssh_string_free_char(hexa);

        if (KMessageBox::Yes != messageBox(WarningYesNo, msg, caption)) {
            closeConnection();
            error(KIO::ERR_USER_CANCELED, QString());
            return;
        }

        /* write the known_hosts file */
        qCDebug(KIO_SFTP_LOG) << "Adding server to known_hosts file.";
        if (ssh_write_knownhost(mSession) < 0) {
            error(KIO::ERR_USER_CANCELED, QString::fromUtf8(ssh_get_error(mSession)));
            closeConnection();
            return;
        }
        break;
    case SSH_SERVER_ERROR:
        ssh_string_free_char((char *)hash);
        error(KIO::ERR_SLAVE_DEFINED, QString::fromUtf8(ssh_get_error(mSession)));
        return;
    }

    qCDebug(KIO_SFTP_LOG) << "Trying to authenticate with the server";

    // Try to login without authentication
    rc = ssh_userauth_none(mSession, nullptr);
    if (rc == SSH_AUTH_ERROR) {
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
        return;
    }

    // This NEEDS to be called after ssh_userauth_none() !!!
    int method = ssh_auth_list(mSession);
    if (rc != SSH_AUTH_SUCCESS && method == 0) {
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed. The server "
                                             "didn't send any authentication methods"));
        return;
    }

    // Try to authenticate with public key first
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_PUBLICKEY)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to authenticate with public key";
        for(;;) {
            rc = ssh_userauth_publickey_auto(mSession, nullptr, nullptr);
            if (rc == SSH_AUTH_ERROR) {
                qCDebug(KIO_SFTP_LOG) << "Public key authentication failed:" <<
                                         QString::fromUtf8(ssh_get_error(mSession));
                closeConnection();
                clearPubKeyAuthInfo();
                error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
                return;
            } else if (rc != SSH_AUTH_DENIED || !mPublicKeyAuthInfo || !mPublicKeyAuthInfo->isModified()) {
                clearPubKeyAuthInfo();
                break;
            }
        }
    }

    // Try to authenticate with GSSAPI
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_GSSAPI_MIC)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to authenticate with GSSAPI";
        rc = ssh_userauth_gssapi(mSession);
        if (rc == SSH_AUTH_ERROR) {
            qCDebug(KIO_SFTP_LOG) << "Public key authentication failed:" <<
                                     QString::fromUtf8(ssh_get_error(mSession));
            closeConnection();
            error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
            return;
        }
    }

    // Try to authenticate with keyboard interactive
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_INTERACTIVE)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to authenticate with keyboard interactive";
        AuthInfo info2 (info);
        rc = authenticateKeyboardInteractive(info2);
        if (rc == SSH_AUTH_SUCCESS) {
            info = info2;
        } else if (rc == SSH_AUTH_ERROR) {
            qCDebug(KIO_SFTP_LOG) << "Keyboard interactive authentication failed:"
                                  << QString::fromUtf8(ssh_get_error(mSession));
            closeConnection();
            error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
            return;
        }
    }

    // Try to authenticate with password
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_PASSWORD)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to authenticate with password";

        info.caption = i18n("SFTP Login");
        info.prompt = i18n("Please enter your username and password.");
        info.comment = info.url.url();
        info.commentLabel = i18n("Site:");
        bool isFirstLoginAttempt = true;

        for(;;) {
            if (!isFirstLoginAttempt || info.password.isEmpty()) {
                info.keepPassword = true; // make the "keep Password" check box visible to the user.
                info.setModified(false);

                QString username (info.username);
                const QString errMsg(isFirstLoginAttempt ? QString() : i18n("Incorrect username or password"));

                qCDebug(KIO_SFTP_LOG) << "Username:" << username << "first attempt?"
                                      << isFirstLoginAttempt << "error:" << errMsg;

                // Handle user canceled or dialog failed to open...

                int errCode = openPasswordDialogV2(info, errMsg);
                if (errCode != 0) {
                    qCDebug(KIO_SFTP_LOG) << "User canceled password/retry dialog";
                    closeConnection();
                    error(errCode, QString());
                    return;
                }

                // If the user name changes, we have to restablish connection again
                // since the user name must always be set before calling ssh_connect.
                if (wasUsernameChanged(username, info)) {
                    qCDebug(KIO_SFTP_LOG) << "Username changed to" << info.username;
                    if (!info.url.userName().isEmpty()) {
                        info.url.setUserName(info.username);
                    }
                    closeConnection();
                    if (!sftpOpenConnection(info)) {
                        return;
                    }
                }
            }

            rc = ssh_userauth_password(mSession, info.username.toUtf8().constData(), info.password.toUtf8().constData());
            if (rc == SSH_AUTH_SUCCESS) {
                break;
            } else if (rc == SSH_AUTH_ERROR) {
                qCDebug(KIO_SFTP_LOG) << "Password authentication failed:"
                                      << QString::fromUtf8(ssh_get_error(mSession));
                closeConnection();
                error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
                return;
            }

            isFirstLoginAttempt = false; // failed attempt to login.
            info.password.clear();       // clear the password after failed attempts.
        }
    }

    // If we're still not authenticated then we need to leave.
    if (rc != SSH_AUTH_SUCCESS) {
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
        return;
    }

    // start sftp session
    qCDebug(KIO_SFTP_LOG) << "Trying to request the sftp session";
    mSftp = sftp_new(mSession);
    if (mSftp == nullptr) {
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Unable to request the SFTP subsystem. "
                                             "Make sure SFTP is enabled on the server."));
        return;
    }

    qCDebug(KIO_SFTP_LOG) << "Trying to initialize the sftp session";
    if (sftp_init(mSftp) < 0) {
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Could not initialize the SFTP session."));
        return;
    }

    // Login succeeded!
    infoMessage(i18n("Successfully connected to %1", mHost));
    if (info.keepPassword) {
        qCDebug(KIO_SFTP_LOG) << "Caching info.username = " << info.username
                              << ", info.url = " << info.url.toDisplayString();
        cacheAuthentication(info);
    }

    // Update the original username in case it was changed!
    if (!mUsername.isEmpty()) {
        mUsername = info.username;
    }

    setTimeoutSpecialCommand(KIO_SFTP_SPECIAL_TIMEOUT);

    mConnected = true;
    connected();

    info.password.fill('x');
    info.password.clear();
}

void sftpProtocol::closeConnection() {
    qCDebug(KIO_SFTP_LOG);

    if (mSftp) {
        sftp_free(mSftp);
        mSftp = nullptr;
    }

    if (mSession) {
        ssh_disconnect(mSession);
        mSession = nullptr;
    }

    mConnected = false;
}

void sftpProtocol::special(const QByteArray &) {
    int rc;
    qCDebug(KIO_SFTP_LOG) << "special(): polling";

    if (!mSftp) {
        error(KIO::ERR_INTERNAL, i18n("Invalid sftp context"));
        return;
    }

    /*
     * ssh_channel_poll() returns the number of bytes that may be read on the
     * channel. It does so by checking the input buffer and eventually the
     * network socket for data to read. If the input buffer is not empty, it
     * will not probe the network (and such not read packets nor reply to
     * keepalives).
     *
     * As ssh_channel_poll can act on two specific buffers (a channel has two
     * different stream: stdio and stderr), polling for data on the stderr
     * stream has more chance of not being in the problematic case (data left
     * in the buffer). Checking the return value (for >0) would be a good idea
     * to debug the problem.
     */
    rc = ssh_channel_poll(mSftp->channel, 0);
    if (rc > 0) {
        rc = ssh_channel_poll(mSftp->channel, 1);
    }

    if (rc < 0) {
        qCDebug(KIO_SFTP_LOG) << "ssh_channel_poll failed: " << ssh_get_error(mSession);
    }

    setTimeoutSpecialCommand(KIO_SFTP_SPECIAL_TIMEOUT);

    finished();
}

void sftpProtocol::open(const QUrl &url, QIODevice::OpenMode mode) {
    qCDebug(KIO_SFTP_LOG) << "open: " << url;

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    const QString path = url.path();
    const QByteArray path_c = path.toUtf8();

    sftp_attributes sb = sftp_lstat(mSftp, path_c.constData());
    if (sb == nullptr) {
        reportError(url, sftp_get_error(mSftp));
        return;
    }

    switch (sb->type) {
    case SSH_FILEXFER_TYPE_DIRECTORY:
        error(KIO::ERR_IS_DIRECTORY, url.toDisplayString());
        sftp_attributes_free(sb);
        return;
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
        error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.toDisplayString());
        sftp_attributes_free(sb);
        return;
    case SSH_FILEXFER_TYPE_SYMLINK:
    case SSH_FILEXFER_TYPE_REGULAR:
        break;
    }

    KIO::filesize_t fileSize = sb->size;
    sftp_attributes_free(sb);

    int flags = 0;

    if (mode & QIODevice::ReadOnly) {
        if (mode & QIODevice::WriteOnly) {
            flags = O_RDWR | O_CREAT;
        } else {
            flags = O_RDONLY;
        }
    } else if (mode & QIODevice::WriteOnly) {
        flags = O_WRONLY | O_CREAT;
    }

    if (mode & QIODevice::Append) {
        flags |= O_APPEND;
    } else if (mode & QIODevice::Truncate) {
        flags |= O_TRUNC;
    }

    if (flags & O_CREAT) {
        mOpenFile = sftp_open(mSftp, path_c.constData(), flags, 0644);
    } else {
        mOpenFile = sftp_open(mSftp, path_c.constData(), flags, 0);
    }

    if (mOpenFile == nullptr) {
        error(KIO::ERR_CANNOT_OPEN_FOR_READING, path);
        return;
    }

    // Determine the mimetype of the file to be retrieved, and emit it.
    // This is mandatory in all slaves (for KRun/BrowserRun to work).
    // If we're not opening the file ReadOnly or ReadWrite, don't attempt to
    // read the file and send the mimetype.
    if (mode & QIODevice::ReadOnly) {
        size_t bytesRequested = 1024;
        ssize_t bytesRead = 0;
        QVarLengthArray<char> buffer(bytesRequested);

        bytesRead = sftp_read(mOpenFile, buffer.data(), bytesRequested);
        if (bytesRead < 0) {
            error(KIO::ERR_COULD_NOT_READ, mOpenUrl.toDisplayString());
            closeWithoutFinish();
            return;
        } else {
            QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
            QMimeDatabase db;
            QMimeType mime = db.mimeTypeForFileNameAndData(mOpenUrl.fileName(), fileData);
            emit mimeType(mime.name());

            // Go back to the beginning of the file.
            sftp_rewind(mOpenFile);
        }
    }

    mOpenUrl = url;

    openOffset = 0;
    totalSize(fileSize);
    position(0);
    opened();
    finished();
}

void sftpProtocol::read(KIO::filesize_t bytes) {
    qCDebug(KIO_SFTP_LOG) << "read, offset = " << openOffset << ", bytes = " << bytes;

    Q_ASSERT(mOpenFile != nullptr);

    QVarLengthArray<char> buffer(bytes);

    ssize_t bytesRead = sftp_read(mOpenFile, buffer.data(), bytes);
    Q_ASSERT(bytesRead <= static_cast<ssize_t>(bytes));

    if (bytesRead < 0) {
        qCDebug(KIO_SFTP_LOG) << "Could not read " << mOpenUrl;
        error(KIO::ERR_COULD_NOT_READ, mOpenUrl.toDisplayString());
        closeWithoutFinish();
        return;
    }

    const QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
    data(fileData);
    finished();
}

void sftpProtocol::write(const QByteArray &data) {
    qCDebug(KIO_SFTP_LOG) << "write, offset = " << openOffset << ", bytes = " << data.size();

    Q_ASSERT(mOpenFile != nullptr);

    ssize_t bytesWritten = sftp_write(mOpenFile, data.data(), data.size());
    if (bytesWritten < 0) {
        qCDebug(KIO_SFTP_LOG) << "Could not write to " << mOpenUrl;
        error(KIO::ERR_COULD_NOT_WRITE, mOpenUrl.toDisplayString());
        closeWithoutFinish();
        return;
    }

    written(bytesWritten);
    finished();
}

void sftpProtocol::seek(KIO::filesize_t offset) {
    qCDebug(KIO_SFTP_LOG) << "seek, offset = " << offset;

    Q_ASSERT(mOpenFile != nullptr);

    if (sftp_seek64(mOpenFile, static_cast<uint64_t>(offset)) < 0) {
        error(KIO::ERR_COULD_NOT_SEEK, mOpenUrl.path());
        closeWithoutFinish();
        return;
    }

    position(sftp_tell64(mOpenFile));
    finished();
}

void sftpProtocol::close() {
    closeWithoutFinish();
    finished();
}

void sftpProtocol::get(const QUrl& url) {
    qCDebug(KIO_SFTP_LOG) << url;

    int errorCode = 0;
    const sftpProtocol::StatusCode cs = sftpGet(url, errorCode);

    // The call to sftpGet should only return server side errors since the file
    // descriptor parameter is set to -1.
    if (cs == sftpProtocol::ServerError && errorCode) {
        error(errorCode, url.toDisplayString());
        return;
    }

    finished();
}

sftpProtocol::StatusCode sftpProtocol::sftpGet(const QUrl& url, int& errorCode, KIO::fileoffset_t offset, int fd) {

    qCDebug(KIO_SFTP_LOG) << url;

    if (!sftpLogin()) {
        return sftpProtocol::ServerError;
    }

    QByteArray path = url.path().toUtf8();

    sftp_file file = nullptr;
    KIO::filesize_t totalbytesread  = 0;
    QByteArray filedata;

    sftp_attributes sb = sftp_lstat(mSftp, path.constData());
    if (sb == nullptr) {
        errorCode = toKIOError(sftp_get_error(mSftp));
        return sftpProtocol::ServerError;
    }

    switch (sb->type) {
    case SSH_FILEXFER_TYPE_DIRECTORY:
        errorCode = KIO::ERR_IS_DIRECTORY;
        sftp_attributes_free(sb);
        return sftpProtocol::ServerError;
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
        errorCode = KIO::ERR_CANNOT_OPEN_FOR_READING;
        sftp_attributes_free(sb);
        return sftpProtocol::ServerError;
    case SSH_FILEXFER_TYPE_SYMLINK:
    case SSH_FILEXFER_TYPE_REGULAR:
        break;
    }

    // Open file
    file = sftp_open(mSftp, path.constData(), O_RDONLY, 0);
    if (file == nullptr) {
        errorCode = KIO::ERR_CANNOT_OPEN_FOR_READING;
        sftp_attributes_free(sb);
        return sftpProtocol::ServerError;
    }

    char mimeTypeBuf[1024];
    ssize_t bytesread = sftp_read(file, mimeTypeBuf, sizeof(mimeTypeBuf));

    if (bytesread < 0) {
        errorCode = KIO::ERR_COULD_NOT_READ;
        return sftpProtocol::ServerError;
    } else  {
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFileNameAndData(url.fileName(), QByteArray(mimeTypeBuf, bytesread));
        if (!mime.isDefault()) {
            emit mimeType(mime.name());
        } else {
            mime = db.mimeTypeForUrl(url);
            emit mimeType(mime.name());
        }
        sftp_rewind(file);
    }

    // Set the total size
    totalSize(sb->size);

    // If offset is not specified, check the "resume" meta-data.
    if (offset < 0) {
        const QString resumeOffsetStr = metaData(QLatin1String("resume"));
        if (!resumeOffsetStr.isEmpty()) {
            bool ok;
            qlonglong resumeOffset = resumeOffsetStr.toLongLong(&ok);
            if (ok) {
                offset = resumeOffset;
            }
        }
    }

    // If we can resume, offset the buffer properly.
    if (offset > 0 && ((unsigned long long) offset < sb->size))
    {
        if (sftp_seek64(file, offset) == 0) {
            canResume();
            totalbytesread = offset;
            qCDebug(KIO_SFTP_LOG) << "Resume offset: " << QString::number(offset);
        }
    }

    bytesread = 0;
    sftpProtocol::GetRequest request(file, sb);

    for (;;) {
        // Enqueue get requests
        if (!request.enqueueChunks()) {
            errorCode = KIO::ERR_COULD_NOT_READ;
            return sftpProtocol::ServerError;
        }

        filedata.clear();
        bytesread = request.readChunks(filedata);
        // Read pending get requests
        if (bytesread == -1) {
            errorCode = KIO::ERR_COULD_NOT_READ;
            return sftpProtocol::ServerError;
        } else if (bytesread == 0) {
            if (file->eof)
                break;
            else
                continue;
        }

        if (fd == -1) {
            data(filedata);
        } else if ((errorCode = writeToFile(fd, filedata.constData(), filedata.size())) != 0) {
            return sftpProtocol::ClientError;
        }
        // increment total bytes read
        totalbytesread += filedata.length();

        processedSize(totalbytesread);
    }

    if (fd == -1)
        data(QByteArray());

    processedSize(static_cast<KIO::filesize_t>(sb->size));
    return sftpProtocol::Success;
}

void sftpProtocol::put(const QUrl& url, int permissions, KIO::JobFlags flags) {
    qCDebug(KIO_SFTP_LOG) << url << ", permissions =" << permissions
                          << ", overwrite =" << (flags & KIO::Overwrite)
                          << ", resume =" << (flags & KIO::Resume);

    qCDebug(KIO_SFTP_LOG) << url;

    int errorCode = 0;
    const sftpProtocol::StatusCode cs = sftpPut(url, permissions, flags, errorCode);

    // The call to sftpPut should only return server side errors since the file
    // descriptor parameter is set to -1.
    if (cs == sftpProtocol::ServerError && errorCode) {
        error(errorCode, url.toDisplayString());
        return;
    }

    finished();
}

sftpProtocol::StatusCode sftpProtocol::sftpPut(const QUrl& url, int permissions, JobFlags flags, int& errorCode, int fd) {
    qCDebug(KIO_SFTP_LOG) << url << ", permissions =" << permissions
                          << ", overwrite =" << (flags & KIO::Overwrite)
                          << ", resume =" << (flags & KIO::Resume);

    if (!sftpLogin()) {
        return sftpProtocol::ServerError;
    }

    const QString dest_orig = url.path();
    const QByteArray dest_orig_c = dest_orig.toUtf8();
    const QString dest_part = dest_orig + ".part";
    const QByteArray dest_part_c = dest_part.toUtf8();
    uid_t owner = 0;
    gid_t group = 0;

    sftp_attributes sb = sftp_lstat(mSftp, dest_orig_c.constData());
    const bool bOrigExists = (sb != nullptr);
    bool bPartExists = false;
    const bool bMarkPartial = config()->readEntry("MarkPartial", true);

    // Don't change permissions of the original file
    if (bOrigExists) {
        permissions = sb->permissions;
        owner = sb->uid;
        group = sb->gid;
    }

    if (bMarkPartial) {
        sftp_attributes sbPart = sftp_lstat(mSftp, dest_part_c.constData());
        bPartExists = (sbPart != nullptr);

        if (bPartExists && !(flags & KIO::Resume) && !(flags & KIO::Overwrite) &&
                sbPart->size > 0 && sbPart->type == SSH_FILEXFER_TYPE_REGULAR) {

            if (fd == -1) {
                // Maybe we can use this partial file for resuming
                // Tell about the size we have, and the app will tell us
                // if it's ok to resume or not.
                qCDebug(KIO_SFTP_LOG) << "calling canResume with " << sbPart->size;
                flags |= canResume(sbPart->size) ? KIO::Resume : KIO::DefaultFlags;
                qCDebug(KIO_SFTP_LOG) << "put got answer " << (flags & KIO::Resume);

            } else {
                KIO::filesize_t pos = seekPos(fd, sbPart->size, SEEK_SET);
                if (pos != sbPart->size) {
                    qCDebug(KIO_SFTP_LOG) << "Failed to seek to" << sbPart->size << "bytes in source file. Reason given" << strerror(errno);
                    sftp_attributes_free(sb);
                    sftp_attributes_free(sbPart);
                    errorCode = ERR_COULD_NOT_SEEK;
                    return sftpProtocol::ClientError;
                }
                flags |= KIO::Resume;
            }
            qCDebug(KIO_SFTP_LOG) << "Resuming at" << sbPart->size;
            sftp_attributes_free(sbPart);
        }
    }

    if (bOrigExists && !(flags & KIO::Overwrite) && !(flags & KIO::Resume)) {
        errorCode = KSFTP_ISDIR(sb) ? KIO::ERR_DIR_ALREADY_EXIST : KIO::ERR_FILE_ALREADY_EXIST;
        sftp_attributes_free(sb);
        return sftpProtocol::ServerError;
    }

    QByteArray dest;
    int result = -1;
    sftp_file file = nullptr;
    StatusCode cs = sftpProtocol::Success;
    KIO::fileoffset_t totalBytesSent = 0;

    // Loop until we got 0 (end of data)
    do {
        QByteArray buffer;

        if (fd == -1) {
            dataReq(); // Request for data
            result = readData(buffer);
        } else {
            char buf[MAX_XFER_BUF_SIZE]; //
            result = ::read(fd, buf, sizeof(buf));
            if(result < 0) {
                errorCode = ERR_COULD_NOT_READ;
                cs = sftpProtocol::ClientError;
                break;
            }
            buffer = QByteArray(buf, result);
        }

        if (result >= 0) {
            if (dest.isEmpty()) {
                if (bMarkPartial) {
                    qCDebug(KIO_SFTP_LOG) << "Appending .part extension to" << dest_orig;
                    dest = dest_part_c;
                    if (bPartExists && !(flags & KIO::Resume)) {
                        qCDebug(KIO_SFTP_LOG) << "Deleting partial file" << dest_part;
                        sftp_unlink(mSftp, dest_part_c.constData());
                        // Catch errors when we try to open the file.
                    }
                } else {
                    dest = dest_orig_c; // Will be automatically truncated below...
                } // bMarkPartial

                if ((flags & KIO::Resume)) {
                    sftp_attributes fstat;

                    qCDebug(KIO_SFTP_LOG) << "Trying to append: " << dest;
                    file = sftp_open(mSftp, dest.constData(), O_RDWR, 0);  // append if resuming
                    if (file) {
                        fstat = sftp_fstat(file);
                        if (fstat) {
                            sftp_seek64(file, fstat->size); // Seek to end TODO
                            totalBytesSent += fstat->size;
                            sftp_attributes_free(fstat);
                        }
                    }
                } else {
                    mode_t initialMode;

                    if (permissions != -1) {
                        initialMode = permissions | S_IWUSR | S_IRUSR;
                    } else {
                        initialMode = 0644;
                    }

                    qCDebug(KIO_SFTP_LOG) << "Trying to open:" << QString(dest) << ", mode=" << QString::number(initialMode);
                    file = sftp_open(mSftp, dest.constData(), O_CREAT | O_TRUNC | O_WRONLY, initialMode);
                } // flags & KIO::Resume

                if (file == nullptr) {
                    qCDebug(KIO_SFTP_LOG) << "COULD NOT WRITE " << QString(dest)
                                          << ", permissions=" << permissions
                                          << ", error=" << ssh_get_error(mSession);
                    if (sftp_get_error(mSftp) == SSH_FX_PERMISSION_DENIED) {
                        errorCode = KIO::ERR_WRITE_ACCESS_DENIED;
                    } else {
                        errorCode = KIO::ERR_CANNOT_OPEN_FOR_WRITING;
                    }
                    cs = sftpProtocol::ServerError;
                    result = -1;
                    continue;
                } // file
            } // dest.isEmpty

            ssize_t bytesWritten = sftp_write(file, buffer.data(), buffer.size());
            if (bytesWritten < 0) {
                errorCode = KIO::ERR_COULD_NOT_WRITE;
                result = -1;
            } else {
                totalBytesSent += bytesWritten;
                emit processedSize(totalBytesSent);
            }
        } // result
    } while (result > 0);
    sftp_attributes_free(sb);

    // An error occurred deal with it.
    if (result < 0) {
        qCDebug(KIO_SFTP_LOG) << "Error during 'put'. Aborting.";

        if (file != nullptr) {
            sftp_close(file);

            sftp_attributes attr = sftp_stat(mSftp, dest.constData());
            if (bMarkPartial && attr != nullptr) {
                size_t size = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
                if (attr->size < size) {
                    sftp_unlink(mSftp, dest.constData());
                }
            }
            sftp_attributes_free(attr);
        }

        //::exit(255);
        return cs;
    }

    if (file == nullptr) { // we got nothing to write out, so we never opened the file
        return sftpProtocol::Success;
    }

    if (sftp_close(file) < 0) {
        qCWarning(KIO_SFTP_LOG) << "Error when closing file descriptor";
        error(KIO::ERR_COULD_NOT_WRITE, dest_orig);
        return sftpProtocol::ServerError;
    }

    // after full download rename the file back to original name
    if (bMarkPartial) {
        // If the original URL is a symlink and we were asked to overwrite it,
        // remove the symlink first. This ensures that we do not overwrite the
        // current source if the symlink points to it.
        if ((flags & KIO::Overwrite)) {
            sftp_unlink(mSftp, dest_orig_c.constData());
        }

        if (sftp_rename(mSftp, dest.constData(), dest_orig_c.constData()) < 0) {
            qCWarning(KIO_SFTP_LOG) << " Couldn't rename " << dest << " to " << dest_orig;
            errorCode = KIO::ERR_CANNOT_RENAME_PARTIAL;
            return sftpProtocol::ServerError;
        }
    }

    // set final permissions
    if (permissions != -1 && !(flags & KIO::Resume)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to set final permissions of " << dest_orig << " to " << QString::number(permissions);
        if (sftp_chmod(mSftp, dest_orig_c.constData(), permissions) < 0) {
            errorCode = -1;  // force copy to call sftpSendWarning...
            return sftpProtocol::ServerError;
        }
    }

    // set original owner and group
    if (bOrigExists) {
        qCDebug(KIO_SFTP_LOG) << "Trying to restore original owner and group of " << dest_orig;
        if (sftp_chown(mSftp, dest_orig_c.constData(), owner, group) < 0) {
            qCWarning(KIO_SFTP_LOG) << "Could not change owner and group for" << dest_orig;
            // warning(i18n( "Could not change owner and group for\n%1", dest_orig));
        }
    }

    // set modification time
    const QString mtimeStr = metaData("modified");
    if (!mtimeStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
        if (dt.isValid()) {
            struct timeval times[2];

            sftp_attributes attr = sftp_lstat(mSftp, dest_orig_c.constData());
            if (attr != nullptr) {
                times[0].tv_sec = attr->atime; //// access time, unchanged
                times[1].tv_sec =  dt.toTime_t(); // modification time
                times[0].tv_usec = times[1].tv_usec = 0;

                qCDebug(KIO_SFTP_LOG) << "Trying to restore mtime for " << dest_orig << " to: " << mtimeStr;
                result = sftp_utimes(mSftp, dest_orig_c.constData(), times);
                if (result < 0) {
                    qCWarning(KIO_SFTP_LOG) << "Failed to set mtime for" << dest_orig;
                }
                sftp_attributes_free(attr);
            }
        }
    }

    return sftpProtocol::Success;
}

void sftpProtocol::copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags)
{
    qCDebug(KIO_SFTP_LOG) << src << " -> " << dest << " , permissions = " << QString::number(permissions)
                          << ", overwrite = " << (flags & KIO::Overwrite)
                          << ", resume = " << (flags & KIO::Resume);

    QString sCopyFile;
    int errorCode = 0;
    StatusCode cs = sftpProtocol::ClientError;
    const bool isSourceLocal = src.isLocalFile();
    const bool isDestinationLocal = dest.isLocalFile();

    if (!isSourceLocal && isDestinationLocal) {                   // sftp -> file
        sCopyFile = dest.toLocalFile();
        cs = sftpCopyGet(src, sCopyFile, permissions, flags, errorCode);
        if (cs == sftpProtocol::ServerError)
            sCopyFile = src.url();
    } else if (isSourceLocal && !isDestinationLocal) {            // file -> sftp
        sCopyFile = src.toLocalFile();
        cs = sftpCopyPut(dest, sCopyFile, permissions, flags, errorCode);
        if (cs == sftpProtocol::ServerError)
            sCopyFile = dest.url();
    } else {
        errorCode = KIO::ERR_UNSUPPORTED_ACTION;
        sCopyFile.clear();
    }

    if (cs != sftpProtocol::Success && errorCode > 0) {
        error(errorCode, sCopyFile);
        return;
    }

    if (errorCode < 0) {
        sftpSendWarning(errorCode, sCopyFile);
    }

    finished();
}

sftpProtocol::StatusCode sftpProtocol::sftpCopyGet(const QUrl& url, const QString& sCopyFile, int permissions, KIO::JobFlags flags, int& errorCode)
{
    qCDebug(KIO_SFTP_LOG) << url << "->" << sCopyFile << ", permissions=" << permissions;

    // check if destination is ok ...
    QT_STATBUF buff;
    const bool bDestExists = (QT_STAT(QFile::encodeName(sCopyFile), &buff) != -1);

    if(bDestExists)  {
        if(S_ISDIR(buff.st_mode)) {
            errorCode = ERR_IS_DIRECTORY;
            return sftpProtocol::ClientError;
        }

        if(!(flags & KIO::Overwrite)) {
            errorCode = ERR_FILE_ALREADY_EXIST;
            return sftpProtocol::ClientError;
        }
    }

    bool bResume = false;
    const QString sPart = sCopyFile + QLatin1String(".part"); // do we have a ".part" file?
    const bool bPartExists = (QT_STAT(QFile::encodeName(sPart), &buff) != -1);
    const bool bMarkPartial = config()->readEntry("MarkPartial", true);
    const QString dest = (bMarkPartial ? sPart : sCopyFile);

    if (bMarkPartial && bPartExists && buff.st_size > 0) {
        if (S_ISDIR(buff.st_mode)) {
            errorCode = ERR_DIR_ALREADY_EXIST;
            return sftpProtocol::ClientError;                            // client side error
        }
        bResume = canResume( buff.st_size );
    }

    if (bPartExists && !bResume)                  // get rid of an unwanted ".part" file
        QFile::remove(sPart);

    // WABA: Make sure that we keep writing permissions ourselves,
    // otherwise we can be in for a surprise on NFS.
    mode_t initialMode;
    if (permissions != -1)
        initialMode = permissions | S_IWUSR;
    else
        initialMode = 0666;

    // open the output file ...
    int fd = -1;
    KIO::fileoffset_t offset = 0;
    if (bResume) {
        fd = QT_OPEN( QFile::encodeName(sPart), O_RDWR );  // append if resuming
        offset = seekPos(fd, 0, SEEK_END);
        if(offset < 0) {
            errorCode = ERR_CANNOT_RESUME;
            ::close(fd);
            return sftpProtocol::ClientError;                            // client side error
        }
        qCDebug(KIO_SFTP_LOG) << "resuming at" << offset;
    }
    else {
        fd = QT_OPEN(QFile::encodeName(dest), O_CREAT | O_TRUNC | O_WRONLY, initialMode);
    }

    if (fd == -1) {
        qCDebug(KIO_SFTP_LOG) << "could not write to" << sCopyFile;
        errorCode = (errno == EACCES) ? ERR_WRITE_ACCESS_DENIED : ERR_CANNOT_OPEN_FOR_WRITING;
        return sftpProtocol::ClientError;
    }

    StatusCode result = sftpGet(url, errorCode, offset, fd);

    if( ::close(fd) && result == sftpProtocol::Success ) {
        errorCode = ERR_COULD_NOT_WRITE;
        result = sftpProtocol::ClientError;
    }

    // handle renaming or deletion of a partial file ...
    if (bMarkPartial) {
        if (result == sftpProtocol::Success) { // rename ".part" on success
            if ( !QFile::rename( QFile::encodeName(sPart), sCopyFile ) ) {
                // If rename fails, try removing the destination first if it exists.
                if (!bDestExists || !QFile::remove(sCopyFile) || !QFile::rename(sPart, sCopyFile)) {
                    qCDebug(KIO_SFTP_LOG) << "cannot rename " << sPart << " to " << sCopyFile;
                    errorCode = ERR_CANNOT_RENAME_PARTIAL;
                    result = sftpProtocol::ClientError;
                }
            }
        }
        else if (QT_STAT( QFile::encodeName(sPart), &buff ) == 0) { // should a very small ".part" be deleted?
            const int size = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
            if (buff.st_size <  size)
                QFile::remove(sPart);
        }
    }

    const QString mtimeStr = metaData("modified");
    if (!mtimeStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
        if (dt.isValid()) {
            struct utimbuf utbuf;
            utbuf.actime = buff.st_atime; // access time, unchanged
            utbuf.modtime = dt.toTime_t(); // modification time
            utime(QFile::encodeName(sCopyFile), &utbuf);
        }
    }

    return result;
}

sftpProtocol::StatusCode sftpProtocol::sftpCopyPut(const QUrl& url, const QString& sCopyFile, int permissions, JobFlags flags, int& errorCode)
{
    qCDebug(KIO_SFTP_LOG) << sCopyFile << "->" << url << ", permissions=" << permissions << ", flags" << flags;

    // check if source is ok ...
    QT_STATBUF buff;
    bool bSrcExists = (QT_STAT(QFile::encodeName(sCopyFile), &buff) != -1);

    if (bSrcExists) {
        if (S_ISDIR(buff.st_mode)) {
            errorCode = ERR_IS_DIRECTORY;
            return sftpProtocol::ClientError;
        }
    } else {
        errorCode = ERR_DOES_NOT_EXIST;
        return sftpProtocol::ClientError;
    }

    const int fd = QT_OPEN(QFile::encodeName(sCopyFile), O_RDONLY);
    if(fd == -1)
    {
        errorCode = ERR_CANNOT_OPEN_FOR_READING;
        return sftpProtocol::ClientError;
    }

    totalSize(buff.st_size);

    // delegate the real work (errorCode gets status) ...
    StatusCode ret = sftpPut(url, permissions, flags, errorCode, fd);
    ::close(fd);
    return ret;
}


void sftpProtocol::stat(const QUrl& url) {
    qCDebug(KIO_SFTP_LOG) << url;

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    if (url.path().isEmpty() || QDir::isRelativePath(url.path()) ||
            url.path().contains("/./") || url.path().contains("/../")) {
        QString cPath;

        if (!url.path().isEmpty()) {
            cPath = canonicalizePath(url.path());
        } else {
            cPath = canonicalizePath(QLatin1String("."));
        }

        if (cPath.isEmpty()) {
            error(KIO::ERR_MALFORMED_URL, url.toDisplayString());
            return;
        }
        QUrl redir(url);
        redir.setPath(cPath);
        redirection(redir);

        qCDebug(KIO_SFTP_LOG) << "redirecting to " << redir.url();

        finished();
        return;
    }

    QByteArray path = url.path().toUtf8();

    const QString sDetails = metaData(QLatin1String("details"));
    const int details = sDetails.isEmpty() ? 2 : sDetails.toInt();

    UDSEntry entry;
    entry.clear();
    if (!createUDSEntry(url.fileName(), path, entry, details)) {
        error(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
        return;
    }

    statEntry(entry);

    finished();
}

void sftpProtocol::mimetype(const QUrl& url){
    qCDebug(KIO_SFTP_LOG) << url;

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    // open() feeds the mimetype
    open(url, QIODevice::ReadOnly);
    // open() finished(), don't finish in close again.
    closeWithoutFinish();
}

void sftpProtocol::listDir(const QUrl& url) {
    qCDebug(KIO_SFTP_LOG) << "list directory: " << url;

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    if (url.path().isEmpty() || QDir::isRelativePath(url.path()) ||
            url.path().contains("/./") || url.path().contains("/../")) {
        QString cPath;

        if (!url.path().isEmpty() ) {
            cPath = canonicalizePath(url.path());
        } else {
            cPath = canonicalizePath(QStringLiteral("."));
        }

        if (cPath.isEmpty()) {
            error(KIO::ERR_MALFORMED_URL, url.toDisplayString());
            return;
        }
        QUrl redir(url);
        redir.setPath(cPath);
        redirection(redir);

        qCDebug(KIO_SFTP_LOG) << "redirecting to " << redir.url();

        finished();
        return;
    }

    QByteArray path = url.path().toUtf8();

    sftp_dir dp = sftp_opendir(mSftp, path.constData());
    if (dp == nullptr) {
        reportError(url, sftp_get_error(mSftp));
        return;
    }

    sftp_attributes dirent = nullptr;
    const QString sDetails = metaData(QLatin1String("details"));
    const int details = sDetails.isEmpty() ? 2 : sDetails.toInt();
    UDSEntry entry;

    qCDebug(KIO_SFTP_LOG) << "readdir: " << path << ", details: " << QString::number(details);

    for (;;) {
        mode_t access;
        char *link;
        bool isBrokenLink = false;
        long long fileType = S_IFREG;
        long long size = 0LL;

        dirent = sftp_readdir(mSftp, dp);
        if (dirent == nullptr) {
            break;
        }

        entry.clear();
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, QFile::decodeName(dirent->name));

        if (dirent->type == SSH_FILEXFER_TYPE_SYMLINK) {
            QByteArray file = path + '/' + QFile::decodeName(dirent->name).toUtf8();

            link = sftp_readlink(mSftp, file.constData());
            if (link == nullptr) {
                sftp_attributes_free(dirent);
                error(KIO::ERR_INTERNAL, i18n("Could not read link: %1", QString::fromUtf8(file)));
                return;
            }
            entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, QFile::decodeName(link));
            free(link);
            // A symlink -> follow it only if details > 1
            if (details > 1) {
                sftp_attributes sb = sftp_stat(mSftp, file.constData());
                if (sb == nullptr) {
                    isBrokenLink = true;
                } else {
                    sftp_attributes_free(dirent);
                    dirent = sb;
                }
            }
        }

        if (isBrokenLink) {
            // It is a link pointing to nowhere
            fileType = S_IFMT - 1;
            access = S_IRWXU | S_IRWXG | S_IRWXO;
            size = 0LL;
        } else {
            switch (dirent->type) {
            case SSH_FILEXFER_TYPE_REGULAR:
                fileType = S_IFREG;
                break;
            case SSH_FILEXFER_TYPE_DIRECTORY:
                fileType = S_IFDIR;
                break;
            case SSH_FILEXFER_TYPE_SYMLINK:
                fileType = S_IFLNK;
                break;
            case SSH_FILEXFER_TYPE_SPECIAL:
            case SSH_FILEXFER_TYPE_UNKNOWN:
                break;
            }

            access = dirent->permissions & 07777;
            size = dirent->size;
        }
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, fileType);
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, access);
        entry.fastInsert(KIO::UDSEntry::UDS_SIZE, size);

        if (details > 0) {
            if (dirent->owner) {
                entry.fastInsert(KIO::UDSEntry::UDS_USER, QString::fromUtf8(dirent->owner));
            } else {
                entry.fastInsert(KIO::UDSEntry::UDS_USER, QString::number(dirent->uid));
            }

            if (dirent->group) {
                entry.fastInsert(KIO::UDSEntry::UDS_GROUP, QString::fromUtf8(dirent->group));
            } else {
                entry.fastInsert(KIO::UDSEntry::UDS_GROUP, QString::number(dirent->gid));
            }

            entry.fastInsert(KIO::UDSEntry::UDS_ACCESS_TIME, dirent->atime);
            entry.fastInsert(KIO::UDSEntry::UDS_MODIFICATION_TIME, dirent->mtime);
            entry.fastInsert(KIO::UDSEntry::UDS_CREATION_TIME, dirent->createtime);
        }

        sftp_attributes_free(dirent);
        listEntry(entry);
    } // for ever
    sftp_closedir(dp);
    finished();
}

void sftpProtocol::mkdir(const QUrl &url, int permissions) {
    qCDebug(KIO_SFTP_LOG) << "create directory: " << url;

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    if (url.path().isEmpty()) {
        error(KIO::ERR_MALFORMED_URL, url.toDisplayString());
        return;
    }
    const QString path = url.path();
    const QByteArray path_c = path.toUtf8();

    // Remove existing file or symlink, if requested.
    if (metaData(QLatin1String("overwrite")) == QLatin1String("true")) {
        qCDebug(KIO_SFTP_LOG) << "overwrite set, remove existing file or symlink: " << url;
        sftp_unlink(mSftp, path_c.constData());
    }

    qCDebug(KIO_SFTP_LOG) << "Trying to create directory: " << path;
    sftp_attributes sb = sftp_lstat(mSftp, path_c.constData());
    if (sb == nullptr) {
        if (sftp_mkdir(mSftp, path_c.constData(), 0777) < 0) {
            reportError(url, sftp_get_error(mSftp));
            sftp_attributes_free(sb);
            return;
        }

        qCDebug(KIO_SFTP_LOG) << "Successfully created directory: " << url;
        if (permissions != -1) {
            // This will report an error or finished.
            chmod(url, permissions);
        } else {
            finished();
        }

        sftp_attributes_free(sb);
        return;
    }

    auto err = KSFTP_ISDIR(sb) ? KIO::ERR_DIR_ALREADY_EXIST : KIO::ERR_FILE_ALREADY_EXIST;
    sftp_attributes_free(sb);
    error(err, path);
}

void sftpProtocol::rename(const QUrl& src, const QUrl& dest, KIO::JobFlags flags) {
    qCDebug(KIO_SFTP_LOG) << "rename " << src << " to " << dest << flags;

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    QByteArray qsrc = src.path().toUtf8();
    QByteArray qdest = dest.path().toUtf8();

    sftp_attributes sb = sftp_lstat(mSftp, qdest.constData());
    if (sb != nullptr) {
        const bool isDir = KSFTP_ISDIR(sb);
        if (!(flags & KIO::Overwrite)) {
            error(isDir ? KIO::ERR_DIR_ALREADY_EXIST : KIO::ERR_FILE_ALREADY_EXIST, dest.url());
            sftp_attributes_free(sb);
            return;
        }

        // Delete the existing destination file/dir...
        if (isDir) {
            if (sftp_rmdir(mSftp, qdest.constData()) < 0) {
                reportError(dest, sftp_get_error(mSftp));
                return;
            }
        } else {
            if (sftp_unlink(mSftp, qdest.constData()) < 0) {
                reportError(dest, sftp_get_error(mSftp));
                return;
            }
        }
    }
    sftp_attributes_free(sb);

    if (sftp_rename(mSftp, qsrc.constData(), qdest.constData()) < 0) {
        reportError(dest, sftp_get_error(mSftp));
        return;
    }

    finished();
}

void sftpProtocol::symlink(const QString &target, const QUrl &dest, KIO::JobFlags flags) {
    qCDebug(KIO_SFTP_LOG) << "link " << target << "->" << dest
                          << ", overwrite = " << (flags & KIO::Overwrite)
                          << ", resume = " << (flags & KIO::Resume);

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    QByteArray t = target.toUtf8();
    QByteArray d = dest.path().toUtf8();

    bool failed = false;
    if (sftp_symlink(mSftp, t.constData(), d.constData()) < 0) {
        if (flags == KIO::Overwrite) {
            sftp_attributes sb = sftp_lstat(mSftp, d.constData());
            if (sb == nullptr) {
                failed = true;
            } else {
                if (sftp_unlink(mSftp, d.constData()) < 0) {
                    failed = true;
                } else {
                    if (sftp_symlink(mSftp, t.constData(), d.constData()) < 0) {
                        failed = true;
                    }
                }
            }
            sftp_attributes_free(sb);
        }
    }

    if (failed) {
        reportError(dest, sftp_get_error(mSftp));
        return;
    }

    finished();
}

void sftpProtocol::chmod(const QUrl& url, int permissions) {
    qCDebug(KIO_SFTP_LOG) << "change permission of " << url << " to " << QString::number(permissions);

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    QByteArray path = url.path().toUtf8();

    if (sftp_chmod(mSftp, path.constData(), permissions) < 0) {
        reportError(url, sftp_get_error(mSftp));
        return;
    }

    finished();
}

void sftpProtocol::del(const QUrl &url, bool isfile){
    qCDebug(KIO_SFTP_LOG) << "deleting " << (isfile ? "file: " : "directory: ") << url;

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    QByteArray path = url.path().toUtf8();

    if (isfile) {
        if (sftp_unlink(mSftp, path.constData()) < 0) {
            reportError(url, sftp_get_error(mSftp));
            return;
        }
    } else {
        if (sftp_rmdir(mSftp, path.constData()) < 0) {
            reportError(url, sftp_get_error(mSftp));
            return;
        }
    }

    finished();
}

void sftpProtocol::slave_status() {
    qCDebug(KIO_SFTP_LOG) << "connected to " << mHost << "?: " << mConnected;
    slaveStatus((mConnected ? mHost : QString()), mConnected);
}

sftpProtocol::GetRequest::GetRequest(sftp_file file, sftp_attributes sb, ushort maxPendingRequests)
    :mFile(file), mSb(sb), mMaxPendingRequests(maxPendingRequests) {

}

bool sftpProtocol::GetRequest::enqueueChunks() {
    sftpProtocol::GetRequest::Request request;

    qCDebug(KIO_SFTP_TRACE_LOG) << "enqueueChunks";

    while (pendingRequests.count() < mMaxPendingRequests) {
        request.expectedLength = MAX_XFER_BUF_SIZE;
        request.startOffset = mFile->offset;
        request.id = sftp_async_read_begin(mFile, request.expectedLength);
        if (request.id < 0) {
            if (pendingRequests.isEmpty()) {
                return false;
            } else {
                break;
            }
        }

        pendingRequests.enqueue(request);

        if (mFile->offset >= mSb->size) {
            // Do not add any more chunks if the offset is larger than the given file size.
            // However this is done after adding a request as the remote file size may
            // have changed in the meantime.
            break;
        }
    }

    qCDebug(KIO_SFTP_TRACE_LOG) << "enqueueChunks done" << QString::number(pendingRequests.size());

    return true;
}

int sftpProtocol::GetRequest::readChunks(QByteArray &data) {

    int totalRead = 0;
    ssize_t bytesread = 0;

    while (!pendingRequests.isEmpty()) {
        sftpProtocol::GetRequest::Request &request = pendingRequests.head();
        int dataSize = data.size() + request.expectedLength;

        data.resize(dataSize);
        if (data.size() < dataSize) {
            // Could not allocate enough memory - skip current chunk
            data.resize(dataSize - request.expectedLength);
            break;
        }

        bytesread = sftp_async_read(mFile, data.data() + totalRead, request.expectedLength, request.id);

        // qCDebug(KIO_SFTP_LOG) << "bytesread=" << QString::number(bytesread);

        if (bytesread == 0 || bytesread == SSH_AGAIN) {
            // Done reading or timeout
            data.resize(data.size() - request.expectedLength);

            if (bytesread == 0) {
                pendingRequests.dequeue(); // This frees QByteArray &data!
            }

            break;
        } else if (bytesread == SSH_ERROR) {
            return -1;
        }

        totalRead += bytesread;

        if (bytesread < request.expectedLength) {
            int rc;

            // If less data is read than expected - requeue the request
            data.resize(data.size() - (request.expectedLength - bytesread));

            // Modify current request
            request.expectedLength -= bytesread;
            request.startOffset += bytesread;

            rc = sftp_seek64(mFile, request.startOffset);
            if (rc < 0) {
                // Failed to continue reading
                return -1;
            }

            request.id = sftp_async_read_begin(mFile, request.expectedLength);

            if (request.id < 0) {
                // Failed to dispatch rerequest
                return -1;
            }

            return totalRead;
        }

        pendingRequests.dequeue();
    }

    return totalRead;
}

sftpProtocol::GetRequest::~GetRequest() {
    sftpProtocol::GetRequest::Request request;
    char buf[MAX_XFER_BUF_SIZE];

    // Remove pending reads to avoid memory leaks
    while (!pendingRequests.isEmpty()) {
        request = pendingRequests.dequeue();
        sftp_async_read(mFile, buf, request.expectedLength, request.id);
    }

    // Close channel & free attributes
    sftp_close(mFile);
    sftp_attributes_free(mSb);
}

void sftpProtocol::requiresUserNameRedirection()
{
    QUrl redirectUrl;
    redirectUrl.setScheme( QLatin1String("sftp") );
    redirectUrl.setUserName( mUsername );
    redirectUrl.setPassword( mPassword );
    redirectUrl.setHost( mHost );
    if (mPort > 0 && mPort != DEFAULT_SFTP_PORT) {
        redirectUrl.setPort( mPort );
    }
    qCDebug(KIO_SFTP_LOG) << "redirecting to" << redirectUrl;
    redirection( redirectUrl );
}

bool sftpProtocol::sftpLogin()
{
    const QString origUsername = mUsername;
    openConnection();
    qCDebug(KIO_SFTP_LOG) << "connected ?" << mConnected << "username: old=" << origUsername << "new=" << mUsername;
    if (!origUsername.isEmpty() && origUsername != mUsername) {
        requiresUserNameRedirection();
        finished();
        return false;
    }
    return mConnected;
}

void sftpProtocol::sftpSendWarning(int errorCode, const QString& url)
{
    switch (errorCode) {
    case -1:
        warning(i18n( "Could not change permissions for\n%1", url));
        break;
    default:
        break;
    }
}

void sftpProtocol::closeWithoutFinish()
{
    sftp_close(mOpenFile);
    mOpenFile = nullptr;
}

void sftpProtocol::clearPubKeyAuthInfo()
{
    if (mPublicKeyAuthInfo) {
        delete mPublicKeyAuthInfo;
        mPublicKeyAuthInfo = nullptr;
    }
}

void sftpProtocol::fileSystemFreeSpace(const QUrl& url)
{
    qCDebug(KIO_SFTP_LOG) << "file system free space of" << url;

    if (!sftpLogin()) {
        // sftpLogin finished()
        return;
    }

    if (sftp_extension_supported(mSftp, "statvfs@openssh.com", "2") == 0) {
        error(ERR_UNSUPPORTED_ACTION, QString());
        return;
    }

    const QByteArray path = url.path().toUtf8();

    sftp_statvfs_t statvfs = sftp_statvfs(mSftp, path.constData());
    if (statvfs == nullptr) {
        reportError(url, sftp_get_error(mSftp));
        return;
    }

    setMetaData(QString::fromLatin1("total"), QString::number(statvfs->f_frsize * statvfs->f_blocks));
    setMetaData(QString::fromLatin1("available"), QString::number(statvfs->f_frsize * statvfs->f_bavail));

    sftp_statvfs_free(statvfs);

    finished();
}
