/*  This file is part of the KDE project

    Copyright(C) 2000 Alexander Neundorf <neundorf@kde.org>,
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

#include "nfsv3.h"

#include <config-runtime.h>

#include <arpa/inet.h>

// This is needed on Solaris so that rpc.h defines clnttcp_create etc.
#ifndef PORTMAP
#define PORTMAP
#endif
#include <rpc/rpc.h> // for rpc calls

#include <errno.h>
#include <grp.h>
#include <memory.h>
#include <netdb.h>
#include <pwd.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QLoggingCategory>
#include <QMimeType>
#include <QMimeDatabase>

#include <KLocalizedString>
#include <kio/global.h>
#include <kio/ioslave_defaults.h>

// This ioslave is for NFS version 3.
#define NFSPROG 100003UL
#define NFSVERS 3UL

#define NFS3_MAXDATA    32768
#define NFS3_MAXPATHLEN PATH_MAX


NFSProtocolV3::NFSProtocolV3(NFSSlave* slave)
    : NFSProtocol(slave),
      m_slave(slave),
      m_mountClient(nullptr),
      m_mountSock(-1),
      m_nfsClient(nullptr),
      m_nfsSock(-1),
      m_readBufferSize(0),
      m_writeBufferSize(0),
      m_readDirSize(0)
{
    qCDebug(LOG_KIO_NFS) << "NFS3::NFS3";

    clnt_timeout.tv_sec = 20;
    clnt_timeout.tv_usec = 0;
}

NFSProtocolV3::~NFSProtocolV3()
{
    closeConnection();
}

bool NFSProtocolV3::isCompatible(bool& connectionError)
{
    qCDebug(LOG_KIO_NFS);

    int ret = -1;

    CLIENT* client = nullptr;
    int sock = 0;
    if (NFSProtocol::openConnection(m_currentHost, NFSPROG, NFSVERS, client, sock) == 0) {
        timeval check_timeout;
        check_timeout.tv_sec = 20;
        check_timeout.tv_usec = 0;

        // Check if the NFS version is compatible
        ret = clnt_call(client, NFSPROC3_NULL,
                        (xdrproc_t) xdr_void, nullptr,
                        (xdrproc_t) xdr_void, nullptr,
                        check_timeout);

        connectionError = false;
    } else {
        qCDebug(LOG_KIO_NFS) << "openConnection failed";
        connectionError = true;
    }

    if (sock != -1) {
        ::close(sock);
    }

    if (client != nullptr) {
        CLNT_DESTROY(client);
    }

    qCDebug(LOG_KIO_NFS) << ret;

    return (ret == RPC_SUCCESS);
}

bool NFSProtocolV3::isConnected() const
{
    return (m_nfsClient != nullptr);
}

void NFSProtocolV3::closeConnection()
{
    qCDebug(LOG_KIO_NFS);

    // Unmount all exported dirs(if any)
    if (m_mountClient != nullptr) {
        clnt_call(m_mountClient, MOUNTPROC3_UMNTALL,
                  (xdrproc_t) xdr_void, nullptr,
                  (xdrproc_t) xdr_void, nullptr,
                  clnt_timeout);
    }

    if (m_mountSock >= 0) {
        ::close(m_mountSock);
        m_mountSock = -1;
    }
    if (m_nfsSock >= 0) {
        ::close(m_nfsSock);
        m_nfsSock = -1;
    }

    if (m_mountClient != nullptr) {
        CLNT_DESTROY(m_mountClient);
        m_mountClient = nullptr;
    }
    if (m_nfsClient != nullptr) {
        CLNT_DESTROY(m_nfsClient);
        m_nfsClient = nullptr;
    }
}

NFSFileHandle NFSProtocolV3::lookupFileHandle(const QString& path)
{
    int rpcStatus;
    LOOKUP3res res;
    if (lookupHandle(path, rpcStatus, res)) {
        NFSFileHandle fh = res.LOOKUP3res_u.resok.object;

        // Is it a link? Get the link target.
        if (res.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.type == NF3LNK) {
            READLINK3args readLinkArgs;
            memset(&readLinkArgs, 0, sizeof(readLinkArgs));
            fh.toFH(readLinkArgs.symlink);

            char dataBuffer[NFS3_MAXPATHLEN];

            READLINK3res readLinkRes;
            memset(&readLinkRes, 0, sizeof(readLinkRes));
            readLinkRes.READLINK3res_u.resok.data = dataBuffer;

            int rpcStatus = clnt_call(m_nfsClient, NFSPROC3_READLINK,
                                      (xdrproc_t) xdr_READLINK3args, reinterpret_cast<caddr_t>(&readLinkArgs),
                                      (xdrproc_t) xdr_READLINK3res, reinterpret_cast<caddr_t>(&readLinkRes),
                                      clnt_timeout);

            if (rpcStatus == RPC_SUCCESS && readLinkRes.status == NFS3_OK) {
                const QString linkDest = QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data);
                QString linkPath;
                if (QFileInfo(linkDest).isAbsolute()) {
                    linkPath = linkDest;
                } else {
                    linkPath = QFileInfo(QFileInfo(path).path(), linkDest).absoluteFilePath();
                }

                LOOKUP3res linkRes;
                if (lookupHandle(linkPath, rpcStatus, linkRes)) {
                    // It's a link, so return the target file handle, and add the link source to it.
                    NFSFileHandle linkFh = linkRes.LOOKUP3res_u.resok.object;
                    linkFh.setLinkSource(res.LOOKUP3res_u.resok.object);

                    qCDebug(LOG_KIO_NFS) << "Found target -" << linkPath;

                    return linkFh;
                }
            }

            // If we have reached this point the file is a link, but we failed to get the target.
            fh.setBadLink();
            qCDebug(LOG_KIO_NFS) << path << "is an invalid link!!";
        }

        return fh;
    }

    return NFSFileHandle();
}

/* Open connection connects to the mount daemon on the server side.
 In order to do this it needs authentication and calls auth_unix_create().
 Then it asks the mount daemon for the exported shares. Then it tries
 to mount all these shares. If this succeeded for at least one of them,
 a client for the nfs daemon is created.
 */
void NFSProtocolV3::openConnection()
{
    qCDebug(LOG_KIO_NFS) << m_currentHost;

    // Destroy the old connection first
    closeConnection();

    int connErr;
    if ((connErr = NFSProtocol::openConnection(m_currentHost, MOUNT_PROGRAM, MOUNT_V3, m_mountClient, m_mountSock)) != 0) {
        closeConnection();
        m_slave->error(connErr, m_currentHost);
        return;
    }

    exports3 exportlist;
    memset(&exportlist, 0, sizeof(exportlist));

    int clnt_stat = clnt_call(m_mountClient, MOUNTPROC3_EXPORT,
                              (xdrproc_t) xdr_void, nullptr,
                              (xdrproc_t) xdr_exports3, reinterpret_cast<caddr_t>(&exportlist),
                              clnt_timeout);

    if (!checkForError(clnt_stat, 0, m_currentHost.toLatin1())) {
        closeConnection();
        return;
    }

    int exportsCount = 0;
    QStringList failList;

    mountres3 fhStatus;
    for (; exportlist != nullptr; exportlist = exportlist->ex_next, exportsCount++) {
        memset(&fhStatus, 0, sizeof(fhStatus));
        clnt_stat = clnt_call(m_mountClient, MOUNTPROC3_MNT,
                              (xdrproc_t) xdr_dirpath3, reinterpret_cast<caddr_t>(&exportlist->ex_dir),
                              (xdrproc_t) xdr_mountres3, reinterpret_cast<caddr_t>(&fhStatus),
                              clnt_timeout);

        if (fhStatus.fhs_status == 0) {
            QString fname = QFileInfo(QDir("/"), exportlist->ex_dir).filePath();

            // Check if the dir is already exported
            if (NFSProtocol::isExportedDir(fname)) {
                continue;
            }

            addFileHandle(fname, static_cast<NFSFileHandle>(fhStatus.mountres3_u.mountinfo.fhandle));
            addExportedDir(fname);
        } else {
            failList.append(exportlist->ex_dir);
        }
    }
    if (failList.size() > 0) {
        m_slave->error(KIO::ERR_CANNOT_MOUNT, i18n("Failed to mount %1", failList.join(", ")));

        // All exports failed to mount, fail
        if (failList.size() == exportsCount) {
            closeConnection();
            return;
        }
    }

    if ((connErr = NFSProtocol::openConnection(m_currentHost, NFSPROG, NFSVERS, m_nfsClient, m_nfsSock)) != 0) {
        closeConnection();
        m_slave->error(connErr, m_currentHost);
    }

    m_slave->connected();

    qCDebug(LOG_KIO_NFS) << "openConnection succeeded";
}

