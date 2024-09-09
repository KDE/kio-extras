/*
 * SPDX-FileCopyrightText: 2001 Lucas Fisher <ljfisher@purdue.edu>
 * SPDX-FileCopyrightText: 2009 Andreas Schneider <mail@cynapses.org>
 * SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kio_sftp.h"
#include <config-runtime.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <memory>
#include <queue>
#include <span>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QScopeGuard>
#include <QScopedPointer>
#include <QVarLengthArray>

#include <KMessageBox>
#include <KUser>

#include <KConfigGroup>
#include <KIO/AuthInfo>
#include <KLocalizedString>
#include <kio/ioworker_defaults.h>

#ifdef Q_OS_WIN
#include <qplatformdefs.h>
#endif

#include "../filenamesearch/kio_filenamesearch_p.h"
#include "kio_sftp_debug.h"
#include "kio_sftp_trace_debug.h"

// For MinGW compatibility
#ifndef QT_STAT_LNK
#define QT_STAT_LNK 0120000
#endif // QT_STAT_LNK

// especially windows misses CLOEXEC, so pretend it doesn't do anything
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

using namespace KIO;
using namespace std::filesystem;

template<typename Output, typename Input>
constexpr Output narrow(Input i)
{
    Output o = static_cast<Input>(i);
    if (i != Input(o)) {
        std::abort();
    }
    return o;
}

namespace std
{
template<>
struct default_delete<struct sftp_file_struct> {
    void operator()(struct sftp_file_struct *ptr) const
    {
        if (ptr) {
            sftp_close(ptr);
        }
    }
};

#if defined(HAVE_SFTP_AIO)
template<>
struct default_delete<struct sftp_aio_struct> {
    void operator()(struct sftp_aio_struct *ptr) const
    {
        sftp_aio_free(ptr);
    }
};
#endif
} // namespace std

using UniqueSFTPFilePtr = std::unique_ptr<struct sftp_file_struct>;

namespace
{
constexpr auto KIO_SFTP_SPECIAL_TIMEOUT_MS = 30;
constexpr auto MAX_PENDING_REQUESTS = 128;

// How big should each data packet be? Definitely not bigger than 64kb or
// you will overflow the 2 byte size variable in a sftp packet.
// TODO: investigate what size we should have and consider changing.
// this seems too large...
// from the RFC:
//   The maximum size of a packet is in practice determined by the client
//   (the maximum size of read or write requests that it sends, plus a few
//   bytes of packet overhead).  All servers SHOULD support packets of at
//   least 34000 bytes (where the packet size refers to the full length,
//   including the header above).  This should allow for reads and writes of
//   at most 32768 bytes.
// In practice that means we can assume that the server supports 32kb,
// it may be more or it could be less. Since there's not really a system in place to
// figure out the maximum (and at least openssh arbitrarily resets the entire
// session if it finds a packet that is too large
// [https://bugs.kde.org/show_bug.cgi?id=404890]) we ought to be more conservative!
// At the same time there's no bug reports about the 60k requests being too large so
// perhaps all popular servers effectively support at least 64k.
constexpr auto MAX_XFER_BUF_SIZE = (60ULL * 1024);

inline bool KSFTP_ISDIR(SFTPAttributesPtr &sb)
{
    return sb->type == SSH_FILEXFER_TYPE_DIRECTORY;
}

// Converts SSH error into KIO error. Only must be called for error handling
// as this will always return an error state and never NoError.
int toKIOError(const int err)
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

// Writes buf into fd.
int writeToFile(int fd, const std::span<const char> &buf)
{
    size_t offset = 0;
    while (offset != buf.size()) {
        const auto written = write(fd, &buf[offset], buf.size() - offset);

        if (written >= 0) {
            offset += written;
            continue;
        }

        switch (errno) {
        case EINTR:
        case EAGAIN:
            continue;
        case EPIPE:
            return ERR_CONNECTION_BROKEN;
        case ENOSPC:
            return ERR_DISK_FULL;
        default:
            return ERR_CANNOT_WRITE;
        }
    }
    return 0;
}

bool wasUsernameChanged(const QString &username, const KIO::AuthInfo &info)
{
    QString loginName(username);
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
int auth_callback(const char *prompt, char *buf, size_t len, int echo, int verify, void *userdata)
{
    if (userdata == nullptr) {
        return -1;
    }

    auto *worker = static_cast<SFTPWorker *>(userdata);
    if (worker->auth_callback(prompt, buf, len, echo, verify, userdata) < 0) {
        return -1;
    }

    return 0;
}

void log_callback(int priority, const char *function, const char *buffer, void *userdata)
{
    if (userdata == nullptr) {
        return;
    }

    auto *worker = static_cast<SFTPWorker *>(userdata);
    worker->log_callback(priority, function, buffer, userdata);
}

inline std::optional<perms> posixToOptionalPerms(uint32_t mode) noexcept
{
    return static_cast<perms>(mode) & perms::mask;
}

inline mode_t permsToPosix(perms mode) noexcept
{
    return static_cast<mode_t>(mode);
}
} // namespace

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.sftp" FILE "sftp.json")
};

extern "C" {
int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("kio_sftp");

    qCDebug(KIO_SFTP_LOG) << "*** Starting kio_sftp ";

    if (argc != 4) {
        qCWarning(KIO_SFTP_LOG) << "Usage: kio_sftp protocol domain-socket1 domain-socket2";
        exit(-1);
    }

    std::span args{argv, std::make_unsigned_t<decltype(argc)>(argc)};
    SFTPWorker worker(args[2], args[3]);
    worker.dispatchLoop();

    qCDebug(KIO_SFTP_LOG) << "*** kio_sftp Done";
    return 0;
}
}

SFTPWorker::SFTPWorker(const QByteArray &poolSocket, const QByteArray &appSocket)
    : WorkerBase(QByteArrayLiteral("kio_sftp"), poolSocket, appSocket)
{
    const auto result = init();
    Q_ASSERT(result.success());
}

SFTPWorker::~SFTPWorker()
{
    qCDebug(KIO_SFTP_LOG) << "pid = " << qApp->applicationPid();
    closeConnection();

    delete mCallbacks;
    delete mPublicKeyAuthInfo; // for precaution

    /* cleanup and shut down crypto stuff */
    ssh_finalize();
}

int SFTPWorker::auth_callback(const char *prompt, char *buf, size_t len, int echo, int verify, void *userdata)
{
    Q_UNUSED(echo)
    Q_UNUSED(verify)
    Q_UNUSED(userdata)

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

    QUrl u(mPublicKeyAuthInfo->url);
    u.setPath(QString());
    mPublicKeyAuthInfo->comment = u.url();
    mPublicKeyAuthInfo->readOnly = true;
    mPublicKeyAuthInfo->prompt = QString::fromUtf8(prompt);
    mPublicKeyAuthInfo->keepPassword = false; // don't save passwords for public key,
    // that's the task of ssh-agent.
    mPublicKeyAuthInfo->setExtraField(QLatin1String("hide-username-line"), true);
    mPublicKeyAuthInfo->setModified(false);

    qCDebug(KIO_SFTP_LOG) << "Entering authentication callback, prompt=" << mPublicKeyAuthInfo->prompt;

    if (openPasswordDialog(*mPublicKeyAuthInfo, errMsg) != 0) {
        qCDebug(KIO_SFTP_LOG) << "User canceled public key password dialog";
        return -1;
    }

    strncpy(buf, mPublicKeyAuthInfo->password.toUtf8().constData(), len - 1);

    mPublicKeyAuthInfo->password.fill('x');
    mPublicKeyAuthInfo->password.clear();

    return 0;
}

void SFTPWorker::log_callback(int priority, const char *function, const char *buffer, void *userdata)
{
    Q_UNUSED(userdata)
    qCDebug(KIO_SFTP_LOG) << "[" << function << "] (" << priority << ") " << buffer;
}

Result SFTPWorker::init()
{
    qCDebug(KIO_SFTP_LOG) << "pid = " << qApp->applicationPid();
    qCDebug(KIO_SFTP_LOG) << "debug = " << getenv("KIO_SFTP_LOG_VERBOSITY");

    // Members are 'value initialized' to zero because of non-user defined ()!
    mCallbacks = new struct ssh_callbacks_struct();
    if (mCallbacks == nullptr) {
        return Result::fail(KIO::ERR_OUT_OF_MEMORY, i18n("Could not allocate callbacks"));
    }

    mCallbacks->userdata = this;
    mCallbacks->auth_function = ::auth_callback;

    ssh_callbacks_init(mCallbacks);

    bool ok = false;
    int level = qEnvironmentVariableIntValue("KIO_SFTP_LOG_VERBOSITY", &ok);
    if (ok) {
        int rc = ssh_set_log_level(level);
        if (rc != SSH_OK) {
            return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set log verbosity."));
        }

        rc = ssh_set_log_userdata(this);
        if (rc != SSH_OK) {
            return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set log userdata."));
        }

        rc = ssh_set_log_callback(::log_callback);
        if (rc != SSH_OK) {
            return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set log callback."));
        }
    }

    return Result::pass();
}

