/*  This file is part of the KDE project

    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>
    SPDX-FileCopyrightText: 2014 Mathias Tillman <master.homer@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "nfsv3.h"
#include "kio_nfs_debug.h"

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
#include <QMimeType>
#include <QMimeDatabase>

#include <KLocalizedString>
#include <kio/global.h>
#include <kio/ioslave_defaults.h>

// For the complete NFSv3 reference see http://tools.ietf.org/html/rfc1813

// This ioslave is for NFS version 3.
#define NFSPROG 100003UL
#define NFSVERS 3UL

#define NFS3_MAXDATA    32768
#define NFS3_MAXPATHLEN PATH_MAX


NFSProtocolV3::NFSProtocolV3(NFSSlave* slave)
    : NFSProtocol(slave),
      m_mountClient(nullptr),
      m_mountSock(-1),
      m_nfsClient(nullptr),
      m_nfsSock(-1),
      m_readBufferSize(0),
      m_writeBufferSize(0),
      m_readDirSize(0)
{
    qCDebug(LOG_KIO_NFS);

    clnt_timeout.tv_sec = 20;
    clnt_timeout.tv_usec = 0;
}

NFSProtocolV3::~NFSProtocolV3()
{
    closeConnection();
}

bool NFSProtocolV3::isCompatible(bool& connectionError)
{
    int ret = -1;

    CLIENT* client = nullptr;
    int sock = 0;
    if (NFSProtocol::openConnection(currentHost(), NFSPROG, NFSVERS, client, sock) == 0) {
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

    qCDebug(LOG_KIO_NFS) << "RPC status" << ret << "connectionError" << connectionError;
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
    NFSFileHandle fh;
    int rpcStatus;
    LOOKUP3res res;
    if (lookupHandle(path, rpcStatus, res)) {
        fh = res.LOOKUP3res_u.resok.object;

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

            if (rpcStatus == RPC_SUCCESS && readLinkRes.status == NFS3_OK)
            {   // get the absolute link target
                QString linkPath = QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data);
                linkPath = QFileInfo(QFileInfo(path).path(), linkPath).absoluteFilePath();

                // As with the tests done in NFSProtocol::isValidLink(), the link
                // target may not be valid on the NFS server (i.e. it may point
                // outside of the exported directories).  Check for this before
                // calling lookupHandle() on the target of the link, as otherwise
                // an error will be set which is not relevant.
                if (isValidPath(linkPath))
                {
                    LOOKUP3res linkRes;
                    if (lookupHandle(linkPath, rpcStatus, linkRes))
                    {
                        // It's a link, so return the target file handle
                        // with the link source recorded in it.
                        NFSFileHandle linkFh = linkRes.LOOKUP3res_u.resok.object;
                        linkFh.setLinkSource(res.LOOKUP3res_u.resok.object);
                        qCDebug(LOG_KIO_NFS) << "Found link target" << linkPath;
                        return linkFh;
                    }
                }
            }

            // If we have reached this point the file is a link,
            // but we failed to read the target.
            fh.setBadLink();
            qCDebug(LOG_KIO_NFS) << "Invalid link" << path;
        }
    }

    return fh;
}

/* Open connection connects to the mount daemon on the server side.
 In order to do this it needs authentication and calls auth_unix_create().
 Then it asks the mount daemon for the exported shares. Then it tries
 to mount all these shares. If this succeeded for at least one of them,
 a client for the nfs daemon is created.
 */