void NFSProtocolV3::listDir(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    // We should always be connected if it reaches this point,
    // but better safe than sorry!
    if (!isConnected()) {
        return;
    }

    if (url.isEmpty()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, url.path());
        return;
    }

    const QString path(url.path());
    // Is it part of an exported(virtual) dir?
    if (isExportedDir(path)) {
        qCDebug(LOG_KIO_NFS) << "Listing virtual dir" << path;

        QStringList virtualList;
        for (QStringList::const_iterator it = getExportedDirs().constBegin(); it != getExportedDirs().constEnd(); ++it) {
            // When an export is multiple levels deep(/mnt/nfs for example) we only
            // want to display one level at a time.
            QString name = (*it);
            name = name.remove(0, path.length());
            if (name.startsWith(QDir::separator())) {
                name = name.mid(1);
            }
            if (name.indexOf(QDir::separator()) != -1) {
                name.truncate(name.indexOf(QDir::separator()));
            }

            if (!virtualList.contains(name)) {
                virtualList.append(name);
            }
        }

        KIO::UDSEntry entry;
        createVirtualDirEntry(entry);
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, ".");
        m_slave->listEntry(entry);

        for (QStringList::const_iterator it = virtualList.constBegin(); it != virtualList.constEnd(); ++it) {
            qCDebug(LOG_KIO_NFS) << "Found " << (*it) << "in exported dir";

            entry.replace(KIO::UDSEntry::UDS_NAME, (*it));
            m_slave->listEntry(entry);
        }

        m_slave->finished();
        return;
    }

    const NFSFileHandle fh = getFileHandle(path);

    // There doesn't seem to be an invalid link error code in KIO, so this will have to do.
    if (fh.isInvalid() || fh.isBadLink()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, path);
        return;
    }

    // Get the preferred read dir size from the server
    if (m_readDirSize == 0) {
        initPreferredSizes(fh);
    }

    READDIRPLUS3args listargs;
    memset(&listargs, 0, sizeof(listargs));
    listargs.dircount = m_readDirSize;
    listargs.maxcount = sizeof(entryplus3) * m_readDirSize;  // Not really sure what this should be set to.

    fh.toFH(listargs.dir);

    READDIRPLUS3res listres;
    memset(&listres, 0, sizeof(listres));

    entryplus3* lastEntry = nullptr;
    do {
        memset(&listres, 0, sizeof(listres));

        // In case that we didn't get all entries we need to set the cookie to the last one we actually received.
        if (lastEntry != nullptr) {
            listargs.cookie = lastEntry->cookie;
        }

        int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_READDIRPLUS,
                                  (xdrproc_t) xdr_READDIRPLUS3args, reinterpret_cast<caddr_t>(&listargs),
                                  (xdrproc_t) xdr_READDIRPLUS3res, reinterpret_cast<caddr_t>(&listres),
                                  clnt_timeout);

        // Not a supported call? Try the old READDIR method.
        if (listres.status == NFS3ERR_NOTSUPP) {
            listDirCompat(url);
            return;
        }

        // Do we have an error? There's not much more we can do but to abort at this point.
        if (!checkForError(clnt_stat, listres.status, path)) {
            return;
        }

        for (entryplus3* dirEntry = listres.READDIRPLUS3res_u.resok.reply.entries; dirEntry != nullptr; dirEntry = dirEntry->nextentry) {
            if (dirEntry->name == QString("..")) {
                continue;
            }

            KIO::UDSEntry entry;
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, dirEntry->name);

            if (dirEntry->name == QString(".")) {
                createVirtualDirEntry(entry);
                completeUDSEntry(entry, dirEntry->name_attributes.post_op_attr_u.attributes);
                m_slave->listEntry(entry);
                continue;
            }

            const QString& filePath = QFileInfo(QDir(path), dirEntry->name).filePath();

            // Is it a symlink ?
            if (dirEntry->name_attributes.post_op_attr_u.attributes.type == NF3LNK) {
                int rpcStatus;
                READLINK3res readLinkRes;
                char nameBuf[NFS3_MAXPATHLEN];
                if (symLinkTarget(filePath, rpcStatus, readLinkRes, nameBuf)) {
                    QString linkDest = QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data);
                    entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, linkDest);

                    bool badLink = true;
                    NFSFileHandle linkFH;
                    if (isValidLink(path, linkDest)) {
                        QString linkPath;
                        if (QFileInfo(linkDest).isAbsolute()) {
                            linkPath = linkDest;
                        } else {
                            linkPath = QFileInfo(path, linkDest).absoluteFilePath();
                        }

                        int rpcStatus;
                        LOOKUP3res lookupRes;
                        if (lookupHandle(linkPath, rpcStatus, lookupRes)) {
                            GETATTR3res attrAndStat;
                            if (getAttr(linkPath, rpcStatus, attrAndStat)) {
                                badLink = false;

                                linkFH = lookupRes.LOOKUP3res_u.resok.object;
                                linkFH.setLinkSource(dirEntry->name_handle.post_op_fh3_u.handle);

                                completeUDSEntry(entry, attrAndStat.GETATTR3res_u.resok.obj_attributes);
                            }
                        }
                    }

                    if (badLink) {
                        linkFH = dirEntry->name_handle.post_op_fh3_u.handle;
                        linkFH.setBadLink();

                        completeBadLinkUDSEntry(entry, dirEntry->name_attributes.post_op_attr_u.attributes);
                    }

                    addFileHandle(filePath, linkFH);
                } else {
                    entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, i18n("Unknown target"));
                    completeBadLinkUDSEntry(entry, dirEntry->name_attributes.post_op_attr_u.attributes);
                }
            } else {
                NFSFileHandle entryFH = dirEntry->name_handle.post_op_fh3_u.handle;
                addFileHandle(filePath, entryFH);
                completeUDSEntry(entry, dirEntry->name_attributes.post_op_attr_u.attributes);
            }

            m_slave->listEntry(entry);

            lastEntry = dirEntry;
        }
    } while (listres.READDIRPLUS3res_u.resok.reply.entries != nullptr && !listres.READDIRPLUS3res_u.resok.reply.eof);

    m_slave->finished();
}