int SFTPWorker::authenticateKeyboardInteractive(AuthInfo &info)
{
    int err = ssh_userauth_kbdint(mSession, nullptr, nullptr);

    while (err == SSH_AUTH_INFO) {
        const QString name = QString::fromUtf8(ssh_userauth_kbdint_getname(mSession));
        const QString instruction = QString::fromUtf8(ssh_userauth_kbdint_getinstruction(mSession));
        const int n = ssh_userauth_kbdint_getnprompts(mSession);

        qCDebug(KIO_SFTP_LOG) << "name=" << name << " instruction=" << instruction << " prompts=" << n;

        for (int iInt = 0; iInt < n; ++iInt) {
            const auto i = static_cast<unsigned int>(iInt); // can only be >0
            char echo = 0;
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

                infoKbdInt.comment = "sftp://" + mUsername + "@" + mHost;

                QString newPrompt;
                if (!instruction.isEmpty()) {
                    newPrompt = instruction + "<br /><br />";
                }
                newPrompt.append(prompt);
                infoKbdInt.prompt = newPrompt;

                infoKbdInt.readOnly = false;
                infoKbdInt.keepPassword = false;

                if (openPasswordDialog(infoKbdInt, i18n("Use the username input field to answer this question.")) == KJob::NoError) {
                    qCDebug(KIO_SFTP_LOG) << "Got the answer from the password dialog";
                    answer = info.username.toUtf8().constData();
                }

                if (ssh_userauth_kbdint_setanswer(mSession, i, answer) < 0) {
                    qCDebug(KIO_SFTP_LOG) << "An error occurred setting the answer: " << ssh_get_error(mSession);
                    return SSH_AUTH_ERROR;
                }
                break;
            }

            if (prompt.startsWith(QLatin1String("password:"), Qt::CaseInsensitive)) {
                info.prompt = i18n("Please enter your password.");
            } else {
                info.prompt = prompt;
            }
            info.comment = info.url.url();
            info.commentLabel = i18n("Site:");
            info.setExtraField(QLatin1String("hide-username-line"), true);

            if (openPasswordDialog(info) == KJob::NoError) {
                qCDebug(KIO_SFTP_LOG) << "Got the answer from the password dialog";
                answer = info.password.toUtf8().constData();
            }

            if (ssh_userauth_kbdint_setanswer(mSession, i, answer) < 0) {
                qCDebug(KIO_SFTP_LOG) << "An error occurred setting the answer: " << ssh_get_error(mSession);
                return SSH_AUTH_ERROR;
            }
        }
        err = ssh_userauth_kbdint(mSession, nullptr, nullptr);
    }

    return err;
}

Result SFTPWorker::reportError(const QUrl &url, int err)
{
    qCDebug(KIO_SFTP_LOG) << "url = " << url << " - err=" << err;

    const int kioError = toKIOError(err);
    Q_ASSERT(kioError != KJob::NoError);

    return Result::fail(kioError, url.toDisplayString());
}

Result SFTPWorker::createUDSEntry(SFTPAttributesPtr sb, UDSEntry &entry, const QByteArray &path, const QString &name, int details)
{
    // caller should make sure it is not null. behavior between stat on a file and listing a dir is different!
    // Also we consume the pointer, hence why it is a SFTPAttributesPtr
    Q_ASSERT(sb.get());

    entry.clear();
    entry.reserve(10);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, name /* awkwardly sftp_lstat doesn't fill the attributes name correctly, calculate from path */);

    bool isBrokenLink = false;
    if (sb->type == SSH_FILEXFER_TYPE_SYMLINK) {
        std::unique_ptr<char, decltype(&free)> link(sftp_readlink(mSftp, path.constData()), free);
        if (!link) {
            return Result::fail(KIO::ERR_INTERNAL,
                                i18nc("error message. %1 is a path, %2 is a numeric error code",
                                      "Could not read link: %1 [%2]",
                                      QString::fromUtf8(path),
                                      QString::number(sftp_get_error(mSftp))));
        }
        entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, QFile::decodeName(link.get()));
        // A symlink -> follow it only if details > 1
        if (details > 1) {
            sftp_attributes sb2 = sftp_stat(mSftp, path.constData());
            if (sb2 == nullptr) {
                isBrokenLink = true;
            } else {
                sb.reset(sb2);
            }
        }
    }

    perms access = perms::none;
    long long fileType = QT_STAT_REG;
    uint64_t size = 0U;
    if (isBrokenLink) {
        // It is a link pointing to nowhere
        fileType = QT_STAT_MASK - 1;
        access = perms::all;
        size = 0LL;
    } else {
        switch (sb->type) {
        case SSH_FILEXFER_TYPE_REGULAR:
            fileType = QT_STAT_REG;
            break;
        case SSH_FILEXFER_TYPE_DIRECTORY:
            fileType = QT_STAT_DIR;
            break;
        case SSH_FILEXFER_TYPE_SYMLINK:
            fileType = QT_STAT_LNK;
            break;
        case SSH_FILEXFER_TYPE_SPECIAL:
        case SSH_FILEXFER_TYPE_UNKNOWN:
            fileType = QT_STAT_MASK - 1;
            break;
        default: // type is an unsigned int and may contain anything, explicitly default to break
            break;
        }
        access = posixToOptionalPerms(sb->permissions).value_or(perms::none);
        size = sb->size;
    }
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, fileType);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, permsToPosix(access));
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, narrow<long long>(size));

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

        if (sb->flags & SSH_FILEXFER_ATTR_CREATETIME) {
            // Availability depends on outside factors.
            // https://bugs.kde.org/show_bug.cgi?id=375305
            entry.fastInsert(KIO::UDSEntry::UDS_CREATION_TIME, narrow<long long>(sb->createtime));
        }
    }

    return Result::pass();
}

QString SFTPWorker::canonicalizePath(const QString &path)
{
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
    ssh_string_free_char(sPath);

    qCDebug(KIO_SFTP_LOG) << "Canonicalized path: " << cPath;

    return cPath;
}

void SFTPWorker::setHost(const QString &host, quint16 port, const QString &user, const QString &pass)
{
    qCDebug(KIO_SFTP_LOG) << user << "@" << host << ":" << port;

    // Close connection if the request is to another server...
    if (host != mHost || port != mPort || user != mUsername || pass != mPassword) {
        closeConnection();
    }

    mHost = host;
    mPort = port;
    mUsername = user;
    mPassword = pass;
}

Result SFTPWorker::sftpOpenConnection(const AuthInfo &info)
{
    closeConnection(); // make sure a potential previous connection is closed. this function may get called to re-connect

    mSession = ssh_new();
    if (mSession == nullptr) {
        return Result::fail(KIO::ERR_OUT_OF_MEMORY, i18n("Could not create a new SSH session."));
    }

    const long timeout_sec = 30;
    const long timeout_usec = 0;

    qCDebug(KIO_SFTP_LOG) << "Creating the SSH session and setting options";

    // Set timeout
    int rc = ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT, &timeout_sec);
    if (rc < 0) {
        return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set a timeout."));
    }
    rc = ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT_USEC, &timeout_usec);
    if (rc < 0) {
        return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set a timeout."));
    }

    // Disable Nagle's Algorithm (TCP_NODELAY). Usually faster for sftp.
    bool nodelay = true;
    rc = ssh_options_set(mSession, SSH_OPTIONS_NODELAY, &nodelay);
    if (rc < 0) {
        return Result::fail(KIO::ERR_INTERNAL, i18n("Could not disable Nagle's Algorithm."));
    }

    // Prefer not to use compression
    rc = ssh_options_set(mSession, SSH_OPTIONS_COMPRESSION_C_S, "none,zlib@openssh.com,zlib");
    if (rc < 0) {
        return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set compression."));
    }

    rc = ssh_options_set(mSession, SSH_OPTIONS_COMPRESSION_S_C, "none,zlib@openssh.com,zlib");
    if (rc < 0) {
        return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set compression."));
    }

    // Set host and port
    rc = ssh_options_set(mSession, SSH_OPTIONS_HOST, mHost.toUtf8().constData());
    if (rc < 0) {
        return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set host."));
    }

    if (mPort > 0) {
        rc = ssh_options_set(mSession, SSH_OPTIONS_PORT, &mPort);
        if (rc < 0) {
            return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set port."));
        }
    }

    // Set the username
    if (!info.username.isEmpty()) {
        rc = ssh_options_set(mSession, SSH_OPTIONS_USER, info.username.toUtf8().constData());
        if (rc < 0) {
            return Result::fail(KIO::ERR_INTERNAL, i18n("Could not set username."));
        }
    }

    // Read ~/.ssh/config
    rc = ssh_options_parse_config(mSession, nullptr);
    if (rc < 0) {
        return Result::fail(KIO::ERR_INTERNAL, i18n("Could not parse the config file."));
    }

    ssh_set_callbacks(mSession, mCallbacks);

    qCDebug(KIO_SFTP_LOG) << "Trying to connect to the SSH server";

    unsigned int effectivePort = mPort;
    if (effectivePort <= 0) {
        effectivePort = DEFAULT_SFTP_PORT;
        ssh_options_get_port(mSession, &effectivePort);
    }

    qCDebug(KIO_SFTP_LOG) << "username=" << mUsername << ", host=" << mHost << ", port=" << effectivePort;

    infoMessage(xi18n("Opening SFTP connection to host %1:%2", mHost, QString::number(effectivePort)));

    /* try to connect */
    rc = ssh_connect(mSession);
    if (rc < 0) {
        const QString errorString = QString::fromUtf8(ssh_get_error(mSession));
        closeConnection();
        return Result::fail(KIO::ERR_WORKER_DEFINED, errorString);
    }

    return Result::pass();
}

struct ServerKeyInspection {
    QByteArray serverPublicKeyType;
    QByteArray fingerprint;
    Result result = Result::pass();

    ServerKeyInspection &withResult(const Result &result)
    {
        this->result = result;
        return *this;
    }
};

Q_REQUIRED_RESULT ServerKeyInspection fingerprint(ssh_session session)
{
    ServerKeyInspection inspection;

    ssh_key srv_pubkey = nullptr;
    const auto freeKey = qScopeGuard([srv_pubkey] {
        ssh_key_free(srv_pubkey);
    });
    int rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0) {
        return inspection.withResult(Result::fail(KIO::ERR_WORKER_DEFINED, QString::fromUtf8(ssh_get_error(session))));
    }

    const char *srv_pubkey_type = ssh_key_type_to_char(ssh_key_type(srv_pubkey));
    if (srv_pubkey_type == nullptr) {
        return inspection.withResult(Result::fail(KIO::ERR_WORKER_DEFINED, i18n("Could not get server public key type name")));
    }
    inspection.serverPublicKeyType = QByteArray(srv_pubkey_type);

    unsigned char *hash = nullptr; // the server hash
    const auto freeHash = qScopeGuard([&hash] {
        ssh_clean_pubkey_hash(&hash);
    });

    size_t hlen = 0;
    rc = ssh_get_publickey_hash(srv_pubkey, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen);
    if (rc != SSH_OK) {
        return inspection.withResult(Result::fail(KIO::ERR_WORKER_DEFINED, i18n("Could not create hash from server public key")));
    }

    char *fingerprint = ssh_get_fingerprint_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
    const auto freeFingerprint = qScopeGuard([fingerprint] {
        ssh_string_free_char(fingerprint);
    });

    if (fingerprint == nullptr) {
        return inspection.withResult(Result::fail(KIO::ERR_WORKER_DEFINED, i18n("Could not create fingerprint for server public key")));
    }

    inspection.fingerprint = fingerprint;

    return inspection.withResult(Result::pass());
}

