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

#include <fcntl.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVarLengthArray>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

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
#include <kconfiggroup.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <libssh/callbacks.h>

#define KIO_SFTP_SPECIAL_TIMEOUT 30
#define ZERO_STRUCTP(x) do { if ((x) != NULL) memset((char *)(x), 0, sizeof(*(x))); } while(0)

using namespace KIO;
extern "C"
{
  int KDE_EXPORT kdemain( int argc, char **argv )
  {
    QCoreApplication app(argc, argv);
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
  info.url.setPort(mPort);
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

  err = ssh_userauth_kbdint(mSession, mUsername.toUtf8().constData(), NULL);
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
        infoKbdInt.url.setPort(mPort);

        infoKbdInt.caption = i18n("SFTP Login");
        infoKbdInt.comment = "sftp://" + mUsername + "@"  + mHost;

        if (!name.isEmpty()) {
          infoKbdInt.caption = QString(i18n("SFTP Login") + " - " + name);
        }

        if (!instruction.isEmpty()) {
          newPrompt = instruction + "\n\n";
        }

        newPrompt.append(prompt + "\n\n");
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
    err = ssh_userauth_kbdint(mSession, mUsername.toUtf8().constData(), NULL);
  }

  return err;
}

void sftpProtocol::reportError(const KUrl &url, const int err) {
  kDebug(KIO_SFTP_DB) << "url = " << url << " - err=" << err;

  switch (err) {
    case SSH_FX_OK:
      break;
    case SSH_FX_NO_SUCH_FILE:
    case SSH_FX_NO_SUCH_PATH:
      error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
      break;
    case SSH_FX_PERMISSION_DENIED:
      error(KIO::ERR_ACCESS_DENIED, url.prettyUrl());
      break;
    case SSH_FX_FILE_ALREADY_EXISTS:
      error(KIO::ERR_FILE_ALREADY_EXIST, url.prettyUrl());
      break;
    case SSH_FX_INVALID_HANDLE:
      error(KIO::ERR_MALFORMED_URL, url.prettyUrl());
      break;
    case SSH_FX_OP_UNSUPPORTED:
      error(KIO::ERR_UNSUPPORTED_ACTION, url.prettyUrl());
      break;
    case SSH_FX_BAD_MESSAGE:
      error(KIO::ERR_UNKNOWN, url.prettyUrl());
      break;
    default:
      error(KIO::ERR_INTERNAL, url.prettyUrl());
      break;
  }
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

void sftpProtocol::setHost(const QString& h, quint16 port, const QString& user, const QString& pass) {
  kDebug(KIO_SFTP_DB) << "setHost(): " << user << "@" << h << ":" << port;

  if (mConnected) {
    closeConnection();
  }

  mHost = h;

  if (port > 0) {
    mPort = port;
  } else {
    struct servent *pse;
    if ((pse = getservbyname("ssh", "tcp") ) == NULL) {
      mPort = 22;
    } else {
      mPort = ntohs(pse->s_port);
    }
  }

  kDebug(KIO_SFTP_DB) << "setHost(): mPort=" << mPort;

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
  info.keepPassword = true; // make the "keep Password" check box visible to the user.

  // Check for cached authentication info if no password is specified...
  if (mPassword.isEmpty()) {
    kDebug(KIO_SFTP_DB) << "checking cache: info.username = " << info.username
      << ", info.url = " << info.url.prettyUrl();

    if (checkCachedAuthentication(info)) {
      mUsername = info.username;
      mPassword = info.password;
    }
  }

  // Start the ssh connection.
  QString msg;     // msg for dialog box
  QString caption; // dialog box caption
  unsigned char *hash = NULL; // the server hash
  char *hexa;
  char *verbosity;
  int rc, state, hlen;
  int timeout_sec = 30, timeout_usec = 0;

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
  if (!mUsername.isEmpty()) {
    rc = ssh_options_set(mSession, SSH_OPTIONS_USER, mUsername.toUtf8().constData());
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
  while (rc != SSH_AUTH_SUCCESS) {

    // Try to authenticate with public key first
    kDebug(KIO_SFTP_DB) << "Trying to authenticate public key";
    if (method & SSH_AUTH_METHOD_PUBLICKEY) {
      rc = ssh_userauth_autopubkey(mSession, NULL);
      if (rc == SSH_AUTH_ERROR) {
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
        return;
      } else if (rc == SSH_AUTH_SUCCESS) {
        break;
      }
    }

    info.caption = i18n("SFTP Login");
    info.prompt = i18n("Please enter your username and password.");
    info.readOnly = false;
    if (firstTime) {
      dlgResult = openPasswordDialog(info);
    } else {
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

    if (mUsername != info.username) {
      kDebug(KIO_SFTP_DB) << "Username changed from " << mUsername
                                    << " to " << info.username;
    }
    mUsername = info.username;
    mPassword = info.password;

    // Try to authenticate with keyboard interactive
    kDebug(KIO_SFTP_DB) << "Trying to authenticate with keyboard interactive";
    if (method & SSH_AUTH_METHOD_INTERACTIVE) {
      rc = authenticateKeyboardInteractive(info);
      if (rc == SSH_AUTH_ERROR) {
        closeConnection();
        error(KIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."));
        return;
      } else if (rc == SSH_AUTH_SUCCESS) {
        break;
      }
    }

    // Try to authenticate with password
    kDebug(KIO_SFTP_DB) << "Trying to authenticate with password";
    if (method & SSH_AUTH_METHOD_PASSWORD) {
      rc = ssh_userauth_password(mSession, mUsername.toUtf8().constData(),
          mPassword.toUtf8().constData());
      if (rc == SSH_AUTH_ERROR) {
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
  info.url.setProtocol("sftp");
  info.url.setHost(mHost);
  info.url.setPort(mPort);
  info.url.setUser(mUsername);
  info.username = mUsername;
  info.password = mPassword;

  kDebug(KIO_SFTP_DB) << "Caching info.username = " << info.username
    << ", info.url = " << info.url.prettyUrl();

  cacheAuthentication(info);

  setTimeoutSpecialCommand(KIO_SFTP_SPECIAL_TIMEOUT);

  mConnected = true;
  connected();

  mPassword.fill('x');
  mPassword.clear();
  info.password.fill('x');
  info.password.clear();

  return;
}

void sftpProtocol::closeConnection() {
  kDebug(KIO_SFTP_DB) << "closeConnection()";

  sftp_free(mSftp);
  mSftp = NULL;

  ssh_disconnect(mSession);
  mSession = NULL;

  mConnected = false;
}

void sftpProtocol::special(const QByteArray &data) {
    int rc;
    kDebug(KIO_SFTP_DB) << "special(): polling";

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

  openConnection();
  if (!mConnected) {
    error(KIO::ERR_CONNECTION_BROKEN, url.prettyUrl());
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

  QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
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
  kDebug(KIO_SFTP_DB) << "get(): " << url;

  openConnection();
  if (!mConnected) {
    return;
  }

  QByteArray path = url.path().toUtf8();

  char buf[MAX_XFER_BUF_SIZE] = {0};
  sftp_file file = NULL;
  ssize_t bytesread = 0;
  // time_t curtime = 0;
  time_t lasttime = 0;
  time_t starttime = 0;
  KIO::filesize_t totalbytesread  = 0;
  QByteArray  filedata;

  sftp_attributes sb = sftp_lstat(mSftp, path.constData());
  if (sb == NULL) {
    reportError(url, sftp_get_error(mSftp));
    return;
  }

  if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
    error(KIO::ERR_IS_DIRECTORY, url.prettyUrl());
    sftp_attributes_free(sb);
    return;
  }
  if (sb->type != SSH_FILEXFER_TYPE_REGULAR) {
    error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyUrl());
    sftp_attributes_free(sb);
    return;
  }

  // Open file
  file = sftp_open(mSftp, path.constData(), O_RDONLY, 0);
  if (file == NULL) {
    error( KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyUrl());
    sftp_attributes_free(sb);
    return;
  }

  // Determine the mimetype of the file to be retrieved, and emit it.
  // This is mandatory in all slaves (for KRun/BrowserRun to work)
  // In real "remote" slaves, this is usually done using findByNameAndContent
  // after receiving some data. But we don't know how much data the mimemagic rules
  // need, so for local files, better use findByUrl with localUrl=true.
  KMimeType::Ptr mt = KMimeType::findByUrl( url, sb->permissions, false /* remote URL */ );
  emit mimeType( mt->name() ); // FIXME test me

  // Set the total size
  totalSize(sb->size);

  const QString resumeOffset = metaData(QLatin1String("resume"));
  if (!resumeOffset.isEmpty()) {
    bool ok;
    KIO::fileoffset_t offset = resumeOffset.toLongLong(&ok);
    if (ok && (offset > 0) && ((unsigned long long) offset < sb->size))
    {
      if (sftp_seek64(file, offset) == 0) {
        canResume();
        totalbytesread = offset;
        kDebug(KIO_SFTP_DB) << "Resume offset: " << QString::number(offset);
      }
    }
  }

  if (file != NULL) {
    bool isFirstPacket = true;
    lasttime = starttime = time(NULL);

    for (;;) {
      bytesread = sftp_read(file, buf, MAX_XFER_BUF_SIZE);
      kDebug(KIO_SFTP_DB) << "bytesread=" << QString::number(bytesread);
      if (bytesread == 0) {
        // All done reading
        break;
      } else if (bytesread < 0) {
        error(KIO::ERR_COULD_NOT_READ, url.prettyUrl());
        sftp_attributes_free(sb);
        return;
      }

      filedata = QByteArray::fromRawData(buf, bytesread);
      if (isFirstPacket) {
        KMimeType::Ptr p_mimeType = KMimeType::findByNameAndContent(url.fileName(), filedata);
        mimeType(p_mimeType->name());
        isFirstPacket = false;
      }
      data(filedata);
      filedata.clear();

      // increment total bytes read
      totalbytesread += bytesread;

      processedSize(totalbytesread);
    }

    sftp_close(file);
    data(QByteArray());
    processedSize(static_cast<KIO::filesize_t>(sb->size));
  }

  sftp_attributes_free(sb);
  finished();
}

void sftpProtocol::put(const KUrl& url, int permissions, KIO::JobFlags flags) {
  kDebug(KIO_SFTP_DB) << "put(): " << url
                      << " , permissions = " << QString::number(permissions)
                      << ", overwrite = " << (flags & KIO::Overwrite)
                      << ", resume = " << (flags & KIO::Resume);

  openConnection();
  if (!mConnected) {
    return;
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
      kDebug(KIO_SFTP_DB) << "put : calling canResume with "
        << QString::number(sbPart->size);

      // Maybe we can use this partial file for resuming
      // Tell about the size we have, and the app will tell us
      // if it's ok to resume or not.
      flags |= canResume(sbPart->size) ? KIO::Resume : KIO::DefaultFlags;

      kDebug(KIO_SFTP_DB) << "put got answer " << (flags & KIO::Resume);

      delete sbPart;
    }
  }

  if (bOrigExists && !(flags & KIO::Overwrite) && !(flags & KIO::Resume)) {
    if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
      error(KIO::ERR_DIR_ALREADY_EXIST, dest_orig);
    } else {
      error(KIO::ERR_FILE_ALREADY_EXIST, dest_orig);
    }
    sftp_attributes_free(sb);
    return;
  }

  int result;
  QByteArray dest;
  sftp_file file = NULL;

  // Loop until we got 0 (end of data)
  do {
    QByteArray buffer;
    dataReq(); // Request for data
    result = readData(buffer);

    if (result >= 0) {
      if (dest.isEmpty()) {
        if (bMarkPartial) {
          kDebug(KIO_SFTP_DB) << "Appending .part extension to " << dest_orig;
          dest = dest_part_c;
          if (bPartExists && !(flags & KIO::Resume)) {
            kDebug(KIO_SFTP_DB) << "Deleting partial file " << dest_part;
            sftp_unlink(mSftp, dest_part_c.constData());
            // Catch errors when we try to open the file.
          }
        } else {
          dest = dest_orig_c;
          if (bOrigExists && !(flags & KIO::Resume)) {
            kDebug(KIO_SFTP_DB) << "Deleting destination file " << dest_orig;
            sftp_unlink(mSftp, dest_orig_c.constData());
            // Catch errors when we try to open the file.
          }
        } // bMarkPartial

        if ((flags & KIO::Resume)) {
          sftp_attributes fstat;

          kDebug(KIO_SFTP_DB) << "Trying to append: " << dest;
          file = sftp_open(mSftp, dest.constData(), O_RDWR, 0);  // append if resuming
          if (file) {
             fstat = sftp_fstat(file);
             if (fstat) {
                sftp_seek64(file, fstat->size); // Seek to end TODO
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

          kDebug(KIO_SFTP_DB) << "Trying to open: " << dest << ", mode=" << QString::number(initialMode);
          file = sftp_open(mSftp, dest.constData(), O_CREAT | O_TRUNC | O_WRONLY, initialMode);
        } // flags & KIO::Resume

        if (file == NULL) {
          kDebug(KIO_SFTP_DB) << "COULD NOT WRITE " << dest
                              << " permissions=" << permissions
                              << " error=" << ssh_get_error(mSession);
          if (sftp_get_error(mSftp) == SSH_FX_PERMISSION_DENIED) {
            error(KIO::ERR_WRITE_ACCESS_DENIED, QString::fromUtf8(dest));
          } else {
            error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, QString::fromUtf8(dest));
          }
          sftp_attributes_free(sb);
          finished();
          return;
        } // file
      } // dest.isEmpty

      ssize_t bytesWritten = sftp_write(file, buffer.data(), buffer.size());
      if (bytesWritten < 0) {
        error(KIO::ERR_COULD_NOT_WRITE, dest_orig);
        result = -1;
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
    finished();
    return;
  }

  if (file == NULL) { // we got nothing to write out, so we never opened the file
    finished();
    return;
  }

  if (sftp_close(file) < 0) {
    kWarning(KIO_SFTP_DB) << "Error when closing file descriptor";
    error(KIO::ERR_COULD_NOT_WRITE, dest_orig);
    return;
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
      error(KIO::ERR_CANNOT_RENAME_PARTIAL, dest_orig);
      return;
    }
  }

  // set final permissions
  if (permissions != -1 && !(flags & KIO::Resume)) {
    kDebug(KIO_SFTP_DB) << "Trying to set final permissions of " << dest_orig << " to " << QString::number(permissions);
    if (sftp_chmod(mSftp, dest_orig_c.constData(), permissions) < 0) {
      warning(i18n( "Could not change permissions for\n%1", dest_orig));
    }
  }

  // set original owner and group
  if (bOrigExists) {
      kDebug(KIO_SFTP_DB) << "Trying to restore original owner and group of " << dest_orig;
      if (sftp_chown(mSftp, dest_orig_c.constData(), owner, group) < 0) {
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

  // We have done our job => finish
  finished();
}

void sftpProtocol::copy(const KUrl &src, const KUrl &dest, int permissions, KIO::JobFlags flags)
{
  kDebug(KIO_SFTP_DB) << src << " -> " << dest << " , permissions = " << QString::number(permissions)
                                      << ", overwrite = " << (flags & KIO::Overwrite)
                                      << ", resume = " << (flags & KIO::Resume);

  error(KIO::ERR_UNSUPPORTED_ACTION, QString());
}

void sftpProtocol::stat(const KUrl& url) {
  kDebug(KIO_SFTP_DB) << url;

  openConnection();
  if (!mConnected) {
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

  openConnection();
  if (!mConnected) {
    return;
  }

  // open() feeds the mimetype
  open(url, QIODevice::ReadOnly);
  close();

  finished();
}

void sftpProtocol::listDir(const KUrl& url) {
  kDebug(KIO_SFTP_DB) << "list directory: " << url;

  openConnection();
  if (!mConnected) {
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
  QList<QByteArray> entryNames;
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
      QByteArray file = QByteArray(path + '/' + QFile::decodeName(dirent->name).toUtf8()).constData();

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

  openConnection();
  if (!mConnected) {
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
  kDebug(KIO_SFTP_DB) << "rename " << src << " to " << dest;

  openConnection();
  if (!mConnected) {
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

    del(dest, sb->type == SSH_FILEXFER_TYPE_DIRECTORY ? true : false);
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

  openConnection();
  if (!mConnected) {
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

  openConnection();
  if (!mConnected) {
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

  openConnection();
  if (!mConnected) {
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