void NFSProtocolV3::listDirCompat(const QUrl& url)
{
    // We should always be connected if it reaches this point,
    // but better safe than sorry!
    if (!isConnected()) {
        return;
    }

    if (url.isEmpty()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, url.path());
    }

    const QString path(url.path());
    // Is it part of an exported (virtual) dir?
    if (NFSProtocol::isExportedDir(path)) {
        QStringList virtualList;
        for (QStringList::const_iterator it = getExportedDirs().constBegin(); it != getExportedDirs().constEnd(); ++it) {
            // When an export is multiple levels deep(mnt/nfs for example) we only
            // want to display one level at a time.
            QString name = (*it);
            name = name.remove(0, path.length());
            if (name.startsWith('/')) {
                name = name.mid(1);
            }
            if (name.indexOf('/') != -1) {
                name.truncate(name.indexOf('/'));
            }

            if (!virtualList.contains(name)) {
                virtualList.append(name);
            }
        }

        for (QStringList::const_iterator it = virtualList.constBegin(); it != virtualList.constEnd(); ++it) {
            qCDebug(LOG_KIO_NFS) << "Found " << (*it) << "in exported dir";

            KIO::UDSEntry entry;
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, (*it));
            createVirtualDirEntry(entry);
            m_slave->listEntry(entry);
        }

        m_slave->finished();
        return;
    }

    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid() || fh.isBadLink()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, path);
        return;
    }

    QStringList filesToList;

    READDIR3args listargs;
    memset(&listargs, 0, sizeof(listargs));
    listargs.count = m_readDirSize;
    fh.toFH(listargs.dir);

    READDIR3res listres;
    entry3* lastEntry = nullptr;
    do {
        memset(&listres, 0, sizeof(listres));

        // In case that we didn't get all entries we need to set the cookie to the last one we actually received
        if (lastEntry != nullptr) {
            listargs.cookie = lastEntry->cookie;
        }

        int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_READDIR,
                                  (xdrproc_t) xdr_READDIR3args, reinterpret_cast<caddr_t>(&listargs),
                                  (xdrproc_t) xdr_READDIR3res, reinterpret_cast<caddr_t>(&listres),
                                  clnt_timeout);

        if (!checkForError(clnt_stat, listres.status, path)) {
            return;
        }

        for (entry3* dirEntry = listres.READDIR3res_u.resok.reply.entries; dirEntry != nullptr; dirEntry = dirEntry->nextentry) {
            if (dirEntry->name != QString(".") && dirEntry->name != QString("..")) {
                filesToList.append(QFile::decodeName(dirEntry->name));
            }

            lastEntry = dirEntry;
        }
    } while (!listres.READDIR3res_u.resok.reply.eof);

    // Loop through all files, getting attributes and link path.
    for (QStringList::const_iterator it = filesToList.constBegin(); it != filesToList.constEnd(); ++it) {
        QString filePath = QFileInfo(QDir(path), (*it)).filePath();

        int rpcStatus;
        LOOKUP3res dirres;
        if (!lookupHandle(filePath, rpcStatus, dirres)) {
            qCDebug(LOG_KIO_NFS) << "Failed to lookup" << filePath << ", rpc:" << rpcStatus << ", nfs:" << dirres.status;
            // Try the next file instead of aborting
            continue;
        }

        KIO::UDSEntry entry;
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, (*it));

        // Is it a symlink?
        if (dirres.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.type == NF3LNK) {
            int rpcStatus;
            READLINK3res readLinkRes;
            char nameBuf[NFS3_MAXPATHLEN];
            if (symLinkTarget(filePath, rpcStatus, readLinkRes, nameBuf)) {
                const QString linkDest = QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data);
                entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, linkDest);

                bool badLink = true;
                NFSFileHandle linkFH;
                if (isValidLink(path, linkDest)) {
                    QString linkPath;
                    if (QFileInfo(linkDest).isAbsolute()) {
                        linkPath = linkDest;
                    } else {
                        linkPath = QFileInfo(path, linkDest).absoluteFilePath();
                    }

                    int rpcStatus;
                    LOOKUP3res lookupRes;
                    if (lookupHandle(linkPath, rpcStatus, lookupRes)) {
                        GETATTR3res attrAndStat;
                        if (getAttr(linkPath, rpcStatus, attrAndStat)) {
                            badLink = false;

                            linkFH = lookupRes.LOOKUP3res_u.resok.object;
                            linkFH.setLinkSource(dirres.LOOKUP3res_u.resok.object);

                            completeUDSEntry(entry, attrAndStat.GETATTR3res_u.resok.obj_attributes);
                        }
                    }
                }

                if (badLink) {
                    linkFH = dirres.LOOKUP3res_u.resok.object;
                    linkFH.setBadLink();

                    completeBadLinkUDSEntry(entry, dirres.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes);
                }

                addFileHandle(filePath, linkFH);
            } else {
                entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, i18n("Unknown target"));
                completeBadLinkUDSEntry(entry, dirres.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes);
            }
        } else {
            addFileHandle(filePath, dirres.LOOKUP3res_u.resok.object);

            completeUDSEntry(entry, dirres.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes);
        }

        m_slave->listEntry(entry);
    }

    m_slave->finished();
}

void NFSProtocolV3::stat(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    // We can't stat an exported dir, but we know it's a dir.
    if (isExportedDir(path)) {
        KIO::UDSEntry entry;

        entry.fastInsert(KIO::UDSEntry::UDS_NAME, path);
        createVirtualDirEntry(entry);

        m_slave->statEntry(entry);
        m_slave->finished();
        return;
    }

    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid()) {
        qCDebug(LOG_KIO_NFS) << "File handle is invalid";
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, path);
        return;
    }

    int rpcStatus;
    GETATTR3res attrAndStat;
    if (!getAttr(path, rpcStatus, attrAndStat)) {
        checkForError(rpcStatus, attrAndStat.status, path);
        return;
    }

    const QFileInfo fileInfo(path);

    KIO::UDSEntry entry;
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, fileInfo.fileName());

    // Is it a symlink?
    if (attrAndStat.GETATTR3res_u.resok.obj_attributes.type == NF3LNK) {
        qCDebug(LOG_KIO_NFS) << "It's a symlink";

        //get the link dest
        QString linkDest;

        int rpcStatus;
        READLINK3res readLinkRes;
        char nameBuf[NFS3_MAXPATHLEN];
        if (symLinkTarget(path, rpcStatus, readLinkRes, nameBuf)) {
            linkDest = QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data);
        } else {
            entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, linkDest);
            completeBadLinkUDSEntry(entry, attrAndStat.GETATTR3res_u.resok.obj_attributes);

            m_slave->statEntry(entry);
            m_slave->finished();
            return;
        }

        qCDebug(LOG_KIO_NFS) << "link dest is" << linkDest;

        entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, linkDest);

        if (!isValidLink(fileInfo.path(), linkDest)) {
            completeBadLinkUDSEntry(entry, attrAndStat.GETATTR3res_u.resok.obj_attributes);
        } else {
            QString linkPath;
            if (QFileInfo(linkDest).isAbsolute()) {
                linkPath = linkDest;
            } else {
                linkPath = QFileInfo(fileInfo.path(), linkDest).absoluteFilePath();
            }

            int rpcStatus;
            GETATTR3res attrAndStat;
            if (!getAttr(linkPath, rpcStatus, attrAndStat)) {
                checkForError(rpcStatus, attrAndStat.status, linkPath);
                return;
            }

            completeUDSEntry(entry, attrAndStat.GETATTR3res_u.resok.obj_attributes);
        }
    } else {
        completeUDSEntry(entry, attrAndStat.GETATTR3res_u.resok.obj_attributes);
    }

    m_slave->statEntry(entry);
    m_slave->finished();
}

void NFSProtocolV3::setHost(const QString& host)
{
    qCDebug(LOG_KIO_NFS) << host;

    if (host.isEmpty()) {
        m_slave->error(KIO::ERR_UNKNOWN_HOST, QString());
        return;
    }

    // No need to update if the host hasn't changed
    if (host == m_currentHost) {
        return;
    }

    m_currentHost = host;
    closeConnection();
}

void NFSProtocolV3::mkdir(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    const QFileInfo fileInfo(path);
    if (isExportedDir(fileInfo.path())) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, path);
        return;
    }

    const NFSFileHandle fh = getFileHandle(fileInfo.path());
    if (fh.isInvalid() || fh.isBadLink()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, path);
        return;
    }

    MKDIR3args createArgs;
    memset(&createArgs, 0, sizeof(createArgs));
    fh.toFH(createArgs.where.dir);

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());
    createArgs.where.name = tmpName.data();

    createArgs.attributes.mode.set_it = true;
    if (permissions == -1) {
        createArgs.attributes.mode.set_mode3_u.mode = 0755;
    } else {
        createArgs.attributes.mode.set_mode3_u.mode = permissions;
    }

    MKDIR3res dirres;
    memset(&dirres, 0, sizeof(dirres));

    int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_MKDIR,
                              (xdrproc_t) xdr_MKDIR3args, reinterpret_cast<caddr_t>(&createArgs),
                              (xdrproc_t) xdr_MKDIR3res, reinterpret_cast<caddr_t>(&dirres),
                              clnt_timeout);

    if (!checkForError(clnt_stat, dirres.status, path)) {
        return;
    }

    m_slave->finished();
}

void NFSProtocolV3::del(const QUrl& url, bool/* isfile*/)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    if (isExportedDir(QFileInfo(path).path())) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, path);
        return;
    }

    int rpcStatus;
    REMOVE3res res;
    if (!remove(path, rpcStatus, res)) {
        checkForError(rpcStatus, res.status, path);
        return;
    }

    m_slave->finished();
}

void NFSProtocolV3::chmod(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    if (isExportedDir(path)) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, path);
        return;
    }

    sattr3 attributes;
    memset(&attributes, 0, sizeof(attributes));
    attributes.mode.set_it = true;
    attributes.mode.set_mode3_u.mode = permissions;

    int rpcStatus;
    SETATTR3res setAttrRes;
    if (!setAttr(path, attributes, rpcStatus, setAttrRes)) {
        checkForError(rpcStatus, setAttrRes.status, path);
        return;
    }

    m_slave->finished();
}