void NFSProtocolV3::openConnection()
{
    const QString host = currentHost();
    qCDebug(LOG_KIO_NFS) << "to" << host;

    // Destroy the old connection first
    closeConnection();

    KIO::Error connErr = NFSProtocol::openConnection(host, MOUNT_PROGRAM, MOUNT_V3, m_mountClient, m_mountSock);
    if (connErr != 0) {
        closeConnection();
        setError(connErr, host);
        return;
    }

    exports3 exportlist;
    memset(&exportlist, 0, sizeof(exportlist));

    int clnt_stat = clnt_call(m_mountClient, MOUNTPROC3_EXPORT,
                              (xdrproc_t) xdr_void, nullptr,
                              (xdrproc_t) xdr_exports3, reinterpret_cast<caddr_t>(&exportlist),
                              clnt_timeout);

    if (!checkForError(clnt_stat, 0, host.toLatin1())) {
        closeConnection();
        return;
    }

    int exportsCount = 0;
    bool mountHint = false;

    mountres3 fhStatus;
    for (; exportlist != nullptr; exportlist = exportlist->ex_next, exportsCount++) {
        memset(&fhStatus, 0, sizeof(fhStatus));
        clnt_stat = clnt_call(m_mountClient, MOUNTPROC3_MNT,
                              (xdrproc_t) xdr_dirpath3, reinterpret_cast<caddr_t>(&exportlist->ex_dir),
                              (xdrproc_t) xdr_mountres3, reinterpret_cast<caddr_t>(&fhStatus),
                              clnt_timeout);

        QString fname = QFileInfo(QDir::root(), exportlist->ex_dir).filePath();
        if (fhStatus.fhs_status == 0) {         // mount succeeded

            // Check if the directory is already noted as exported,
            // if so there is no need to add it again.
            if (isExportedDir(fname)) continue;

            // Save the exported directory and its NFS file handle.
            addFileHandle(fname, static_cast<NFSFileHandle>(fhStatus.mountres3_u.mountinfo.fhandle));
            addExportedDir(fname);
        } else {                    // mount failed with error
            qCDebug(LOG_KIO_NFS) << "Cannot mount" << fname << "- status" << fhStatus.fhs_status;

            // Even if the mount failed, record the directory path as exported
            // so that it can be listed as a virtual directory.  However, do
            // not record its (invalid) file handle in the cache.  Trying to
            // access the directory in any way other than just listing it, or
            // accessing anything below it, will be detected in
            // NFSProtocol::getFileHandle() and fail with an appropriate
            // error.
            if (!isExportedDir(fname)) addExportedDir(fname);

            // Many modern NFS servers by default reject any access attempted to
            // them from a non-reserved source port (i.e. above 1024).  Since
            // this KIO slave runs as a normal user, it is not able to use the
            // reserved port numbers and hence the access will be rejected.  Show
            // a hint if this could possibly be the problem - only once, as the
            // server may have many exported directories.
            if (fhStatus.fhs_status == MNT3ERR_ACCES) {
                if (!mountHint) {
                    qCDebug(LOG_KIO_NFS) << "Check that the NFS server is exporting the filesystem";
                    qCDebug(LOG_KIO_NFS) << "with appropriate access permissions.  Note that it must";
                    qCDebug(LOG_KIO_NFS) << "allow mount requests originating from an unprivileged";
                    qCDebug(LOG_KIO_NFS) << "source port (see exports(5), the 'insecure' option may";
                    qCDebug(LOG_KIO_NFS) << "be required).";
                    mountHint = true;
                }
            }
        }
    }

    // If nothing can be mounted then there is no point trying to open the
    // NFS server connection here.  However, call openConnection() anyway
    // and pretend that we are connected so that listing virtual directories
    // will work.
    if ((connErr = NFSProtocol::openConnection(host, NFSPROG, NFSVERS, m_nfsClient, m_nfsSock)) != 0) {
        closeConnection();
        setError(connErr, host);
    }

    slave()->connected();

    qCDebug(LOG_KIO_NFS) << "openConnection succeeded";
}


