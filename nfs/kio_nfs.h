/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>
    SPDX-FileCopyrightText: 2014 Mathias Tillman <master.homer@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KIO_NFS_H
#define KIO_NFS_H

#include <KConfigGroup>
#include <KIO/Global>
#include <KIO/WorkerBase>

#include <QHash>
#include <QMap>
#include <QString>
#include <QStringList>

#include <memory>

#include "rpc_nfs3_prot.h"

class NFSProtocol;

class NFSWorker : public QObject, public KIO::WorkerBase
{
    Q_OBJECT

public:
    NFSWorker(const QByteArray &pool, const QByteArray &app);
    ~NFSWorker() override;

    KIO::WorkerResult openConnection() override;
    void closeConnection() override;

    void setHost(const QString &host, quint16 port, const QString &user, const QString &pass) override;

    KIO::WorkerResult put(const QUrl &url, int _mode, KIO::JobFlags _flags) override;
    KIO::WorkerResult get(const QUrl &url) override;
    KIO::WorkerResult listDir(const QUrl &url) override;
    KIO::WorkerResult symlink(const QString &target, const QUrl &dest, KIO::JobFlags) override;
    KIO::WorkerResult stat(const QUrl &url) override;
    KIO::WorkerResult mkdir(const QUrl &url, int permissions) override;
    KIO::WorkerResult del(const QUrl &url, bool isfile) override;
    KIO::WorkerResult chmod(const QUrl &url, int permissions) override;
    KIO::WorkerResult rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;
    KIO::WorkerResult copy(const QUrl &src, const QUrl &dest, int mode, KIO::JobFlags flags) override;

    bool usedirplus3() const
    {
        return (m_usedirplus3);
    }

protected:
    // Verifies the URL, current protocol and connection state
    KIO::WorkerResult verifyProtocol(const QUrl &url);

private:
    std::unique_ptr<NFSProtocol> m_protocol;

    // We need to cache these because the @openConnection call is responsible
    // for creating the protocol, and the @setHost call might happen before that.
    QString m_host;
    QString m_user;

    bool m_usedirplus3;

    KIO::Error m_errorId;
    QString m_errorText;
};

class NFSFileHandle
{
public:
    NFSFileHandle() = default;
    NFSFileHandle(const NFSFileHandle &handle);
    NFSFileHandle(const fhandle3 &src);
    NFSFileHandle(const nfs_fh3 &src);
    ~NFSFileHandle();

    // Copies the handle data to an nfs file handle
    void toFH(nfs_fh3 &fh) const;

    // Copies the source link handle data to an nfs file handle
    void toFHLink(nfs_fh3 &fh) const;

    NFSFileHandle &operator=(const NFSFileHandle &src);
    NFSFileHandle &operator=(const fhandle3 &src);
    NFSFileHandle &operator=(const nfs_fh3 &src);

    bool isInvalid() const
    {
        return m_size == 0 && m_linkSize == 0;
    }
    bool isLink() const
    {
        return m_isLink;
    }
    bool isBadLink() const
    {
        return (m_isLink && m_linkSize == 0);
    }

    void setLinkSource(const nfs_fh3 &src);
    void setBadLink()
    {
        m_isLink = true;
        m_linkSize = 0;
    }

protected:
    char *m_handle = nullptr;
    unsigned int m_size = 0u;

    // Set to the link source's handle.
    char *m_linkHandle = nullptr;
    unsigned int m_linkSize = 0u;
    bool m_isLink = false;
};

// MaybeNFSFileHandle: either a file handle or a worker result to pass along
// (ideally, this should be an expected<>, but we have neither tl:: nor std::expected available)
using MaybeNFSFileHandle = std::variant<KIO::WorkerResult, NFSFileHandle>;

inline bool handleIsValid(const MaybeNFSFileHandle &h)
{
    return std::holds_alternative<NFSFileHandle>(h);
}

inline NFSFileHandle &getHandle(MaybeNFSFileHandle &h)
{
    return std::get<NFSFileHandle>(h);
}

