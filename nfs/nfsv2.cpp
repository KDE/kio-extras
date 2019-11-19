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

#include "nfsv2.h"

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
#include <QMimeDatabase>
#include <QMimeType>

#include <KLocalizedString>
#include <kio/global.h>
#include <kio/ioslave_defaults.h>
#include <iostream>

// This is for NFS version 2.
#define NFSPROG 100003UL
#define NFSVERS 2UL


NFSProtocolV2::NFSProtocolV2(NFSSlave* slave)
    :  NFSProtocol(slave),
       m_slave(slave),
       m_mountClient(nullptr),
       m_mountSock(-1),
       m_nfsClient(nullptr),
       m_nfsSock(-1)
{
    qCDebug(LOG_KIO_NFS) << "NFS2::NFS2";

    clnt_timeout.tv_sec = 20;
    clnt_timeout.tv_usec = 0;
}

NFSProtocolV2::~NFSProtocolV2()
{
    closeConnection();
}

bool NFSProtocolV2::isCompatible(bool& connectionError)
{
    int ret = -1;

    CLIENT* client = nullptr;
    int sock = 0;
    if (NFSProtocol::openConnection(m_currentHost, NFSPROG, NFSVERS, client, sock) == 0) {
        // Check if the NFS version is compatible
        ret = clnt_call(client, NFSPROC_NULL,
                        (xdrproc_t) xdr_void, nullptr,
                        (xdrproc_t) xdr_void, nullptr, clnt_timeout);

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

bool NFSProtocolV2::isConnected() const
{
    return (m_nfsClient != nullptr);
}

void NFSProtocolV2::closeConnection()
{
    qCDebug(LOG_KIO_NFS);

    // Unmount all exported dirs(if any)
    if (m_mountClient != nullptr) {
        clnt_call(m_mountClient, MOUNTPROC_UMNTALL,
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

NFSFileHandle NFSProtocolV2::lookupFileHandle(const QString& path)
{
    int rpcStatus;
    diropres res;
    if (lookupHandle(path, rpcStatus, res)) {
        NFSFileHandle fh = res.diropres_u.diropres.file;

        // It it a link? Get the link target.
        if (res.diropres_u.diropres.attributes.type == NFLNK) {
            nfs_fh readLinkArgs;
            fh.toFH(readLinkArgs);

            char dataBuffer[NFS_MAXPATHLEN];

            readlinkres readLinkRes;
            memset(&readLinkRes, 0, sizeof(readLinkRes));
            readLinkRes.readlinkres_u.data = dataBuffer;

            int rpcStatus = clnt_call(m_nfsClient, NFSPROC_READLINK,
                                      (xdrproc_t) xdr_nfs_fh, reinterpret_cast<caddr_t>(&readLinkArgs),
                                      (xdrproc_t) xdr_readlinkres, reinterpret_cast<caddr_t>(&readLinkRes),
                                      clnt_timeout);

            if (rpcStatus == RPC_SUCCESS && readLinkRes.status == NFS_OK) {
                const QString linkDest = QString::fromLocal8Bit(readLinkRes.readlinkres_u.data);
                QString linkPath;
                if (QFileInfo(linkDest).isAbsolute()) {
                    linkPath = linkDest;
                } else {
                    linkPath = QFileInfo(QFileInfo(path).path(), linkDest).absoluteFilePath();
                }

                diropres linkRes;
                if (lookupHandle(linkPath, rpcStatus, linkRes)) {
                    NFSFileHandle linkFH = linkRes.diropres_u.diropres.file;
                    linkFH.setLinkSource(res.diropres_u.diropres.file);

                    qCDebug(LOG_KIO_NFS) << "Found target -" << linkPath;

                    return linkFH;
                }
            }

            // If we have reached this point the file is a link, but we failed to get the target.
            fh.setBadLink();
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
void NFSProtocolV2::openConnection()
{
    qCDebug(LOG_KIO_NFS) << m_currentHost;

    int connErr;
    if ((connErr = NFSProtocol::openConnection(m_currentHost, MOUNTPROG, MOUNTVERS, m_mountClient, m_mountSock)) != 0) {
        // Close the connection and send the error id to the slave
        closeConnection();
        m_slave->error(connErr, m_currentHost);
        return;
    }

    exports exportlist;
    memset(&exportlist, 0, sizeof(exportlist));

    int clnt_stat = clnt_call(m_mountClient, MOUNTPROC_EXPORT,
                              (xdrproc_t) xdr_void, nullptr,
                              (xdrproc_t) xdr_exports, reinterpret_cast<caddr_t>(&exportlist),
                              clnt_timeout);

    if (!checkForError(clnt_stat, 0, m_currentHost.toLatin1())) {
        return;
    }

    int exportsCount = 0;
    QStringList failList;

    fhstatus fhStatus;
    for (; exportlist != nullptr; exportlist = exportlist->ex_next, exportsCount++) {
        memset(&fhStatus, 0, sizeof(fhStatus));

        clnt_stat = clnt_call(m_mountClient, MOUNTPROC_MNT,
                              (xdrproc_t) xdr_dirpath, reinterpret_cast<caddr_t>(&exportlist->ex_dir),
                              (xdrproc_t) xdr_fhstatus, reinterpret_cast<caddr_t>(&fhStatus),
                              clnt_timeout);

        if (fhStatus.fhs_status == 0) {
            QString fname = QFileInfo(QDir("/"), exportlist->ex_dir).filePath();

            // Check if the dir is already exported
            if (NFSProtocol::isExportedDir(fname)) {
                continue;
            }

            addFileHandle(fname, static_cast<NFSFileHandle>(fhStatus.fhstatus_u.fhs_fhandle));
            addExportedDir(fname);
        } else {
            failList.append(exportlist->ex_dir);
        }
    }

    // Check if some exported dirs failed to mount
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

void NFSProtocolV2::listDir(const QUrl& url)
{
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

    // The root "directory" is just a list of the exported directories,
    // so list them here.
    if (isExportedDir(path)) {
        qCDebug(LOG_KIO_NFS) << "Listing exported dir";

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
            qCDebug(LOG_KIO_NFS) << (*it) << "found in exported dir";

            KIO::UDSEntry entry;
            entry.insert(KIO::UDSEntry::UDS_NAME, (*it));

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

    readdirargs listargs;
    memset(&listargs, 0, sizeof(listargs));
    listargs.count = 1024 * sizeof(entry);
    fh.toFH(listargs.dir);

    readdirres listres;

    QStringList filesToList;
    entry* lastEntry = nullptr;
    do {
        memset(&listres, 0, sizeof(listres));
        // In case that we didn't get all entries we need to set the cookie to the last one we actually received.
        if (lastEntry != nullptr) {
            memcpy(listargs.cookie, lastEntry->cookie, NFS_COOKIESIZE);
        }

        int clnt_stat = clnt_call(m_nfsClient, NFSPROC_READDIR,
                                  (xdrproc_t) xdr_readdirargs, reinterpret_cast<caddr_t>(&listargs),
                                  (xdrproc_t) xdr_readdirres, reinterpret_cast<caddr_t>(&listres),
                                  clnt_timeout);

        if (!checkForError(clnt_stat, listres.status, path)) {
            return;
        }

        for (entry* dirEntry = listres.readdirres_u.reply.entries; dirEntry != nullptr; dirEntry = dirEntry->nextentry) {
            if (dirEntry->name != QString(".") && dirEntry->name != QString("..")) {
                filesToList.append(QFile::decodeName(dirEntry->name));
            }

            lastEntry = dirEntry;
        }
    } while (!listres.readdirres_u.reply.eof);

    KIO::UDSEntry entry;
    for (QStringList::const_iterator it = filesToList.constBegin(); it != filesToList.constEnd(); ++it) {
        QString filePath = QFileInfo(QDir(path), (*it)).filePath();

        int rpcStatus;
        diropres dirres;
        if (!lookupHandle(filePath, rpcStatus, dirres)) {
            qCDebug(LOG_KIO_NFS) << "Failed to lookup" << filePath << ", rpc:" << rpcStatus << ", nfs:" << dirres.status;
            // Try the next file instead of failing
            continue;
        }

        entry.clear();
        entry.insert(KIO::UDSEntry::UDS_NAME, (*it));

        //is it a symlink ?
        if (dirres.diropres_u.diropres.attributes.type == NFLNK) {
            int rpcStatus;
            readlinkres readLinkRes;
            char nameBuf[NFS_MAXPATHLEN];
            if (readLink(filePath, rpcStatus, readLinkRes, nameBuf)) {
                const QString linkDest = QString::fromLocal8Bit(readLinkRes.readlinkres_u.data);
                entry.insert(KIO::UDSEntry::UDS_LINK_DEST, linkDest);

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
                    diropres lookupRes;
                    if (lookupHandle(linkPath, rpcStatus, lookupRes)) {
                        attrstat attrAndStat;
                        if (getAttr(linkPath, rpcStatus, attrAndStat)) {
                            badLink = false;

                            linkFH = lookupRes.diropres_u.diropres.file;
                            linkFH.setLinkSource(dirres.diropres_u.diropres.file);

                            completeUDSEntry(entry, attrAndStat.attrstat_u.attributes);
                        }
                    }

                }

                if (badLink) {
                    linkFH = dirres.diropres_u.diropres.file;
                    linkFH.setBadLink();

                    completeBadLinkUDSEntry(entry, dirres.diropres_u.diropres.attributes);
                }

                addFileHandle(filePath, linkFH);
            } else {
                entry.insert(KIO::UDSEntry::UDS_LINK_DEST, i18n("Unknown target"));
                completeBadLinkUDSEntry(entry, dirres.diropres_u.diropres.attributes);
            }
        } else {
            addFileHandle(filePath, dirres.diropres_u.diropres.file);
            completeUDSEntry(entry, dirres.diropres_u.diropres.attributes);
        }

        m_slave->listEntry(entry);
    }

    m_slave->finished();
}

void NFSProtocolV2::stat(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    if (isExportedDir(path)) {
        KIO::UDSEntry entry;

        entry.insert(KIO::UDSEntry::UDS_NAME, path);
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
    attrstat attrAndStat;
    if (!getAttr(path, rpcStatus, attrAndStat)) {
        checkForError(rpcStatus, attrAndStat.status, path);
        return;
    }

    const QFileInfo fileInfo(path);

    KIO::UDSEntry entry;
    entry.insert(KIO::UDSEntry::UDS_NAME, fileInfo.fileName());

    // Is it a symlink?
    if (attrAndStat.attrstat_u.attributes.type == NFLNK) {
        qCDebug(LOG_KIO_NFS) << "It's a symlink";

        QString linkDest;

        int rpcStatus;
        readlinkres readLinkRes;
        char nameBuf[NFS_MAXPATHLEN];
        if (readLink(path, rpcStatus, readLinkRes, nameBuf)) {
            linkDest = QString::fromLocal8Bit(readLinkRes.readlinkres_u.data);
        } else {
            entry.insert(KIO::UDSEntry::UDS_LINK_DEST, i18n("Unknown target"));
            completeBadLinkUDSEntry(entry, attrAndStat.attrstat_u.attributes);

            m_slave->statEntry(entry);
            m_slave->finished();
            return;
        }

        qCDebug(LOG_KIO_NFS) << "link dest is" << linkDest;

        entry.insert(KIO::UDSEntry::UDS_LINK_DEST, linkDest);
        if (!isValidLink(fileInfo.path(), linkDest)) {
            completeBadLinkUDSEntry(entry, attrAndStat.attrstat_u.attributes);
        } else {
            QString linkPath;
            if (QFileInfo(linkDest).isAbsolute()) {
                linkPath = linkDest;
            } else {
                linkPath = QFileInfo(fileInfo.path(), linkDest).absoluteFilePath();
            }

            int rpcStatus;
            attrstat attrAndStat;
            if (!getAttr(linkPath, rpcStatus, attrAndStat)) {
                checkForError(rpcStatus, attrAndStat.status, linkPath);
                return;
            }

            completeUDSEntry(entry, attrAndStat.attrstat_u.attributes);
        }
    } else {
        completeUDSEntry(entry, attrAndStat.attrstat_u.attributes);
    }

    m_slave->statEntry(entry);
    m_slave->finished();
}

void NFSProtocolV2::setHost(const QString& host)
{
    qCDebug(LOG_KIO_NFS) << host;
    if (host.isEmpty()) {
        m_slave->error(KIO::ERR_UNKNOWN_HOST, QString());
        return;
    }

    if (host == m_currentHost) {
        return;
    }

    // Set the new host and close the current connection
    m_currentHost = host;
    closeConnection();
}

void NFSProtocolV2::mkdir(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    const QFileInfo fileInfo(path);
    if (isExportedDir(fileInfo.path())) {
        m_slave->error(KIO::ERR_WRITE_ACCESS_DENIED, path);
        return;
    }

    const NFSFileHandle fh = getFileHandle(fileInfo.path());
    if (fh.isInvalid() || fh.isBadLink()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, path);
        return;
    }

    createargs createArgs;
    fh.toFH(createArgs.where.dir);

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());
    createArgs.where.name = tmpName.data();

    if (permissions == -1) {
        createArgs.attributes.mode = 0755;
    } else {
        createArgs.attributes.mode = permissions;
    }

    diropres dirres;
    memset(&dirres, 0, sizeof(diropres));

    int clnt_stat = clnt_call(m_nfsClient, NFSPROC_MKDIR,
                              (xdrproc_t) xdr_createargs, reinterpret_cast<caddr_t>(&createArgs),
                              (xdrproc_t) xdr_diropres, reinterpret_cast<caddr_t>(&dirres),
                              clnt_timeout);

    if (!checkForError(clnt_stat, dirres.status, path)) {
        return;
    }

    m_slave->finished();
}

void NFSProtocolV2::del(const QUrl& url, bool)
{
    int rpcStatus;
    nfsstat nfsStatus;
    if (!remove(url.path(), rpcStatus, nfsStatus)) {
        checkForError(rpcStatus, nfsStatus, url.path());

        qCDebug(LOG_KIO_NFS) << "Could not delete" << url;
        return;
    }

    m_slave->finished();
}

void NFSProtocolV2::chmod(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    if (isExportedDir(path)) {
        m_slave->error(KIO::ERR_ACCESS_DENIED, path);
        return;
    }

    sattr attributes;
    memset(&attributes, 0xFF, sizeof(attributes));
    attributes.mode = permissions;

    int rpcStatus;
    nfsstat result;
    if (!setAttr(path, attributes, rpcStatus, result)) {
        checkForError(rpcStatus, result, path);
        return;
    }


    m_slave->finished();
}

void NFSProtocolV2::get(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());

    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid() || fh.isBadLink()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, path);
        return;
    }

    readargs readArgs;
    fh.toFH(readArgs.file);
    readArgs.offset = 0;
    readArgs.count = NFS_MAXDATA;
    readArgs.totalcount = NFS_MAXDATA;

    readres readRes;
    memset(&readRes, 0, sizeof(readres));

    char buf[NFS_MAXDATA];
    readRes.readres_u.reply.data.data_val = buf;

    bool validRead = false;
    int offset = 0;
    QByteArray readBuffer;
    do {
        int clnt_stat = clnt_call(m_nfsClient, NFSPROC_READ,
                                  (xdrproc_t) xdr_readargs, reinterpret_cast<caddr_t>(&readArgs),
                                  (xdrproc_t) xdr_readres, reinterpret_cast<caddr_t>(&readRes),
                                  clnt_timeout);

        if (!checkForError(clnt_stat, readRes.status, path)) {
            return;
        }

        if (readArgs.offset == 0) {
            m_slave->totalSize(readRes.readres_u.reply.attributes.size);

            const QMimeDatabase db;
            const QMimeType type = db.mimeTypeForFileNameAndData(url.fileName(), readBuffer);
            m_slave->mimeType(type.name());
        }

        offset = readRes.readres_u.reply.data.data_len;
        readArgs.offset += offset;
        if (offset > 0) {
            validRead = true;

            readBuffer = QByteArray::fromRawData(readRes.readres_u.reply.data.data_val, offset);
            m_slave->data(readBuffer);
            readBuffer.clear();

            m_slave->processedSize(readArgs.offset);
        }

    } while (offset > 0);

    if (validRead) {
        m_slave->data(QByteArray());
        m_slave->processedSize(readArgs.offset);
    }

    m_slave->finished();
}

void NFSProtocolV2::put(const QUrl& url, int _mode, KIO::JobFlags flags)
{
    qCDebug(LOG_KIO_NFS) << url << _mode;

    const QString destPath(url.path());

    const QFileInfo fileInfo(destPath);
    if (isExportedDir(fileInfo.path())) {
        m_slave->error(KIO::ERR_WRITE_ACCESS_DENIED, destPath);
        return;
    }

    NFSFileHandle destFH = getFileHandle(destPath);
    if (destFH.isBadLink()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, destPath);
        return;
    }

    //the file exists and we don't want to overwrite
    if (!destFH.isInvalid() && (!(flags & KIO::Overwrite))) {
        m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    int rpcStatus;
    diropres dirOpRes;
    if (!create(destPath, _mode, rpcStatus, dirOpRes)) {
        checkForError(rpcStatus, dirOpRes.status, fileInfo.fileName());
        return;
    }

    destFH = dirOpRes.diropres_u.diropres.file.data;

    writeargs writeArgs;
    memset(&writeArgs, 0, sizeof(writeargs));
    destFH.toFH(writeArgs.file);
    writeArgs.beginoffset = 0;
    writeArgs.totalcount = 0;
    writeArgs.offset = 0;

    attrstat attrStat;

    int result = 0, bytesWritten = 0;
    do {
        // Request new data
        m_slave->dataReq();

        QByteArray buffer;
        result = m_slave->readData(buffer);

        char* data = buffer.data();
        int bytesToWrite = buffer.size(), writeNow = 0;
        if (result > 0) {
            do {
                if (bytesToWrite > NFS_MAXDATA) {
                    writeNow = NFS_MAXDATA;
                } else {
                    writeNow = bytesToWrite;
                }

                writeArgs.data.data_val = data;
                writeArgs.data.data_len = writeNow;

                int clnt_stat = clnt_call(m_nfsClient, NFSPROC_WRITE,
                                          (xdrproc_t) xdr_writeargs, reinterpret_cast<caddr_t>(&writeArgs),
                                          (xdrproc_t) xdr_attrstat, reinterpret_cast<caddr_t>(&attrStat),
                                          clnt_timeout);

                if (!checkForError(clnt_stat, attrStat.status, fileInfo.fileName())) {
                    return;
                }

                bytesWritten += writeNow;
                writeArgs.offset = bytesWritten;

                data = data + writeNow;
                bytesToWrite -= writeNow;
            } while (bytesToWrite > 0);
        }
    } while (result > 0);

    m_slave->finished();
}

void NFSProtocolV2::rename(const QUrl& src, const QUrl& dest, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

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
    nfsstat nfsStatus;
    if (!rename(src.path(), destPath, rpcStatus, nfsStatus)) {
        if (!checkForError(rpcStatus, nfsStatus, destPath)) {
            return;
        }
    }

    m_slave->finished();
}

void NFSProtocolV2::copySame(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    const QString srcPath(src.path());

    const NFSFileHandle srcFH = getFileHandle(srcPath);
    if (srcFH.isInvalid()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath = dest.path();
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
        readlinkres readLinkRes;
        char nameBuf[NFS_MAXPATHLEN];
        if (!readLink(srcPath, rpcStatus, readLinkRes, nameBuf)) {
            m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
            return;
        }

        const QString linkPath = QString::fromLocal8Bit(readLinkRes.readlinkres_u.data);

        nfsstat linkRes;
        if (!symLink(linkPath, destPath, rpcStatus, linkRes)) {
            checkForError(rpcStatus, linkRes, linkPath);
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
        diropres partRes;
        if (lookupHandle(partFilePath, rpcStatus, partRes)) {
            if (bMarkPartial && partRes.diropres_u.diropres.attributes.size > 0) {
                if (partRes.diropres_u.diropres.attributes.type == NFDIR) {
                    m_slave->error(KIO::ERR_IS_DIRECTORY, partFilePath);
                    return;
                }

                bResume = m_slave->canResume(partRes.diropres_u.diropres.attributes.size);
                if (bResume) {
                    resumeOffset = partRes.diropres_u.diropres.attributes.size;
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
        diropres dirOpRes;
        if (!create(createPath, _mode, rpcStatus, dirOpRes)) {
            checkForError(rpcStatus, dirOpRes.status, createPath);
            return;
        }

        destFH = dirOpRes.diropres_u.diropres.file.data;
    } else {
        // Since we are resuming it's implied that we are using a part file,
        // which should exist at this point.
        destFH = getFileHandle(partFilePath);

        qCDebug(LOG_KIO_NFS) << "Resuming old transfer";
    }

    char buf[NFS_MAXDATA];

    writeargs writeArgs;
    destFH.toFH(writeArgs.file);
    writeArgs.beginoffset = 0;
    writeArgs.totalcount = 0;
    writeArgs.offset = 0;
    writeArgs.data.data_val = buf;

    readargs readArgs;
    srcFH.toFH(readArgs.file);
    readArgs.offset = 0;
    readArgs.count = NFS_MAXDATA;
    readArgs.totalcount = NFS_MAXDATA;

    if (bResume) {
        writeArgs.offset = resumeOffset;
        readArgs.offset = resumeOffset;
    }

    readres readRes;
    memset(&readRes, 0, sizeof(readres));
    readRes.readres_u.reply.data.data_val = buf;

    attrstat attrStat;
    memset(&attrStat, 0, sizeof(attrstat));

    bool error = false;
    int bytesRead = 0;
    do {
        int clnt_stat = clnt_call(m_nfsClient, NFSPROC_READ,
                                  (xdrproc_t) xdr_readargs, reinterpret_cast<caddr_t>(&readArgs),
                                  (xdrproc_t) xdr_readres, reinterpret_cast<caddr_t>(&readRes),
                                  clnt_timeout);

        if (!checkForError(clnt_stat, readRes.status, destPath)) {
            error = true;
            break;
        }

        bytesRead = readRes.readres_u.reply.data.data_len;

        // We should only send out the total size and mimetype at the start of the transfer
        if (readArgs.offset == 0 || (bResume && writeArgs.offset == resumeOffset)) {
            m_slave->totalSize(readRes.readres_u.reply.attributes.size);

            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFileNameAndData(src.fileName(), QByteArray::fromRawData(writeArgs.data.data_val, bytesRead));
            m_slave->mimeType(type.name());
        }


        if (bytesRead > 0) {
            readArgs.offset += bytesRead;

            writeArgs.data.data_len = bytesRead;

            clnt_stat = clnt_call(m_nfsClient, NFSPROC_WRITE,
                                  (xdrproc_t) xdr_writeargs, reinterpret_cast<caddr_t>(&writeArgs),
                                  (xdrproc_t) xdr_attrstat, reinterpret_cast<caddr_t>(&attrStat),
                                  clnt_timeout);

            if (!checkForError(clnt_stat, attrStat.status, destPath)) {
                error = true;
                break;
            }

            writeArgs.offset += bytesRead;

            m_slave->processedSize(readArgs.offset);
        }
    } while (bytesRead > 0);

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
                qCDebug(LOG_KIO_NFS) << "Failed to rename" << partFilePath << "to" << destPath;
                m_slave->error(KIO::ERR_CANNOT_RENAME_PARTIAL, partFilePath);
                return;
            }
        }

        // Restore modification time
        int rpcStatus;
        attrstat attrRes;
        if (getAttr(srcPath, rpcStatus, attrRes)) {
            sattr attributes;
            memset(&attributes, 0xFF, sizeof(attributes));
            attributes.mtime.seconds = attrRes.attrstat_u.attributes.mtime.seconds;
            attributes.mtime.useconds = attrRes.attrstat_u.attributes.mtime.useconds;

            nfsstat attrSetRes;
            if (!setAttr(destPath, attributes, rpcStatus, attrSetRes)) {
                qCDebug(LOG_KIO_NFS) << "Failed to restore mtime, ignoring..." << rpcStatus << attrSetRes;
            }
        }

        qCDebug(LOG_KIO_NFS) << "Copied" << writeArgs.offset << "bytes of data";

        m_slave->processedSize(readArgs.offset);
        m_slave->finished();
    }
}

void NFSProtocolV2::copyFrom(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    const QString srcPath(src.path());

    const NFSFileHandle srcFH = getFileHandle(srcPath);
    if (srcFH.isInvalid()) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath(dest.path());

    // The file exists and we don't want to overwrite
    if (QFile::exists(destPath) && (_flags & KIO::Overwrite) == 0) {
        m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    // Is it a link? No need to copy the data then, just copy the link destination.
    if (srcFH.isLink()) {
        qCDebug(LOG_KIO_NFS) << "Is a link";

        int rpcStatus;
        readlinkres readLinkRes;
        char nameBuf[NFS_MAXPATHLEN];
        if (!readLink(srcPath, rpcStatus, readLinkRes, nameBuf)) {
            m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
            return;
        }

        QFile::link(QString::fromLocal8Bit(readLinkRes.readlinkres_u.data), destPath);

        m_slave->finished();
        return;
    }

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

    char buf[NFS_MAXDATA];

    readargs readArgs;
    srcFH.toFH(readArgs.file);
    if (bResume) {
        readArgs.offset = partInfo.size();
    } else {
        readArgs.offset = 0;
    }
    readArgs.count = NFS_MAXDATA;
    readArgs.totalcount = NFS_MAXDATA;

    readres readRes;
    memset(&readRes, 0, sizeof(readres));
    readRes.readres_u.reply.data.data_val = buf;

    attrstat attrStat;
    memset(&attrStat, 0, sizeof(attrstat));

    bool error = false;
    int bytesRead = 0;
    do {
        int clnt_stat = clnt_call(m_nfsClient, NFSPROC_READ,
                                  (xdrproc_t) xdr_readargs, reinterpret_cast<caddr_t>(&readArgs),
                                  (xdrproc_t) xdr_readres, reinterpret_cast<caddr_t>(&readRes),
                                  clnt_timeout);

        if (!checkForError(clnt_stat, readRes.status, destPath)) {
            error = true;
            break;
        }

        bytesRead = readRes.readres_u.reply.data.data_len;

        if (readArgs.offset == 0) {
            m_slave->totalSize(readRes.readres_u.reply.attributes.size);

            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFileNameAndData(src.fileName(), QByteArray::fromRawData(readRes.readres_u.reply.data.data_val, bytesRead));
            m_slave->mimeType(type.name());
        }


        if (bytesRead > 0) {
            readArgs.offset += bytesRead;

            if (destFile.write(readRes.readres_u.reply.data.data_val, bytesRead) != bytesRead) {
                m_slave->error(KIO::ERR_CANNOT_WRITE, destPath);

                error = true;
                break;
            }

            m_slave->processedSize(readArgs.offset);
        }
    } while (bytesRead > 0);

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
                qCDebug(LOG_KIO_NFS) << "Failed to rename" << sPart << "to" << destPath;
                m_slave->error(KIO::ERR_CANNOT_RENAME_PARTIAL, sPart);
                return;
            }
        }

        // Restore the mtime on the file.
        const QString mtimeStr = m_slave->metaData("modified");
        if (!mtimeStr.isEmpty()) {
            QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
            if (dt.isValid()) {
                qCDebug(LOG_KIO_NFS) << "Setting modification time to" << dt.toTime_t();

                struct utimbuf utbuf;
                utbuf.actime = QFileInfo(destPath).lastRead().toTime_t(); // access time, unchanged
                utbuf.modtime = dt.toTime_t(); // modification time
                utime(QFile::encodeName(destPath).constData(), &utbuf);
            }
        }

        qCDebug(LOG_KIO_NFS) << "Copied" << readArgs.offset << "bytes of data";

        m_slave->processedSize(readArgs.offset);
        m_slave->finished();
    }
}

void NFSProtocolV2::copyTo(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    // The source does not exist, how strange.
    const QString srcPath(src.path());
    if (!QFile::exists(srcPath)) {
        m_slave->error(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath(dest.path());
    if (isExportedDir(destPath)) {
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
        nfsstat linkRes;
        if (!symLink(symlinkTarget, destPath, rpcStatus, linkRes)) {
            checkForError(rpcStatus, linkRes, symlinkTarget);
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
        diropres partRes;
        if (lookupHandle(partFilePath, rpcStatus, partRes)) {
            if (bMarkPartial && partRes.diropres_u.diropres.attributes.size > 0) {
                if (partRes.diropres_u.diropres.attributes.type == NFDIR) {
                    m_slave->error(KIO::ERR_IS_DIRECTORY, partFilePath);
                    return;
                }

                bResume = m_slave->canResume(partRes.diropres_u.diropres.attributes.size);
                if (bResume) {
                    resumeOffset = partRes.diropres_u.diropres.attributes.size;
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
        diropres dirOpRes;
        if (!create(createPath, _mode, rpcStatus, dirOpRes)) {
            checkForError(rpcStatus, dirOpRes.status, createPath);
            return;
        }

        destFH = dirOpRes.diropres_u.diropres.file.data;
    } else {
        // Since we are resuming it's implied that we are using a part file,
        // which should exist at this point.
        destFH = getFileHandle(partFilePath);

        qCDebug(LOG_KIO_NFS) << "Resuming old transfer";
    }

    // Send the total size to the slave.
    m_slave->totalSize(srcFile.size());

    // Set up write arguments.
    char buf[NFS_MAXDATA];

    writeargs writeArgs;
    memset(&writeArgs, 0, sizeof(writeargs));
    destFH.toFH(writeArgs.file);
    writeArgs.data.data_val = buf;
    writeArgs.beginoffset = 0;
    writeArgs.totalcount = 0;
    if (bResume) {
        writeArgs.offset = resumeOffset;
    } else {
        writeArgs.offset = 0;
    }

    attrstat attrStat;
    memset(&attrStat, 0, sizeof(attrstat));

    bool error = false;
    int bytesRead = 0;
    do {
        bytesRead = srcFile.read(writeArgs.data.data_val, NFS_MAXDATA);
        if (bytesRead < 0) {
            m_slave->error(KIO::ERR_CANNOT_READ, srcPath);

            error = true;
            break;
        }

        if (bytesRead > 0) {
            writeArgs.data.data_len = bytesRead;

            int clnt_stat = clnt_call(m_nfsClient, NFSPROC_WRITE,
                                      (xdrproc_t) xdr_writeargs, reinterpret_cast<caddr_t>(&writeArgs),
                                      (xdrproc_t) xdr_attrstat, reinterpret_cast<caddr_t>(&attrStat),
                                      clnt_timeout);

            if (!checkForError(clnt_stat, attrStat.status, destPath)) {
                error = true;
                break;
            }

            writeArgs.offset += bytesRead;

            m_slave->processedSize(writeArgs.offset);
        }
    } while (bytesRead > 0);

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
                qCDebug(LOG_KIO_NFS) << "Failed to rename" << partFilePath << "to" << destPath;
                m_slave->error(KIO::ERR_CANNOT_RENAME_PARTIAL, partFilePath);
                return;
            }
        }

        // Restore the mtime on the file.
        const QString mtimeStr = m_slave->metaData("modified");
        if (!mtimeStr.isEmpty()) {
            QDateTime dt = QDateTime::fromString(mtimeStr, Qt::ISODate);
            if (dt.isValid()) {
                sattr attributes;
                memset(&attributes, 0xFF, sizeof(attributes));
                attributes.mtime.seconds = dt.toTime_t();
                attributes.mtime.useconds = attributes.mtime.seconds * 1000000ULL;

                int rpcStatus;
                nfsstat attrSetRes;
                if (!setAttr(destPath, attributes, rpcStatus, attrSetRes)) {
                    qCDebug(LOG_KIO_NFS) << "Failed to restore mtime, ignoring..." << rpcStatus << attrSetRes;
                }
            }
        }

        qCDebug(LOG_KIO_NFS) << "Copied" << writeArgs.offset << "bytes of data";

        m_slave->processedSize(writeArgs.offset);
        m_slave->finished();
    }
}

void NFSProtocolV2::symlink(const QString& target, const QUrl& dest, KIO::JobFlags flags)
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
    nfsstat res;
    if (!symLink(target, destPath, rpcStatus, res)) {
        checkForError(rpcStatus, res, destPath);
        return;
    }

    m_slave->finished();

}


bool NFSProtocolV2::create(const QString& path, int mode, int& rpcStatus, diropres& result)
{
    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    if (!isConnected()) {
        result.status = NFSERR_ACCES;
        return false;
    }

    const QFileInfo fileInfo(path);
    if (isExportedDir(fileInfo.path())) {
        result.status = NFSERR_ACCES;
        return false;
    }

    const NFSFileHandle directoryFH = getFileHandle(fileInfo.path());
    if (directoryFH.isInvalid()) {
        result.status = NFSERR_NOENT;
        return false;
    }

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());

    createargs args;
    directoryFH.toFH(args.where.dir);
    args.where.name = tmpName.data();

    memset(&args.attributes, 0xFF, sizeof(sattr));
    if (mode == -1) {
        args.attributes.mode = 0644;
    } else {
        args.attributes.mode = mode;
    }
    args.attributes.uid = geteuid();
    args.attributes.gid = getegid();
    args.attributes.size = 0;

    rpcStatus = clnt_call(m_nfsClient, NFSPROC_CREATE,
                          (xdrproc_t) xdr_createargs, reinterpret_cast<caddr_t>(&args),
                          (xdrproc_t) xdr_diropres, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS_OK);
}

bool NFSProtocolV2::getAttr(const QString& path, int& rpcStatus, attrstat& result)
{
    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    if (!isConnected()) {
        result.status = NFSERR_ACCES;
        return false;
    }

    const NFSFileHandle fileFH = getFileHandle(path);
    if (fileFH.isInvalid()) {
        result.status = NFSERR_NOENT;
        return false;
    }

    nfs_fh fh;
    fileFH.toFH(fh);

    rpcStatus = clnt_call(m_nfsClient, NFSPROC_GETATTR,
                          (xdrproc_t) xdr_nfs_fh, reinterpret_cast<caddr_t>(&fh),
                          (xdrproc_t) xdr_attrstat, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS_OK);
}

bool NFSProtocolV2::lookupHandle(const QString& path, int& rpcStatus, diropres& result)
{
    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    if (!isConnected()) {
        result.status = NFSERR_ACCES;
        return false;
    }

    const QFileInfo fileInfo(path);

    const NFSFileHandle parentFH = getFileHandle(fileInfo.path());
    if (parentFH.isInvalid()) {
        result.status = NFSERR_NOENT;
        return false;
    }

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());

    diropargs dirargs;
    memset(&dirargs, 0, sizeof(diropargs));
    parentFH.toFH(dirargs.dir);
    dirargs.name = tmpName.data();

    memset(&result, 0, sizeof(diropres));

    rpcStatus = clnt_call(m_nfsClient, NFSPROC_LOOKUP,
                          (xdrproc_t) xdr_diropargs, reinterpret_cast<caddr_t>(&dirargs),
                          (xdrproc_t) xdr_diropres, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS_OK);
}

bool NFSProtocolV2::readLink(const QString& path, int& rpcStatus, readlinkres& result, char* dataBuffer)
{
    const NFSFileHandle fh = getFileHandle(path);

    nfs_fh nfsFH;
    if (fh.isLink() && !fh.isBadLink()) {
        fh.toFHLink(nfsFH);
    } else {
        fh.toFH(nfsFH);
    }

    result.readlinkres_u.data = dataBuffer;

    rpcStatus = clnt_call(m_nfsClient, NFSPROC_READLINK,
                          (xdrproc_t) xdr_nfs_fh, reinterpret_cast<caddr_t>(&nfsFH),
                          (xdrproc_t) xdr_readlinkres, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result.status == NFS_OK);
}

bool NFSProtocolV2::remove(const QString& path)
{
    int rpcStatus;
    nfsstat nfsStatus;

    return remove(path, rpcStatus, nfsStatus);
}

bool NFSProtocolV2::remove(const QString& path, int& rpcStatus, nfsstat& result)
{
    qCDebug(LOG_KIO_NFS) << path;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    if (!isConnected()) {
        result = NFSERR_PERM;
        return false;
    }

    const QFileInfo fileInfo(path);
    if (isExportedDir(fileInfo.path())) {
        result = NFSERR_ACCES;
        return false;
    }

    const NFSFileHandle directoryFH = getFileHandle(fileInfo.path());
    if (directoryFH.isInvalid()) {
        result = NFSERR_NOENT;
        return false;
    }

    int rpcLookupStatus;
    diropres lookupRes;
    if (!lookupHandle(path, rpcLookupStatus, lookupRes)) {
        result = NFSERR_NOENT;
        return false;
    }

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());

    diropargs dirargs;
    memset(&dirargs, 0, sizeof(diropargs));
    directoryFH.toFH(dirargs.dir);
    dirargs.name = tmpName.data();

    if (lookupRes.diropres_u.diropres.attributes.type != NFDIR) {
        rpcStatus = clnt_call(m_nfsClient, NFSPROC_REMOVE,
                              (xdrproc_t) xdr_diropargs, reinterpret_cast<caddr_t>(&dirargs),
                              (xdrproc_t) xdr_nfsstat, reinterpret_cast<caddr_t>(&result),
                              clnt_timeout);
    } else {
        rpcStatus = clnt_call(m_nfsClient, NFSPROC_RMDIR,
                              (xdrproc_t) xdr_diropargs, reinterpret_cast<caddr_t>(&dirargs),
                              (xdrproc_t) xdr_nfsstat, reinterpret_cast<caddr_t>(&result),
                              clnt_timeout);
    }

    bool ret = (rpcStatus == RPC_SUCCESS && result == NFS_OK);
    if (ret) {
        removeFileHandle(path);
    }

    return ret;
}

bool NFSProtocolV2::rename(const QString& src, const QString& dest)
{
    int rpcStatus;
    nfsstat result;

    return rename(src, dest, rpcStatus, result);
}

bool NFSProtocolV2::rename(const QString& src, const QString& dest, int& rpcStatus, nfsstat& result)
{
    qCDebug(LOG_KIO_NFS) << src << dest;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    const QFileInfo srcFileInfo(src);
    if (isExportedDir(srcFileInfo.path())) {
        result = NFSERR_ACCES;
        return false;
    }

    const NFSFileHandle srcDirectoryFH = getFileHandle(srcFileInfo.path());
    if (srcDirectoryFH.isInvalid()) {
        result = NFSERR_NOENT;
        return false;
    }

    const QFileInfo destFileInfo(dest);
    if (isExportedDir(destFileInfo.path())) {
        result = NFSERR_ACCES;
        return false;
    }

    const NFSFileHandle destDirectoryFH = getFileHandle(destFileInfo.path());
    if (destDirectoryFH.isInvalid()) {
        result = NFSERR_NOENT;
        return false;
    }

    renameargs renameArgs;
    memset(&renameArgs, 0, sizeof(renameargs));

    QByteArray srcByteName = QFile::encodeName(srcFileInfo.fileName());
    srcDirectoryFH.toFH(renameArgs.from.dir);
    renameArgs.from.name = srcByteName.data();

    QByteArray destByteName = QFile::encodeName(destFileInfo.fileName());
    destDirectoryFH.toFH(renameArgs.to.dir);
    renameArgs.to.name = destByteName.data();

    rpcStatus = clnt_call(m_nfsClient, NFSPROC_RENAME,
                          (xdrproc_t) xdr_renameargs, reinterpret_cast<caddr_t>(&renameArgs),
                          (xdrproc_t) xdr_nfsstat, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    bool ret = (rpcStatus == RPC_SUCCESS && result == NFS_OK);
    if (ret) {
        // Can we actually find the new handle?
        int lookupStatus;
        diropres lookupRes;
        if (lookupHandle(dest, lookupStatus, lookupRes)) {
            // Remove the old file, and add the new one
            removeFileHandle(src);
            addFileHandle(dest, lookupRes.diropres_u.diropres.file);
        }
    }

    return ret;
}

bool NFSProtocolV2::setAttr(const QString& path, const sattr& attributes, int& rpcStatus, nfsstat& result)
{
    qCDebug(LOG_KIO_NFS) << path;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid()) {
        result = NFSERR_NOENT;
        return false;
    }

    sattrargs sAttrArgs;
    fh.toFH(sAttrArgs.file);
    memcpy(&sAttrArgs.attributes, &attributes, sizeof(attributes));

    rpcStatus = clnt_call(m_nfsClient, NFSPROC_SETATTR,
                          (xdrproc_t) xdr_sattrargs, reinterpret_cast<caddr_t>(&sAttrArgs),
                          (xdrproc_t) xdr_nfsstat, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    return (rpcStatus == RPC_SUCCESS && result == NFS_OK);
}

bool NFSProtocolV2::symLink(const QString& target, const QString& dest, int& rpcStatus, nfsstat& result)
{
    qCDebug(LOG_KIO_NFS) << target << dest;

    memset(&rpcStatus, 0, sizeof(int));
    memset(&result, 0, sizeof(result));

    // Remove dest first, we don't really care about the return value at this point,
    // the symlink call will fail if dest was not removed correctly.
    remove(dest);


    const QFileInfo fileInfo(dest);
    if (isExportedDir(fileInfo.path())) {
        result = NFSERR_ACCES;
        return false;
    }

    const NFSFileHandle fh = getFileHandle(fileInfo.path());
    if (fh.isInvalid()) {
        result = NFSERR_NOENT;
        return false;
    }

    QByteArray fromBytes = QFile::encodeName(fileInfo.fileName());
    QByteArray toBytes = QFile::encodeName(target);

    symlinkargs symLinkArgs;
    memset(&symLinkArgs, 0, sizeof(symLinkArgs));

    fh.toFH(symLinkArgs.from.dir);
    symLinkArgs.from.name = fromBytes.data();
    symLinkArgs.to = toBytes.data();

    rpcStatus = clnt_call(m_nfsClient, NFSPROC_SYMLINK,
                          (xdrproc_t) xdr_symlinkargs, reinterpret_cast<caddr_t>(&symLinkArgs),
                          (xdrproc_t) xdr_nfsstat, reinterpret_cast<caddr_t>(&result),
                          clnt_timeout);

    // Add the new handle to the cache
    NFSFileHandle destFH = getFileHandle(dest);
    if (!destFH.isInvalid()) {
        addFileHandle(dest, destFH);
    }

    return (rpcStatus == RPC_SUCCESS && result == NFS_OK);
}


void NFSProtocolV2::completeUDSEntry(KIO::UDSEntry& entry, const fattr& attributes)
{
    entry.insert(KIO::UDSEntry::UDS_SIZE, attributes.size);
    entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, attributes.mtime.seconds);
    entry.insert(KIO::UDSEntry::UDS_ACCESS_TIME, attributes.atime.seconds);
    entry.insert(KIO::UDSEntry::UDS_CREATION_TIME, attributes.ctime.seconds);
    entry.insert(KIO::UDSEntry::UDS_ACCESS, (attributes.mode & 07777));
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, attributes.mode & S_IFMT); // extract file type

    QString str;

    const uid_t uid = attributes.uid;
    if (!m_usercache.contains(uid)) {
        struct passwd* user = getpwuid(uid);
        if (user) {
            m_usercache.insert(uid, QString::fromLatin1(user->pw_name));
            str = user->pw_name;
        } else {
            str = QString::number(uid);
        }
    } else {
        str = m_usercache.value(uid);
    }

    entry.insert(KIO::UDSEntry::UDS_USER, str);

    const gid_t gid = attributes.gid;
    if (!m_groupcache.contains(gid)) {
        struct group* grp = getgrgid(gid);
        if (grp) {
            m_groupcache.insert(gid, QString::fromLatin1(grp->gr_name));
            str = grp->gr_name;
        } else {
            str = QString::number(gid);
        }
    } else {
        str = m_groupcache.value(gid);
    }

    entry.insert(KIO::UDSEntry::UDS_GROUP, str);
}

void NFSProtocolV2::completeBadLinkUDSEntry(KIO::UDSEntry& entry, const fattr& attributes)
{
    entry.insert(KIO::UDSEntry::UDS_SIZE, 0LL);
    entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, attributes.mtime.seconds);
    entry.insert(KIO::UDSEntry::UDS_ACCESS_TIME, attributes.atime.seconds);
    entry.insert(KIO::UDSEntry::UDS_CREATION_TIME, attributes.ctime.seconds);
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFMT - 1);
    entry.insert(KIO::UDSEntry::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IRWXO);
    entry.insert(KIO::UDSEntry::UDS_USER, attributes.uid);
    entry.insert(KIO::UDSEntry::UDS_GROUP, attributes.gid);
}