Result SFTPWorker::openConnectionWithoutCloseOnError()
{
    if (mConnected) {
        return Result::pass();
    }

    if (mHost.isEmpty()) {
        qCDebug(KIO_SFTP_LOG) << "openConnection(): Need hostname...";
        return Result::fail(KIO::ERR_UNKNOWN_HOST, QString());
    }

    AuthInfo info;
    info.url.setScheme("sftp");
    info.url.setHost(mHost);
    if (mPort > 0 && mPort != DEFAULT_SFTP_PORT) {
        info.url.setPort(mPort);
    }
    info.url.setUserName(mUsername);
    info.username = mUsername;

    // Check for cached authentication info if no password is specified...
    if (mPassword.isEmpty()) {
        qCDebug(KIO_SFTP_LOG) << "checking cache: info.username =" << info.username << ", info.url =" << info.url.toDisplayString();
        checkCachedAuthentication(info);
    } else {
        info.password = mPassword;
    }

    // Start the ssh connection.

    // Attempt to start a ssh session and establish a connection with the server.
    if (auto openResult = sftpOpenConnection(info); !openResult.success()) {
        return openResult;
    }

    // get the hash
    qCDebug(KIO_SFTP_LOG) << "Getting the SSH server hash";
    const ServerKeyInspection inspection = fingerprint(mSession);
    if (!inspection.result.success()) {
        return inspection.result;
    }
    const QString fingerprint = QString::fromUtf8(inspection.fingerprint);
    const QString serverPublicKeyType = QString::fromUtf8(inspection.serverPublicKeyType);

    qCDebug(KIO_SFTP_LOG) << "Checking if the SSH server is known";

    /* check the server public key hash */
    enum ssh_known_hosts_e state = ssh_session_is_known_server(mSession);
    switch (state) {
    case SSH_KNOWN_HOSTS_OTHER: {
        const QString errorString = i18n(
            "An %1 host key for this server was "
            "not found, but another type of key exists.\n"
            "An attacker might change the default server key to confuse your "
            "client into thinking the key does not exist.\n"
            "Please contact your system administrator.\n"
            "%2",
            serverPublicKeyType,
            QString::fromUtf8(ssh_get_error(mSession)));
        return Result::fail(KIO::ERR_WORKER_DEFINED, errorString);
    }
    case SSH_KNOWN_HOSTS_CHANGED:
    case SSH_KNOWN_HOSTS_NOT_FOUND:
    case SSH_KNOWN_HOSTS_UNKNOWN: {
        QString caption;
        QString msg;

        if (state == SSH_KNOWN_HOSTS_CHANGED) {
            caption = i18nc("@title:window", "Host Identity Change");
            msg = xi18nc("@info",
                         "<para>The host key for the server <emphasis>%1</emphasis> has changed.</para>"
                         "<para>This could either mean that DNS spoofing is happening or the IP "
                         "address for the host and its host key have changed at the same time.</para>"
                         "<para>The %2 key fingerprint sent by the remote host is:"
                         "<bcode>%3</bcode>"
                         "Are you sure you want to continue connecting?</para>",
                         mHost,
                         serverPublicKeyType,
                         fingerprint);
        } else {
            caption = i18nc("@title:window", "Host Verification Failure");
            msg = xi18nc("@info",
                         "<para>The authenticity of host <emphasis>%1</emphasis> cannot be established.</para>"
                         "<para>The %2 key fingerprint is:"
                         "<bcode>%3</bcode>"
                         "Are you sure you want to continue connecting?</para>",
                         mHost,
                         serverPublicKeyType,
                         fingerprint);
        }

        if (KMessageBox::Continue != messageBox(WorkerBase::WarningContinueCancel, msg, caption, i18nc("@action:button", "Connect Anyway"))) {
            return Result::fail(KIO::ERR_USER_CANCELED);
        }

        /* write the known_hosts file */
        qCDebug(KIO_SFTP_LOG) << "Adding server to known_hosts file.";
        int rc = ssh_session_update_known_hosts(mSession);
        if (rc != SSH_OK) {
            return Result::fail(KIO::ERR_USER_CANCELED, QString::fromUtf8(ssh_get_error(mSession)));
        }
        break;
    }
    case SSH_KNOWN_HOSTS_ERROR:
        return Result::fail(KIO::ERR_WORKER_DEFINED, QString::fromUtf8(ssh_get_error(mSession)));
    case SSH_KNOWN_HOSTS_OK:
        break;
    }

    qCDebug(KIO_SFTP_LOG) << "Trying to authenticate with the server";

    // Try to login without authentication
    int rc = ssh_userauth_none(mSession, nullptr);
    if (rc == SSH_AUTH_ERROR) {
        return Result::fail(KIO::ERR_CANNOT_LOGIN, i18n("Authentication failed."));
    }

    // This NEEDS to be called after ssh_userauth_none() !!!
    int method = ssh_auth_list(mSession);
    if (rc != SSH_AUTH_SUCCESS && method == 0) {
        return Result::fail(KIO::ERR_CANNOT_LOGIN,
                            i18n("Authentication failed. The server "
                                 "didn't send any authentication methods"));
    }

    // Try to authenticate with public key first
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_PUBLICKEY)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to authenticate with public key";
        for (;;) {
            rc = ssh_userauth_publickey_auto(mSession, nullptr, nullptr);
            if (rc == SSH_AUTH_ERROR) {
                qCDebug(KIO_SFTP_LOG) << "Public key authentication failed:" << QString::fromUtf8(ssh_get_error(mSession));
                clearPubKeyAuthInfo();
                return Result::fail(KIO::ERR_CANNOT_LOGIN, i18n("Authentication failed."));
            }
            if (rc != SSH_AUTH_DENIED || !mPublicKeyAuthInfo || !mPublicKeyAuthInfo->isModified()) {
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
            qCDebug(KIO_SFTP_LOG) << "Public key authentication failed:" << QString::fromUtf8(ssh_get_error(mSession));
            return Result::fail(KIO::ERR_CANNOT_LOGIN, i18n("Authentication failed."));
        }
    }

    // Try to authenticate with keyboard interactive
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_INTERACTIVE)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to authenticate with keyboard interactive";
        AuthInfo info2(info);
        rc = authenticateKeyboardInteractive(info2);
        if (rc == SSH_AUTH_SUCCESS) {
            info = info2;
        } else if (rc == SSH_AUTH_ERROR) {
            qCDebug(KIO_SFTP_LOG) << "Keyboard interactive authentication failed:" << QString::fromUtf8(ssh_get_error(mSession));
            return Result::fail(KIO::ERR_CANNOT_LOGIN, i18n("Authentication failed."));
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

        for (;;) {
            if (!isFirstLoginAttempt || info.password.isEmpty()) {
                info.keepPassword = true; // make the "keep Password" check box visible to the user.
                info.setModified(false);

                QString username(info.username);
                const QString errMsg(isFirstLoginAttempt ? QString() : i18n("Incorrect username or password"));

                qCDebug(KIO_SFTP_LOG) << "Username:" << username << "first attempt?" << isFirstLoginAttempt << "error:" << errMsg;

                // Handle user canceled or dialog failed to open...

                int errCode = openPasswordDialog(info, errMsg);
                if (errCode != 0) {
                    qCDebug(KIO_SFTP_LOG) << "User canceled password/retry dialog";
                    return Result::fail(errCode, QString());
                }

                // If the user name changes, we have to re-establish connection again
                // since the user name must always be set before calling ssh_connect.
                if (wasUsernameChanged(username, info)) {
                    qCDebug(KIO_SFTP_LOG) << "Username changed to" << info.username;
                    if (!info.url.userName().isEmpty()) {
                        info.url.setUserName(info.username);
                    }
                    const auto result = sftpOpenConnection(info);
                    if (!result.success()) {
                        return result;
                    }
                }
            }

            rc = ssh_userauth_password(mSession, info.username.toUtf8().constData(), info.password.toUtf8().constData());
            if (rc == SSH_AUTH_SUCCESS) {
                break;
            }
            if (rc == SSH_AUTH_ERROR) {
                qCDebug(KIO_SFTP_LOG) << "Password authentication failed:" << QString::fromUtf8(ssh_get_error(mSession));
                return Result::fail(KIO::ERR_CANNOT_LOGIN, i18n("Authentication failed."));
            }

            isFirstLoginAttempt = false; // failed attempt to login.
            info.password.clear(); // clear the password after failed attempts.
        }
    }

    // If we're still not authenticated then we need to leave.
    if (rc != SSH_AUTH_SUCCESS) {
        return Result::fail(KIO::ERR_CANNOT_LOGIN, i18n("Authentication failed."));
    }

    // start sftp session
    qCDebug(KIO_SFTP_LOG) << "Trying to request the sftp session";
    mSftp = sftp_new(mSession);
    if (mSftp == nullptr) {
        return Result::fail(KIO::ERR_CANNOT_LOGIN, i18n("Unable to request the SFTP subsystem. Make sure SFTP is enabled on the server."));
    }

    qCDebug(KIO_SFTP_LOG) << "Trying to initialize the sftp session";
    if (sftp_init(mSftp) < 0) {
        return Result::fail(KIO::ERR_CANNOT_LOGIN, i18n("Could not initialize the SFTP session."));
    }

    // Login succeeded!
    infoMessage(i18n("Successfully connected to %1", mHost));
    if (info.keepPassword) {
        qCDebug(KIO_SFTP_LOG) << "Caching info.username = " << info.username << ", info.url = " << info.url.toDisplayString();
        cacheAuthentication(info);
    }

    // Update the original username in case it was changed!
    if (!mUsername.isEmpty()) {
        mUsername = info.username;
    }

    setTimeoutSpecialCommand(KIO_SFTP_SPECIAL_TIMEOUT_MS);

    mConnected = true;

    info.password.fill('x');
    info.password.clear();

    return Result::pass();
}