void NFSProtocolV3::get(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid() || fh.isBadLink()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, path);
        return;
    }

    // Get the optimal read buffer size.
    if (m_readBufferSize == 0) {
        initPreferredSizes(fh);
    }

    READ3args readArgs;
    memset(&readArgs, 0, sizeof(readArgs));
    fh.toFH(readArgs.file);
    readArgs.offset = 0;
    readArgs.count = m_readBufferSize;

    READ3res readRes;
    memset(&readRes, 0, sizeof(readRes));
    readRes.READ3res_u.resok.data.data_len = m_readBufferSize;
    readRes.READ3res_u.resok.data.data_val = new char[m_readBufferSize];

    // Most likely indicates out of memory
    if (!readRes.READ3res_u.resok.data.data_val) {
        m_slave->error(KIO::ERR_OUT_OF_MEMORY, path);
        return;
    }

    bool validRead = false;
    bool hasError = false;
    int read = 0;
    QByteArray readBuffer;
    do {
        int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_READ,
                                  (xdrproc_t) xdr_READ3args, reinterpret_cast<caddr_t>(&readArgs),
                                  (xdrproc_t) xdr_READ3res, reinterpret_cast<caddr_t>(&readRes),
                                  clnt_timeout);

        // We are trying to read a directory, fail quietly
        if (readRes.status == NFS3ERR_ISDIR) {
            break;
        }

        if (!checkForError(clnt_stat, readRes.status, path)) {
            hasError = true;
            break;
        }

        read = readRes.READ3res_u.resok.count;
        readBuffer.setRawData(readRes.READ3res_u.resok.data.data_val, read);

        if (readArgs.offset == 0) {
            const QMimeDatabase db;
            const QMimeType type = db.mimeTypeForFileNameAndData(url.fileName(), readBuffer);
            m_slave->mimeType(type.name());

            m_slave->totalSize(readRes.READ3res_u.resok.file_attributes.post_op_attr_u.attributes.size);
        }

        readArgs.offset += read;
        if (read > 0) {
            validRead = true;

            m_slave->data(readBuffer);
            m_slave->processedSize(readArgs.offset);
        }

    } while (read > 0);

    if (readRes.READ3res_u.resok.data.data_val != nullptr) {
        delete [] readRes.READ3res_u.resok.data.data_val;
    }

    // Only send the read data to the slave if we have actually sent some.
    if (validRead) {
        m_slave->data(QByteArray());
        m_slave->processedSize(readArgs.offset);
    }

    if (!hasError) {
        m_slave->finished();
    }
}

void NFSProtocolV3::put(const QUrl& url, int _mode, KIO::JobFlags flags)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString destPath(url.path());
    if (isExportedDir(QFileInfo(destPath).path())) {
        m_slave->error(KIO::ERR_WRITE_ACCESS_DENIED, destPath);
        return;
    }

    NFSFileHandle destFH = getFileHandle(destPath);
    if (destFH.isBadLink()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, destPath);
        return;
    }

    // the file exists and we don't want to overwrite
    if (!destFH.isInvalid() && ((flags & KIO::Overwrite) == 0)) {
        m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    // Get the optimal write buffer size
    if (m_writeBufferSize == 0) {
        initPreferredSizes(destFH);
    }

    int rpcStatus;
    CREATE3res createRes;
    if (!create(destPath, _mode, rpcStatus, createRes)) {
        checkForError(rpcStatus, createRes.status, destPath);
        return;
    }

    // We created the file successfully.
    destFH = createRes.CREATE3res_u.resok.obj.post_op_fh3_u.handle;

    int result;

    WRITE3args writeArgs;
    memset(&writeArgs, 0, sizeof(writeArgs));

    destFH.toFH(writeArgs.file);
    writeArgs.offset = 0;
    writeArgs.stable = FILE_SYNC;

    WRITE3res writeRes;
    memset(&writeRes, 0, sizeof(writeRes));

    // Loop until we get 0 (end of data).
    int bytesWritten = 0;
    bool error = false;
    do {
        QByteArray buffer;
        m_slave->dataReq();
        result = m_slave->readData(buffer);

        if (result > 0) {
            char* data = buffer.data();
            uint32 bytesToWrite = buffer.size();
            int writeNow(0);

            do {
                if (bytesToWrite > m_writeBufferSize) {
                    writeNow = m_writeBufferSize;
                } else {
                    writeNow = bytesToWrite;
                }

                writeArgs.data.data_val = data;
                writeArgs.data.data_len = writeNow;
                writeArgs.count = writeNow;

                int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_WRITE,
                                          (xdrproc_t) xdr_WRITE3args, reinterpret_cast<caddr_t>(&writeArgs),
                                          (xdrproc_t) xdr_WRITE3res, reinterpret_cast<caddr_t>(&writeRes),
                                          clnt_timeout);

                if (!checkForError(clnt_stat, writeRes.status, destPath)) {
                    error = true;
                    break;
                }

                writeNow = writeRes.WRITE3res_u.resok.count;

                bytesWritten += writeNow;
                writeArgs.offset = bytesWritten;

                data = data + writeNow;
                bytesToWrite -= writeNow;
            } while (bytesToWrite > 0);
        }

        if (error) {
            break;
        }
    } while (result > 0);

    if (!error) {
        m_slave->finished();
    }
}

void NFSProtocolV3::rename(const QUrl& src, const QUrl& dest, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << dest;

    const QString srcPath(src.path());
    if (isExportedDir(srcPath)) {
        m_slave->error(KIO::ERR_CANNOT_RENAME, srcPath);
        return;
    }

    const QString destPath(dest.path());
    if (isExportedDir(destPath)) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, destPath);
        return;
    }

    if (!getFileHandle(destPath).isInvalid() && (_flags & KIO::Overwrite) == 0) {
        m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    int rpcStatus;
    RENAME3res res;
    if (!rename(srcPath, destPath, rpcStatus, res)) {
        checkForError(rpcStatus, res.status, destPath);
        return;
    }

    m_slave->finished();
}

