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

#ifndef __kio_sftp_h__
#define __kio_sftp_h__

#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <libssh/callbacks.h>

#include <QtCore/QQueue>

namespace KIO {
  class AuthInfo;
}

class sftpProtocol : public KIO::SlaveBase
{

public:
  sftpProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
  virtual ~sftpProtocol();
  virtual void setHost(const QString &h, quint16 port, const QString& user, const QString& pass);
  virtual void get(const KUrl &url);
  virtual void listDir(const KUrl &url) ;
  virtual void mimetype(const KUrl &url);
  virtual void stat(const KUrl &url);
  virtual void copy(const KUrl &src, const KUrl &dest, int permissions, KIO::JobFlags flags);
  virtual void put(const KUrl &url, int permissions, KIO::JobFlags flags);
  virtual void closeConnection();
  virtual void slave_status();
  virtual void del(const KUrl &url, bool isfile);
  virtual void chmod(const KUrl &url, int permissions);
  virtual void symlink(const QString &target, const KUrl &dest, KIO::JobFlags flags);
  virtual void rename(const KUrl &src, const KUrl &dest, KIO::JobFlags flags);
  virtual void mkdir(const KUrl &url, int permissions);
  virtual void openConnection();

  // KIO::FileJob interface
  virtual void open(const KUrl &url, QIODevice::OpenMode mode);
  virtual void read(KIO::filesize_t size);
  virtual void write(const QByteArray &data);
  virtual void seek(KIO::filesize_t offset);
  virtual void close();
  virtual void special(const QByteArray &data);

  // libssh authentication callback (note that this is called by the
  // global ::auth_callback() call.
  int auth_callback(const char *prompt, char *buf, size_t len,
    int echo, int verify, void *userdata);

  // libssh logging callback (note that this is called by the
  // global ::log_callback() call.
  void log_callback(ssh_session session, int priority, const char *message,
    void *userdata);

private: // Private variables
  /** True if ioslave is connected to sftp server. */
  bool mConnected;

  /** Host we are connected to. */
  QString mHost;

  /** Port we are connected to. */
  int mPort;

  /** The ssh session for the connection */
  ssh_session mSession;

  /** The sftp session for the connection */
  sftp_session mSftp;

  /** Username to use when connecting */
  QString mUsername;

  /** User's password */
  QString mPassword;

  /** The open file */
  sftp_file mOpenFile;

  /** The open URL */
  KUrl mOpenUrl;

  ssh_callbacks mCallbacks;

  /** Version of the sftp protocol we are using. */
  int sftpVersion;

  struct Status
  {
    int code;
    KIO::filesize_t size;
    QString text;
  };

  // KIO::FileJob interface
  /** The opened handle */
  QByteArray openHandle;
  KUrl openUrl;
  KIO::filesize_t openOffset;

  /**
   * Holds public key authentication info for proper retry handling.
   */
  KIO::AuthInfo* mPublicKeyAuthInfo;

  /**
   * GetRequest encapsulates several SFTP get requests into a single object.
   * As SFTP messages are limited to MAX_XFER_BUF_SIZE several requests
   * should be sent simultaneously in order to increase transfer speeds.
   */
  class GetRequest {
  public:
    /**
     * Creates a new GetRequest object.
     * @param file the sftp_file object which should be transferred.
     * @param sb the attributes of that sftp_file object.
     * @param maxPendingRequests the maximum number of parallel requests to start with.
     *                            The number will be adjusted automatically depending
     *                            on the connection speed.
     */
    GetRequest(sftp_file file, sftp_attributes sb, ushort maxPendingRequests = 1);
    /**
     * Removes all pending requests and closes the SFTP channel and attributes
     * in order to avoid memory leaks.
     */
    ~GetRequest();

    /**
     * Starts up to maxPendingRequests file requests. Reading is performed in the
     * via the readChunks method.
     */
    bool enqueueChunks();
    /**
     * Attemps to read all pending chunks in the given QByteArray.
     * @param data the array into which the data should be saved (it should be empty).
     * @return 0 on EOF or timeout, -1 on error and the number of bytes read otherwise.
     */
    int readChunks(QByteArray &data);
  private:
    struct Request {
      /** Identifier as returned by the sftp_async_read_begin call */
      int id;
      /** The number of bytes expected to be returned */
      uint32_t expectedLength;
      /** The SSH start offset when this request was made */
      uint64_t startOffset;
    };
  private:
    sftp_file mFile;
    sftp_attributes mSb;
    ushort mMaxPendingRequests;
    QQueue<Request> pendingRequests;
  };


private: // private methods

  int authenticateKeyboardInteractive(KIO::AuthInfo &info);

  void reportError(const KUrl &url, const int err);

  bool createUDSEntry(const QString &filename, const QByteArray &path,
                      KIO::UDSEntry &entry, short int details);

  QString canonicalizePath(const QString &path);
  void requiresUserNameRedirection();
  void clearPubKeyAuthInfo();
  bool sftpLogin();
  bool sftpOpenConnection(const KIO::AuthInfo&);
  void sftpSendWarning(int errorCode, const QString& url);

  /**
    * Status Code returned from ftpPut() and ftpGet(), used to select
    * source or destination url for error messages
    */
  typedef enum {
    Success,
    ClientError,
    ServerError
  } StatusCode;

  StatusCode sftpGet(const KUrl& url, int& errorCode, KIO::fileoffset_t offset = -1, int fd = -1);
  StatusCode sftpPut(const KUrl& url, int permissions, KIO::JobFlags flags, int& errorCode, int fd = -1);

  StatusCode sftpCopyGet(const KUrl& url, const QString& src, int permissions, KIO::JobFlags flags, int& errorCode);
  StatusCode sftpCopyPut(const KUrl& url, const QString& dest, int permissions, KIO::JobFlags flags, int& errorCode);
};

#endif