void NFSProtocolV3::listDir(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path = listDirInternal(url);      // check path, list virtual dir
    if (path.isEmpty()) return;             // no more to do

    const NFSFileHandle fh = getFileHandle(path);
    // There doesn't seem to be an invalid link error code in KIO,
    // so this will have to do.
    if (fh.isInvalid() || fh.isBadLink()) {
        setError(KIO::ERR_DOES_NOT_EXIST, path);
        return;
    }

    // Get the preferred read dir size from the server
    if (m_readDirSize == 0) {
        initPreferredSizes(fh);
    }

    if (!slave()->usedirplus3())            // want compatibility mode listing
    {
        listDirCompat(url);
        return;
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
            qCDebug(LOG_KIO_NFS) << "NFS server does not support READDIRPLUS3, listing in compatibility mode";
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
                slave()->listEntry(entry);
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
                    if (isValidLink(path, linkDest))
                    {
                        const QString linkPath = QFileInfo(path, linkDest).absoluteFilePath();
                        // get the absolute link target
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

                // Some NFS servers seem to return names from the READDIRPLUS3
                // call without a valid file handle or attributes.  This has
                // been observed with a WD MyCloud NAS running Debian wheezy.
                // The READDIR3 call does not have this problem, but by the time
                // it can be detected it is too late to repeat the listing in
                // compatibility mode since some listEntry()'s may already have
                // been done.
                //
                // Accept the name, and assume null values for the attributes.
                // Do not save the invalid file handle in the cache;  if that
                // subdirectory is accessed later on it will be listed again
                // which seems to work properly.
                if (entryFH.isInvalid()) {
                    qCDebug(LOG_KIO_NFS) << "NFS server returned invalid handle for" << (path+"/"+dirEntry->name);
                    completeInvalidUDSEntry(entry);
                } else {
                    addFileHandle(filePath, entryFH);
                    completeUDSEntry(entry, dirEntry->name_attributes.post_op_attr_u.attributes);
                }
            }

            slave()->listEntry(entry);

            lastEntry = dirEntry;
        }
    } while (listres.READDIRPLUS3res_u.resok.reply.entries != nullptr && !listres.READDIRPLUS3res_u.resok.reply.eof);
}


