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

#include <cerrno>
#include <cstring>
#include <unistd.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVarLengthArray>

#include <kapplication.h>
#include <kuser.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <kmimetype.h>
#include <kde_file.h>
#include <kconfiggroup.h>
#include <kio/ioslave_defaults.h>


#define KIO_SFTP_SPECIAL_TIMEOUT 30
#define ZERO_STRUCTP(x) do { if ((x) != NULL) memset((char *)(x), 0, sizeof(*(x))); } while(0)

// How big should each data packet be? Definitely not bigger than 64kb or
// you will overflow the 2 byte size variable in a sftp packet.
#define MAX_XFER_BUF_SIZE (60 * 1024)
#define KIO_SFTP_DB 7120
// Maximum amount of data which can be sent from the KIOSlave in one chunk
// see TransferJob::slotDataReq (max_size variable) for the value
#define MAX_TRANSFER_SIZE (14 * 1024 * 1024)

using namespace KIO;
extern "C"
{
  int KDE_EXPORT kdemain( int argc, char **argv )
  {
    QCoreApplication app(argc, argv);
    KComponentData componentData( "kio_sftp" );
    (void) KGlobal::locale();

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

// Converts SSH error into KIO error
static int toKIOError (const int err)
{
  switch (err) {
    case SSH_FX_OK:
      break;
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
  return 0;
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
    while ((offset=KDE_lseek(fd, pos, mode)) == EAGAIN);
    return offset;
}

// The callback function for libssh
int auth_callback(const char *prompt, char *buf, size_t len,
    int echo, int verify, void *userdata) {
  if (userdata == NULL) {
    return -1;
  }

  sftpProtocol *slave = (sftpProtocol *) userdata;

  if (slave->auth_callback(prompt, buf, len, echo, verify, userdata) < 0) {
    return -1;
  }

  return 0;
}

void log_callback(ssh_session session, int priority, const char *message,
    void *userdata) {
  if (userdata == NULL) {
    return;
  }

  sftpProtocol *slave = (sftpProtocol *) userdata;

  slave->log_callback(session, priority, message, userdata);
}

int sftpProtocol::auth_callback(const char *prompt, char *buf, size_t len,
    int echo, int verify, void *userdata) {
  QString i_prompt = QString::fromUtf8(prompt);

  // unused variables
  (void) echo;
  (void) verify;
  (void) userdata;

  kDebug(KIO_SFTP_DB) << "Entering authentication callback, prompt=" << i_prompt;

  KIO::AuthInfo info;

  info.url.setProtocol("sftp");
  info.url.setHost(mHost);
  if (mPort > 0 && mPort != DEFAULT_SFTP_PORT) {
      info.url.setPort(mPort);
  }
  info.url.setUser(mUsername);

  info.comment = "sftp://" + mUsername + "@"  + mHost;
  info.username = i_prompt;
  info.readOnly = true;
  info.prompt = i_prompt;
  info.keepPassword = false; // don't save passwords for public key,
                             // that's the task of ssh-agent.

  if (!openPasswordDialog(info)) {
    kDebug(KIO_SFTP_DB) << "Password dialog failed";
    return -1;
  }

  strncpy(buf, info.password.toUtf8().constData(), len - 1);

  info.password.fill('x');

  return 0;
}

void sftpProtocol::log_callback(ssh_session session, int priority,
    const char *message, void *userdata) {
  (void) session;
  (void) userdata;

  kDebug(KIO_SFTP_DB) << "[" << priority << "] " << message;
}

int sftpProtocol::authenticateKeyboardInteractive(AuthInfo &info) {
  QString name, instruction, prompt;
  int err = SSH_AUTH_ERROR;

  kDebug(KIO_SFTP_DB) << "Entering keyboard interactive function";

  err = ssh_userauth_kbdint(mSession, NULL, NULL);
  while (err == SSH_AUTH_INFO) {
    int n = 0;
    int i = 0;

    name = QString::fromUtf8(ssh_userauth_kbdint_getname(mSession));
    instruction = QString::fromUtf8(ssh_userauth_kbdint_getinstruction(mSession));
    n = ssh_userauth_kbdint_getnprompts(mSession);

    kDebug(KIO_SFTP_DB) << "name=" << name << " instruction=" << instruction
      << " prompts" << n;

    for (i = 0; i < n; ++i) {
      char echo;
      const char *answer = "";

      prompt = QString::fromUtf8(ssh_userauth_kbdint_getprompt(mSession, i, &echo));
      kDebug(KIO_SFTP_DB) << "prompt=" << prompt << " echo=" << QString::number(echo);
      if (echo) {
        // See RFC4256 Section 3.3 User Interface
        QString newPrompt;
        KIO::AuthInfo infoKbdInt;

        infoKbdInt.url.setProtocol("sftp");
        infoKbdInt.url.setHost(mHost);
        if (mPort > 0 && mPort != DEFAULT_SFTP_PORT) {
            infoKbdInt.url.setPort(mPort);
        }

        infoKbdInt.caption = i18n("SFTP Login");
        infoKbdInt.comment = "sftp://" + mUsername + "@"  + mHost;

        if (!name.isEmpty()) {
          infoKbdInt.caption = QString(i18n("SFTP Login") + " - " + name);
        }

        if (!instruction.isEmpty()) {
          newPrompt = instruction + "<br /><br />";
        }
        newPrompt.append(prompt);
        infoKbdInt.prompt = newPrompt;

        infoKbdInt.readOnly = false;
        infoKbdInt.keepPassword = false;

        if (openPasswordDialog(infoKbdInt, i18n("Use the username input field to answer this question."))) {
          kDebug(KIO_SFTP_DB) << "Got the answer from the password dialog";
          answer = info.username.toUtf8().constData();
        }

        if (ssh_userauth_kbdint_setanswer(mSession, i, answer) < 0) {
          kDebug(KIO_SFTP_DB) << "An error occurred setting the answer: "
            << ssh_get_error(mSession);
          return SSH_AUTH_ERROR;
        }
        break;
      } else {
        if (prompt.contains("Password", Qt::CaseInsensitive)) {
          answer = mPassword.toUtf8().constData();
        } else {
          info.readOnly = true; // set username readonly
          info.prompt = prompt;

          if (openPasswordDialog(info)) {
            kDebug(KIO_SFTP_DB) << "Got the answer from the password dialog";
            answer = info.password.toUtf8().constData();
          }
        }

        if (ssh_userauth_kbdint_setanswer(mSession, i, answer) < 0) {
          kDebug(KIO_SFTP_DB) << "An error occurred setting the answer: "
            << ssh_get_error(mSession);
          return SSH_AUTH_ERROR;
        }
      }
    }
    err = ssh_userauth_kbdint(mSession, NULL, NULL);
  }

  return err;
}

void sftpProtocol::reportError(const KUrl &url, const int err) {
  kDebug(KIO_SFTP_DB) << "url = " << url << " - err=" << err;

  const int kioError = toKIOError(err);
  if (kioError)
      error(kioError, url.prettyUrl());
}

bool sftpProtocol::createUDSEntry(const QString &filename, const QByteArray &path,
      UDSEntry &entry, short int details) {
  mode_t type;
  mode_t access;
  char *link;

  Q_ASSERT(entry.count() == 0);

  sftp_attributes sb = sftp_lstat(mSftp, path.constData());
  if (sb == NULL) {
    return false;
  }

  entry.insert(KIO::UDSEntry::UDS_NAME, filename);

  if (sb->type == SSH_FILEXFER_TYPE_SYMLINK) {
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    link = sftp_readlink(mSftp, path.constData());
    if (link == NULL) {
      sftp_attributes_free(sb);
      return false;
    }
    entry.insert(KIO::UDSEntry::UDS_LINK_DEST, QFile::decodeName(link));
    delete link;
    // A symlink -> follow it only if details > 1
    if (details > 1) {
      sftp_attributes sb2 = sftp_stat(mSftp, path.constData());
      if (sb2 == NULL) {
        // It is a link pointing to nowhere
        type = S_IFMT - 1;
        access = S_IRWXU | S_IRWXG | S_IRWXO;
        entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, type);
        entry.insert( KIO::UDSEntry::UDS_ACCESS, access);
        entry.insert( KIO::UDSEntry::UDS_SIZE, 0LL );

        goto notype;
      }
      sftp_attributes_free(sb);
      sb = sb2;
    }
  }

  switch (sb->type) {
    case SSH_FILEXFER_TYPE_REGULAR:
      entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
      break;
    case SSH_FILEXFER_TYPE_DIRECTORY:
      entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
      break;
    case SSH_FILEXFER_TYPE_SYMLINK:
      entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFLNK);
      break;
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
      entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFMT - 1);
      break;
  }

  access = sb->permissions & 07777;
  entry.insert(KIO::UDSEntry::UDS_ACCESS, access);

  entry.insert(KIO::UDSEntry::UDS_SIZE, sb->size);

notype:
  if (details > 0) {
    if (sb->owner) {
      entry.insert(KIO::UDSEntry::UDS_USER, QString::fromUtf8(sb->owner));
    } else {
      entry.insert(KIO::UDSEntry::UDS_USER, QString::number(sb->uid));
    }

    if (sb->group) {
      entry.insert(KIO::UDSEntry::UDS_GROUP, QString::fromUtf8(sb->group));
    } else {
      entry.insert(KIO::UDSEntry::UDS_GROUP, QString::number(sb->gid));
    }

    entry.insert(KIO::UDSEntry::UDS_ACCESS_TIME, sb->atime);
    entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, sb->mtime);
    entry.insert(KIO::UDSEntry::UDS_CREATION_TIME, sb->createtime);
  }

  sftp_attributes_free(sb);

  return true;
}

