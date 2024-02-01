/*
 * SPDX-FileCopyrightText: 2001 Lucas Fisher <ljfisher@purdue.edu>
 * SPDX-FileCopyrightText: 2009 Andreas Schneider <mail@cynapses.org>
 * SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef __kio_sftp_h__
#define __kio_sftp_h__

#include <KIO/Global>
#include <KIO/WorkerBase>

#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <QQueue>
#include <QUrl>

#include <QCoroGenerator>

namespace KIO
{
class AuthInfo;
} // namespace KIO

using Result = KIO::WorkerResult;

namespace std
{
template<>
struct default_delete<struct sftp_attributes_struct> {
    void operator()(struct sftp_attributes_struct *ptr) const
    {
        sftp_attributes_free(ptr);
    }
};
} // namespace std

using SFTPAttributesPtr = std::unique_ptr<sftp_attributes_struct>;

class SFTPWorker : public KIO::WorkerBase
{
public:
    explicit SFTPWorker(const QByteArray &poolSocket, const QByteArray &appSocket);
    ~SFTPWorker() override;
    Q_DISABLE_COPY_MOVE(SFTPWorker)

    void setHost(const QString &h, quint16 port, const QString &user, const QString &pass) override;
    Q_REQUIRED_RESULT Result get(const QUrl &url) override;
    Q_REQUIRED_RESULT Result listDir(const QUrl &url) override;
    Q_REQUIRED_RESULT Result mimetype(const QUrl &url) override;
    Q_REQUIRED_RESULT Result stat(const QUrl &url) override;
    Q_REQUIRED_RESULT Result copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags) override;
    Q_REQUIRED_RESULT Result put(const QUrl &url, int permissions, KIO::JobFlags flags) override;
    void closeConnection() override;
    void worker_status() override;
    Q_REQUIRED_RESULT Result del(const QUrl &url, bool isfile) override;
    Q_REQUIRED_RESULT Result chmod(const QUrl &url, int permissions) override;
    Q_REQUIRED_RESULT Result symlink(const QString &target, const QUrl &dest, KIO::JobFlags flags) override;
    Q_REQUIRED_RESULT Result rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;
    Q_REQUIRED_RESULT Result mkdir(const QUrl &url, int permissions) override;
    Q_REQUIRED_RESULT Result openConnection() override;

    // KIO::FileJob interface
    Q_REQUIRED_RESULT Result open(const QUrl &url, QIODevice::OpenMode mode) override;
    Q_REQUIRED_RESULT Result read(KIO::filesize_t size) override;
    Q_REQUIRED_RESULT Result write(const QByteArray &data) override;
    Q_REQUIRED_RESULT Result seek(KIO::filesize_t offset) override;
    Q_REQUIRED_RESULT Result truncate(KIO::filesize_t length) override;
    Q_REQUIRED_RESULT Result close() override;
    Q_REQUIRED_RESULT Result special(const QByteArray &data) override;

    // libssh authentication callback (note that this is called by the
    // global ::auth_callback() call.
    int auth_callback(const char *prompt, char *buf, size_t len, int echo, int verify, void *userdata);

    // libssh logging callback (note that this is called by the
    // global ::log_callback() call.
    void log_callback(int priority, const char *function, const char *buffer, void *userdata);

    // Must call after construction!
    // Bit rubbish, but we need to return something on init.
    Q_REQUIRED_RESULT Result init();

    Q_REQUIRED_RESULT Result fileSystemFreeSpace(const QUrl &url) override;

private: // Private variables
    /** True if worker is connected to sftp server. */
    bool mConnected = false;

    /** Host we are connected to. */
    QString mHost;

    /** Port we are connected to. */
    int mPort = -1;

    /** The ssh session for the connection */
    ssh_session mSession = nullptr;

    /** The sftp session for the connection */
    sftp_session mSftp = nullptr;

    /** Username to use when connecting */
    QString mUsername;

    /** User's password */
    QString mPassword;

    /** The open file */
    sftp_file mOpenFile = nullptr;

    /** The open URL */
    QUrl mOpenUrl;

    ssh_callbacks mCallbacks = nullptr;

    // KIO::FileJob interface
    KIO::filesize_t openOffset = 0;

    /**
     * Holds public key authentication info for proper retry handling.
     */
    KIO::AuthInfo *mPublicKeyAuthInfo = nullptr;

    /**
     * GetRequest encapsulates several SFTP get requests into a single object.
     * As SFTP messages are limited to MAX_XFER_BUF_SIZE several requests
     * should be sent simultaneously in order to increase transfer speeds.
     */
    class GetRequest
    {
    public:
        /**
         * Creates a new GetRequest object.
         * Requests do not take ownership of the SFTP pointers! The caller is
         * responsible for freeing them.
         * @param file the sftp_file object which should be transferred.
         * @param size the total size of the file.
         * @param maxPendingRequests the maximum number of parallel requests to start with.
         *                           The more are pending the higher the potential memory
         *                           foot print, however if the connection allows it
         *                           we'll get better throughput.
         */
        GetRequest(sftp_file file, uint64_t size, ushort maxPendingRequests = 128);
        /**
         * Removes all pending requests and closes the SFTP channel and attributes
         * in order to avoid memory leaks.
         */
        ~GetRequest();
        Q_DISABLE_COPY_MOVE(GetRequest)

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
        size_t readChunks(QByteArray &data);

    private:
        struct Request {
            /** Identifier as returned by the sftp_async_read_begin call */
            int id;
            /** The number of bytes expected to be returned */
            uint32_t expectedLength;
            /** The SSH start offset when this request was made */
            uint64_t startOffset;
        };

        sftp_file m_file;
        const uint64_t m_size; // size of file (max readable)
        ushort m_maxPendingRequests;
        QQueue<Request> m_pendingRequests;
    };

    struct ReadResponse {
        // NOTE: older GCC versions have trouble when using structs that were aggregate initialized and crash on
        // double-destruction problems. If you want to get rid of the ctors make double sure that reasonable GCC
        // versions are fine.
        ReadResponse() = default;
        explicit ReadResponse(const QByteArray &filedata_)
            : filedata(filedata_)
        {
        }
        explicit ReadResponse(int error_)
            : error(error_)
        {
        }

        QByteArray filedata;
        int error = KJob::NoError;
    };
    QCoro::Generator<ReadResponse> asyncRead(sftp_file file, size_t size);

    struct WriteResponse {
        size_t bytes = 0;
        int error = KJob::NoError;
    };
    QCoro::Generator<WriteResponse> asyncWrite(sftp_file file, QCoro::Generator<ReadResponse> reader);