void NFSProtocolV3::copySame(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    const QString srcPath(src.path());
    if (isExportedDir(QFileInfo(srcPath).path())) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, srcPath);
        return;
    }

    const NFSFileHandle srcFH = getFileHandle(srcPath);
    if (srcFH.isInvalid()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath(dest.path());
    if (isExportedDir(QFileInfo(destPath).path())) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, destPath);
        return;
    }

    // The file exists and we don't want to overwrite
    if (!getFileHandle(destPath).isInvalid() && (_flags & KIO::Overwrite) == 0) {
        m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    // Is it a link? No need to copy the data then, just copy the link destination.
    if (srcFH.isLink()) {
        //get the link dest
        int rpcStatus;
        READLINK3res readLinkRes;
        char nameBuf[NFS3_MAXPATHLEN];
        if (!symLinkTarget(srcPath, rpcStatus, readLinkRes, nameBuf)) {
            m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
            return;
        }

        const QString linkPath = QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data);

        SYMLINK3res linkRes;
        if (!symLink(linkPath, destPath, rpcStatus, linkRes)) {
            checkForError(rpcStatus, linkRes.status, linkPath);
            return;
        }

        m_slave->finished();
        return;
    }

    unsigned long resumeOffset = 0;
    bool bResume = false;
    const QString partFilePath = destPath + QLatin1String(".part");
    const NFSFileHandle partFH = getFileHandle(partFilePath);
    const bool bPartExists = !partFH.isInvalid();
    const bool bMarkPartial = m_slave->configValue(QStringLiteral("MarkPartial"), true);

    if (bPartExists) {
        int rpcStatus;
        LOOKUP3res partRes;
        if (lookupHandle(partFilePath, rpcStatus, partRes)) {
            if (bMarkPartial && partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size > 0) {
                if (partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.type == NF3DIR) {
                    m_slave->error(KIO::ERR_IS_DIRECTORY, partFilePath);
                    return;
                }

                bResume = m_slave->canResume(partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size);
                if (bResume) {
                    resumeOffset = partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size;
                }
            }
        }

        // Remove the part file if we are not resuming
        if (!bResume) {
            if (!remove(partFilePath)) {
                qCDebug(LOG_KIO_NFS) << "Could not remove part file, ignoring...";
            }
        }
    }

    // Create the file if we are not resuming a parted transfer,
    // or if we are not using part files(bResume is false in that case)
    NFSFileHandle destFH;
    if (!bResume) {
        QString createPath;
        if (bMarkPartial) {
            createPath = partFilePath;
        } else {
            createPath = destPath;
        }

        int rpcStatus;
        CREATE3res createRes;
        if (!create(createPath, _mode, rpcStatus, createRes)) {
            checkForError(rpcStatus, createRes.status, createPath);
            return;
        }

        destFH = createRes.CREATE3res_u.resok.obj.post_op_fh3_u.handle;
    } else {
        // Since we are resuming it's implied that we are using a part file,
        // which should exist at this point.
        destFH = getFileHandle(partFilePath);

        qCDebug(LOG_KIO_NFS) << "Resuming old transfer";
    }

    // Check what buffer size we should use, always use the smallest one.
    const int bufferSize = (m_readBufferSize < m_writeBufferSize) ? m_readBufferSize : m_writeBufferSize;

    WRITE3args writeArgs;
    memset(&writeArgs, 0, sizeof(writeArgs));

    destFH.toFH(writeArgs.file);
    writeArgs.offset = 0;
    writeArgs.data.data_val = new char[bufferSize];
    writeArgs.stable = FILE_SYNC;

    READ3args readArgs;
    memset(&readArgs, 0, sizeof(readArgs));

    srcFH.toFH(readArgs.file);
    readArgs.offset = 0;
    readArgs.count = bufferSize;

    if (bResume) {
        writeArgs.offset = resumeOffset;
        readArgs.offset = resumeOffset;
    }

    READ3res readRes;
    readRes.READ3res_u.resok.data.data_val = writeArgs.data.data_val;

    WRITE3res writeRes;
    memset(&writeRes, 0, sizeof(WRITE3res));

    bool error = false;
    int bytesRead = 0;
    do {
        int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_READ,
                                  (xdrproc_t) xdr_READ3args, reinterpret_cast<caddr_t>(&readArgs),
                                  (xdrproc_t) xdr_READ3res, reinterpret_cast<caddr_t>(&readRes),
                                  clnt_timeout);

        if (!checkForError(clnt_stat, readRes.status, srcPath)) {
            error = true;
            break;
        }

        bytesRead = readRes.READ3res_u.resok.data.data_len;

        // We should only send out the total size and mimetype at the start of the transfer
        if (readArgs.offset == 0 || (bResume && writeArgs.offset == resumeOffset)) {
            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFileNameAndData(src.fileName(), QByteArray::fromRawData(writeArgs.data.data_val, bytesRead));
            m_slave->mimeType(type.name());

            m_slave->totalSize(readRes.READ3res_u.resok.file_attributes.post_op_attr_u.attributes.size);
        }

        if (bytesRead > 0) {
            readArgs.offset += bytesRead;

            writeArgs.count = bytesRead;
            writeArgs.data.data_len = bytesRead;

            clnt_stat = clnt_call(m_nfsClient, NFSPROC3_WRITE,
                                  (xdrproc_t) xdr_WRITE3args, reinterpret_cast<caddr_t>(&writeArgs),
                                  (xdrproc_t) xdr_WRITE3res, reinterpret_cast<caddr_t>(&writeRes),
                                  clnt_timeout);

            if (!checkForError(clnt_stat, writeRes.status, destPath)) {
                error = true;
                break;
            }

            writeArgs.offset += bytesRead;

            m_slave->processedSize(readArgs.offset);
        }
    } while (bytesRead > 0);

    delete [] writeArgs.data.data_val;

    if (error) {
        if (bMarkPartial) {
            // Remove the part file if it's smaller than the minimum keep size.
            const unsigned int size = m_slave->configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
            if (writeArgs.offset <  size) {
                if (!remove(partFilePath)) {
                    qCDebug(LOG_KIO_NFS) << "Could not remove part file, ignoring...";
                }
            }
        }
    } else {
        // Rename partial file to its original name.
        if (bMarkPartial) {
            // Remove the destination file(if it exists)
            if (!getFileHandle(destPath).isInvalid() && !remove(destPath)) {
                qCDebug(LOG_KIO_NFS) << "Could not remove destination file" << destPath << ", ignoring...";
            }

            if (!rename(partFilePath, destPath)) {
                qCDebug(LOG_KIO_NFS) << "failed to rename" << partFilePath << "to" << destPath;
                m_slave->error(KIO::ERR_CANNOT_RENAME_PARTIAL, partFilePath);
                return;
            }
        }

        // Restore modification time
        int rpcStatus;
        GETATTR3res attrRes;
        if (getAttr(srcPath, rpcStatus, attrRes)) {
            sattr3 attributes;
            memset(&attributes, 0, sizeof(attributes));
            attributes.mtime.set_it = SET_TO_CLIENT_TIME;
            attributes.mtime.set_mtime_u.mtime.seconds = attrRes.GETATTR3res_u.resok.obj_attributes.mtime.seconds;
            attributes.mtime.set_mtime_u.mtime.nseconds = attrRes.GETATTR3res_u.resok.obj_attributes.mtime.nseconds;

            SETATTR3res attrSetRes;
            if (!setAttr(destPath, attributes, rpcStatus, attrSetRes)) {
                qCDebug(LOG_KIO_NFS) << "Failed to restore mtime, ignoring..." << rpcStatus << attrSetRes.status;
            }
        }

        qCDebug(LOG_KIO_NFS) << "Copied" << writeArgs.offset << "bytes of data";

        m_slave->processedSize(readArgs.offset);
        m_slave->finished();
    }
}

void NFSProtocolV3::copyFrom(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    const QString srcPath(src.path());
    const NFSFileHandle srcFH = getFileHandle(srcPath);
    if (srcFH.isInvalid()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath(dest.path());
    // The file exists and we don't want to overwrite.
    if (QFile::exists(destPath) && (_flags & KIO::Overwrite) == 0) {
        m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    // Is it a link? No need to copy the data then, just copy the link destination.
    if (srcFH.isLink()) {
        qCDebug(LOG_KIO_NFS) << "Is a link";

        //get the link dest
        int rpcStatus;
        READLINK3res readLinkRes;
        char nameBuf[NFS3_MAXPATHLEN];
        if (!symLinkTarget(srcPath, rpcStatus, readLinkRes, nameBuf)) {
            m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
            return;
        }

        QFile::link(QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data), destPath);

        m_slave->finished();
        return;
    }

    if (m_readBufferSize == 0) {
        initPreferredSizes(srcFH);
    }

    unsigned int resumeOffset = 0;
    bool bResume = false;
    const QFileInfo partInfo(destPath + QLatin1String(".part"));
    const bool bPartExists = partInfo.exists();
    const bool bMarkPartial = m_slave->configValue(QStringLiteral("MarkPartial"), true);

    if (bMarkPartial && bPartExists && partInfo.size() > 0) {
        if (partInfo.isDir()) {
            m_slave->error(KIO::ERR_IS_DIRECTORY, partInfo.absoluteFilePath());
            return;
        }

        bResume = m_slave->canResume(partInfo.size());
        resumeOffset = partInfo.size();
    }

    if (bPartExists && !bResume) {
        QFile::remove(partInfo.absoluteFilePath());
    }

    QFile::OpenMode openMode;
    QString outFileName;
    if (bResume) {
        outFileName = partInfo.absoluteFilePath();
        openMode = QFile::WriteOnly | QFile::Append;
    } else {
        outFileName = (bMarkPartial ? partInfo.absoluteFilePath() : destPath);
        openMode = QFile::WriteOnly | QFile::Truncate;
    }

    QFile destFile(outFileName);
    if (!bResume) {
        QFile::Permissions perms;
        if (_mode == -1) {
            perms = QFile::ReadOwner | QFile::WriteOwner;
        } else {
            perms = KIO::convertPermissions(_mode | QFile::WriteOwner);
        }

        destFile.setPermissions(perms);
    }

    if (!destFile.open(openMode)) {
        switch (destFile.error()) {
        case QFile::OpenError:
            if (bResume) {
                m_slave->error(KIO::ERR_CANNOT_RESUME, destPath);
            } else {
                m_slave->error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, destPath);
            }
            break;
        case QFile::PermissionsError:
            m_slave->error(KIO::ERR_WRITE_ACCESS_DENIED, destPath);
            break;
        default:
            m_slave->error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, destPath);
            break;
        }
        return;
    }

    READ3args readArgs;
    srcFH.toFH(readArgs.file);
    if (bResume) {
        readArgs.offset = resumeOffset;
    } else {
        readArgs.offset = 0;
    }
    readArgs.count = m_readBufferSize;

    READ3res readRes;
    memset(&readRes, 0, sizeof(readres));
    readRes.READ3res_u.resok.data.data_val = new char[m_readBufferSize];
    readRes.READ3res_u.resok.data.data_len = m_readBufferSize;

    bool error = false;
    unsigned long bytesToRead = 0, bytesRead = 0;
    do {
        int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_READ,
                                  (xdrproc_t) xdr_READ3args, reinterpret_cast<caddr_t>(&readArgs),
                                  (xdrproc_t) xdr_READ3res, reinterpret_cast<caddr_t>(&readRes),
                                  clnt_timeout);

        if (!checkForError(clnt_stat, readRes.status, destPath)) {
            error = true;
            break;
        }

        bytesRead = readRes.READ3res_u.resok.count;

        if (readArgs.offset == 0 || (bResume && readArgs.offset == resumeOffset)) {
            bytesToRead = readRes.READ3res_u.resok.file_attributes.post_op_attr_u.attributes.size;

            m_slave->totalSize(bytesToRead);

            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFileNameAndData(src.fileName(), QByteArray::fromRawData(readRes.READ3res_u.resok.data.data_val, bytesRead));
            m_slave->mimeType(type.name());
        }

        if (bytesRead > 0) {
            readArgs.offset += bytesRead;

            if (destFile.write(readRes.READ3res_u.resok.data.data_val, bytesRead) < 0) {
                m_slave->error(KIO::ERR_CANNOT_WRITE, destPath);

                error = true;
                break;
            }

            m_slave->processedSize(readArgs.offset);
        }
    } while (readArgs.offset < bytesToRead);

    delete [] readRes.READ3res_u.resok.data.data_val;

    // Close the file so we can modify the modification time later.
    destFile.close();

    if (error) {
        if (bMarkPartial) {
            // Remove the part file if it's smaller than the minimum keep
            const int size = m_slave->configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
            if (partInfo.size() <  size) {
                QFile::remove(partInfo.absoluteFilePath());
            }
        }
    } else {
        // Rename partial file to its original name.
        if (bMarkPartial) {
            const QString sPart = partInfo.absoluteFilePath();
            if (QFile::exists(destPath)) {
                QFile::remove(destPath);
            }
            if (!QFile::rename(sPart, destPath)) {
                qCDebug(LOG_KIO_NFS) << "failed to rename" << sPart << "to" << destPath;
                m_slave->error(KIO::ERR_CANNOT_RENAME_PARTIAL, sPart);
                return;
            }
        }

        // Restore the mtime on the file.
        const QString mtimeStr = m_slave->metaData("modified");
        if (!mtimeStr.isEmpty()) {
            QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
            if (dt.isValid()) {
                struct utimbuf utbuf;
                utbuf.actime = QFileInfo(destPath).lastRead().toSecsSinceEpoch(); // access time, unchanged
                utbuf.modtime = dt.toSecsSinceEpoch(); // modification time
                utime(QFile::encodeName(destPath).constData(), &utbuf);
            }
        }

        qCDebug(LOG_KIO_NFS) << "Copied" << readArgs.offset << "bytes of data";

        m_slave->processedSize(readArgs.offset);
        m_slave->finished();
    }
}