QString sftpProtocol::canonicalizePath(const QString &path) {
  kDebug(KIO_SFTP_DB) << "Path to canonicalize: " << path;
  QString cPath;
  char *sPath = NULL;

  if (path.isEmpty()) {
    return cPath;
  }

  sPath = sftp_canonicalize_path(mSftp, path.toUtf8().constData());
  if (sPath == NULL) {
    kDebug(KIO_SFTP_DB) << "Could not canonicalize path: " << path;
    return cPath;
  }

  cPath = QFile::decodeName(sPath);
  delete sPath;

  kDebug(KIO_SFTP_DB) << "Canonicalized path: " << cPath;

  return cPath;
}

sftpProtocol::sftpProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
             : SlaveBase("kio_sftp", pool_socket, app_socket),
                  mConnected(false), mPort(-1), mSession(NULL), mSftp(NULL) {
#ifndef Q_WS_WIN
  kDebug(KIO_SFTP_DB) << "pid = " << getpid();

  kDebug(KIO_SFTP_DB) << "debug = " << getenv("KIO_SFTP_LOG_VERBOSITY");
#endif

  mCallbacks = (ssh_callbacks) malloc(sizeof(struct ssh_callbacks_struct));
  if (mCallbacks == NULL) {
    error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not allocate callbacks"));
    return;
  }
  ZERO_STRUCTP(mCallbacks);

  mCallbacks->userdata = this;
  mCallbacks->auth_function = ::auth_callback;
  if (getenv("KIO_SFTP_LOG_VERBOSITY")) {
    mCallbacks->log_function = ::log_callback;
  }

  ssh_callbacks_init(mCallbacks);
}

sftpProtocol::~sftpProtocol() {
#ifndef Q_WS_WIN
  kDebug(KIO_SFTP_DB) << "pid = " << getpid();
#endif
  closeConnection();

  delete mCallbacks;

  /* cleanup and shut down cryto stuff */
  ssh_finalize();
}