inline const NFSFileHandle &getHandle(const MaybeNFSFileHandle &h)
{
    return std::get<NFSFileHandle>(h);
}

inline KIO::WorkerResult getHandleError(const MaybeNFSFileHandle &h)
{
    return std::get<KIO::WorkerResult>(h);
}

typedef QMap<QString, NFSFileHandle> NFSFileHandleMap;

class NFSProtocol
{
public:
    explicit NFSProtocol(NFSWorker *worker);
    virtual ~NFSProtocol()
    {
    }

    virtual bool isCompatible(bool &connectionError) = 0;
    virtual bool isConnected() const = 0;

    virtual KIO::WorkerResult openConnection() = 0;
    virtual void closeConnection() = 0;

    virtual KIO::WorkerResult setHost(const QString &host, const QString &user = QString());

    virtual KIO::WorkerResult put(const QUrl &url, int _mode, KIO::JobFlags _flags) = 0;
    virtual KIO::WorkerResult get(const QUrl &url) = 0;
    virtual KIO::WorkerResult listDir(const QUrl &url) = 0;
    virtual KIO::WorkerResult symlink(const QString &target, const QUrl &dest, KIO::JobFlags) = 0;
    virtual KIO::WorkerResult stat(const QUrl &url) = 0;
    virtual KIO::WorkerResult mkdir(const QUrl &url, int permissions) = 0;
    virtual KIO::WorkerResult del(const QUrl &url, bool isfile) = 0;
    virtual KIO::WorkerResult chmod(const QUrl &url, int permissions) = 0;
    virtual KIO::WorkerResult rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) = 0;

    KIO::WorkerResult copy(const QUrl &src, const QUrl &dest, int mode, KIO::JobFlags flags);

protected:
    // Copy from NFS to NFS
    virtual KIO::WorkerResult copySame(const QUrl &src, const QUrl &dest, int mode, KIO::JobFlags flags) = 0;
    // Copy from NFS to local
    virtual KIO::WorkerResult copyFrom(const QUrl &src, const QUrl &dest, int mode, KIO::JobFlags flags) = 0;
    // Copy from local to NFS
    virtual KIO::WorkerResult copyTo(const QUrl &src, const QUrl &dest, int mode, KIO::JobFlags flags) = 0;

    // Look up a file handle
    virtual std::optional<NFSFileHandle> lookupFileHandle(const QString &path) = 0;

    // Modify the exported dirs.
    void addExportedDir(const QString &path);
    const QStringList &getExportedDirs();
    bool isExportedDir(const QString &path);
    void removeExportedDir(const QString &path);

    // File handle cache functions.
    void addFileHandle(const QString &path, NFSFileHandle fh);
    MaybeNFSFileHandle getFileHandle(const QString &path);
    void removeFileHandle(const QString &path);

    // Make sure that the path is actually a part of an nfs share.
    bool isValidPath(const QString &path);
    KIO::WorkerResult validLinkResult(const QString &parentDir, const QString &linkDest);

    KIO::Error openConnection(const QString &host, int prog, int vers, CLIENT *&client, int &sock);

    KIO::WorkerResult checkResult(int clientStat, int nfsStat, const QString &text);

    void createVirtualDirEntry(KIO::UDSEntry &entry);

    QString listDirInternal(const QUrl &url);
    QString statInternal(const QUrl &url);
    void completeUDSEntry(KIO::UDSEntry &entry, uid_t uid, gid_t gid);
    void completeInvalidUDSEntry(KIO::UDSEntry &entry);

    QString currentHost() const
    {
        return (m_currentHost);
    }
    NFSWorker *worker() const
    {
        return (m_worker);
    }

private:
    NFSWorker *m_worker;
    QString m_currentHost;
    QString m_currentUser;

    NFSFileHandleMap m_handleCache;
    QStringList m_exportedDirs;

    QHash<uid_t, QString> m_usercache;
    QHash<gid_t, QString> m_groupcache;
};

#endif