private: // private methods
    int authenticateKeyboardInteractive(KIO::AuthInfo &info);

    Q_REQUIRED_RESULT Result reportError(const QUrl &url, const int err);

    Q_REQUIRED_RESULT Result createUDSEntry(SFTPAttributesPtr sb, KIO::UDSEntry &entry, const QByteArray &path, const QString &name, int details);

    QString canonicalizePath(const QString &path);
    void requiresUserNameRedirection();
    void clearPubKeyAuthInfo();
    Q_REQUIRED_RESULT Result sftpLogin();
    Q_REQUIRED_RESULT Result sftpOpenConnection(const KIO::AuthInfo &);

    Q_REQUIRED_RESULT Result sftpGet(const QUrl &url, KIO::fileoffset_t offset = -1, int fd = -1);
    Q_REQUIRED_RESULT Result sftpPut(const QUrl &url, int permissions, KIO::JobFlags flags, int fd = -1);

    Q_REQUIRED_RESULT Result sftpCopyGet(const QUrl &url, const QString &src, int permissions, KIO::JobFlags flags);
    Q_REQUIRED_RESULT Result sftpCopyPut(const QUrl &url, const QString &dest, int permissions, KIO::JobFlags flags);
    Q_REQUIRED_RESULT Result sftpSendMimetype(sftp_file file, const QUrl &url);
    Q_REQUIRED_RESULT Result openConnectionWithoutCloseOnError();
};

#endif