void sftpProtocol::setHost(const QString& host, quint16 port, const QString& user, const QString& pass) {
  kDebug(KIO_SFTP_DB) << user << "@" << host << ":" << port;

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

void sftpProtocol::openConnection() {

  if (mConnected) {
    return;
  }

  kDebug(KIO_SFTP_DB) << "username=" << mUsername << ", host=" << mHost << ", port=" << mPort;

  infoMessage(i18n("Opening SFTP connection to host %1:%2", mHost, mPort));

  if (mHost.isEmpty()) {
    kDebug(KIO_SFTP_DB) << "openConnection(): Need hostname...";
    error(KIO::ERR_UNKNOWN_HOST, i18n("No hostname specified."));
    return;
  }

  AuthInfo info;
  info.url.setProtocol("sftp");
  info.url.setHost(mHost);
  if ( mPort > 0 && mPort != DEFAULT_SFTP_PORT ) {
      info.url.setPort(mPort);
  }
  info.url.setUser(mUsername);
  info.username = mUsername;

  // Check for cached authentication info if no password is specified...
  if (mPassword.isEmpty()) {
    kDebug(KIO_SFTP_DB) << "checking cache: info.username =" << info.username
                        << ", info.url =" << info.url.prettyUrl();
    checkCachedAuthentication(info);
  } else {
      info.password = mPassword;
  }

  // Start the ssh connection.
  QString msg;     // msg for dialog box
  QString caption; // dialog box caption
  unsigned char *hash = NULL; // the server hash
  char *hexa;
  char *verbosity;
  int rc, state, hlen;
  int timeout_sec = 30, timeout_usec = 0;

login_start:
  mSession = ssh_new();
  if (mSession == NULL) {
    error(KIO::ERR_INTERNAL, i18n("Could not create a new SSH session."));
    return;
  }

  kDebug(KIO_SFTP_DB) << "Creating the SSH session and setting options";

  // Set timeout
  rc = ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT, &timeout_sec);
  if (rc < 0) {
    error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not set a timeout."));
    return;
  }
  rc = ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT_USEC, &timeout_usec);
  if (rc < 0) {
    error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not set a timeout."));
    return;
  }

  // Don't use any compression
  rc = ssh_options_set(mSession, SSH_OPTIONS_COMPRESSION_C_S, "none");
  if (rc < 0) {
    error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not set compression."));
    return;
  }

  rc = ssh_options_set(mSession, SSH_OPTIONS_COMPRESSION_S_C, "none");
  if (rc < 0) {
    error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not set compression."));
    return;
  }

  // Set host and port
  rc = ssh_options_set(mSession, SSH_OPTIONS_HOST, mHost.toUtf8().constData());
  if (rc < 0) {
    error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not set host."));
    return;
  }

  if (mPort > 0) {
    rc = ssh_options_set(mSession, SSH_OPTIONS_PORT, &mPort);
    if (rc < 0) {
        error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not set port."));
      return;
    }
  }

  // Set the username
  if (!info.username.isEmpty()) {
    rc = ssh_options_set(mSession, SSH_OPTIONS_USER, info.username.toUtf8().constData());
    if (rc < 0) {
      error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not set username."));
      return;
    }
  }

  verbosity = getenv("KIO_SFTP_LOG_VERBOSITY");
  if (verbosity) {
    rc = ssh_options_set(mSession, SSH_OPTIONS_LOG_VERBOSITY_STR, verbosity);
    if (rc < 0) {
      error(KIO::ERR_OUT_OF_MEMORY, i18n("Could not set log verbosity."));
      return;
    }
  }

  // Read ~/.ssh/config
  rc = ssh_options_parse_config(mSession, NULL);
  if (rc < 0) {
    error(KIO::ERR_INTERNAL, i18n("Could not parse the config file."));
    return;
  }

  ssh_set_callbacks(mSession, mCallbacks);

  kDebug(KIO_SFTP_DB) << "Trying to connect to the SSH server";

  /* try to connect */
  rc = ssh_connect(mSession);
  if (rc < 0) {
    error(KIO::ERR_COULD_NOT_CONNECT, QString::fromUtf8(ssh_get_error(mSession)));
    closeConnection();
    return;
  }

  kDebug(KIO_SFTP_DB) << "Getting the SSH server hash";

  /* get the hash */
  hlen = ssh_get_pubkey_hash(mSession, &hash);
  if (hlen < 0) {
    error(KIO::ERR_COULD_NOT_CONNECT, QString::fromUtf8(ssh_get_error(mSession)));
    closeConnection();
    return;
  }

  kDebug(KIO_SFTP_DB) << "Checking if the SSH server is known";

  /* check the server public key hash */
  state = ssh_is_server_known(mSession);
  switch (state) {
    case SSH_SERVER_KNOWN_OK:
      break;
    case SSH_SERVER_FOUND_OTHER:
      delete hash;
      error(KIO::ERR_CONNECTION_BROKEN, i18n("The host key for this server was "
            "not found, but another type of key exists.\n"
            "An attacker might change the default server key to confuse your "
            "client into thinking the key does not exist.\n"
            "Please contact your system administrator.\n%1", QString::fromUtf8(ssh_get_error(mSession))));
      closeConnection();
      return;
    case SSH_SERVER_KNOWN_CHANGED:
      hexa = ssh_get_hexa(hash, hlen);
      delete hash;
      /* TODO print known_hosts file, port? */
      error(KIO::ERR_CONNECTION_BROKEN, i18n("The host key for the server %1 has changed.\n"
          "This could either mean that DNS SPOOFING is happening or the IP "
          "address for the host and its host key have changed at the same time.\n"
          "The fingerprint for the key sent by the remote host is:\n %2\n"
          "Please contact your system administrator.\n%3",
          mHost, QString::fromUtf8(hexa), QString::fromUtf8(ssh_get_error(mSession))));
      delete hexa;
      closeConnection();
      return;
    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN:
      hexa = ssh_get_hexa(hash, hlen);
      delete hash;
      caption = i18n("Warning: Cannot verify host's identity.");
      msg = i18n("The authenticity of host %1 cannot be established.\n"
        "The key fingerprint is: %2\n"
        "Are you sure you want to continue connecting?", mHost, hexa);
      delete hexa;

      if (KMessageBox::Yes != messageBox(WarningYesNo, msg, caption)) {
        closeConnection();
        error(KIO::ERR_USER_CANCELED, QString());
        return;
      }

      /* write the known_hosts file */
      kDebug(KIO_SFTP_DB) << "Adding server to known_hosts file.";
      if (ssh_write_knownhost(mSession) < 0) {
        error(KIO::ERR_USER_CANCELED, QString::fromUtf8(ssh_get_error(mSession)));
        closeConnection();
        return;
      }
      break;
    case SSH_SERVER_ERROR:
      delete hash;
      error(KIO::ERR_COULD_NOT_CONNECT, QString::fromUtf8(ssh_get_error(mSession)));
      return;
  }

  kDebug(KIO_SFTP_DB) << "Trying to authenticate with the server";

  // Try to authenticate
  rc = ssh_userauth_none(mSession, NULL);
  if (rc == SSH_AUTH_ERROR) {
    closeConnection();
    error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
    return;
  }

  int method = ssh_auth_list(mSession);
  bool firstTime = true;
  bool dlgResult;

  info.caption = i18n("SFTP Login");
  info.prompt = i18n("Please enter your username and password.");
  info.comment = QLatin1String("sftp://") + mHost + QLatin1Char(':') + QString::number(mPort);
  info.commentLabel = i18n("site:");

  while (rc != SSH_AUTH_SUCCESS) {
    // Try to authenticate with public key first
    if (method & SSH_AUTH_METHOD_PUBLICKEY) {
      kDebug(KIO_SFTP_DB) << "Trying to authenticate public key";
      rc = ssh_userauth_autopubkey(mSession, NULL);
      if (rc == SSH_AUTH_ERROR) {
        kDebug(KIO_SFTP_DB) << "Public key authentication failed: " <<
               QString::fromUtf8(ssh_get_error(mSession));
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
        return;
      } else if (rc == SSH_AUTH_SUCCESS) {
        break;
      }
    }

    if (!firstTime || info.password.isEmpty()) {

      info.keepPassword = true; // make the "keep Password" check box visible to the user.
      info.setModified(false);

      QString username (info.username);

      if (firstTime) {
        dlgResult = openPasswordDialog(info);
      } else {
        kDebug(KIO_SFTP_DB) << "First attempt failed! Re-prompting user";
        dlgResult = openPasswordDialog(info, i18n("Incorrect username or password"));
      }

      // Handle user canceled or dialog failed to open...
      if (!dlgResult) {
        kDebug(KIO_SFTP_DB) << "User canceled, dlgResult = " << dlgResult;
        closeConnection();
        error(KIO::ERR_USER_CANCELED, QString());
        return;
      }

      firstTime = false;

      if (info.isModified() && !username.isEmpty() && username != info.username) {
        kDebug(KIO_SFTP_DB) << "Username changed from" << username << "to" << info.username;
        info.url.setUser(info.username);
        closeConnection();
        goto login_start;
      }
    }

    // Try to authenticate with keyboard interactive
    if (method & SSH_AUTH_METHOD_INTERACTIVE) {
      kDebug(KIO_SFTP_DB) << "Trying to authenticate with keyboard interactive";
      AuthInfo info2 (info);
      rc = authenticateKeyboardInteractive(info2);
      if (rc == SSH_AUTH_ERROR) {
        kDebug(KIO_SFTP_DB) << "Keyboard interactive authentication failed: " <<
               QString::fromUtf8(ssh_get_error(mSession));
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
        return;
      } else if (rc == SSH_AUTH_SUCCESS) {
        info = info2;
        break;
      }
    }

    // Try to authenticate with password
    if (method & SSH_AUTH_METHOD_PASSWORD) {
      kDebug(KIO_SFTP_DB) << "Trying to authenticate with password";
      rc = ssh_userauth_password(mSession, info.username.toUtf8().constData(), info.password.toUtf8().constData());
      if (rc == SSH_AUTH_ERROR) {
        kDebug(KIO_SFTP_DB) << "Password authentication failed: " <<
               QString::fromUtf8(ssh_get_error(mSession));
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
        return;
      } else if (rc == SSH_AUTH_SUCCESS) {
        break;
      }
    }
  }

  // start sftp session
  kDebug(KIO_SFTP_DB) << "Trying to request the sftp session";
  mSftp = sftp_new(mSession);
  if (mSftp == NULL) {
    closeConnection();
    error(KIO::ERR_COULD_NOT_LOGIN, i18n("Unable to request the SFTP subsystem. "
          "Make sure SFTP is enabled on the server."));
    return;
  }

  kDebug(KIO_SFTP_DB) << "Trying to initialize the sftp session";
  if (sftp_init(mSftp) < 0) {
    closeConnection();
    error(KIO::ERR_COULD_NOT_LOGIN, i18n("Could not initialize the SFTP session."));
    return;
  }

  // Login succeeded!
  infoMessage(i18n("Successfully connected to %1", mHost));
  if (info.keepPassword) {
    kDebug(KIO_SFTP_DB) << "Caching info.username = " << info.username
                        << ", info.url = " << info.url.prettyUrl();
    cacheAuthentication(info);
    mUsername = info.username;
  }

  setTimeoutSpecialCommand(KIO_SFTP_SPECIAL_TIMEOUT);

  mConnected = true;
  connected();

  info.password.fill('x');
  info.password.clear();
}