void NFSProtocolV3::listDirCompat(const QUrl& url)
{
    const QString path(url.path());

    if (isExportedDir(path))
    {
        // We should never get here, if the path is an exported dir
        // it will have been checked in listDir() and there will have been
        // no attempt to access the NFS server.
        qCWarning(LOG_KIO_NFS) << "Called for an exported dir";
        setError(KIO::ERR_INTERNAL, path);
        return;
    }

    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid() || fh.isBadLink()) {
        setError(KIO::ERR_DOES_NOT_EXIST, path);
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
            if (dirEntry->name != QString("..")) {
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
                if (isValidLink(path, linkDest))
                {
                    const QString linkPath = QFileInfo(path, linkDest).absoluteFilePath();
                    // get the absolute link target
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

        slave()->listEntry(entry);
    }
}

void NFSProtocolV3::stat(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path = statInternal(url);     // check path, process virtual dir
    if (path.isEmpty()) return;             // no more to do

    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid())
    {
        qCDebug(LOG_KIO_NFS) << "File handle is invalid";
        setError(KIO::ERR_DOES_NOT_EXIST, path);
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

            slave()->statEntry(entry);
            return;                 // have result, no more to do
        }

        qCDebug(LOG_KIO_NFS) << "link dest is" << linkDest;

        entry.fastInsert(KIO::UDSEntry::UDS_LINK_DEST, linkDest);

        if (!isValidLink(fileInfo.path(), linkDest)) {
            completeBadLinkUDSEntry(entry, attrAndStat.GETATTR3res_u.resok.obj_attributes);
        } else {
            const QString linkPath = QFileInfo(fileInfo.path(), linkDest).absoluteFilePath();
            // get the absolute link target
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

    slave()->statEntry(entry);
}


void NFSProtocolV3::mkdir(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    const QFileInfo fileInfo(path);
    if (isExportedDir(fileInfo.path())) {
        setError(KIO::ERR_ACCESS_DENIED, path);
        return;
    }

    const NFSFileHandle fh = getFileHandle(fileInfo.path());
    if (fh.isInvalid() || fh.isBadLink()) {
        setError(KIO::ERR_DOES_NOT_EXIST, path);
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
    checkForError(clnt_stat, dirres.status, path);
}


void NFSProtocolV3::del(const QUrl& url, bool/* isfile*/)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    if (isExportedDir(QFileInfo(path).path())) {
        setError(KIO::ERR_ACCESS_DENIED, path);
        return;
    }

    int rpcStatus;
    REMOVE3res res;
    remove(path, rpcStatus, res);
    checkForError(rpcStatus, res.status, path);
}


void NFSProtocolV3::chmod(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    if (isExportedDir(path)) {
        setError(KIO::ERR_ACCESS_DENIED, path);
        return;
    }

    sattr3 attributes;
    memset(&attributes, 0, sizeof(attributes));
    attributes.mode.set_it = true;
    attributes.mode.set_mode3_u.mode = permissions;

    int rpcStatus;
    SETATTR3res setAttrRes;
    setAttr(path, attributes, rpcStatus, setAttrRes);
    checkForError(rpcStatus, setAttrRes.status, path);
}


void NFSProtocolV3::get(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    const NFSFileHandle fh = getFileHandle(path);
    if (fh.isInvalid() || fh.isBadLink()) {
        setError(KIO::ERR_DOES_NOT_EXIST, path);
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
        setError(KIO::ERR_OUT_OF_MEMORY, path);
        return;
    }

    bool validRead = false;
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
            break;
        }

        read = readRes.READ3res_u.resok.count;
        readBuffer.setRawData(readRes.READ3res_u.resok.data.data_val, read);

        if (readArgs.offset == 0) {
            const QMimeDatabase db;
            const QMimeType type = db.mimeTypeForFileNameAndData(url.fileName(), readBuffer);
            slave()->mimeType(type.name());

            slave()->totalSize(readRes.READ3res_u.resok.file_attributes.post_op_attr_u.attributes.size);
        }

        readArgs.offset += read;
        if (read > 0) {
            validRead = true;

            slave()->data(readBuffer);
            slave()->processedSize(readArgs.offset);
        }

    } while (read > 0);

    if (readRes.READ3res_u.resok.data.data_val != nullptr) {
        delete [] readRes.READ3res_u.resok.data.data_val;
    }

    // Only send the read data to the slave if we have actually sent some.
    if (validRead) {
        slave()->data(QByteArray());
        slave()->processedSize(readArgs.offset);
    }
}

