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

#include "kio_nfs.h"

#include <config-runtime.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <arpa/inet.h>
#include <netdb.h>

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QHostInfo>

#include <KLocalizedString>
#include <kio/global.h>

#include "nfsv2.h"
#include "nfsv3.h"

using namespace KIO;
using namespace std;

Q_LOGGING_CATEGORY(LOG_KIO_NFS, "kde.kio-nfs")


extern "C" int Q_DECL_EXPORT kdemain(int argc, char** argv);

int kdemain(int argc, char** argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: kio_nfs protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    qCDebug(LOG_KIO_NFS) << "NFS: kdemain: starting";

    NFSSlave slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}

NFSSlave::NFSSlave(const QByteArray& pool, const QByteArray& app)
    :  KIO::SlaveBase("nfs", pool, app),
       m_protocol(nullptr)
{

    qCDebug(LOG_KIO_NFS) << pool << app;
}
NFSSlave::~NFSSlave()
{
    if (m_protocol != nullptr) {
        delete m_protocol;
    }
}

void NFSSlave::openConnection()
{
    qCDebug(LOG_KIO_NFS) << "openConnection";

    if (m_protocol != nullptr) {
        m_protocol->openConnection();
    } else {
        bool connectionError = false;

        int version = 4;
        while (version > 1) {
            qCDebug(LOG_KIO_NFS) << "Trying NFS version" << version;

            // We need to create a new NFS protocol handler
            switch (version) {
            case 4: {
                // TODO
                qCDebug(LOG_KIO_NFS) << "NFSv4 is not supported at this time";
            }
            break;
            case 3: {
                m_protocol = new NFSProtocolV3(this);
            }
            break;
            case 2: {
                m_protocol = new NFSProtocolV2(this);
            }
            break;
            }

            // Unimplemented protocol version
            if (m_protocol == nullptr) {
                version--;
                continue;
            }

            m_protocol->setHost(m_host);
            if (m_protocol->isCompatible(connectionError)) {
                break;
            }

            version--;
            delete m_protocol;
            m_protocol = nullptr;
        }


        if (m_protocol == nullptr) {
            // If we could not find a compatible protocol, send an error.
            if (!connectionError) {
                error(KIO::ERR_CANNOT_CONNECT, i18n("%1: Unsupported NFS version", m_host));
            } else {
                error(KIO::ERR_CANNOT_CONNECT, m_host);
            }
        } else {
            // Otherwise we open the connection
            m_protocol->openConnection();
        }
    }
}

void NFSSlave::closeConnection()
{
    qCDebug(LOG_KIO_NFS);

    if (m_protocol != nullptr) {
        m_protocol->closeConnection();
    }
}

void NFSSlave::setHost(const QString& host, quint16 /*port*/, const QString& /*user*/, const QString& /*pass*/)
{
    qCDebug(LOG_KIO_NFS);

    if (m_protocol != nullptr) {
        // New host? New protocol!
        if (m_host != host) {
            qCDebug(LOG_KIO_NFS) << "Deleting old protocol";
            delete m_protocol;
            m_protocol = nullptr;
        } else {
            m_protocol->setHost(host);
        }
    }

    m_host = host;
}

void NFSSlave::put(const QUrl& url, int _mode, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->put(url, _mode, _flags);
    }
}

void NFSSlave::get(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->get(url);
    }
}

void NFSSlave::listDir(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    if (verifyProtocol()) {
        m_protocol->listDir(url);
    }
}

void NFSSlave::symlink(const QString& target, const QUrl& dest, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->symlink(target, dest, _flags);
    }
}

void NFSSlave::stat(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->stat(url);
    }
}

void NFSSlave::mkdir(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->mkdir(url, permissions);
    }
}

void NFSSlave::del(const QUrl& url, bool isfile)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->del(url, isfile);
    }
}

void NFSSlave::chmod(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->chmod(url, permissions);
    }
}

void NFSSlave::rename(const QUrl& src, const QUrl& dest, KIO::JobFlags flags)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->rename(src, dest, flags);
    }
}

void NFSSlave::copy(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol()) {
        m_protocol->copy(src, dest, mode, flags);
    }
}

bool NFSSlave::verifyProtocol()
{
    const bool haveProtocol = (m_protocol != nullptr);
    if (!haveProtocol) {
        openConnection();

        if (m_protocol == nullptr) {
            // We have failed.... :(
            qCDebug(LOG_KIO_NFS) << "Could not find a compatible protocol version!!";
            return false;
        }

        // If we are not connected @openConnection will have sent an
        // error message to the client, so it's safe to return here.
        if (!m_protocol->isConnected()) {
            return false;
        }
    } else if (!m_protocol->isConnected()) {
        m_protocol->openConnection();
        if (!m_protocol->isConnected()) {
            return false;
        }
    }

    if (m_protocol->isConnected()) {
        return true;
    }

    finished();
    return false;
}