void sftpProtocol::closeConnection() {
  kDebug(KIO_SFTP_DB);

  if (mSftp) {
    sftp_free(mSftp);
    mSftp = NULL;
  }

  if (mSession) {
    ssh_disconnect(mSession);
    mSession = NULL;
  }

  mConnected = false;
}

void sftpProtocol::special(const QByteArray &) {
    int rc;
    kDebug(KIO_SFTP_DB) << "special(): polling";

    if (!mSftp)
        return;

    /*
     * channel_poll() returns the number of bytes that may be read on the
     * channel. It does so by checking the input buffer and eventually the
     * network socket for data to read. If the input buffer is not empty, it
     * will not probe the network (and such not read packets nor reply to
     * keepalives).
     *
     * As channel_poll can act on two specific buffers (a channel has two
     * different stream: stdio and stderr), polling for data on the stderr
     * stream has more chance of not being in the problematic case (data left
     * in the buffer). Checking the return value (for >0) would be a good idea
     * to debug the problem.
     */
    rc = channel_poll(mSftp->channel, 0);
    if (rc > 0) {
        rc = channel_poll(mSftp->channel, 1);
    }

    if (rc < 0) {
        kDebug(KIO_SFTP_DB) << "channel_poll failed: " << ssh_get_error(mSession);
    }

    setTimeoutSpecialCommand(KIO_SFTP_SPECIAL_TIMEOUT);
}

void sftpProtocol::open(const KUrl &url, QIODevice::OpenMode mode) {
  kDebug(KIO_SFTP_DB) << "open: " << url;

  if (!sftpConnect()) {
    return;
  }

  const QString path = url.path();
  const QByteArray path_c = path.toUtf8();

  sftp_attributes sb = sftp_lstat(mSftp, path_c.constData());
  if (sb == NULL) {
    reportError(url, sftp_get_error(mSftp));
    return;
  }

  switch (sb->type) {
    case SSH_FILEXFER_TYPE_DIRECTORY:
      error(KIO::ERR_IS_DIRECTORY, url.prettyUrl());
      sftp_attributes_free(sb);
      return;
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
      error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyUrl());
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

  if (mOpenFile == NULL) {
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
      error(KIO::ERR_COULD_NOT_READ, mOpenUrl.prettyUrl());
      close();
      return;
    } else {
      QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
      KMimeType::Ptr p_mimeType = KMimeType::findByNameAndContent(mOpenUrl.fileName(), fileData);
      emit mimeType(p_mimeType->name());

      // Go back to the beginning of the file.
      sftp_rewind(mOpenFile);
    }
  }

  mOpenUrl = url;

  openOffset = 0;
  totalSize(fileSize);
  position(0);
  opened();
}