void NFSProtocolV3::copyTo(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    // The source does not exist, how strange
    const QString srcPath(src.path());
    if (!QFile::exists(srcPath)) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath(dest.path());
    if (isExportedDir(QFileInfo(destPath).path())) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, destPath);
        return;
    }

    // The file exists and we don't want to overwrite.
    if (!getFileHandle(destPath).isInvalid() && (_flags & KIO::Overwrite) == 0) {
        m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    // Is it a link? No need to copy the data then, just copy the link destination.
    const QString symlinkTarget = QFile::symLinkTarget(srcPath);
    if (!symlinkTarget.isEmpty()) {
        int rpcStatus;
        SYMLINK3res linkRes;
        if (!symLink(symlinkTarget, destPath, rpcStatus, linkRes)) {
            checkForError(rpcStatus, linkRes.status, symlinkTarget);
            return;
        }

        m_slave->finished();
        return;
    }

    unsigned long resumeOffset = 0;
    bool bResume = false;
    const QString partFilePath = destPath + QLatin1String(".part");
    const NFSFileHandle partFH = getFileHandle(partFilePath);
    const bool bPartExists = !partFH.isInvalid();
    const bool bMarkPartial = m_slave->configValue(QStringLiteral("MarkPartial"), true);

    if (bPartExists) {
        int rpcStatus;
        LOOKUP3res partRes;
        if (lookupHandle(partFilePath, rpcStatus, partRes)) {
            if (bMarkPartial && partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size > 0) {
                if (partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.type == NF3DIR) {
                    m_slave->error(KIO::ERR_IS_DIRECTORY, partFilePath);
                    return;
                }

                bResume = m_slave->canResume(partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size);
                if (bResume) {
                    resumeOffset = partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size;
                }
            }
        }

        // Remove the part file if we are not resuming
        if (!bResume) {
            if (!remove(partFilePath)) {
                qCDebug(LOG_KIO_NFS) << "Could not remove part file, ignoring...";
            }
        }
    }

    // Open the source file
    QFile srcFile(srcPath);
    if (!srcFile.open(QIODevice::ReadOnly)) {
        m_slave->error(KIO::ERR_CANNOT_OPEN_FOR_READING, srcPath);
        return;
    }

    // Create the file if we are not resuming a parted transfer,
    // or if we are not using part files (bResume is false in that case)
    NFSFileHandle destFH;
    if (!bResume) {
        QString createPath;
        if (bMarkPartial) {
            createPath = partFilePath;
        } else {
            createPath = destPath;
        }

        int rpcStatus;
        CREATE3res createRes;
        if (!create(createPath, _mode, rpcStatus, createRes)) {
            checkForError(rpcStatus, createRes.status, createPath);
            return;
        }

        destFH = createRes.CREATE3res_u.resok.obj.post_op_fh3_u.handle;
    } else {
        // Since we are resuming it's implied that we are using a part file,
        // which should exist at this point.
        destFH = getFileHandle(partFilePath);

        qCDebug(LOG_KIO_NFS) << "Resuming old transfer";
    }

    // Send the total size to the slave.
    m_slave->totalSize(srcFile.size());

    // Get the optimal write buffer size
    if (m_writeBufferSize == 0) {
        initPreferredSizes(destFH);
    }

    // Set up write arguments.
    WRITE3args writeArgs;
    memset(&writeArgs, 0, sizeof(writeArgs));
    destFH.toFH(writeArgs.file);
    writeArgs.data.data_val = new char[m_writeBufferSize];
    writeArgs.stable = FILE_SYNC;
    if (bResume) {
        writeArgs.offset = resumeOffset;
    } else {
        writeArgs.offset = 0;
    }

    WRITE3res writeRes;
    memset(&writeRes, 0 , sizeof(writeRes));

    bool error = false;
    int bytesRead = 0;
    do {
        memset(writeArgs.data.data_val, 0, m_writeBufferSize);

        bytesRead = srcFile.read(writeArgs.data.data_val, m_writeBufferSize);
        if (bytesRead < 0) {
            m_slave->error(KIO::ERR_CANNOT_READ, srcPath);

            error = true;
            break;
        }

        if (bytesRead > 0) {
            writeArgs.count = bytesRead;
            writeArgs.data.data_len = bytesRead;

            int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_WRITE,
                                      (xdrproc_t) xdr_WRITE3args, reinterpret_cast<caddr_t>(&writeArgs),
                                      (xdrproc_t) xdr_WRITE3res, reinterpret_cast<caddr_t>(&writeRes),
                                      clnt_timeout);

            if (!checkForError(clnt_stat, writeRes.status, destPath)) {
                error = true;
                break;
            }

            writeArgs.offset += bytesRead;

            m_slave->processedSize(writeArgs.offset);
        }
    } while (bytesRead > 0);

    delete [] writeArgs.data.data_val;

    if (error) {
        if (bMarkPartial) {
            // Remove the part file if it's smaller than the minimum keep size.
            const unsigned int size = m_slave->configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
            if (writeArgs.offset <  size) {
                if (!remove(partFilePath)) {
                    qCDebug(LOG_KIO_NFS) << "Could not remove part file, ignoring...";
                }
            }
        }
    } else {
        // Rename partial file to its original name.
        if (bMarkPartial) {
            // Remove the destination file(if it exists)
            if (!getFileHandle(destPath).isInvalid() && !remove(destPath)) {
                qCDebug(LOG_KIO_NFS) << "Could not remove destination file" << destPath << ", ignoring...";
            }

            if (!rename(partFilePath, destPath)) {
                qCDebug(LOG_KIO_NFS) << "failed to rename" << partFilePath << "to" << destPath;
                m_slave->error(KIO::ERR_CANNOT_RENAME_PARTIAL, partFilePath);
                return;
            }
        }

        // Restore the mtime on the file.
        const QString mtimeStr = m_slave->metaData("modified");
        if (!mtimeStr.isEmpty()) {
            QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
            if (dt.isValid()) {
                sattr3 attributes;
                memset(&attributes, 0, sizeof(attributes));
                attributes.mtime.set_it = SET_TO_CLIENT_TIME;
                attributes.mtime.set_mtime_u.mtime.seconds = dt.toSecsSinceEpoch();
                attributes.mtime.set_mtime_u.mtime.nseconds = attributes.mtime.set_mtime_u.mtime.seconds * 1000000000ULL;

                int rpcStatus;
                SETATTR3res attrSetRes;
                if (!setAttr(destPath, attributes, rpcStatus, attrSetRes)) {
                    qCDebug(LOG_KIO_NFS) << "Failed to restore mtime, ignoring..." << rpcStatus << attrSetRes.status;
                }
            }
        }

        qCDebug(LOG_KIO_NFS) << "Copied" << writeArgs.offset << "bytes of data";

        m_slave->processedSize(writeArgs.offset);
        m_slave->finished();
    }
}