Result SFTPWorker::openConnection()
{
    const Result result = openConnectionWithoutCloseOnError();
    if (!result.success()) {
        closeConnection();
    }
    return result;
}

void SFTPWorker::closeConnection()
{
    qCDebug(KIO_SFTP_LOG);

    if (mSftp) {
        sftp_free(mSftp);
        mSftp = nullptr;
    }

    if (mSession) {
        ssh_disconnect(mSession);
        ssh_free(mSession);
        mSession = nullptr;
    }

    mConnected = false;
}

Result SFTPWorker::special(const QByteArray &)
{
    qCDebug(KIO_SFTP_LOG) << "special(): polling";

    if (!mSftp) {
        return Result::fail(KIO::ERR_INTERNAL, i18n("Invalid sftp context"));
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
    int rc = ssh_channel_poll(mSftp->channel, 0);
    if (rc > 0) {
        rc = ssh_channel_poll(mSftp->channel, 1);
    }

    if (rc < 0) { // first or second poll failed
        qCDebug(KIO_SFTP_LOG) << "ssh_channel_poll failed: "
                              << "- SFTP error:" << sftp_get_error(mSftp) << "- SSH error:" << ssh_get_error_code(mSession)
                              << "- SSH errorString:" << ssh_get_error(mSession);
    }

    setTimeoutSpecialCommand(KIO_SFTP_SPECIAL_TIMEOUT_MS);

    return Result::pass();
}

Result SFTPWorker::open(const QUrl &url, QIODevice::OpenMode mode)
{
    qCDebug(KIO_SFTP_LOG) << "open: " << url;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    const QString path = url.path();
    const QByteArray path_c = path.toUtf8();

    SFTPAttributesPtr sb(sftp_lstat(mSftp, path_c.constData()));
    if (sb == nullptr) {
        return reportError(url, sftp_get_error(mSftp));
    }

    switch (sb->type) {
    case SSH_FILEXFER_TYPE_DIRECTORY:
        return Result::fail(KIO::ERR_IS_DIRECTORY, url.toDisplayString());
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
        return Result::fail(KIO::ERR_CANNOT_OPEN_FOR_READING, url.toDisplayString());
    case SSH_FILEXFER_TYPE_SYMLINK:
    case SSH_FILEXFER_TYPE_REGULAR:
    default: // type is an unsigned int and may contain anything, explicitly default to break
        break;
    }

    KIO::filesize_t fileSize = sb->size;

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
        mOpenFile = sftp_open(mSftp, path_c.constData(), flags, permsToPosix(perms::owner_read | perms::owner_write | perms::group_read | perms::others_read));
    } else {
        mOpenFile = sftp_open(mSftp, path_c.constData(), flags, 0);
    }

    if (mOpenFile == nullptr) {
        return Result::fail(toKIOError(sftp_get_error(mSftp)), path);
    }

    // Determine the mimetype of the file to be retrieved, and emit it.
    // This is mandatory in all workers (for KRun/BrowserRun to work).
    // If we're not opening the file ReadOnly or ReadWrite, don't attempt to
    // read the file and send the mimetype.
    if (mode & QIODevice::ReadOnly) {
        if (const Result result = sftpSendMimetype(mOpenFile, mOpenUrl); !result.success()) {
            (void)close();
            return result;
        }
    }

    mOpenUrl = url;

    openOffset = 0;
    totalSize(fileSize);
    position(0);

    return Result::pass();
}

Result SFTPWorker::read(KIO::filesize_t bytes)
{
    qCDebug(KIO_SFTP_LOG) << "read, offset = " << openOffset << ", bytes = " << bytes;

    Q_ASSERT(mOpenFile != nullptr);

    QVarLengthArray<char> buffer(narrow<qsizetype>(bytes));

    ssize_t bytesRead = sftp_read(mOpenFile, buffer.data(), bytes);
    Q_ASSERT(bytesRead <= static_cast<ssize_t>(bytes));

    if (bytesRead < 0) {
        qCDebug(KIO_SFTP_LOG) << "Could not read" << mOpenUrl << sftp_get_error(mSftp) << ssh_get_error_code(mSession) << ssh_get_error(mSession);
        (void)close();
        return Result::fail(KIO::ERR_CANNOT_READ, mOpenUrl.toDisplayString());
    }

    const QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
    data(fileData);

    return Result::pass();
}

Result SFTPWorker::write(const QByteArray &data)
{
    qCDebug(KIO_SFTP_LOG) << "write, offset = " << openOffset << ", bytes = " << data.size();

    Q_ASSERT(mOpenFile != nullptr);

    for (const auto &response : asyncWrite(mOpenFile, [data]() -> QCoro::Generator<ReadResponse> {
             co_yield ReadResponse(data);
         }())) {
        if (response.error != KJob::NoError) {
            qCDebug(KIO_SFTP_LOG) << "Could not write to " << mOpenUrl;
            (void)close();
            return Result::fail(KIO::ERR_CANNOT_WRITE, mOpenUrl.toDisplayString());
        }
        // We don't care about progress in this function. We'll emit a single written() once done.
    }

    written(data.size());

    return Result::pass();
}

Result SFTPWorker::seek(KIO::filesize_t offset)
{
    qCDebug(KIO_SFTP_LOG) << "seek, offset = " << offset;

    Q_ASSERT(mOpenFile != nullptr);

    if (sftp_seek64(mOpenFile, static_cast<uint64_t>(offset)) < 0) {
        (void)close();
        return Result::fail(KIO::ERR_CANNOT_SEEK, mOpenUrl.path());
    }

    position(sftp_tell64(mOpenFile));

    return Result::pass();
}

Result SFTPWorker::truncate(KIO::filesize_t length)
{
    qCDebug(KIO_SFTP_LOG) << "truncate, length =" << length;

    Q_ASSERT(mOpenFile);

    int errorCode = KJob::NoError;
    SFTPAttributesPtr attr(sftp_fstat(mOpenFile));
    if (attr) {
        attr->size = length;
        if (sftp_setstat(mSftp, mOpenUrl.path().toUtf8().constData(), attr.get()) == 0) {
            truncated(length);
        } else {
            errorCode = toKIOError(sftp_get_error(mSftp));
        }
    } else {
        errorCode = toKIOError(sftp_get_error(mSftp));
    }

    if (errorCode) {
        (void)close();
        return Result::fail(errorCode == KIO::ERR_INTERNAL ? KIO::ERR_CANNOT_TRUNCATE : errorCode, mOpenUrl.path());
    }

    return Result::pass();
}

Result SFTPWorker::close()
{
    if (mOpenFile) {
        sftp_close(mOpenFile);
    }
    mOpenFile = nullptr;
    return Result::pass();
}

Result SFTPWorker::get(const QUrl &url)
{
    qCDebug(KIO_SFTP_LOG) << url;

    const auto result = sftpGet(url);
    if (!result.success()) {
        return Result::fail(result.error(), url.toDisplayString());
    }

    return Result::pass();
}

Result SFTPWorker::sftpSendMimetype(sftp_file file, const QUrl &url)
{
    constexpr int readLimit = 1024; // entirely arbitrary
    std::array<char, readLimit> mimeTypeBuf{};
    const ssize_t bytesRead = sftp_read(file, mimeTypeBuf.data(), readLimit);
    if (bytesRead < 0) {
        return Result::fail(KIO::ERR_CANNOT_READ, url.toString());
    }

    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFileNameAndData(url.fileName(), QByteArray(mimeTypeBuf.data(), bytesRead));
    if (!mime.isDefault()) {
        mimeType(mime.name());
    } else {
        mimeType(db.mimeTypeForUrl(url).name());
    }
    sftp_rewind(file);

    return Result::pass();
}

Result SFTPWorker::sftpGet(const QUrl &url, KIO::fileoffset_t offset, int fd)
{
    qCDebug(KIO_SFTP_LOG) << url;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    QByteArray path = url.path().toUtf8();

    KIO::filesize_t totalbytesread = 0;

    SFTPAttributesPtr sb(sftp_lstat(mSftp, path.constData()));
    if (sb == nullptr) {
        return Result::fail(toKIOError(sftp_get_error(mSftp)), url.toString());
    }

    switch (sb->type) {
    case SSH_FILEXFER_TYPE_DIRECTORY:
        return Result::fail(KIO::ERR_IS_DIRECTORY, url.toString());
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
        return Result::fail(KIO::ERR_CANNOT_OPEN_FOR_READING, url.toString());
    case SSH_FILEXFER_TYPE_SYMLINK:
    case SSH_FILEXFER_TYPE_REGULAR:
    default: // type is an unsigned int and may contain anything, explicitly default to break
        break;
    }

    // Open file
    const UniqueSFTPFilePtr file(sftp_open(mSftp, path.constData(), O_RDONLY, 0));
    if (!file) {
        return Result::fail(KIO::ERR_CANNOT_OPEN_FOR_READING, url.toString());
    }

    if (const Result result = sftpSendMimetype(file.get(), url); !result.success()) {
        return result;
    }

    // Set the total size
    totalSize(sb->size);

    // If offset is not specified, check the "resume" meta-data.
    if (offset < 0) {
        const QString resumeOffsetStr = metaData(QLatin1String("resume"));
        if (!resumeOffsetStr.isEmpty()) {
            bool ok = false;
            qlonglong resumeOffset = resumeOffsetStr.toLongLong(&ok);
            if (ok) {
                offset = resumeOffset;
            }
        }
    }

    // If we can resume, offset the buffer properly.
    if (offset > 0 && ((unsigned long long)offset < sb->size)) {
        if (sftp_seek64(file.get(), offset) == 0) {
            canResume();
            totalbytesread = offset;
            qCDebug(KIO_SFTP_LOG) << "Resume offset: " << QString::number(offset);
        }
    }

    auto reader = asyncRead(file.get(), sb->size);
    for (const auto &response : reader) {
        if (response.error != KJob::NoError) {
            return Result::fail(response.error, url.toString());
        }

        if (fd == -1) {
            data(response.filedata);
        } else if (int error = writeToFile(fd, response.filedata); error != KJob::NoError) {
            return Result::fail(error, url.toString());
        }

        // increment total bytes read
        totalbytesread += response.filedata.length();
        processedSize(totalbytesread);
    }

    if (fd == -1) {
        data(QByteArray());
    }

    processedSize(static_cast<KIO::filesize_t>(sb->size));
    return Result::pass();
}