void sftpProtocol::read(KIO::filesize_t bytes) {
  kDebug(KIO_SFTP_DB) << "read, offset = " << openOffset << ", bytes = " << bytes;

  Q_ASSERT(mOpenFile != NULL);

  QVarLengthArray<char> buffer(bytes);

  ssize_t bytesRead = sftp_read(mOpenFile, buffer.data(), bytes);
  Q_ASSERT(bytesRead <= static_cast<ssize_t>(bytes));

  if (bytesRead < 0) {
    kDebug(KIO_SFTP_DB) << "Could not read " << mOpenUrl;
    error(KIO::ERR_COULD_NOT_READ, mOpenUrl.prettyUrl());
    close();
    return;
  }

  const QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
  data(fileData);
}

void sftpProtocol::write(const QByteArray &data) {
  kDebug(KIO_SFTP_DB) << "write, offset = " << openOffset << ", bytes = " << data.size();

  Q_ASSERT(mOpenFile != NULL);

  ssize_t bytesWritten = sftp_write(mOpenFile, data.data(), data.size());
  if (bytesWritten < 0) {
    kDebug(KIO_SFTP_DB) << "Could not write to " << mOpenUrl;
    error(KIO::ERR_COULD_NOT_WRITE, mOpenUrl.prettyUrl());
    close();
    return;
  }

  written(bytesWritten);
}

void sftpProtocol::seek(KIO::filesize_t offset) {
  kDebug(KIO_SFTP_DB) << "seek, offset = " << offset;

  Q_ASSERT(mOpenFile != NULL);

  if (sftp_seek64(mOpenFile, static_cast<uint64_t>(offset)) < 0) {
    error(KIO::ERR_COULD_NOT_SEEK, mOpenUrl.path());
    close();
  }

  position(sftp_tell64(mOpenFile));
}

void sftpProtocol::close() {
  sftp_close(mOpenFile);

  mOpenFile = NULL;
  finished();
}

void sftpProtocol::get(const KUrl& url) {
  kDebug(KIO_SFTP_DB) << url;

  int errorCode = 0;
  const sftpProtocol::StatusCode cs = sftpGet(url, errorCode);

  if (cs == sftpProtocol::Success) {
    finished();
    return;
  }

  // The call to sftpGet should only return server side errors since the file
  // descriptor parameter is set to -1.
  if (cs == sftpProtocol::ServerError && errorCode) {
     error(errorCode, url.prettyUrl());
  }
}

sftpProtocol::StatusCode sftpProtocol::sftpGet(const KUrl& url, int& errorCode, KIO::fileoffset_t offset, int fd) {

  kDebug(KIO_SFTP_DB) << url;

  if (!sftpConnect()) {
    return sftpProtocol::ServerError;
  }

  QByteArray path = url.path().toUtf8();

  sftp_file file = NULL;
  KIO::filesize_t totalbytesread  = 0;
  QByteArray filedata;

  sftp_attributes sb = sftp_lstat(mSftp, path.constData());
  if (sb == NULL) {
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
  if (file == NULL) {
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
    int accuracy = 0;
    KMimeType::Ptr mime = KMimeType::findByNameAndContent(url.fileName(), QByteArray(mimeTypeBuf, bytesread), 0, &accuracy);
    if (!mime->isDefault() && accuracy == 100) {
      emit mimeType(mime->name());
    } else {
      accuracy = 0;
      mime = KMimeType::findByUrl(url.fileName(), 0, false, true, &accuracy);
      emit mimeType(mime->name());
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
      kDebug(KIO_SFTP_DB) << "Resume offset: " << QString::number(offset);
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
    } else if ((errorCode = writeToFile(fd, filedata.constData(), filedata.size()) != 0)) {
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

void sftpProtocol::put(const KUrl& url, int permissions, KIO::JobFlags flags) {
  kDebug(KIO_SFTP_DB) << url << ", permissions =" << permissions
                      << ", overwrite =" << (flags & KIO::Overwrite)
                      << ", resume =" << (flags & KIO::Resume);

  kDebug(KIO_SFTP_DB) << url;

  int errorCode = 0;
  const sftpProtocol::StatusCode cs = sftpPut(url, permissions, flags, errorCode);

  if (cs == sftpProtocol::Success) {
    finished();
    return;
  }

  // The call to sftpPut should only return server side errors since the file
  // descriptor parameter is set to -1.
  if (cs == sftpProtocol::ServerError && errorCode) {
     error(errorCode, url.prettyUrl());
  }
}

sftpProtocol::StatusCode sftpProtocol::sftpPut(const KUrl& url, int permissions, JobFlags flags, int& errorCode, int fd) {
  kDebug(KIO_SFTP_DB) << url << ", permissions =" << permissions
                      << ", overwrite =" << (flags & KIO::Overwrite)
                      << ", resume =" << (flags & KIO::Resume);

  if (!sftpConnect()) {
    return sftpProtocol::ServerError;
  }

  const QString dest_orig = url.path();
  const QByteArray dest_orig_c = dest_orig.toUtf8();
  const QString dest_part = dest_orig + ".part";
  const QByteArray dest_part_c = dest_part.toUtf8();
  uid_t owner = 0;
  gid_t group = 0;

  sftp_attributes sb = sftp_lstat(mSftp, dest_orig_c.constData());
  const bool bOrigExists = (sb != NULL);
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
    bPartExists = (sbPart != NULL);

    if (bPartExists && !(flags & KIO::Resume) && !(flags & KIO::Overwrite) &&
        sbPart->size > 0 && sbPart->type == SSH_FILEXFER_TYPE_REGULAR) {

      if (fd == -1) {
        // Maybe we can use this partial file for resuming
        // Tell about the size we have, and the app will tell us
        // if it's ok to resume or not.
        kDebug(KIO_SFTP_DB) << "calling canResume with " << sbPart->size;
        flags |= canResume(sbPart->size) ? KIO::Resume : KIO::DefaultFlags;
        kDebug(KIO_SFTP_DB) << "put got answer " << (flags & KIO::Resume);

      } else {
        KIO::filesize_t pos = seekPos(fd, sbPart->size, SEEK_SET);
        if (pos != sbPart->size) {
          kDebug(KIO_SFTP_DB) << "Failed to seek to" << sbPart->size << "bytes in source file. Reason given" << strerror(errno);
          sftp_attributes_free(sb);
          sftp_attributes_free(sbPart);
          errorCode = ERR_COULD_NOT_SEEK;
          return sftpProtocol::ClientError;
        }
        flags |= KIO::Resume;
      }
      kDebug(KIO_SFTP_DB) << "Resuming at" << sbPart->size;
      sftp_attributes_free(sbPart);
    }
  }

  if (bOrigExists && !(flags & KIO::Overwrite) && !(flags & KIO::Resume)) {
    if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
      errorCode = KIO::ERR_DIR_ALREADY_EXIST;
    } else {
      errorCode = KIO::ERR_FILE_ALREADY_EXIST;
    }
    sftp_attributes_free(sb);
    return sftpProtocol::ServerError;
  }

  QByteArray dest;
  int result = -1;
  sftp_file file = NULL;
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
          kDebug(KIO_SFTP_DB) << "Appending .part extension to" << dest_orig;
          dest = dest_part_c;
          if (bPartExists && !(flags & KIO::Resume)) {
            kDebug(KIO_SFTP_DB) << "Deleting partial file" << dest_part;
            sftp_unlink(mSftp, dest_part_c.constData());
            // Catch errors when we try to open the file.
          }
        } else {
          dest = dest_orig_c; // Will be automatically truncated below...
        } // bMarkPartial

        if ((flags & KIO::Resume)) {
          sftp_attributes fstat;

          kDebug(KIO_SFTP_DB) << "Trying to append: " << dest;
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

          kDebug(KIO_SFTP_DB) << "Trying to open:" << dest << ", mode=" << QString::number(initialMode);
          file = sftp_open(mSftp, dest.constData(), O_CREAT | O_TRUNC | O_WRONLY, initialMode);
        } // flags & KIO::Resume

        if (file == NULL) {
          kDebug(KIO_SFTP_DB) << "COULD NOT WRITE " << dest
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
    kDebug(KIO_SFTP_DB) << "Error during 'put'. Aborting.";

    if (file != NULL) {
      sftp_close(file);

      sftp_attributes attr = sftp_stat(mSftp, dest.constData());
      if (bMarkPartial && attr != NULL) {
        size_t size = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
        if (attr->size < size) {
          sftp_unlink(mSftp, dest.constData());
        }
      }
      delete attr;
      sftp_attributes_free(attr);
    }

    //::exit(255);
    return cs;
  }

  if (file == NULL) { // we got nothing to write out, so we never opened the file
    return sftpProtocol::Success;
  }

  if (sftp_close(file) < 0) {
    kWarning(KIO_SFTP_DB) << "Error when closing file descriptor";
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
      kWarning(KIO_SFTP_DB) << " Couldn't rename " << dest << " to " << dest_orig;
      errorCode = KIO::ERR_CANNOT_RENAME_PARTIAL;
      return sftpProtocol::ServerError;
    }
  }

  // set final permissions
  if (permissions != -1 && !(flags & KIO::Resume)) {
    kDebug(KIO_SFTP_DB) << "Trying to set final permissions of " << dest_orig << " to " << QString::number(permissions);
    if (sftp_chmod(mSftp, dest_orig_c.constData(), permissions) < 0) {
      errorCode = -1;  // force copy to call sftpSendWarning...
      return sftpProtocol::ServerError;
    }
  }

  // set original owner and group
  if (bOrigExists) {
    kDebug(KIO_SFTP_DB) << "Trying to restore original owner and group of " << dest_orig;
    if (sftp_chown(mSftp, dest_orig_c.constData(), owner, group) < 0) {
      kWarning(KIO_SFTP_DB) << "Could not change owner and group for" << dest_orig;
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
      if (attr != NULL) {
        times[0].tv_sec = attr->atime; //// access time, unchanged
        times[1].tv_sec =  dt.toTime_t(); // modification time
        times[0].tv_usec = times[1].tv_usec = 0;

        sftp_utimes(mSftp, dest_orig_c.constData(), times);
        sftp_attributes_free(attr);
      }
    }
  }

  return sftpProtocol::Success;
}

