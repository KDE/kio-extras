/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>,
        2014 Mathias Tillman <master.homer@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KIO_NFS_H
#define KIO_NFS_H

#include <kio/slavebase.h>
#include <kio/global.h>
#include <kconfiggroup.h>

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QLoggingCategory>

#include "rpc_nfs2_prot.h"
#include "rpc_nfs3_prot.h"

Q_DECLARE_LOGGING_CATEGORY(LOG_KIO_NFS);

class NFSProtocol;

class NFSSlave : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:
    NFSSlave(const QByteArray& pool, const QByteArray& app);
    ~NFSSlave();

    void openConnection() Q_DECL_OVERRIDE;
    void closeConnection() Q_DECL_OVERRIDE;

    void setHost(const QString& host, quint16 port, const QString& user, const QString& pass) Q_DECL_OVERRIDE;

    void put(const QUrl& url, int _mode, KIO::JobFlags _flags) Q_DECL_OVERRIDE;
    void get(const QUrl& url) Q_DECL_OVERRIDE;
    void listDir(const QUrl& url) Q_DECL_OVERRIDE;
    void symlink(const QString& target, const QUrl& dest, KIO::JobFlags) Q_DECL_OVERRIDE;
    void stat(const QUrl& url) Q_DECL_OVERRIDE;
    void mkdir(const QUrl& url, int permissions) Q_DECL_OVERRIDE;
    void del(const QUrl& url, bool isfile) Q_DECL_OVERRIDE;
    void chmod(const QUrl& url, int permissions) Q_DECL_OVERRIDE;
    void rename(const QUrl& src, const QUrl& dest, KIO::JobFlags flags) Q_DECL_OVERRIDE;
    void copy(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags) Q_DECL_OVERRIDE;

protected:
    // Verifies the current protocol and connection state, returns true if valid.
    bool verifyProtocol();

private:
    NFSProtocol* m_protocol;

    // We need to cache this because the @openConnection call is responsible
    // for creating the protocol, and the @setHost call might happen before that.
    QString m_host;
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
        return m_isInvalid;
    }
    void setInvalid()
    {
        m_isInvalid = true;
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

    bool m_isInvalid;
    bool m_isLink;
};

typedef QMap<QString, NFSFileHandle> NFSFileHandleMap;

class NFSProtocol
{
public:
    NFSProtocol(NFSSlave* slave);
    virtual ~NFSProtocol() {}

    virtual bool isCompatible(bool &connectionError) = 0;
    virtual bool isConnected() const = 0;

    virtual void openConnection() = 0;
    virtual void closeConnection() = 0;

    virtual void setHost(const QString& host) = 0;

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

    int openConnection(const QString& host, int prog, int vers, CLIENT*& client, int& sock);

    bool checkForError(int clientStat, int nfsStat, const QString& text);

    void createVirtualDirEntry(KIO::UDSEntry& entry);

private:
    NFSSlave* m_slave;

    NFSFileHandleMap m_handleCache;
    QStringList m_exportedDirs;
};

#endif