Result SFTPWorker::put(const QUrl &url, int permissions, KIO::JobFlags flags)
{
    qCDebug(KIO_SFTP_LOG) << url << ", permissions =" << permissions << ", overwrite =" << (flags & KIO::Overwrite) << ", resume =" << (flags & KIO::Resume);

    qCDebug(KIO_SFTP_LOG) << url;

    return sftpPut(url, permissions, flags);
}

Result SFTPWorker::sftpPut(const QUrl &url, int permissionsMode, JobFlags flags, int fd)
{
    qCDebug(KIO_SFTP_LOG) << url << ", permissions =" << permissionsMode << ", overwrite =" << (flags & KIO::Overwrite)
                          << ", resume =" << (flags & KIO::Resume);

    auto permissions(posixToOptionalPerms(permissionsMode));

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    const QString dest_orig = url.path();
    const QByteArray dest_orig_c = dest_orig.toUtf8();
    const QString dest_part = dest_orig + ".part";
    const QByteArray dest_part_c = dest_part.toUtf8();
    uid_t owner = 0;
    gid_t group = 0;

    SFTPAttributesPtr sb(sftp_lstat(mSftp, dest_orig_c.constData()));
    const bool bOrigExists = (sb != nullptr);
    bool bPartExists = false;
    const bool bMarkPartial = configValue(QStringLiteral("MarkPartial"), true);

    // Don't change permissions of the original file
    if (bOrigExists) {
        permissions = posixToOptionalPerms(sb->permissions);
        owner = sb->uid;
        group = sb->gid;
    }

    if (bMarkPartial) {
        SFTPAttributesPtr sbPart(sftp_lstat(mSftp, dest_part_c.constData()));
        bPartExists = (sbPart != nullptr);

        if (bPartExists && !(flags & KIO::Resume) && !(flags & KIO::Overwrite) && sbPart->size > 0 && sbPart->type == SSH_FILEXFER_TYPE_REGULAR) {
            if (fd == -1) {
                // Maybe we can use this partial file for resuming
                // Tell about the size we have, and the app will tell us
                // if it's ok to resume or not.
                qCDebug(KIO_SFTP_LOG) << "calling canResume with " << sbPart->size;
                flags |= canResume(sbPart->size) ? KIO::Resume : KIO::DefaultFlags;
                qCDebug(KIO_SFTP_LOG) << "put got answer " << (flags & KIO::Resume);

            } else {
                KIO::filesize_t pos = QT_LSEEK(fd, narrow<off_t>(sbPart->size), SEEK_SET);
                if (pos != sbPart->size) {
                    qCDebug(KIO_SFTP_LOG) << "Failed to seek to" << sbPart->size << "bytes in source file. Reason given:" << strerror(errno);
                    return Result::fail(ERR_CANNOT_SEEK, url.toString());
                }
                flags |= KIO::Resume;
            }
            qCDebug(KIO_SFTP_LOG) << "Resuming at" << sbPart->size;
        }
    }

    if (bOrigExists && !(flags & KIO::Overwrite) && !(flags & KIO::Resume)) {
        const int error = KSFTP_ISDIR(sb) ? KIO::ERR_DIR_ALREADY_EXIST : KIO::ERR_FILE_ALREADY_EXIST;
        return Result::fail(error, url.toString());
    }

    QByteArray dest;
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

    UniqueSFTPFilePtr destFile{};
    KIO::fileoffset_t totalBytesSent = 0;
    if ((flags & KIO::Resume)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to open for append: " << dest;
        destFile.reset(sftp_open(mSftp, dest.constData(), O_RDWR, 0)); // append if resuming
        if (destFile) {
            SFTPAttributesPtr fstat(sftp_fstat(destFile.get()));
            if (fstat) {
                sftp_seek64(destFile.get(), fstat->size); // Seek to end TODO
                totalBytesSent += narrow<KIO::fileoffset_t>(fstat->size);
            }
        }
    } else {
        perms initialMode = perms::owner_read | perms::owner_write | perms::group_read | perms::others_read;
        if (permissions.has_value()) {
            initialMode = permissions.value() | perms::owner_write | perms::owner_read;
        }

        qCDebug(KIO_SFTP_LOG) << "Trying to open:" << QString(dest) << ", mode=" << QString::number(permsToPosix(initialMode));
        destFile.reset(sftp_open(mSftp, dest.constData(), O_CREAT | O_TRUNC | O_WRONLY, permsToPosix(initialMode)));
    } // flags & KIO::Resume

    auto closeOnError = [&destFile, bMarkPartial, this, dest, url](int errorCode) -> Result {
        qCDebug(KIO_SFTP_LOG) << "Error during 'put'. Aborting.";

        if (destFile != nullptr) {
            destFile = nullptr; // close to force a data flush before we stat

            SFTPAttributesPtr attr(sftp_stat(mSftp, dest.constData()));
            if (bMarkPartial && attr != nullptr) {
                size_t size = configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
                if (attr->size < size) {
                    sftp_unlink(mSftp, dest.constData());
                }
            }
        }

        return errorCode == KJob::NoError ? Result::pass() : Result::fail(errorCode, url.toString());
    };

    if (destFile == nullptr) {
        qCDebug(KIO_SFTP_LOG) << "COULD NOT WRITE " << QString(dest) << ", permissions=" << permsToPosix(permissions.value_or(perms::none))
                              << ", error=" << ssh_get_error(mSession);
        if (sftp_get_error(mSftp) == SSH_FX_PERMISSION_DENIED) {
            return closeOnError(KIO::ERR_WRITE_ACCESS_DENIED);
        }
        return closeOnError(KIO::ERR_CANNOT_OPEN_FOR_WRITING);
    } // file

    auto reader = [fd, this]() -> QCoro::Generator<ReadResponse> {
        for (ssize_t result = 1; result > 0;) {
            ReadResponse response;
            if (fd == -1) {
                dataReq(); // Request for data
                result = readData(response.filedata);
                if (result < 0) {
                    qCDebug(KIO_SFTP_LOG) << "unexpected error during readData";
                }
            } else {
                std::array<char, MAX_XFER_BUF_SIZE> buf{};
                result = ::read(fd, buf.data(), buf.size());
                if (result < 0) {
                    qCDebug(KIO_SFTP_LOG) << "failed to read" << errno;
                    response.error = ERR_CANNOT_READ;
                } else {
                    response.filedata = QByteArray(buf.data(), narrow<qsizetype>(result));
                }
            }

            if (result == 0) {
                // proftpd stumbles over zero size writes.
                // https://bugs.kde.org/show_bug.cgi?id=419999
                // http://bugs.proftpd.org/show_bug.cgi?id=4398
                // At this point we'll have opened the file and thus created it.
                // It's safe to break here as even in the ideal scenario that the server
                // doesn't fall over, the write code is pointless because zero size writes
                // do absolutely nothing.
                break;
            }

            co_yield response;
        }
    };

    for (const auto &response : asyncWrite(destFile.get(), reader())) {
        if (response.error != KJob::NoError) {
            qCDebug(KIO_SFTP_LOG) << "totalBytesSent at error:" << totalBytesSent;
            return closeOnError(KIO::ERR_CANNOT_WRITE);
        }

        totalBytesSent += narrow<decltype(totalBytesSent)>(response.bytes);
        processedSize(totalBytesSent);
    }

    if (destFile == nullptr) { // we got nothing to write out, so we never opened the file
        return Result::pass();
    }

    if (sftp_close(destFile.release()) < 0) {
        qCWarning(KIO_SFTP_LOG) << "Error when closing file descriptor";
        return Result::fail(KIO::ERR_CANNOT_WRITE, url.toString());
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
            return Result::fail(ERR_CANNOT_RENAME_PARTIAL, url.toString());
        }
    }

    // set final permissions
    if (permissions.has_value() && !(flags & KIO::Resume)) {
        qCDebug(KIO_SFTP_LOG) << "Trying to set final permissions of " << dest_orig << " to " << QString::number(permsToPosix(permissions.value()));
        if (sftp_chmod(mSftp, dest_orig_c.constData(), permsToPosix(permissions.value())) < 0) {
            warning(i18n("Could not change permissions for\n%1", url.toString()));
            return Result::pass();
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
            std::array<struct timeval, 2> times{};

            SFTPAttributesPtr attr(sftp_lstat(mSftp, dest_orig_c.constData()));
            if (attr != nullptr) {
                times[0].tv_sec = attr->atime; //// access time, unchanged
                times[1].tv_sec = dt.toSecsSinceEpoch(); // modification time
                times[0].tv_usec = times[1].tv_usec = 0;

                qCDebug(KIO_SFTP_LOG) << "Trying to restore mtime for " << dest_orig << " to: " << mtimeStr;
                int result = sftp_utimes(mSftp, dest_orig_c.constData(), times.data());
                if (result < 0) {
                    qCWarning(KIO_SFTP_LOG) << "Failed to set mtime for" << dest_orig;
                }
            }
        }
    }

    return Result::pass();
}

Result SFTPWorker::copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags)
{
    qCDebug(KIO_SFTP_LOG) << src << " -> " << dest << " , permissions = " << QString::number(permissions) << ", overwrite = " << (flags & KIO::Overwrite)
                          << ", resume = " << (flags & KIO::Resume);

    const bool isSourceLocal = src.isLocalFile();
    const bool isDestinationLocal = dest.isLocalFile();

    if (!isSourceLocal && isDestinationLocal) { // sftp -> file
        return sftpCopyGet(src, dest.toLocalFile(), permissions, flags);
    }
    if (isSourceLocal && !isDestinationLocal) { // file -> sftp
        return sftpCopyPut(dest, src.toLocalFile(), permissions, flags);
    }

    return Result::fail(ERR_UNSUPPORTED_ACTION);
}