void NFSProtocolV3::put(const QUrl& url, int _mode, KIO::JobFlags flags)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString destPath(url.path());
    if (isExportedDir(QFileInfo(destPath).path())) {
        setError(KIO::ERR_WRITE_ACCESS_DENIED, destPath);
        return;
    }

    NFSFileHandle destFH = getFileHandle(destPath);
    if (destFH.isBadLink()) {
        setError(KIO::ERR_DOES_NOT_EXIST, destPath);
        return;
    }

    // the file exists and we don't want to overwrite
    if (!destFH.isInvalid() && ((flags & KIO::Overwrite) == 0)) {
        setError(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    // Get the optimal write buffer size
    if (m_writeBufferSize == 0) {
        initPreferredSizes(destFH);
    }

    destFH = create(destPath, _mode);
    if (destFH.isInvalid()) {
        return;
    }
    // We created the file successfully.

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
        slave()->dataReq();
        result = slave()->readData(buffer);

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
}

void NFSProtocolV3::rename(const QUrl& src, const QUrl& dest, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << dest;

    const QString srcPath(src.path());
    if (isExportedDir(srcPath)) {
        setError(KIO::ERR_CANNOT_RENAME, srcPath);
        return;
    }

    const QString destPath(dest.path());
    if (isExportedDir(destPath)) {
        setError(KIO::ERR_ACCESS_DENIED, destPath);
        return;
    }

    if (!getFileHandle(destPath).isInvalid() && (_flags & KIO::Overwrite) == 0) {
        setError(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    int rpcStatus;
    RENAME3res res;
    rename(srcPath, destPath, rpcStatus, res);
    checkForError(rpcStatus, res.status, destPath);
}


void NFSProtocolV3::copySame(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    const QString srcPath(src.path());
    if (isExportedDir(QFileInfo(srcPath).path())) {
        setError(KIO::ERR_ACCESS_DENIED, srcPath);
        return;
    }

    const NFSFileHandle srcFH = getFileHandle(srcPath);
    if (srcFH.isInvalid()) {
        setError(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath(dest.path());
    if (isExportedDir(QFileInfo(destPath).path())) {
        setError(KIO::ERR_ACCESS_DENIED, destPath);
        return;
    }

    // The file exists and we don't want to overwrite
    if (!getFileHandle(destPath).isInvalid() && (_flags & KIO::Overwrite) == 0) {
        setError(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    // Is it a link? No need to copy the data then, just copy the link destination.
    if (srcFH.isLink()) {
        //get the link dest
        int rpcStatus;
        READLINK3res readLinkRes;
        char nameBuf[NFS3_MAXPATHLEN];
        if (!symLinkTarget(srcPath, rpcStatus, readLinkRes, nameBuf)) {
            setError(KIO::ERR_DOES_NOT_EXIST, srcPath);
            return;
        }

        const QString linkPath = QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data);

        SYMLINK3res linkRes;
        symLink(linkPath, destPath, rpcStatus, linkRes);
        checkForError(rpcStatus, linkRes.status, linkPath);
        return;                     // done, no more to do
    }

    unsigned long resumeOffset = 0;
    bool bResume = false;
    const QString partFilePath = destPath + QLatin1String(".part");
    const NFSFileHandle partFH = getFileHandle(partFilePath);
    const bool bPartExists = !partFH.isInvalid();
    const bool bMarkPartial = slave()->configValue(QStringLiteral("MarkPartial"), true);

    if (bPartExists) {
        int rpcStatus;
        LOOKUP3res partRes;
        if (lookupHandle(partFilePath, rpcStatus, partRes)) {
            if (bMarkPartial && partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size > 0) {
                if (partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.type == NF3DIR) {
                    setError(KIO::ERR_IS_DIRECTORY, partFilePath);
                    return;
                }

                bResume = slave()->canResume(partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size);
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

        destFH = create(createPath, _mode);
        if (destFH.isInvalid()) {
            return;
        }
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
            slave()->mimeType(type.name());

            slave()->totalSize(readRes.READ3res_u.resok.file_attributes.post_op_attr_u.attributes.size);
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

            slave()->processedSize(readArgs.offset);
        }
    } while (bytesRead > 0);

    delete [] writeArgs.data.data_val;

    if (error) {
        if (bMarkPartial) {
            // Remove the part file if it's smaller than the minimum keep size.
            const unsigned int size = slave()->configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
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
                setError(KIO::ERR_CANNOT_RENAME_PARTIAL, partFilePath);
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

        slave()->processedSize(readArgs.offset);
    }
}

void NFSProtocolV3::copyFrom(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    const QString srcPath(src.path());
    const NFSFileHandle srcFH = getFileHandle(srcPath);
    if (srcFH.isInvalid()) {
        setError(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath(dest.path());
    // The file exists and we don't want to overwrite.
    if (QFile::exists(destPath) && (_flags & KIO::Overwrite) == 0) {
        setError(KIO::ERR_FILE_ALREADY_EXIST, destPath);
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
            setError(KIO::ERR_DOES_NOT_EXIST, srcPath);
            return;
        }

        QFile::link(QString::fromLocal8Bit(readLinkRes.READLINK3res_u.resok.data), destPath);
        return;                     // done, no more to do
    }

    if (m_readBufferSize == 0) {
        initPreferredSizes(srcFH);
    }

    unsigned int resumeOffset = 0;
    bool bResume = false;
    const QFileInfo partInfo(destPath + QLatin1String(".part"));
    const bool bPartExists = partInfo.exists();
    const bool bMarkPartial = slave()->configValue(QStringLiteral("MarkPartial"), true);

    if (bMarkPartial && bPartExists && partInfo.size() > 0) {
        if (partInfo.isDir()) {
            setError(KIO::ERR_IS_DIRECTORY, partInfo.absoluteFilePath());
            return;
        }

        bResume = slave()->canResume(partInfo.size());
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
                setError(KIO::ERR_CANNOT_RESUME, destPath);
            } else {
                setError(KIO::ERR_CANNOT_OPEN_FOR_WRITING, destPath);
            }
            break;
        case QFile::PermissionsError:
            setError(KIO::ERR_WRITE_ACCESS_DENIED, destPath);
            break;
        default:
            setError(KIO::ERR_CANNOT_OPEN_FOR_WRITING, destPath);
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

            slave()->totalSize(bytesToRead);

            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFileNameAndData(src.fileName(), QByteArray::fromRawData(readRes.READ3res_u.resok.data.data_val, bytesRead));
            slave()->mimeType(type.name());
        }

        if (bytesRead > 0) {
            readArgs.offset += bytesRead;

            if (destFile.write(readRes.READ3res_u.resok.data.data_val, bytesRead) < 0) {
                setError(KIO::ERR_CANNOT_WRITE, destPath);

                error = true;
                break;
            }

            slave()->processedSize(readArgs.offset);
        }
    } while (readArgs.offset < bytesToRead);

    delete [] readRes.READ3res_u.resok.data.data_val;

    // Close the file so we can modify the modification time later.
    destFile.close();

    if (error) {
        if (bMarkPartial) {
            // Remove the part file if it's smaller than the minimum keep
            const int size = slave()->configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
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
                setError(KIO::ERR_CANNOT_RENAME_PARTIAL, sPart);
                return;
            }
        }

        // Restore the mtime on the file.
        const QString mtimeStr = slave()->metaData("modified");
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

        slave()->processedSize(readArgs.offset);
    }
}

void NFSProtocolV3::copyTo(const QUrl& src, const QUrl& dest, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS) << src << "to" << dest;

    // The source does not exist, how strange
    const QString srcPath(src.path());
    if (!QFile::exists(srcPath)) {
        setError(KIO::ERR_DOES_NOT_EXIST, srcPath);
        return;
    }

    const QString destPath(dest.path());
    if (isExportedDir(QFileInfo(destPath).path())) {
        setError(KIO::ERR_ACCESS_DENIED, destPath);
        return;
    }

    // The file exists and we don't want to overwrite.
    if (!getFileHandle(destPath).isInvalid() && (_flags & KIO::Overwrite) == 0) {
        setError(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }

    // Is it a link? No need to copy the data then, just copy the link destination.
    const QString symlinkTarget = QFile::symLinkTarget(srcPath);
    if (!symlinkTarget.isEmpty()) {
        int rpcStatus;
        SYMLINK3res linkRes;

        symLink(symlinkTarget, destPath, rpcStatus, linkRes);
        checkForError(rpcStatus, linkRes.status, symlinkTarget);
        return;                     // done, no more to do
    }

    unsigned long resumeOffset = 0;
    bool bResume = false;
    const QString partFilePath = destPath + QLatin1String(".part");
    const NFSFileHandle partFH = getFileHandle(partFilePath);
    const bool bPartExists = !partFH.isInvalid();
    const bool bMarkPartial = slave()->configValue(QStringLiteral("MarkPartial"), true);

    if (bPartExists) {
        int rpcStatus;
        LOOKUP3res partRes;
        if (lookupHandle(partFilePath, rpcStatus, partRes)) {
            if (bMarkPartial && partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size > 0) {
                if (partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.type == NF3DIR) {
                    setError(KIO::ERR_IS_DIRECTORY, partFilePath);
                    return;
                }

                bResume = slave()->canResume(partRes.LOOKUP3res_u.resok.obj_attributes.post_op_attr_u.attributes.size);
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
        setError(KIO::ERR_CANNOT_OPEN_FOR_READING, srcPath);
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

        destFH = create(createPath, _mode);
        if (destFH.isInvalid()) {
            return;
        }
    } else {
        // Since we are resuming it's implied that we are using a part file,
        // which should exist at this point.
        destFH = getFileHandle(partFilePath);

        qCDebug(LOG_KIO_NFS) << "Resuming old transfer";
    }

    // Send the total size to the slave.
    slave()->totalSize(srcFile.size());

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
    memset(&writeRes, 0, sizeof(writeRes));

    bool error = false;
    int bytesRead = 0;
    do {
        memset(writeArgs.data.data_val, 0, m_writeBufferSize);

        bytesRead = srcFile.read(writeArgs.data.data_val, m_writeBufferSize);
        if (bytesRead < 0) {
            setError(KIO::ERR_CANNOT_READ, srcPath);

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

            slave()->processedSize(writeArgs.offset);
        }
    } while (bytesRead > 0);

    delete [] writeArgs.data.data_val;

    if (error) {
        if (bMarkPartial) {
            // Remove the part file if it's smaller than the minimum keep size.
            const unsigned int size = slave()->configValue(QStringLiteral("MinimumKeepSize"), DEFAULT_MINIMUM_KEEP_SIZE);
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
                setError(KIO::ERR_CANNOT_RENAME_PARTIAL, partFilePath);
                return;
            }
        }

        // Restore the mtime on the file.
        const QString mtimeStr = slave()->metaData("modified");
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

        slave()->processedSize(writeArgs.offset);
    }
}

void NFSProtocolV3::symlink(const QString& target, const QUrl& dest, KIO::JobFlags flags)
{
    const QString destPath(dest.path());
    if (isExportedDir(QFileInfo(destPath).path())) {
        setError(KIO::ERR_ACCESS_DENIED, destPath);
        return;
    }

    if (!getFileHandle(destPath).isInvalid() && (flags & KIO::Overwrite) == 0) {
        setError(KIO::ERR_FILE_ALREADY_EXIST, destPath);
        return;
    }


    int rpcStatus;
    SYMLINK3res res;
    symLink(target, destPath, rpcStatus, res);
    checkForError(rpcStatus, res.status, destPath);
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


NFSFileHandle NFSProtocolV3::create(const QString& path, int mode)
{
    qCDebug(LOG_KIO_NFS) << path;

    if (!isConnected()) {
        setError(KIO::ERR_CANNOT_CONNECT, path);
        return NFSFileHandle();
    }

    const QFileInfo fileInfo(path);

    const NFSFileHandle directoryFH = getFileHandle(fileInfo.path());
    if (directoryFH.isInvalid()) {
        setError(KIO::ERR_DOES_NOT_EXIST, fileInfo.path());
        return NFSFileHandle();
    }

    QByteArray tmpName = QFile::encodeName(fileInfo.fileName());

    int rpcStatus = 0;
    CREATE3res result;
    memset(&result, 0, sizeof(result));

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

    if (rpcStatus != RPC_SUCCESS || result.status != NFS3_OK)
    {
        checkForError(rpcStatus, result.status, path);
        return NFSFileHandle();
    }

    return result.CREATE3res_u.resok.obj.post_op_fh3_u.handle;
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

    NFSProtocol::completeUDSEntry(entry, attributes.uid, attributes.gid);
}


void NFSProtocolV3::completeBadLinkUDSEntry(KIO::UDSEntry& entry, const fattr3& attributes)
{
    entry.replace(KIO::UDSEntry::UDS_MODIFICATION_TIME, attributes.mtime.seconds);
    entry.replace(KIO::UDSEntry::UDS_ACCESS_TIME, attributes.atime.seconds);

    NFSProtocol::completeInvalidUDSEntry(entry);
}