NFSFileHandle::NFSFileHandle()
    : m_handle(nullptr),
      m_size(0),
      m_linkHandle(nullptr),
      m_linkSize(0),
      m_isInvalid(true),
      m_isLink(false)
{

}

NFSFileHandle::NFSFileHandle(const NFSFileHandle& src)
    :  NFSFileHandle()
{
    (*this) = src;
}

NFSFileHandle::NFSFileHandle(const fhandle3& src)
    :  NFSFileHandle()
{
    (*this) = src;
}

NFSFileHandle::NFSFileHandle(const fhandle& src)
    :  NFSFileHandle()
{
    (*this) = src;
}

NFSFileHandle::NFSFileHandle(const nfs_fh3& src)
    :  NFSFileHandle()
{
    (*this) = src;
}

NFSFileHandle::NFSFileHandle(const nfs_fh& src)
    :  NFSFileHandle()
{
    (*this) = src;
}

NFSFileHandle::~NFSFileHandle()
{
    if (m_handle != nullptr) {
        delete [] m_handle;
    }
    if (m_linkHandle != nullptr) {
        delete [] m_linkHandle;
    }
}

void NFSFileHandle::toFH(nfs_fh3& fh) const
{
    fh.data.data_len = m_size;
    fh.data.data_val = m_handle;
}

void NFSFileHandle::toFH(nfs_fh& fh) const
{
    memcpy(fh.data, m_handle, m_size);
}

void NFSFileHandle::toFHLink(nfs_fh3& fh) const
{
    fh.data.data_len = m_linkSize;
    fh.data.data_val = m_linkHandle;
}

void NFSFileHandle::toFHLink(nfs_fh& fh) const
{
    memcpy(fh.data, m_linkHandle, m_size);
}

NFSFileHandle& NFSFileHandle::operator=(const NFSFileHandle& src)
{
    if (src.m_size > 0) {
        if (m_handle != nullptr) {
            delete [] m_handle;
            m_handle = nullptr;
        }

        m_size = src.m_size;
        m_handle = new char[m_size];
        memcpy(m_handle, src.m_handle, m_size);
    }
    if (src.m_linkSize > 0) {
        if (m_linkHandle != nullptr) {
            delete [] m_linkHandle;
            m_linkHandle = nullptr;
        }

        m_linkSize = src.m_linkSize;
        m_linkHandle = new char[m_linkSize];
        memcpy(m_linkHandle, src.m_linkHandle, m_linkSize);
    }

    m_isInvalid = src.m_isInvalid;
    m_isLink = src.m_isLink;
    return *this;
}

NFSFileHandle& NFSFileHandle::operator=(const fhandle3& src)
{
    if (m_handle != nullptr) {
        delete [] m_handle;
        m_handle = nullptr;
    }

    m_size = src.fhandle3_len;
    m_handle = new char[m_size];
    memcpy(m_handle, src.fhandle3_val, m_size);
    m_isInvalid = false;
    return *this;
}

NFSFileHandle& NFSFileHandle::operator=(const fhandle& src)
{
    if (m_handle != nullptr) {
        delete [] m_handle;
        m_handle = nullptr;
    }

    m_size = NFS_FHSIZE;
    m_handle = new char[m_size];
    memcpy(m_handle, src, m_size);
    m_isInvalid = false;
    return *this;
}

NFSFileHandle& NFSFileHandle::operator=(const nfs_fh3& src)
{
    if (m_handle != nullptr) {
        delete [] m_handle;
        m_handle = nullptr;
    }

    m_size = src.data.data_len;
    m_handle = new char[m_size];
    memcpy(m_handle, src.data.data_val, m_size);
    m_isInvalid = false;
    return *this;
}

NFSFileHandle& NFSFileHandle::operator=(const nfs_fh& src)
{
    if (m_handle != nullptr) {
        delete [] m_handle;
        m_handle = nullptr;
    }

    m_size = NFS_FHSIZE;
    m_handle = new char[m_size];
    memcpy(m_handle, src.data, m_size);
    m_isInvalid = false;
    return *this;
}