Result SFTPWorker::sftpCopyGet(const QUrl &url, const QString &sCopyFile, int permissionsMode, KIO::JobFlags flags)
{
    qCDebug(KIO_SFTP_LOG) << url << "->" << sCopyFile << ", permissions=" << permissionsMode;

    auto permissions = posixToOptionalPerms(permissionsMode);

    // check if destination is ok ...
    QFileInfo copyFile(sCopyFile);
    const bool bDestExists = copyFile.exists();
    if (bDestExists) {
        if (copyFile.isDir()) {
            return Result::fail(ERR_IS_DIRECTORY, sCopyFile);
        }

        if (!(flags & KIO::Overwrite)) {
            return Result::fail(ERR_FILE_ALREADY_EXIST, sCopyFile);
        }
    }

    bool bResume = false;
    const QString sPart = sCopyFile + QLatin1String(".part"); // do we have a ".part" file?
    QFileInfo partFile(sPart);
    const bool bPartExists = partFile.exists();
    const bool bMarkPartial = configValue(QStringLiteral("MarkPartial"), true);
    const QString dest = (bMarkPartial ? sPart : sCopyFile);

    if (bMarkPartial && bPartExists) {
        if (partFile.isDir()) {
            return Result::fail(ERR_FILE_ALREADY_EXIST, sCopyFile);
        }
        if (partFile.size() > 0) {
            bResume = canResume(copyFile.size());
        }
    }

    if (bPartExists && !bResume) { // get rid of an unwanted ".part" file
        QFile::remove(sPart);
    }

    // WABA: Make sure that we keep writing permissions ourselves,
    // otherwise we can be in for a surprise on NFS.
    perms initialMode = perms::owner_read | perms::owner_write | perms::group_read | perms::group_write | perms::others_read | perms::others_write;
    if (permissions.has_value()) {
        initialMode = permissions.value() | perms::owner_write;
    }

    // open the output file ...
    int fd = -1;
    KIO::fileoffset_t offset = 0;
    if (bResume) {
        fd = QT_OPEN(QFile::encodeName(sPart).constData(), O_RDWR | O_CLOEXEC); // append if resuming
        offset = QT_LSEEK(fd, partFile.size(), SEEK_SET);
        if (offset != partFile.size()) {
            qCDebug(KIO_SFTP_LOG) << "Failed to seek to" << partFile.size() << "bytes in target file. Reason given:" << strerror(errno);
            ::close(fd);
            return Result::fail(ERR_CANNOT_RESUME, sCopyFile);
        }
        qCDebug(KIO_SFTP_LOG) << "resuming at" << offset;
    } else {
        fd = QT_OPEN(QFile::encodeName(dest).constData(), O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, permsToPosix(initialMode));
    }

    if (fd == -1) {
        qCDebug(KIO_SFTP_LOG) << "could not write to" << sCopyFile;
        return Result::fail((errno == EACCES) ? ERR_WRITE_ACCESS_DENIED : ERR_CANNOT_OPEN_FOR_WRITING, sCopyFile);
    }

    const auto getResult = sftpGet(url, offset, fd);

    // The following is a bit awkward, we'll override the internal error
    // in cleanup conditions, so these are subject to change until we return.
    int errorCode = getResult.error();
    QString errorString = getResult.errorString();

    if (::close(fd) && getResult.success()) {
        errorCode = ERR_CANNOT_WRITE;
        errorString = sCopyFile;
    }

    // handle renaming or deletion of a partial file ...
    if (bMarkPartial) {
        if (getResult.success()) { // rename ".part" on success
            if (!QFile::rename(sPart, sCopyFile)) {
                // If rename fails, try removing the destination first if it exists.
                if (!bDestExists || !QFile::remove(sCopyFile) || !QFile::rename(sPart, sCopyFile)) {
                    qCDebug(KIO_SFTP_LOG) << "cannot rename " << sPart << " to " << sCopyFile;
                    errorCode = ERR_CANNOT_RENAME_PARTIAL;
                    errorString = sCopyFile;
                }
            }
        } else {
            partFile.refresh();
            const int size = configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
            if (partFile.exists() && partFile.size() < size) { // should a very small ".part" be deleted?
                QFile::remove(sPart);
            }
        }
    }

    const QString mtimeStr = metaData("modified");
    if (!mtimeStr.isEmpty()) {
        QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
        if (dt.isValid()) {
            QFile receivedFile(sCopyFile);
            if (receivedFile.exists()) {
                if (!receivedFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
                    QString error_msg = receivedFile.errorString();
                    qCDebug(KIO_SFTP_LOG) << "Couldn't update modified time : " << error_msg;
                } else {
                    receivedFile.setFileTime(dt, QFileDevice::FileModificationTime);
                }
            }
        }
    }

    return errorCode == KJob::NoError ? Result::pass() : Result::fail(errorCode, errorString);
}

Result SFTPWorker::sftpCopyPut(const QUrl &url, const QString &sCopyFile, int permissions, JobFlags flags)
{
    qCDebug(KIO_SFTP_LOG) << sCopyFile << "->" << url << ", permissions=" << permissions << ", flags" << flags;

    // check if source is ok ...
    QFileInfo copyFile(sCopyFile);
    bool bSrcExists = copyFile.exists();
    if (bSrcExists) {
        if (copyFile.isDir()) {
            return Result::fail(ERR_IS_DIRECTORY, sCopyFile);
        }
    } else {
        return Result::fail(ERR_DOES_NOT_EXIST, sCopyFile);
    }

    const int fd = QT_OPEN(QFile::encodeName(sCopyFile).constData(), O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        return Result::fail(ERR_CANNOT_OPEN_FOR_READING, sCopyFile);
    }

    totalSize(copyFile.size());

    // delegate the real work (errorCode gets status) ...
    const auto result = sftpPut(url, permissions, flags, fd);
    ::close(fd);
    return result;
}

Result SFTPWorker::stat(const QUrl &url)
{
    qCDebug(KIO_SFTP_LOG) << url;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    if (url.path().isEmpty() || QDir::isRelativePath(url.path()) || url.path().contains("/./") || url.path().contains("/../")) {
        QString cPath;

        if (!url.path().isEmpty()) {
            cPath = canonicalizePath(url.path());
        } else {
            cPath = canonicalizePath(QLatin1String("."));
        }

        if (cPath.isEmpty()) {
            return Result::fail(KIO::ERR_MALFORMED_URL, url.toDisplayString());
        }
        QUrl redir(url);
        redir.setPath(cPath);
        redirection(redir);

        qCDebug(KIO_SFTP_LOG) << "redirecting to " << redir.url();

        return Result::pass();
    }

    QByteArray path = url.path().toUtf8();

    const QString sDetails = metaData(QLatin1String("details"));
    const int details = sDetails.isEmpty() ? 2 : sDetails.toInt();

    sftp_attributes_struct *attributes = sftp_lstat(mSftp, path.constData());
    if (attributes == nullptr) {
        return Result::fail(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
    }

    UDSEntry entry;
    auto result = createUDSEntry(SFTPAttributesPtr(attributes), entry, path, QFileInfo(path).fileName(), details);
    if (!result.success()) {
        return result;
    }
    statEntry(entry);

    return Result::pass();
}

Result SFTPWorker::mimetype(const QUrl &url)
{
    qCDebug(KIO_SFTP_LOG) << url;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    // open() feeds the mimetype
    const auto result = open(url, QIODevice::ReadOnly);
    (void)close();

    return result;
}

Result SFTPWorker::listDir(const QUrl &url)
{
    qCDebug(KIO_SFTP_LOG) << "list directory: " << url;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    if (url.path().isEmpty() || QDir::isRelativePath(url.path()) || url.path().contains("/./") || url.path().contains("/../")) {
        QString cPath;

        if (!url.path().isEmpty()) {
            cPath = canonicalizePath(url.path());
        } else {
            cPath = canonicalizePath(QStringLiteral("."));
        }

        if (cPath.isEmpty()) {
            return Result::fail(KIO::ERR_MALFORMED_URL, url.toDisplayString());
        }
        QUrl redir(url);
        redir.setPath(cPath);
        redirection(redir);

        qCDebug(KIO_SFTP_LOG) << "redirecting to " << redir.url();

        return Result::pass();
    }

    QByteArray path = url.path().toUtf8();

    sftp_dir dp = sftp_opendir(mSftp, path.constData());
    if (dp == nullptr) {
        return reportError(url, sftp_get_error(mSftp));
    }

    const QString sDetails = metaData(QLatin1String("details"));
    const int details = sDetails.isEmpty() ? 2 : sDetails.toInt();

    qCDebug(KIO_SFTP_LOG) << "readdir: " << path << ", details: " << QString::number(details);

    UDSEntry entry; // internally this is backed by a heap'd Private, no need allocating that repeatedly
    for (;;) {
        sftp_attributes_struct *attributes = sftp_readdir(mSftp, dp);
        if (attributes == nullptr) {
            break;
        }

        const QByteArray name = QFile::decodeName(attributes->name).toUtf8();
        const QByteArray filePath = path + '/' + name;
        const Result result = createUDSEntry(SFTPAttributesPtr(attributes), entry, filePath, QString::fromUtf8(name), details);
        if (!result.success()) {
            // Failing to list one entry in a directory is not a fatal problem. Log the problem and move on.
            qCWarning(KIO_SFTP_LOG) << result.error() << result.errorString();
            continue;
        }

        listEntry(entry);
    } // for ever
    sftp_closedir(dp);
    return Result::pass();
}

Result SFTPWorker::mkdir(const QUrl &url, int permissions)
{
    qCDebug(KIO_SFTP_LOG) << "create directory: " << url;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    if (url.path().isEmpty()) {
        return Result::fail(KIO::ERR_MALFORMED_URL, url.toDisplayString());
    }
    const QString path = url.path();
    const QByteArray path_c = path.toUtf8();

    // Remove existing file or symlink, if requested.
    if (metaData(QLatin1String("overwrite")) == QLatin1String("true")) {
        qCDebug(KIO_SFTP_LOG) << "overwrite set, remove existing file or symlink: " << url;
        sftp_unlink(mSftp, path_c.constData());
    }

    qCDebug(KIO_SFTP_LOG) << "Trying to create directory: " << path;
    SFTPAttributesPtr sb(sftp_lstat(mSftp, path_c.constData()));

    // Note: Remote end, at least in case of openssh-server
    // applies umask. Probably most implementations do.
    const int defaultDirectoryPermissions = S_IRWXU | S_IRWXG | S_IRWXO;

    if (permissions == -1) {
        permissions = defaultDirectoryPermissions;
    }
    if (sb == nullptr) {
        if (sftp_mkdir(mSftp, path_c.constData(), permissions) < 0) {
            return reportError(url, sftp_get_error(mSftp));
        }

        qCDebug(KIO_SFTP_LOG) << "Successfully created directory: " << url;
        return Result::pass();
    }

    auto err = KSFTP_ISDIR(sb) ? KIO::ERR_DIR_ALREADY_EXIST : KIO::ERR_FILE_ALREADY_EXIST;
    return Result::fail(err, path);
}

Result SFTPWorker::rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags)
{
    qCDebug(KIO_SFTP_LOG) << "rename " << src << " to " << dest << flags;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    QByteArray qsrc = src.path().toUtf8();
    QByteArray qdest = dest.path().toUtf8();

    SFTPAttributesPtr sb(sftp_lstat(mSftp, qdest.constData()));
    if (sb != nullptr) {
        const bool isDir = KSFTP_ISDIR(sb);
        if (!(flags & KIO::Overwrite)) {
            return Result::fail(isDir ? KIO::ERR_DIR_ALREADY_EXIST : KIO::ERR_FILE_ALREADY_EXIST, dest.url());
        }

        // Delete the existing destination file/dir...
        if (isDir) {
            if (sftp_rmdir(mSftp, qdest.constData()) < 0) {
                return reportError(dest, sftp_get_error(mSftp));
            }
        } else {
            if (sftp_unlink(mSftp, qdest.constData()) < 0) {
                return reportError(dest, sftp_get_error(mSftp));
            }
        }
    }

    if (sftp_rename(mSftp, qsrc.constData(), qdest.constData()) < 0) {
        return reportError(dest, sftp_get_error(mSftp));
    }

    return Result::pass();
}