void sftpProtocol::copy(const KUrl &src, const KUrl &dest, int permissions, KIO::JobFlags flags)
{
  kDebug(KIO_SFTP_DB) << src << " -> " << dest << " , permissions = " << QString::number(permissions)
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

  // On success or errorcode < 0, emit the finished signal and
  // send a warning message to the client if errorCode < 0.
  if (cs == sftpProtocol::Success || errorCode < 0) {
    if (errorCode < 0)
      sftpSendWarning(errorCode, sCopyFile);
    finished();
    return;
  }

  if (errorCode) {
    error(errorCode, sCopyFile);
  }
}

sftpProtocol::StatusCode sftpProtocol::sftpCopyGet(const KUrl& url, const QString& sCopyFile, int permissions, KIO::JobFlags flags, int& errorCode)
{
  kDebug(KIO_SFTP_DB) << url << "->" << sCopyFile << ", permissions=" << permissions;

  // check if destination is ok ...
  KDE_struct_stat buff;
  const bool bDestExists = (KDE::stat(sCopyFile, &buff) != -1);

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
  const bool bPartExists = (KDE::stat(sPart, &buff) != -1);
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
    fd = KDE::open( sPart, O_RDWR );  // append if resuming
    offset = seekPos(fd, 0, SEEK_END);
    if(offset < 0) {
      errorCode = ERR_CANNOT_RESUME;
      return sftpProtocol::ClientError;                            // client side error
    }
    kDebug(KIO_SFTP_DB) << "resuming at" << offset;
  }
  else {
    fd = KDE::open(dest, O_CREAT | O_TRUNC | O_WRONLY, initialMode);
  }

  if (fd == -1) {
    kDebug(KIO_SFTP_DB) << "could not write to" << sCopyFile;
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
      if ( KDE::rename( sPart, sCopyFile ) ) {
        // If rename fails, try removing the destination first if it exists.
        if (!bDestExists || !(QFile::remove(sCopyFile) && KDE::rename(sPart, sCopyFile) == 0)) {
            kDebug(KIO_SFTP_DB) << "cannot rename " << sPart << " to " << sCopyFile;
            errorCode = ERR_CANNOT_RENAME_PARTIAL;
            result = sftpProtocol::ClientError;
        }
      }
    }
    else if (KDE::stat( sPart, &buff ) == 0) { // should a very small ".part" be deleted?
      const int size = config()->readEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
      if (buff.st_size <  size)
        QFile::remove(sPart);
    }
  }

  return result;
}

sftpProtocol::StatusCode sftpProtocol::sftpCopyPut(const KUrl& url, const QString& sCopyFile, int permissions, JobFlags flags, int& errorCode)
{
  kDebug(KIO_SFTP_DB) << sCopyFile << "->" << url << ", permissions=" << permissions << ", flags" << flags;

  // check if source is ok ...
  KDE_struct_stat buff;
  bool bSrcExists = (KDE::stat(sCopyFile, &buff) != -1);

  if (bSrcExists) {
    if (S_ISDIR(buff.st_mode)) {
      errorCode = ERR_IS_DIRECTORY;
      return sftpProtocol::ClientError;
    }
  } else {
    errorCode = ERR_DOES_NOT_EXIST;
    return sftpProtocol::ClientError;
  }

  const int fd = KDE::open(sCopyFile, O_RDONLY);
  if(fd == -1)
  {
    errorCode = ERR_CANNOT_OPEN_FOR_READING;
    return sftpProtocol::ClientError;
  }

  totalSize(buff.st_size);

  // delegate the real work (errorCode gets status) ...
  return sftpPut(url, permissions, flags, errorCode, fd);
}