void NFSFileHandle::setLinkSource(const nfs_fh3& src)
{
    if (m_linkHandle != nullptr) {
        delete [] m_linkHandle;
        m_linkHandle = nullptr;
    }

    m_linkSize = src.data.data_len;
    m_linkHandle = new char[m_linkSize];
    memcpy(m_linkHandle, src.data.data_val, m_linkSize);
    m_isLink = true;
}

void NFSFileHandle::setLinkSource(const nfs_fh& src)
{
    if (m_linkHandle != nullptr) {
        delete [] m_linkHandle;
        m_linkHandle = nullptr;
    }

    m_linkSize = NFS_FHSIZE;
    m_linkHandle = new char[m_linkSize];
    memcpy(m_linkHandle, src.data, m_linkSize);
    m_isLink = true;
}


NFSProtocol::NFSProtocol(NFSSlave* slave)
    : m_slave(slave)
{

}

void NFSProtocol::copy(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags)
{
    if (src.isLocalFile()) {
        copyTo(src, dest, mode, flags);
    } else if (dest.isLocalFile()) {
        copyFrom(src, dest, mode, flags);
    } else {
        copySame(src, dest, mode, flags);
    }
}

void NFSProtocol::addExportedDir(const QString& path)
{
    m_exportedDirs.append(path);
}

const QStringList& NFSProtocol::getExportedDirs()
{
    return m_exportedDirs;
}

bool NFSProtocol::isExportedDir(const QString& path)
{
    // If the path is the root filesystem we always return true.
    if (QFileInfo(path).isRoot()) {
        return true;
    }

    for (QStringList::const_iterator it = m_exportedDirs.constBegin(); it != m_exportedDirs.constEnd(); ++it) {
        if (path.length() < (*it).length() && (*it).startsWith(path)) {
            QString rest = (*it).mid(path.length());
            if (rest.isEmpty() || rest[0] == QDir::separator()) {
                qCDebug(LOG_KIO_NFS) << "isExportedDir" << path << "returning true";

                return true;
            }
        }
    }

    return false;
}

void NFSProtocol::removeExportedDir(const QString& path)
{
    m_exportedDirs.removeOne(path);
}

void NFSProtocol::addFileHandle(const QString& path, NFSFileHandle fh)
{
    m_handleCache.insert(path, fh);
}

NFSFileHandle NFSProtocol::getFileHandle(const QString& path)
{
    if (!isConnected()) {
        return NFSFileHandle();
    }

    if (!isValidPath(path)) {
        qCDebug(LOG_KIO_NFS) << path << "is not a valid path";
        return NFSFileHandle();
    }

    // The handle may already be in the cache, check it now.
    // The exported dirs are always in the cache.
    if (m_handleCache.contains(path)) {
        return m_handleCache[path];
    }

    // Loop detected, abort.
    if (QFileInfo(path).path() == path) {
        return NFSFileHandle();
    }

    // Look up the file handle from the protocol
    NFSFileHandle childFH = lookupFileHandle(path);
    if (!childFH.isInvalid()) {
        m_handleCache.insert(path, childFH);
    }

    return childFH;
}

void NFSProtocol::removeFileHandle(const QString& path)
{
    m_handleCache.remove(path);
}

bool NFSProtocol::isValidPath(const QString& path)
{
    if (path.isEmpty() || path == QDir::separator()) {
        return true;
    }

    for (QStringList::const_iterator it = m_exportedDirs.constBegin(); it != m_exportedDirs.constEnd(); ++it) {
        if ((path.length() == (*it).length() && path.startsWith((*it))) || (path.startsWith((*it) + QDir::separator()))) {
            return true;
        }
    }

    return false;
}

bool NFSProtocol::isValidLink(const QString& parentDir, const QString& linkDest)
{
    if (linkDest.isEmpty()) {
        return false;
    }

    if (QFileInfo(linkDest).isAbsolute()) {
        return (!getFileHandle(linkDest).isInvalid());
    } else {
        QString absDest = QFileInfo(parentDir, linkDest).filePath();
        absDest = QDir::cleanPath(absDest);
        return (!getFileHandle(absDest).isInvalid());
    }

    return false;
}