Result SFTPWorker::symlink(const QString &target, const QUrl &dest, KIO::JobFlags flags)
{
    qCDebug(KIO_SFTP_LOG) << "link " << target << "->" << dest << ", overwrite = " << (flags & KIO::Overwrite) << ", resume = " << (flags & KIO::Resume);

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    QByteArray t = target.toUtf8();
    QByteArray d = dest.path().toUtf8();

    bool failed = false;
    if (sftp_symlink(mSftp, t.constData(), d.constData()) < 0) {
        if (flags == KIO::Overwrite) {
            SFTPAttributesPtr sb(sftp_lstat(mSftp, d.constData()));
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
        }
    }

    if (failed) {
        return reportError(dest, sftp_get_error(mSftp));
    }

    return Result::pass();
}

Result SFTPWorker::chmod(const QUrl &url, int permissions)
{
    qCDebug(KIO_SFTP_LOG) << "change permission of " << url << " to " << QString::number(permissions);

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    QByteArray path = url.path().toUtf8();

    if (sftp_chmod(mSftp, path.constData(), permissions) < 0) {
        return reportError(url, sftp_get_error(mSftp));
    }

    return Result::pass();
}

Result SFTPWorker::del(const QUrl &url, bool isfile)
{
    qCDebug(KIO_SFTP_LOG) << "deleting " << (isfile ? "file: " : "directory: ") << url;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    QByteArray path = url.path().toUtf8();

    if (isfile) {
        if (sftp_unlink(mSftp, path.constData()) < 0) {
            return reportError(url, sftp_get_error(mSftp));
        }
    } else {
        if (sftp_rmdir(mSftp, path.constData()) < 0) {
            return reportError(url, sftp_get_error(mSftp));
        }
    }

    return Result::pass();
}

void SFTPWorker::worker_status()
{
    qCDebug(KIO_SFTP_LOG) << "connected to " << mHost << "?: " << mConnected;
    workerStatus((mConnected ? mHost : QString()), mConnected);
}

void SFTPWorker::requiresUserNameRedirection()
{
    QUrl redirectUrl;
    redirectUrl.setScheme(QLatin1String("sftp"));
    redirectUrl.setUserName(mUsername);
    redirectUrl.setPassword(mPassword);
    redirectUrl.setHost(mHost);
    if (mPort > 0 && mPort != DEFAULT_SFTP_PORT) {
        redirectUrl.setPort(mPort);
    }
    qCDebug(KIO_SFTP_LOG) << "redirecting to" << redirectUrl;
    redirection(redirectUrl);
}

Result SFTPWorker::sftpLogin()
{
    const QString origUsername = mUsername;
    if (auto openResult = openConnection(); !openResult.success()) {
        return openResult;
    }
    qCDebug(KIO_SFTP_LOG) << "connected ?" << mConnected << "username: old=" << origUsername << "new=" << mUsername;
    if (!origUsername.isEmpty() && origUsername != mUsername) {
        requiresUserNameRedirection();
        return Result::fail();
    }
    return mConnected ? Result::pass() : Result::fail();
}

void SFTPWorker::clearPubKeyAuthInfo()
{
    if (mPublicKeyAuthInfo) {
        delete mPublicKeyAuthInfo;
        mPublicKeyAuthInfo = nullptr;
    }
}

Result SFTPWorker::fileSystemFreeSpace(const QUrl &url)
{
    qCDebug(KIO_SFTP_LOG) << "file system free space of" << url;

    if (auto loginResult = sftpLogin(); !loginResult.success()) {
        return loginResult;
    }

    if (sftp_extension_supported(mSftp, "statvfs@openssh.com", "2") == 0) {
        return Result::fail(ERR_UNSUPPORTED_ACTION, QString());
    }

    const QByteArray path = url.path().isEmpty() ? QByteArrayLiteral("/") : url.path().toUtf8();

    sftp_statvfs_t statvfs = sftp_statvfs(mSftp, path.constData());
    if (statvfs == nullptr) {
        return reportError(url, sftp_get_error(mSftp));
    }

    setMetaData(QString::fromLatin1("total"), QString::number(statvfs->f_frsize * statvfs->f_blocks));
    setMetaData(QString::fromLatin1("available"), QString::number(statvfs->f_frsize * statvfs->f_bavail));

    sftp_statvfs_free(statvfs);

    return Result::pass();
}

#if defined(HAVE_SFTP_AIO)
using UniqueAIO = std::unique_ptr<struct sftp_aio_struct>;

QCoro::Generator<SFTPWorker::ReadResponse> SFTPWorker::asyncRead(sftp_file file, size_t size)
{
    const auto limit = sftp_limits(file->sftp);
    const auto freeLimit = qScopeGuard([limit] {
        sftp_limits_free(limit);
    });
    if (!limit) {
        qCWarning(KIO_SFTP_LOG) << "Failed to sftp_limits" //
                                << "- SFTP error:" << sftp_get_error(file->sftp) //
                                << "- SSH error:" << ssh_get_error_code(file->sftp->session) //
                                << "- SSH errorString:" << ssh_get_error(file->sftp->session);
        co_yield ReadResponse(KIO::ERR_CANNOT_READ);
        co_return;
    }

    const auto chunkSize = std::min<ssize_t>(narrow<ssize_t>(limit->max_read_length), MAX_XFER_BUF_SIZE);

    size_t queuedBytes = 0;
    std::queue<UniqueAIO> pendingRequests;

    auto queueChunkMaybe = [&pendingRequests, &queuedBytes, chunkSize, size, file]() -> int {
        if (queuedBytes >= size) {
            return KJob::NoError;
        }

        const auto requestLength = std::min<size_t>(chunkSize, size - queuedBytes);
        sftp_aio aio = nullptr;
        if (sftp_aio_begin_read(file, requestLength, &aio) == SSH_ERROR) {
            qCWarning(KIO_SFTP_LOG) << "Failed to sftp_aio_begin_read" //
                                    << "- SFTP error:" << sftp_get_error(file->sftp) //
                                    << "- SSH error:" << ssh_get_error_code(file->sftp->session) //
                                    << "- SSH errorString:" << ssh_get_error(file->sftp->session);
            return KIO::ERR_CANNOT_READ;
        }

        pendingRequests.emplace(aio);
        queuedBytes += requestLength;
        return KJob::NoError;
    };

    // Queue a bunch of requests
    while (pendingRequests.size() < MAX_PENDING_REQUESTS && queuedBytes < size) {
        if (auto error = queueChunkMaybe(); error != KJob::NoError) {
            // Cleanup of pending requests happens through queue destruction
            co_yield ReadResponse(error);
            co_return;
        }
    }

    // pop-read-queue_new-yield loop until all requests are processed
    size_t receivedBytes = 0;
    std::array<char, MAX_XFER_BUF_SIZE> buffer{};
    std::span bufferSpan{buffer};
    while (!pendingRequests.empty()) {
        auto aio = pendingRequests.front().release();
        pendingRequests.pop();
        if (auto error = queueChunkMaybe(); error != KJob::NoError) {
            // Cleanup of pending requests happens through queue destruction
            co_yield ReadResponse(error);
            co_return;
        }

        ssize_t readBytes = 0;
        while (true) {
            auto readSpan = bufferSpan.subspan(readBytes);
            readBytes = sftp_aio_wait_read(&aio, readSpan.data(), readSpan.size());
            if (readBytes == SSH_AGAIN) {
                continue;
            }
            if (readBytes == SSH_ERROR) {
                qCWarning(KIO_SFTP_TRACE_LOG) << "read SSH_ERROR";
                co_yield ReadResponse(KIO::ERR_CANNOT_READ);
                co_return;
            }

            receivedBytes += readBytes;
            if (readBytes != chunkSize && receivedBytes != size) { // short read
                qCWarning(KIO_SFTP_TRACE_LOG) << "unexpected short read. the file probably was truncated";
                co_yield ReadResponse(KIO::ERR_CANNOT_READ);
                co_return;
            }

            co_yield ReadResponse(QByteArray(buffer.data(), readBytes));
            break;
        }
    }
}

