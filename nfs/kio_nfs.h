/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>
    SPDX-FileCopyrightText: 2014 Mathias Tillman <master.homer@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KIO_NFS_H
#define KIO_NFS_H

#include <kio/slavebase.h>
#include <kio/global.h>
#include <kconfiggroup.h>

#include <QHash>
#include <QMap>
#include <QString>
#include <QStringList>

#include "rpc_nfs2_prot.h"
#include "rpc_nfs3_prot.h"

class NFSProtocol;

class NFSSlave : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:
    NFSSlave(const QByteArray& pool, const QByteArray& app);
    ~NFSSlave() override;

    void openConnection() override;
    void closeConnection() override;

    void setHost(const QString& host, quint16 port, const QString& user, const QString& pass) override;

    void put(const QUrl& url, int _mode, KIO::JobFlags _flags) override;
    void get(const QUrl& url) override;
    void listDir(const QUrl& url) override;
    void symlink(const QString& target, const QUrl& dest, KIO::JobFlags) override;
    void stat(const QUrl& url) override;
    void mkdir(const QUrl& url, int permissions) override;
    void del(const QUrl& url, bool isfile) override;
    void chmod(const QUrl& url, int permissions) override;
    void rename(const QUrl& src, const QUrl& dest, KIO::JobFlags flags) override;
    void copy(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags) override;

    void setError(KIO::Error errid, const QString &text);
    // NFSProtocol should not call these.  See setError() and finishOperation().
    void finished() = delete;
    void error(int errid, const QString &text) = delete;

    bool usedirplus3() const					{ return (m_usedirplus3); }

protected:
    // Verifies the URL, current protocol and connection state, returns true if valid.
    bool verifyProtocol(const QUrl &url);
    void finishOperation();

private:
    NFSProtocol* m_protocol;

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
    NFSFileHandle();
    NFSFileHandle(const NFSFileHandle& handle);
    NFSFileHandle(const fhandle3& src);
    NFSFileHandle(const fhandle& src);
    NFSFileHandle(const nfs_fh3& src);
    NFSFileHandle(const nfs_fh& src);
    ~NFSFileHandle();

    // Copies the handle data to an nfs file handle
    void toFH(nfs_fh3& fh) const;
    void toFH(nfs_fh& fh) const;

    // Copies the source link handle data to an nfs file handle
    void toFHLink(nfs_fh3& fh) const;
    void toFHLink(nfs_fh& fh) const;

    NFSFileHandle& operator=(const NFSFileHandle& src);
    NFSFileHandle& operator=(const fhandle3& src);
    NFSFileHandle& operator=(const fhandle& src);
    NFSFileHandle& operator=(const nfs_fh3& src);
    NFSFileHandle& operator=(const nfs_fh& src);

    bool isInvalid() const
    {
        return m_size==0 && m_linkSize==0;
    }
    bool isLink() const
    {
        return m_isLink;
    }
    bool isBadLink() const
    {
        return (m_isLink && m_linkSize == 0);
    }

    void setLinkSource(const nfs_fh3& src);
    void setLinkSource(const nfs_fh& src);
    void setBadLink()
    {
        m_isLink = true;
        m_linkSize = 0;
    }

protected:
    char* m_handle;
    unsigned int m_size;

    // Set to the link source's handle.
    char* m_linkHandle;
    unsigned int m_linkSize;
    bool m_isLink;
};

typedef QMap<QString, NFSFileHandle> NFSFileHandleMap;


class NFSProtocol
{
public:
    explicit NFSProtocol(NFSSlave* slave);
    virtual ~NFSProtocol() {}

    virtual bool isCompatible(bool &connectionError) = 0;
    virtual bool isConnected() const = 0;

    virtual void openConnection() = 0;
    virtual void closeConnection() = 0;

    virtual void setHost(const QString &host, const QString &user = QString());

    virtual void put(const QUrl& url, int _mode, KIO::JobFlags _flags) = 0;
    virtual void get(const QUrl& url) = 0;
    virtual void listDir(const QUrl& url) = 0;
    virtual void symlink(const QString& target, const QUrl& dest, KIO::JobFlags) = 0;
    virtual void stat(const QUrl& url) = 0;
    virtual void mkdir(const QUrl& url, int permissions) = 0;
    virtual void del(const QUrl& url, bool isfile) = 0;
    virtual void chmod(const QUrl& url, int permissions) = 0;
    virtual void rename(const QUrl& src, const QUrl& dest, KIO::JobFlags flags) = 0;

    void copy(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags);

protected:
    // Copy from NFS to NFS
    virtual void copySame(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags) = 0;
    // Copy from NFS to local
    virtual void copyFrom(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags) = 0;
    // Copy from local to NFS
    virtual void copyTo(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags) = 0;

    // Look up a file handle
    virtual NFSFileHandle lookupFileHandle(const QString& path) = 0;

    // Modify the exported dirs.
    void addExportedDir(const QString& path);
    const QStringList& getExportedDirs();
    bool isExportedDir(const QString& path);
    void removeExportedDir(const QString& path);

    // File handle cache functions.
    void addFileHandle(const QString& path, NFSFileHandle fh);
    NFSFileHandle getFileHandle(const QString& path);
    void removeFileHandle(const QString& path);

    // Make sure that the path is actually a part of an nfs share.
    bool isValidPath(const QString& path);
    bool isValidLink(const QString& parentDir, const QString& linkDest);

    KIO::Error openConnection(const QString& host, int prog, int vers, CLIENT*& client, int& sock);

    bool checkForError(int clientStat, int nfsStat, const QString& text);

    void createVirtualDirEntry(KIO::UDSEntry& entry);

    QString listDirInternal(const QUrl &url);
    QString statInternal(const QUrl &url);
    void completeUDSEntry(KIO::UDSEntry &entry, uid_t uid, gid_t gid);
    void completeInvalidUDSEntry(KIO::UDSEntry &entry);

    QString currentHost() const					{ return (m_currentHost); }
    NFSSlave *slave() const					{ return (m_slave); }
    void setError(KIO::Error errid, const QString &text)	{ m_slave->setError(errid, text); }

private:
    NFSSlave* m_slave;
    QString m_currentHost;
    QString m_currentUser;

    NFSFileHandleMap m_handleCache;
    QStringList m_exportedDirs;

    QHash<uid_t, QString> m_usercache;
    QHash<gid_t, QString> m_groupcache;
};

#endif