void sftpProtocol::stat(const KUrl& url) {
  kDebug(KIO_SFTP_DB) << url;

  if (!sftpConnect()) {
    return;
  }

  if (! url.hasPath() || QDir::isRelativePath(url.path()) ||
      url.path().contains("/./") || url.path().contains("/../")) {
    QString cPath;

    if (url.hasPath()) {
      cPath = canonicalizePath(url.path());
    } else {
      cPath = canonicalizePath(QLatin1String("."));
    }

    if (cPath.isEmpty()) {
      error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
      return;
    }
    KUrl redir(url);
    redir.setPath(cPath);
    redirection(redir);

    kDebug(KIO_SFTP_DB) << "redirecting to " << redir.url();

    finished();
    return;
  }

  QByteArray path = url.path().toUtf8();

  const QString sDetails = metaData(QLatin1String("details"));
  const int details = sDetails.isEmpty() ? 2 : sDetails.toInt();

  UDSEntry entry;
  entry.clear();
  if (!createUDSEntry(url.fileName(), path, entry, details)) {
    error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
    return;
  }

  statEntry(entry);

  finished();
}

void sftpProtocol::mimetype(const KUrl& url){
  kDebug(KIO_SFTP_DB) << url;

  if (!sftpConnect()) {
    return;
  }

  // open() feeds the mimetype
  open(url, QIODevice::ReadOnly);
  close();

  finished();
}

void sftpProtocol::listDir(const KUrl& url) {
  kDebug(KIO_SFTP_DB) << "list directory: " << url;

  if (!sftpConnect()) {
    return;
  }

  if (! url.hasPath() || QDir::isRelativePath(url.path()) ||
      url.path().contains("/./") || url.path().contains("/../")) {
    QString cPath;

    if (url.hasPath()) {
      cPath = canonicalizePath(url.path());
    } else {
      cPath = canonicalizePath(QString("."));
    }

    if (cPath.isEmpty()) {
      error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
      return;
    }
    KUrl redir(url);
    redir.setPath(cPath);
    redirection(redir);

    kDebug(KIO_SFTP_DB) << "redirecting to " << redir.url();

    finished();
    return;
  }

  QByteArray path = url.path().toUtf8();

  sftp_dir dp = sftp_opendir(mSftp, path.constData());
  if (dp == NULL) {
    reportError(url, sftp_get_error(mSftp));
    return;
  }

  sftp_attributes dirent = NULL;
  const QString sDetails = metaData(QLatin1String("details"));
  const int details = sDetails.isEmpty() ? 2 : sDetails.toInt();
  UDSEntry entry;

  kDebug(KIO_SFTP_DB) << "readdir: " << path << ", details: " << QString::number(details);

  for (;;) {
    mode_t access;
    mode_t type;
    char *link;

    dirent = sftp_readdir(mSftp, dp);
    if (dirent == NULL) {
      break;
    }

    entry.clear();
    entry.insert(KIO::UDSEntry::UDS_NAME, QFile::decodeName(dirent->name));

    if (dirent->type == SSH_FILEXFER_TYPE_SYMLINK) {
      QByteArray file = path + '/' + QFile::decodeName(dirent->name).toUtf8();

      entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);

      link = sftp_readlink(mSftp, file.constData());
      if (link == NULL) {
        sftp_attributes_free(dirent);
        error(KIO::ERR_INTERNAL, i18n("Could not read link: %1", QString::fromUtf8(file)));
        return;
      }
      entry.insert(KIO::UDSEntry::UDS_LINK_DEST, QFile::decodeName(link));
      delete link;
      // A symlink -> follow it only if details > 1
      if (details > 1) {
        sftp_attributes sb = sftp_stat(mSftp, file.constData());
        if (sb == NULL) {
          // It is a link pointing to nowhere
          type = S_IFMT - 1;
          access = S_IRWXU | S_IRWXG | S_IRWXO;
          entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, type);
          entry.insert( KIO::UDSEntry::UDS_ACCESS, access);
          entry.insert( KIO::UDSEntry::UDS_SIZE, 0LL );

          goto notype;
        }
        sftp_attributes_free(dirent);
        dirent = sb;
      }
    }

    switch (dirent->type) {
      case SSH_FILEXFER_TYPE_REGULAR:
        entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
        break;
      case SSH_FILEXFER_TYPE_DIRECTORY:
        entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        break;
      case SSH_FILEXFER_TYPE_SYMLINK:
        entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFLNK);
        break;
      case SSH_FILEXFER_TYPE_SPECIAL:
      case SSH_FILEXFER_TYPE_UNKNOWN:
        break;
    }

    access = dirent->permissions & 07777;
    entry.insert(KIO::UDSEntry::UDS_ACCESS, access);

    entry.insert(KIO::UDSEntry::UDS_SIZE, dirent->size);

notype:
    if (details > 0) {
      if (dirent->owner) {
          entry.insert(KIO::UDSEntry::UDS_USER, QString::fromUtf8(dirent->owner));
      } else {
          entry.insert(KIO::UDSEntry::UDS_USER, QString::number(dirent->uid));
      }

      if (dirent->group) {
          entry.insert(KIO::UDSEntry::UDS_GROUP, QString::fromUtf8(dirent->group));
      } else {
          entry.insert(KIO::UDSEntry::UDS_GROUP, QString::number(dirent->gid));
      }

      entry.insert(KIO::UDSEntry::UDS_ACCESS_TIME, dirent->atime);
      entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, dirent->mtime);
      entry.insert(KIO::UDSEntry::UDS_CREATION_TIME, dirent->createtime);
    }

    sftp_attributes_free(dirent);
    listEntry(entry, false);
  } // for ever
  sftp_closedir(dp);
  listEntry(entry, true); // ready

  finished();
}