QCoro::Generator<SFTPWorker::WriteResponse> SFTPWorker::asyncWrite(sftp_file file, QCoro::Generator<ReadResponse> reader)
{
    const auto limit = sftp_limits(file->sftp);
    const auto freeLimit = qScopeGuard([limit] {
        sftp_limits_free(limit);
    });
    if (!limit) {
        qCWarning(KIO_SFTP_LOG) << "Failed to sftp_limits" //
                                << "- SFTP error:" << sftp_get_error(file->sftp) //
                                << "- SSH error:" << ssh_get_error_code(file->sftp->session) //
                                << "- SSH errorString:" << ssh_get_error(file->sftp->session);
        co_yield {.error = KIO::ERR_CANNOT_READ};
        co_return;
    }

    std::queue<UniqueAIO> pendingRequests;

    auto readIt = reader.begin();
    auto readEnd = reader.end();
    auto queueChunkMaybe = [file, &pendingRequests, &readIt, &readEnd, limit]() -> int {
        if (readIt == readEnd) {
            return KJob::NoError;
        }

        const auto &readResponse = *readIt;
        if (readResponse.error != KJob::NoError) {
            return readResponse.error;
        }

        const auto requestLength = std::min<size_t>(narrow<size_t>(limit->max_write_length), readResponse.filedata.size());
        sftp_aio aio = nullptr;

        std::span bufferSpan{readResponse.filedata};
        while (!bufferSpan.empty()) {
            const auto writeSpan = bufferSpan.first(std::min(requestLength, bufferSpan.size()));
            if (sftp_aio_begin_write(file, writeSpan.data(), writeSpan.size(), &aio) == SSH_ERROR) {
                qCWarning(KIO_SFTP_LOG) << "Failed to sftp_aio_begin_write" //
                                        << "- SFTP error:" << sftp_get_error(file->sftp) //
                                        << "- SSH error:" << ssh_get_error_code(file->sftp->session) //
                                        << "- SSH errorString:" << ssh_get_error(file->sftp->session);
                return KIO::ERR_CANNOT_READ;
            }

            pendingRequests.emplace(aio);
            bufferSpan = bufferSpan.subspan(writeSpan.size());
        }

        ++readIt;
        return KJob::NoError;
    };

    // Queue a bunch of requests
    while (pendingRequests.size() < MAX_PENDING_REQUESTS && readIt != reader.end()) {
        if (auto error = queueChunkMaybe(); error != KJob::NoError) {
            // Cleanup of pending requests happens through queue destruction
            co_yield {.error = error};
            co_return;
        }
    }

    while (!pendingRequests.empty()) {
        auto aio = pendingRequests.front().release();
        pendingRequests.pop();
        if (auto error = queueChunkMaybe(); error != KJob::NoError) {
            // Cleanup of pending requests happens through queue destruction
            co_yield {.error = error};
            co_return;
        }

        ssize_t writtenBytes = 0;
        while (true) {
            writtenBytes = sftp_aio_wait_write(&aio);
            if (writtenBytes == SSH_AGAIN) {
                continue;
            }
            if (writtenBytes == SSH_ERROR) {
                qCWarning(KIO_SFTP_TRACE_LOG) << "write SSH_ERROR";
                co_yield {.error = KIO::ERR_CANNOT_WRITE};
                co_return;
            }

            co_yield {.bytes = std::make_unsigned_t<size_t>(writtenBytes)};
            break;
        }
    }
}

#else

SFTPWorker::GetRequest::GetRequest(sftp_file file, uint64_t size, ushort maxPendingRequests)
    : m_file(file)
    , m_size(size)
    , m_maxPendingRequests(maxPendingRequests)
{
}

bool SFTPWorker::GetRequest::enqueueChunks()
{
    SFTPWorker::GetRequest::Request request;

    qCDebug(KIO_SFTP_TRACE_LOG) << "enqueueChunks";

    while (m_pendingRequests.count() < m_maxPendingRequests) {
        request.expectedLength = MAX_XFER_BUF_SIZE;
        request.startOffset = m_file->offset;
        request.id = sftp_async_read_begin(m_file, request.expectedLength);
        if (request.id < 0) {
            if (m_pendingRequests.isEmpty()) {
                return false;
            }
            break;
        }

        m_pendingRequests.enqueue(request);

        if (m_file->offset >= m_size) {
            // Do not add any more chunks if the offset is larger than the given file size.
            // However this is done after adding a request as the remote file size may
            // have changed in the meantime.
            break;
        }
    }

    qCDebug(KIO_SFTP_TRACE_LOG) << "enqueueChunks done" << QString::number(m_pendingRequests.size());

    return true;
}

size_t SFTPWorker::GetRequest::readChunks(QByteArray &data)
{
    size_t totalRead = 0;
    ssize_t bytesread = 0;
    const uint64_t initialOffset = m_file->offset;

    while (!m_pendingRequests.isEmpty()) {
        SFTPWorker::GetRequest::Request &request = m_pendingRequests.head();
        auto dataSize = data.size() + request.expectedLength;

        data.resize(dataSize);
        if (data.size() < dataSize) {
            // Could not allocate enough memory - skip current chunk
            data.resize(dataSize - request.expectedLength);
            break;
        }

        bytesread = sftp_async_read(m_file, data.data() + totalRead, request.expectedLength, request.id);

        // qCDebug(KIO_SFTP_LOG) << "bytesread=" << QString::number(bytesread);

        if (bytesread == 0 || bytesread == SSH_AGAIN) {
            // Done reading or timeout
            data.resize(data.size() - request.expectedLength);

            if (bytesread == 0) {
                m_pendingRequests.dequeue(); // This frees QByteArray &data!
            }

            break;
        }
        if (bytesread == SSH_ERROR) {
            return -1;
        }

        totalRead += bytesread;

        if (bytesread < request.expectedLength) {
            int rc = -1;

            // If less data is read than expected - requeue the request
            data.resize(data.size() - (request.expectedLength - bytesread));

            // Modify current request
            request.expectedLength -= bytesread;
            request.startOffset += bytesread;

            rc = sftp_seek64(m_file, request.startOffset);
            if (rc < 0) {
                // Failed to continue reading
                return -1;
            }

            request.id = sftp_async_read_begin(m_file, request.expectedLength);

            if (request.id < 0) {
                // Failed to dispatch re-request
                return -1;
            }

            // Move the offset back to where it was before the read.
            // The way this works is that originally the offset is at the maximum of all pending requests,
            // read then reduces that by the amount that it came up short, we then seek to where the short request
            // left off and make another request for the remaining data. After that we need to move the offset
            // back to the original value - without the reduction because we re-requested the missing data!
            rc = sftp_seek64(m_file, initialOffset);
            if (rc < 0) {
                // Failed to continue reading
                return -1;
            }

            return totalRead;
        }

        m_pendingRequests.dequeue();
    }

    return totalRead;
}

SFTPWorker::GetRequest::~GetRequest()
{
    SFTPWorker::GetRequest::Request request{};
    std::array<char, MAX_XFER_BUF_SIZE> buf{};

    // Remove pending reads to avoid memory leaks
    while (!m_pendingRequests.isEmpty()) {
        request = m_pendingRequests.dequeue();
        sftp_async_read(m_file, buf.data(), request.expectedLength, request.id);
    }
}

QCoro::Generator<SFTPWorker::ReadResponse> SFTPWorker::asyncRead(sftp_file file, size_t size)
{
    SFTPWorker::GetRequest request(file, size);

    while (true) {
        // Enqueue get requests
        if (!request.enqueueChunks()) {
            co_yield SFTPWorker::ReadResponse(KIO::ERR_CANNOT_READ);
            break;
        }

        QByteArray filedata;
        const auto bytesread = request.readChunks(filedata);
        // Read pending get requests
        if (bytesread == -1) {
            co_yield SFTPWorker::ReadResponse(KIO::ERR_CANNOT_READ);
            break;
        }
        if (bytesread == 0) {
            if (file->eof) {
                break;
            }
            continue;
        }

        co_yield SFTPWorker::ReadResponse(filedata);
    }
}

QCoro::Generator<SFTPWorker::WriteResponse> SFTPWorker::asyncWrite(sftp_file file, QCoro::Generator<ReadResponse> reader)
{
    // TODO: enqueue requests.
    // Similarly to reading we may enqueue a number of requests to mitigate the
    // network overhead and speed up things. Serial iteration gives super poor
    // performance.

    for (const auto &response : reader) {
        if (response.error) {
            co_yield {.error = response.error};
            break;
        }

        // Break up into multiple requests in case a single request would be too large.
        // Servers can impose an arbitrary size limit that we don't want to hit.
        // https://bugs.kde.org/show_bug.cgi?id=404890
        off_t offset = 0;
        while (offset < response.filedata.size()) {
            const auto length = qMin<int>(MAX_XFER_BUF_SIZE, response.filedata.size() - offset);
            auto bytesWritten = sftp_write(file, response.filedata.data() + offset, length);
            if (bytesWritten < 0) {
                qCDebug(KIO_SFTP_LOG) << "Failed to sftp_write" << length << "bytes."
                                      << "- Already written (for this call):" << offset << "- Return of sftp_write:" << bytesWritten
                                      << "- SFTP error:" << sftp_get_error(mSftp) << "- SSH error:" << ssh_get_error_code(mSession)
                                      << "- SSH errorString:" << ssh_get_error(mSession);
                co_yield {.error = KIO::ERR_CANNOT_WRITE};
                break;
            }
            co_yield {.bytes = std::make_unsigned_t<size_t>(bytesWritten)};
            offset += bytesWritten;
        }
    }
}
#endif // HAVE_SFTP_AIO

#include "kio_sftp.moc"