int NFSProtocol::openConnection(const QString& host, int prog, int vers, CLIENT*& client, int& sock)
{
    if (host.isEmpty()) {
        return KIO::ERR_UNKNOWN_HOST;
    }
    struct sockaddr_in server_addr;
    if (host[0] >= '0' && host[0] <= '9') {
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(host.toLatin1());
    } else {
        struct hostent* hp = gethostbyname(host.toLatin1());
        if (hp == nullptr) {
            return KIO::ERR_UNKNOWN_HOST;
        }
        server_addr.sin_family = AF_INET;
        memcpy(&server_addr.sin_addr, hp->h_addr, hp->h_length);
    }

    server_addr.sin_port = 0;

    sock = RPC_ANYSOCK;
    client = clnttcp_create(&server_addr, prog, vers, &sock, 0, 0);
    if (client == nullptr) {
        server_addr.sin_port = 0;
        sock = RPC_ANYSOCK;

        timeval pertry_timeout;
        pertry_timeout.tv_sec = 3;
        pertry_timeout.tv_usec = 0;
        client = clntudp_create(&server_addr, prog, vers, pertry_timeout, &sock);
        if (client == nullptr) {
            ::close(sock);
            return KIO::ERR_CANNOT_CONNECT;
        }
    }

    QString hostName = QHostInfo::localHostName();
    QString domainName = QHostInfo::localDomainName();
    if (!domainName.isEmpty()) {
        hostName = hostName + QLatin1Char('.') + domainName;
    }

    client->cl_auth = authunix_create(hostName.toUtf8().data(), geteuid(), getegid(), 0, nullptr);

    return 0;
}

bool NFSProtocol::checkForError(int clientStat, int nfsStat, const QString& text)
{
    if (clientStat != RPC_SUCCESS) {
        qCDebug(LOG_KIO_NFS) << "RPC error" << clientStat << text;

        m_slave->error(KIO::ERR_INTERNAL_SERVER, i18n("RPC error %1", clientStat));
        return false;
    }

    if (nfsStat != NFS_OK) {
        qCDebug(LOG_KIO_NFS) << "NFS error:" << nfsStat << text;
        switch (nfsStat) {
        case NFSERR_PERM:
            m_slave->error(KIO::ERR_ACCESS_DENIED, text);
            break;
        case NFSERR_NOENT:
            m_slave->error(KIO::ERR_DOES_NOT_EXIST, text);
            break;
        //does this mapping make sense ?
        case NFSERR_IO:
            m_slave->error(KIO::ERR_INTERNAL_SERVER, text);
            break;
        //does this mapping make sense ?
        case NFSERR_NXIO:
            m_slave->error(KIO::ERR_DOES_NOT_EXIST, text);
            break;
        case NFSERR_ACCES:
            m_slave->error(KIO::ERR_ACCESS_DENIED, text);
            break;
        case NFSERR_EXIST:
            m_slave->error(KIO::ERR_FILE_ALREADY_EXIST, text);
            break;
        //does this mapping make sense ?
        case NFSERR_NODEV:
            m_slave->error(KIO::ERR_DOES_NOT_EXIST, text);
            break;
        case NFSERR_NOTDIR:
            m_slave->error(KIO::ERR_IS_FILE, text);
            break;
        case NFSERR_ISDIR:
            m_slave->error(KIO::ERR_IS_DIRECTORY, text);
            break;
        //does this mapping make sense ?
        case NFSERR_FBIG:
            m_slave->error(KIO::ERR_INTERNAL_SERVER, text);
            break;
        //does this mapping make sense ?
        case NFSERR_NOSPC:
            m_slave->error(KIO::ERR_INTERNAL_SERVER, i18n("No space left on device"));
            break;
        case NFSERR_ROFS:
            m_slave->error(KIO::ERR_CANNOT_WRITE, i18n("Read only file system"));
            break;
        case NFSERR_NAMETOOLONG:
            m_slave->error(KIO::ERR_INTERNAL_SERVER, i18n("Filename too long"));
            break;
        case NFSERR_NOTEMPTY:
            m_slave->error(KIO::ERR_CANNOT_RMDIR, text);
            break;
        //does this mapping make sense ?
        case NFSERR_DQUOT:
            m_slave->error(KIO::ERR_INTERNAL_SERVER, i18n("Disk quota exceeded"));
            break;
        case NFSERR_STALE:
            m_slave->error(KIO::ERR_DOES_NOT_EXIST, text);
            break;
        default:
            m_slave->error(KIO::ERR_UNKNOWN, i18n("NFS error %1 - %2", nfsStat, text));
            break;
        }
        return false;
    }
    return true;
}

void NFSProtocol::createVirtualDirEntry(UDSEntry& entry)
{
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
    entry.insert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    entry.insert(KIO::UDSEntry::UDS_USER, QString::fromLatin1("root"));
    entry.insert(KIO::UDSEntry::UDS_GROUP, QString::fromLatin1("root"));
    // Dummy size.
    entry.insert(KIO::UDSEntry::UDS_SIZE, 0);
}

#include "moc_kio_nfs.cpp"