void sftpProtocol::mkdir(const KUrl &url, int permissions) {
  kDebug(KIO_SFTP_DB) << "create directory: " << url;

  if (!sftpConnect()) {
    return;
  }

  if (url.path().isEmpty()) {
    error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
    return;
  }
  const QString path = url.path();
  const QByteArray path_c = path.toUtf8();

  // Remove existing file or symlink, if requested.
  if (metaData(QLatin1String("overwrite")) == QLatin1String("true")) {
    kDebug(KIO_SFTP_DB) << "overwrite set, remove existing file or symlink: " << url;
    sftp_unlink(mSftp, path_c.constData());
  }

  kDebug(KIO_SFTP_DB) << "Trying to create directory: " << path;
  sftp_attributes sb = sftp_lstat(mSftp, path_c.constData());
  if (sb == NULL) {
    if (sftp_mkdir(mSftp, path_c.constData(), 0777) < 0) {
      reportError(url, sftp_get_error(mSftp));
      sftp_attributes_free(sb);
      return;
    } else {
      kDebug(KIO_SFTP_DB) << "Successfully created directory: " << url;
      if (permissions != -1) {
        chmod(url, permissions);
      } else {
        finished();
      }
      sftp_attributes_free(sb);
      return;
    }
  }

  if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
    error(KIO::ERR_DIR_ALREADY_EXIST, path);
  } else {
    error(KIO::ERR_FILE_ALREADY_EXIST, path);
  }

  sftp_attributes_free(sb);
  return;
}

void sftpProtocol::rename(const KUrl& src, const KUrl& dest, KIO::JobFlags flags) {
  kDebug(KIO_SFTP_DB) << "rename " << src << " to " << dest << flags;

  if (!sftpConnect()) {
    return;
  }

  QByteArray qsrc = src.path().toUtf8();
  QByteArray qdest = dest.path().toUtf8();

  sftp_attributes sb = sftp_lstat(mSftp, qdest.constData());
  if (sb != NULL) {
    if (!(flags & KIO::Overwrite)) {
      if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
        error(KIO::ERR_DIR_ALREADY_EXIST, dest.url());
      } else {
        error(KIO::ERR_FILE_ALREADY_EXIST, dest.url());
      }
      sftp_attributes_free(sb);
      return;
    }

    // Delete the existing destination file/dir...
    if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
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

void sftpProtocol::symlink(const QString &target, const KUrl &dest, KIO::JobFlags flags) {
  kDebug(KIO_SFTP_DB) << "link " << target << "->" << dest
                      << ", overwrite = " << (flags & KIO::Overwrite)
                      << ", resume = " << (flags & KIO::Resume);

  if (!sftpConnect()) {
    return;
  }

  QByteArray t = target.toUtf8();
  QByteArray d = dest.path().toUtf8();

  bool failed = false;
  if (sftp_symlink(mSftp, t.constData(), d.constData()) < 0) {
    if (flags == KIO::Overwrite) {
      sftp_attributes sb = sftp_lstat(mSftp, d.constData());
      if (sb == NULL) {
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

void sftpProtocol::chmod(const KUrl& url, int permissions) {
  kDebug(KIO_SFTP_DB) << "change permission of " << url << " to " << QString::number(permissions);

  if (!sftpConnect()) {
    return;
  }

  QByteArray path = url.path().toUtf8();

  if (sftp_chmod(mSftp, path.constData(), permissions) < 0) {
    reportError(url, sftp_get_error(mSftp));
    return;
  }

  finished();
}

void sftpProtocol::del(const KUrl &url, bool isfile){
  kDebug(KIO_SFTP_DB) << "deleting " << (isfile ? "file: " : "directory: ") << url;

  if (!sftpConnect()) {
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
  kDebug(KIO_SFTP_DB) << "connected to " << mHost << "?: " << mConnected;
  slaveStatus((mConnected ? mHost : QString()), mConnected);
}

sftpProtocol::GetRequest::GetRequest(sftp_file file, sftp_attributes sb, ushort maxPendingRequests)
    :mFile(file), mSb(sb), mMaxPendingRequests(maxPendingRequests) {

}

bool sftpProtocol::GetRequest::enqueueChunks() {
  sftpProtocol::GetRequest::Request request;

  kDebug(KIO_SFTP_DB) << "enqueueChunks";

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

    if (mFile->offset > mSb->size) {
      // Do not add any more chunks if the offset is larger than the given file size.
      // However this is done after adding a request as the remote file size may
      // have changed in the meantime.
      break;
    }
  }

  kDebug(KIO_SFTP_DB) << "enqueueChunks done" << QString::number(pendingRequests.size());

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

    // kDebug(KIO_SFTP_DB) << "bytesread=" << QString::number(bytesread);

    if (bytesread == 0 || bytesread == SSH_AGAIN) {
      if (bytesread == 0) {
        pendingRequests.dequeue();
      } else {
        // Decrease maximum pending requests as we did not receive data fast enough
        mMaxPendingRequests = qMax(1, mMaxPendingRequests / 2);
      }
      // Done reading or timeout
      data.resize(data.size() - request.expectedLength);
      break;
    } else if (bytesread == SSH_ERROR) {
      return -1;
    }

    totalRead += bytesread;

    if (bytesread < request.expectedLength) {
      // If less data is read than expected - requeue the request
      data.resize(data.size() - (request.expectedLength - bytesread));

      // Save current file offset
      uint64_t oldOffset = mFile->offset;
      mFile->offset = request.startOffset + bytesread;

      // Modify current request
      request.expectedLength = request.expectedLength - bytesread;
      request.startOffset = mFile->offset;
      request.id = sftp_async_read_begin(mFile, request.expectedLength);

      // Restore the file offset
      mFile->offset = oldOffset;

      if (request.id < 0) {
        // Failed to dispatch rerequest
        return -1;
      }

      return totalRead;
    }

    pendingRequests.dequeue();
  }

  // Adjust maximum pending requests
  mMaxPendingRequests = qMin(mMaxPendingRequests * 2, MAX_TRANSFER_SIZE / MAX_XFER_BUF_SIZE);

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
    KUrl redirectUrl;
    redirectUrl.setProtocol( QLatin1String("sftp") );
    redirectUrl.setUser( mUsername );
    redirectUrl.setPass( mPassword );
    redirectUrl.setHost( mHost );
    if (mPort > 0 && mPort != DEFAULT_SFTP_PORT) {
        redirectUrl.setPort( mPort );
    }
    kDebug(KIO_SFTP_DB) << "redirecting to" << redirectUrl;
    redirection( redirectUrl );
}

bool sftpProtocol::sftpConnect()
{
    const QString origUsername = mUsername;
    openConnection();
    kDebug(KIO_SFTP_DB) << "connected ?" << mConnected << "username: old=" << origUsername << "new=" << mUsername;
    if (!origUsername.isEmpty() && origUsername != mUsername) {
        requiresUserNameRedirection();
        finished();
        return false;
    }
    if (!mConnected) {
        return false;
    }

    return true;
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
