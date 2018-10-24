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

#include <kio/global.h>
#include <kio/slavebase.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <libssh/callbacks.h>

#include <QQueue>

namespace KIO {
class AuthInfo;
}

class sftpProtocol : public KIO::SlaveBase
{
public:
    sftpProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
    ~sftpProtocol() override;
    void setHost(const QString &h, quint16 port, const QString& user, const QString& pass) override;
    void get(const QUrl &url) override;
    void listDir(const QUrl &url) override ;
    void mimetype(const QUrl &url) override;
    void stat(const QUrl &url) override;
    void copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags) override;
    void put(const QUrl &url, int permissions, KIO::JobFlags flags) override;
    void closeConnection() override;
    void slave_status() override;
    void del(const QUrl &url, bool isfile) override;
    void chmod(const QUrl &url, int permissions) override;
    void symlink(const QString &target, const QUrl &dest, KIO::JobFlags flags) override;
    void rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;
    void mkdir(const QUrl &url, int permissions) override;
    void openConnection() override;

    // KIO::FileJob interface
    void open(const QUrl &url, QIODevice::OpenMode mode) override;
    void read(KIO::filesize_t size) override;
    void write(const QByteArray &data) override;
    void seek(KIO::filesize_t offset) override;
    void close() override;
    void special(const QByteArray &data) override;

    // libssh authentication callback (note that this is called by the
    // global ::auth_callback() call.
    int auth_callback(const char *prompt, char *buf, size_t len,
                      int echo, int verify, void *userdata);

    // libssh logging callback (note that this is called by the
    // global ::log_callback() call.
    void log_callback(int priority, const char *function, const char *buffer,
                      void *userdata);

protected:
    void virtual_hook(int id, void *data) override;

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
    QUrl mOpenUrl;

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
    QUrl openUrl;
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
         *                           The more are pending the higher the potential memory
         *                           foot print, however if the connection allows it
         *                           we'll get better throughput.
         */
        GetRequest(sftp_file file, sftp_attributes sb, ushort maxPendingRequests = 128);
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
         * Attempts to read all pending chunks in the given QByteArray.
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

        sftp_file mFile;
        sftp_attributes mSb;
        ushort mMaxPendingRequests;
        QQueue<Request> pendingRequests;
    };

private: // private methods
    int authenticateKeyboardInteractive(KIO::AuthInfo &info);

    void reportError(const QUrl &url, const int err);

    bool createUDSEntry(const QString &filename, const QByteArray &path,
                        KIO::UDSEntry &entry, short int details);

    QString canonicalizePath(const QString &path);
    void requiresUserNameRedirection();
    void clearPubKeyAuthInfo();
    bool sftpLogin();
    bool sftpOpenConnection(const KIO::AuthInfo&);
    void sftpSendWarning(int errorCode, const QString& url);

    // Close without error() or finish() call (in case of errors for example)
    void closeWithoutFinish();

    /**
    * Status Code returned from ftpPut() and ftpGet(), used to select
    * source or destination url for error messages
    */
    typedef enum {
        Success,
        ClientError,
        ServerError
    } StatusCode;

    StatusCode sftpGet(const QUrl& url, int& errorCode, KIO::fileoffset_t offset = -1, int fd = -1);
    StatusCode sftpPut(const QUrl& url, int permissions, KIO::JobFlags flags, int& errorCode, int fd = -1);

    StatusCode sftpCopyGet(const QUrl& url, const QString& src, int permissions, KIO::JobFlags flags, int& errorCode);
    StatusCode sftpCopyPut(const QUrl& url, const QString& dest, int permissions, KIO::JobFlags flags, int& errorCode);

    void fileSystemFreeSpace(const QUrl& url);  // KF6 TODO: Once a virtual fileSystemFreeSpace method in SlaveBase exists, override it
};

#endif
