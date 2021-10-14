/*
 * SPDX-FileCopyrightText: 2001 Lucas Fisher <ljfisher@purdue.edu>
 * SPDX-FileCopyrightText: 2009 Andreas Schneider <mail@cynapses.org>
 * SPDX-FileCopyrightText: 2020-2021 Harald Sitter <sitter@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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

/**
 * Result type for returning error context.
 *
 * This is meant to be returned by functions that do not have a simple
 * error conditions that could be represented by returning a bool, or
 * when the contextual error string can only be correctly constructed
 * inside the function. When using the Result type always mark the
 * function Q_REQUIRED_RESULT to enforce handling of the Result.
 *
 * The Result is forwarded all the way to the frontend API where it is
 * turned into an error() or finished() call.
 */
struct Result
{
    bool success;
    int error;
    QString errorString;

    Q_REQUIRED_RESULT inline static Result fail(int _error = KIO::ERR_UNKNOWN,
            const QString &_errorString = QString())
    {
        return Result { false, _error, _errorString };
    }

    Q_REQUIRED_RESULT inline static Result pass()
    {
        return Result { true, 0, QString() };
    }
};


// sftp_attributes must be freed. Use this ScopedPtr to ensure they always are!
struct ScopedPointerCustomDeleter
{
    static inline void cleanup(sftp_attributes attr)
    {
        sftp_attributes_free(attr);
    }
};
typedef QScopedPointer<sftp_attributes_struct, ScopedPointerCustomDeleter> SFTPAttributesPtr;

class SFTPSlave;

class SFTPInternal
{
public:
    explicit SFTPInternal(SFTPSlave *qptr);
    ~SFTPInternal();
    void setHost(const QString &h, quint16 port, const QString& user, const QString& pass);
    Q_REQUIRED_RESULT Result get(const QUrl &url);
    Q_REQUIRED_RESULT Result listDir(const QUrl &url);
    Q_REQUIRED_RESULT Result mimetype(const QUrl &url);
    Q_REQUIRED_RESULT Result stat(const QUrl &url);
    Q_REQUIRED_RESULT Result copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags);
    Q_REQUIRED_RESULT Result put(const QUrl &url, int permissions, KIO::JobFlags flags);
    void closeConnection();
    void slave_status();
    Q_REQUIRED_RESULT Result del(const QUrl &url, bool isfile);
    Q_REQUIRED_RESULT Result chmod(const QUrl &url, int permissions);
    Q_REQUIRED_RESULT Result symlink(const QString &target, const QUrl &dest, KIO::JobFlags flags);
    Q_REQUIRED_RESULT Result rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags);
    Q_REQUIRED_RESULT Result mkdir(const QUrl &url, int permissions);
    Q_REQUIRED_RESULT Result openConnection();

    // KIO::FileJob interface
    Q_REQUIRED_RESULT Result open(const QUrl &url, QIODevice::OpenMode mode);
    Q_REQUIRED_RESULT Result read(KIO::filesize_t size);
    Q_REQUIRED_RESULT Result write(const QByteArray &data);
    Q_REQUIRED_RESULT Result seek(KIO::filesize_t offset);
    Q_REQUIRED_RESULT Result truncate(KIO::filesize_t length);
    void close();
    Q_REQUIRED_RESULT Result special(const QByteArray &data);

    // libssh authentication callback (note that this is called by the
    // global ::auth_callback() call.
    int auth_callback(const char *prompt, char *buf, size_t len,
                      int echo, int verify, void *userdata);

    // libssh logging callback (note that this is called by the
    // global ::log_callback() call.
    void log_callback(int priority, const char *function, const char *buffer,
                      void *userdata);


    // Must call after construction!
    // Bit rubbish, but we need to return something on init.
    Q_REQUIRED_RESULT Result init();

    Q_REQUIRED_RESULT Result fileSystemFreeSpace(const QUrl &url);  // KF6 TODO: Once a virtual fileSystemFreeSpace method in SlaveBase exists, override it
private: // Private variables
    /** Fronting SlaveBase instance */
    SFTPSlave *q = nullptr;

    /** True if ioslave is connected to sftp server. */
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
    class GetRequest {
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

        sftp_file m_file;
        const uint64_t m_size; // size of file (max readable)
        ushort m_maxPendingRequests;
        QQueue<Request> m_pendingRequests;
    };

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

    /**
     * sftp_write wrapper breaking buffer into suitable pieces
     * \param onWritten acts as callback, for each written block.
     */
    Q_REQUIRED_RESULT bool sftpWrite(sftp_file fd,
                                     const QByteArray &buffer,
                                     const std::function<void(int bytes)> &onWritten);

    Q_REQUIRED_RESULT Result sftpCopyGet(const QUrl &url, const QString &src, int permissions, KIO::JobFlags flags);
    Q_REQUIRED_RESULT Result sftpCopyPut(const QUrl &url, const QString &dest, int permissions, KIO::JobFlags flags);
    Q_REQUIRED_RESULT Result sftpSendMimetype(sftp_file file, const QUrl &url);
    Q_REQUIRED_RESULT Result openConnectionWithoutCloseOnError();
};

/**
 * Fronting class.
 * The purpose of this is to separate the slave interface from the slave logic and force state
 * convergence.
 * Specifically logic code must not call finalization API error()/finished() but instead move
 * a single Result object up the call chain. This is to prevent overwriting errors and/or finished
 * finality states and broken-state situations those can cause.
 */
class SFTPSlave : public KIO::SlaveBase
{
public:
    SFTPSlave(const QByteArray &pool_socket, const QByteArray &app_socket);
    ~SFTPSlave() override = default;
    void setHost(const QString &host, quint16 port, const QString &user, const QString &pass) override;
    void get(const QUrl &url) override;
    void listDir(const QUrl &url) override ;
    void mimetype(const QUrl &url) override;
    void stat(const QUrl &url) override;
    void copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags) override;
    void put(const QUrl &url, int permissions, KIO::JobFlags flags) override;
    void slave_status() override;
    void del(const QUrl &url, bool isfile) override;
    void chmod(const QUrl &url, int permissions) override;
    void symlink(const QString &target, const QUrl &dest, KIO::JobFlags flags) override;
    void rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;
    void mkdir(const QUrl &url, int permissions) override;
    void openConnection() override;
    void closeConnection() override;

    // KIO::FileJob interface
    void open(const QUrl &url, QIODevice::OpenMode mode) override;
    void read(KIO::filesize_t size) override;
    void write(const QByteArray &data) override;
    void seek(KIO::filesize_t offset) override;
    void truncate(KIO::filesize_t length);
    void close() override;
    void special(const QByteArray &data) override;
    void virtual_hook(int id, void *data) override;

private:
    // WARNING: All members and all logic not confined to one of the public functions
    //   must go into SftpInternal!

    /**
     * Overridden to prevent SftpInternal from easily calling
     * q->opened(). Use a Result return type on error conditions
     * instead. When there was no error Result the
     * connection is considered opened.
     *
     * SftpInternal must not call any state-changing signals!
     */
    void opened()
    {
        SlaveBase::opened();
    }

    /**
     * @see opened()
     */
    void error(int _errid, const QString &_text)
    {
        SlaveBase::error(_errid, _text);
    }

    /**
     * @see opened()
     */
    void finished()
    {
        SlaveBase::finished();
    }

    /**
     * Calls finished() or error() as appropriate
     */
    void finalize(const Result &result);

    /**
     * Calls error() if and only if the result is an error
     */
    void maybeError(const Result &result);

    QScopedPointer<SFTPInternal> d { new SFTPInternal(this) };
};

#endif