void NFSProtocolV3::symlink(const QString& target, const QUrl& dest, KIO::JobFlags flags)
{
    const QString destPath(dest.path());
    if (isExportedDir(QFileInfo(destPath).path())) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, destPath);
        return;
    }

    if (!getFileHandle(destPath).isInvalid() && (flags & KIO::Overwrite) == 0) {
        m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }


    int rpcStatus;
    SYMLINK3res res;
    if (!symLink(target, destPath, rpcStatus, res)) {
        checkForError(rpcStatus, res.status, destPath);
        return;
    }

    m_slave->finished();
}

void NFSProtocolV3::initPreferredSizes(const NFSFileHandle& fh)
{
    FSINFO3args fsArgs;
    memset(&fsArgs, 0, sizeof(fsArgs));
    fh.toFH(fsArgs.fsroot);

    FSINFO3res fsRes;
    memset(&fsRes, 0, sizeof(fsRes));

    int clnt_stat = clnt_call(m_nfsClient, NFSPROC3_FSINFO,
                              (xdrproc_t) xdr_FSINFO3args, reinterpret_cast<caddr_t>(&fsArgs),
                              (xdrproc_t) xdr_FSINFO3res, reinterpret_cast<caddr_t>(&fsRes),
                              clnt_timeout);

    if (clnt_stat == RPC_SUCCESS && fsRes.status == NFS3_OK) {
        m_writeBufferSize = fsRes.FSINFO3res_u.resok.wtpref;
        m_readBufferSize = fsRes.FSINFO3res_u.resok.rtpref;
        m_readDirSize = fsRes.FSINFO3res_u.resok.dtpref;
    } else {
        m_writeBufferSize = NFS3_MAXDATA;
        m_readBufferSize = NFS3_MAXDATA;
        m_readDirSize = NFS3_MAXDATA;
    }

    qCDebug(LOG_KIO_NFS) << "Preferred sizes - write" << m_writeBufferSize << ", read" << m_readBufferSize << ", read dir" << m_readDirSize;
}


bool NFSProtocolV3::create(const QString& path, int mode, int& rpcStatus, CREATE3res& result)
{
    qCDebug(LOG_KIO_NFS) << path;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    if (!isConnected()) {
        result.status = NFS3ERR_ACCES;
        return false;
    }

    const QFileInfo fileInfo(path);

    const NFSFileHandle directoryFH = getFileHandle(fileInfo.path());
    if (directoryFH.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());

    CREATE3args args;
    memset(&args, 0, sizeof(args));

    directoryFH.toFH(args.where.dir);
    args.where.name = tmpName.data();

    args.how.createhow3_u.obj_attributes.mode.set_it = true;
    args.how.createhow3_u.obj_attributes.uid.set_it = true;
    args.how.createhow3_u.obj_attributes.gid.set_it = true;
    args.how.createhow3_u.obj_attributes.size.set_it = true;

    if (mode == -1) {
        args.how.createhow3_u.obj_attributes.mode.set_mode3_u.mode = 0644;
    } else {
        args.how.createhow3_u.obj_attributes.mode.set_mode3_u.mode = mode;
    }
    args.how.createhow3_u.obj_attributes.uid.set_uid3_u.uid = geteuid();
    args.how.createhow3_u.obj_attributes.gid.set_gid3_u.gid = getegid();
    args.how.createhow3_u.obj_attributes.size.set_size3_u.size = 0;

    rpcStatus = clnt_call(m_nfsClient, NFSPROC3_CREATE,
                          (xdrproc_t) xdr_CREATE3args, reinterpret_cast<caddr_t>(&args),
                          (xdrproc_t) xdr_CREATE3res, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS3_OK);
}

bool NFSProtocolV3::getAttr(const QString& path, int& rpcStatus, GETATTR3res& result)
{
    qCDebug(LOG_KIO_NFS) << path;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    if (!isConnected()) {
        result.status = NFS3ERR_ACCES;
        return false;
    }

    const NFSFileHandle fileFH = getFileHandle(path);
    if (fileFH.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    GETATTR3args args;
    memset(&args, 0, sizeof(GETATTR3args));
    fileFH.toFH(args.object);

    rpcStatus = clnt_call(m_nfsClient, NFSPROC3_GETATTR,
                          (xdrproc_t) xdr_GETATTR3args, reinterpret_cast<caddr_t>(&args),
                          (xdrproc_t) xdr_GETATTR3res, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS3_OK);
}

bool NFSProtocolV3::lookupHandle(const QString& path, int& rpcStatus, LOOKUP3res& result)
{
    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    if (!isConnected()) {
        result.status = NFS3ERR_ACCES;
        return false;
    }

    const QFileInfo fileInfo(path);

    const NFSFileHandle parentFH = getFileHandle(fileInfo.path());
    if (parentFH.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());

    // do the rpc call
    LOOKUP3args args;
    memset(&args, 0, sizeof(args));
    parentFH.toFH(args.what.dir);
    args.what.name = tmpName.data();

    rpcStatus = clnt_call(m_nfsClient, NFSPROC3_LOOKUP,
                          (xdrproc_t) xdr_LOOKUP3args, reinterpret_cast<caddr_t>(&args),
                          (xdrproc_t) xdr_LOOKUP3res, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS3_OK);
}

bool NFSProtocolV3::symLinkTarget(const QString& path, int& rpcStatus, READLINK3res& result, char* dataBuffer)
{
    qCDebug(LOG_KIO_NFS) << path;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    READLINK3args readLinkArgs;
    memset(&readLinkArgs, 0, sizeof(readLinkArgs));
    if (fh.isLink() && !fh.isBadLink()) {
        fh.toFHLink(readLinkArgs.symlink);
    } else {
        fh.toFH(readLinkArgs.symlink);
    }

    result.READLINK3res_u.resok.data = dataBuffer;

    rpcStatus = clnt_call(m_nfsClient, NFSPROC3_READLINK,
                          (xdrproc_t) xdr_READLINK3args, reinterpret_cast<caddr_t>(&readLinkArgs),
                          (xdrproc_t) xdr_READLINK3res, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS3_OK);
}

bool NFSProtocolV3::remove(const QString& path)
{
    int rpcStatus;
    REMOVE3res result;

    return remove(path, rpcStatus, result);
}

bool NFSProtocolV3::remove(const QString& path, int& rpcStatus, REMOVE3res& result)
{
    qCDebug(LOG_KIO_NFS) << path;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    if (!isConnected()) {
        result.status = NFS3ERR_PERM;
        return false;
    }

    const QFileInfo fileInfo(path);
    if (isExportedDir(fileInfo.path())) {
        result.status = NFS3ERR_ACCES;
        return false;
    }

    const NFSFileHandle directoryFH = getFileHandle(fileInfo.path());
    if (directoryFH.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    int rpcLookupStatus;
    LOOKUP3res lookupRes;
    if (!lookupHandle(path, rpcLookupStatus, lookupRes)) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());

    REMOVE3args args;
    memset(&args, 0, sizeof(args));
    directoryFH.toFH(args.object.dir);
    args.object.name = tmpName.data();

    if (lookupRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.type != NF3DIR) {
        rpcStatus = clnt_call(m_nfsClient, NFSPROC3_REMOVE,
                              (xdrproc_t) xdr_REMOVE3args, reinterpret_cast<caddr_t>(&args),
                              (xdrproc_t) xdr_REMOVE3res, reinterpret_cast<caddr_t>(&result),
                              clnt_timeout);
    } else {
        rpcStatus = clnt_call(m_nfsClient, NFSPROC3_RMDIR,
                              (xdrproc_t) xdr_RMDIR3args, reinterpret_cast<caddr_t>(&args),
                              (xdrproc_t) xdr_RMDIR3res, reinterpret_cast<caddr_t>(&result),
                              clnt_timeout);
    }

    bool ret = (rpcStatus == RPC_SUCCESS && result.status == NFS3_OK);
    if (ret) {
        // Remove it from the cache as well
        removeFileHandle(path);
    }

    return ret;
}

bool NFSProtocolV3::rename(const QString& src, const QString& dest)
{
    int rpcStatus;
    RENAME3res result;

    return rename(src, dest, rpcStatus, result);
}

bool NFSProtocolV3::rename(const QString& src, const QString& dest, int& rpcStatus, RENAME3res& result)
{
    qCDebug(LOG_KIO_NFS) << src << dest;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    const QFileInfo srcFileInfo(src);
    if (isExportedDir(srcFileInfo.path())) {
        result.status = NFS3ERR_ACCES;
        return false;
    }

    const NFSFileHandle srcDirectoryFH = getFileHandle(srcFileInfo.path());
    if (srcDirectoryFH.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    const QFileInfo destFileInfo(dest);
    if (isExportedDir(destFileInfo.path())) {
        result.status = NFS3ERR_ACCES;
        return false;
    }

    const NFSFileHandle destDirectoryFH = getFileHandle(destFileInfo.path());
    if (destDirectoryFH.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    RENAME3args args;
    memset(&args, 0, sizeof(args));

    QByteArray srcByteName = QFile::encodeName(srcFileInfo.fileName());
    srcDirectoryFH.toFH(args.from.dir);
    args.from.name = srcByteName.data();

    QByteArray destByteName = QFile::encodeName(destFileInfo.fileName());
    destDirectoryFH.toFH(args.to.dir);
    args.to.name = destByteName.data();

    rpcStatus = clnt_call(m_nfsClient, NFSPROC3_RENAME,
                          (xdrproc_t) xdr_RENAME3args, reinterpret_cast<caddr_t>(&args),
                          (xdrproc_t) xdr_RENAME3res, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    bool ret = (rpcStatus == RPC_SUCCESS && result.status == NFS3_OK);
    if (ret) {
        // Can we actually find the new handle?
        int lookupStatus;
        LOOKUP3res lookupRes;
        if (lookupHandle(dest, lookupStatus, lookupRes)) {
            // Remove the old file, and add the new one
            removeFileHandle(src);
            addFileHandle(dest, lookupRes.LOOKUP3res_u.resok.object);
        }
    }

    return ret;
}

bool NFSProtocolV3::setAttr(const QString& path, const sattr3& attributes, int& rpcStatus, SETATTR3res& result)
{
    qCDebug(LOG_KIO_NFS) << path;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    SETATTR3args setAttrArgs;
    memset(&setAttrArgs, 0, sizeof(setAttrArgs));
    fh.toFH(setAttrArgs.object);
    memcpy(&setAttrArgs.new_attributes, &attributes, sizeof(attributes));

    rpcStatus = clnt_call(m_nfsClient, NFSPROC3_SETATTR,
                          (xdrproc_t) xdr_SETATTR3args, reinterpret_cast<caddr_t>(&setAttrArgs),
                          (xdrproc_t) xdr_SETATTR3res, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS3_OK);
}

bool NFSProtocolV3::symLink(const QString& target, const QString& dest, int& rpcStatus, SYMLINK3res& result)
{
    qCDebug(LOG_KIO_NFS) << target << dest;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    // Remove dest first, we don't really care about the return value at this point,
    // the symlink call will fail if dest was not removed correctly.
    remove(dest);

    const QFileInfo fileInfo(dest);

    const NFSFileHandle fh = getFileHandle(fileInfo.path());
    if (fh.isInvalid()) {
        result.status = NFS3ERR_NOENT;
        return false;
    }

    QByteArray tmpStr = QFile::encodeName(fileInfo.fileName());
    QByteArray tmpStr2 = QFile::encodeName(target);

    SYMLINK3args symLinkArgs;
    memset(&symLinkArgs, 0, sizeof(symLinkArgs));

    fh.toFH(symLinkArgs.where.dir);
    symLinkArgs.where.name = tmpStr.data();
    symLinkArgs.symlink.symlink_data = tmpStr2.data();

    rpcStatus = clnt_call(m_nfsClient, NFSPROC3_SYMLINK,
                          (xdrproc_t) xdr_SYMLINK3args, reinterpret_cast<caddr_t>(&symLinkArgs),
                          (xdrproc_t) xdr_SYMLINK3res, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    // Add the new handle to the cache
    NFSFileHandle destFH = getFileHandle(dest);
    if (!destFH.isInvalid()) {
        addFileHandle(dest, destFH);
    }

    return (rpcStatus == RPC_SUCCESS && result.status == NFS3_OK);
}


// This function and completeBadLinkUDSEntry() must use KIO::UDSEntry::replace()
// because they may be called with a UDSEntry that has already been partially
// filled in by NFSProtocol::createVirtualDirEntry().

void NFSProtocolV3::completeUDSEntry(KIO::UDSEntry& entry, const fattr3& attributes)
{
    entry.replace(KIO::UDSEntry::UDS_SIZE, attributes.size);
    entry.replace(KIO::UDSEntry::UDS_MODIFICATION_TIME, attributes.mtime.seconds);
    entry.replace(KIO::UDSEntry::UDS_ACCESS_TIME, attributes.atime.seconds);
    entry.replace(KIO::UDSEntry::UDS_CREATION_TIME, attributes.ctime.seconds);

    // Some servers still send the file type information in the mode, even though
    // the reference specifies NFSv3 shouldn't, so we need to work around that here.
    // Not sure this is the best way to do it, but it works.
    if (attributes.mode > 0777) {
        entry.replace(KIO::UDSEntry::UDS_ACCESS, (attributes.mode & 07777));
    } else {
        entry.replace(KIO::UDSEntry::UDS_ACCESS, attributes.mode);
    }

    unsigned int type;
    switch (attributes.type) {
    case NF3DIR:
        type = S_IFDIR;
        break;
    case NF3BLK:
        type = S_IFBLK;
        break;
    case NF3CHR:
        type = S_IFCHR;
        break;
    case NF3LNK:
        type = S_IFLNK;
        break;
    case NF3SOCK:
        type = S_IFSOCK;
        break;
    case NF3FIFO:
        type = S_IFIFO;
        break;
    default:
        type = S_IFREG;
        break;
    }

    entry.replace(KIO::UDSEntry::UDS_FILE_TYPE, type);

    QString str;

    const uid_t uid = attributes.uid;
    if (!m_usercache.contains(uid)) {
        const struct passwd *user = getpwuid(uid);
        if (user) {
            m_usercache.insert(uid, QString::fromLatin1(user->pw_name));
            str = user->pw_name;
        } else {
            str = QString::number(uid);
        }
    } else {
        str = m_usercache.value(uid);
    }

    entry.replace(KIO::UDSEntry::UDS_USER, str);

    const gid_t gid = attributes.gid;
    if (!m_groupcache.contains(gid)) {
        const struct group *grp = getgrgid(gid);
        if (grp) {
            m_groupcache.insert(gid, QString::fromLatin1(grp->gr_name));
            str = grp->gr_name;
        } else {
            str = QString::number(gid);
        }
    } else {
        str = m_groupcache.value(gid);
    }

    entry.replace(KIO::UDSEntry::UDS_GROUP, str);
}

void NFSProtocolV3::completeBadLinkUDSEntry(KIO::UDSEntry& entry, const fattr3& attributes)
{
    entry.replace(KIO::UDSEntry::UDS_SIZE, 0LL);
    entry.replace(KIO::UDSEntry::UDS_MODIFICATION_TIME, attributes.mtime.seconds);
    entry.replace(KIO::UDSEntry::UDS_ACCESS_TIME, attributes.atime.seconds);
    entry.replace(KIO::UDSEntry::UDS_CREATION_TIME, attributes.ctime.seconds);
    entry.replace(KIO::UDSEntry::UDS_FILE_TYPE, S_IFMT - 1);
    entry.replace(KIO::UDSEntry::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IRWXO);
    // The UDS_USER and UDS_GROUP must be string values.  It would be possible
    // to look up appropriate values as in completeUDSEntry() above, but it seems
    // pointless to go to that trouble for an unusable bad link.
    entry.replace(KIO::UDSEntry::UDS_USER, QString::fromLatin1("root"));
    entry.replace(KIO::UDSEntry::UDS_GROUP, QString::fromLatin1("root"));
}
