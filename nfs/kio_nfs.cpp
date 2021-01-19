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
#include <QCoreApplication>

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
    QCoreApplication app(argc, argv);
    app.setApplicationName(QLatin1String("kio_nfs"));

    if (argc != 4) {
        fprintf(stderr, "Usage: kio_nfs protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    NFSSlave slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}


// Both the insertion and lookup in the file handle cache (managed by
// NFSProtocol), and the use of QFileInfo to locate a parent directory,
// are sensitive to paths having trailing slashes.  In order to keep
// everything consistent, any URLs passed in must be cleaned before using
// them as a fileystem or NFS protocol path.

static QUrl cleanPath(const QUrl &url)
{
    return (url.adjusted(QUrl::StripTrailingSlash|QUrl::NormalizePathSegments));
}


NFSSlave::NFSSlave(const QByteArray& pool, const QByteArray& app)
    :  KIO::SlaveBase("nfs", pool, app),
       m_protocol(nullptr),
       m_errorId(KIO::Error(0))
{
    qCDebug(LOG_KIO_NFS) << pool << app;
}

NFSSlave::~NFSSlave()
{
    delete m_protocol;
}

void NFSSlave::openConnection()
{
    qCDebug(LOG_KIO_NFS);

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
                setError(KIO::ERR_SLAVE_DEFINED, i18n("Cannot find an NFS version that host '%1' supports", m_host));
            } else {
                setError(KIO::ERR_CANNOT_CONNECT, m_host);
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

    if (verifyProtocol(url)) {
        m_protocol->put(cleanPath(url), _mode, _flags);
    }
    finishOperation();
}

void NFSSlave::get(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol(url)) {
        m_protocol->get(cleanPath(url));
    }
    finishOperation();
}

void NFSSlave::listDir(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS) << url;

    if (verifyProtocol(url)) {
        m_protocol->listDir(cleanPath(url));
    }
    finishOperation();
}

void NFSSlave::symlink(const QString& target, const QUrl& dest, KIO::JobFlags _flags)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol(dest)) {
        m_protocol->symlink(target, cleanPath(dest), _flags);
    }
    finishOperation();
}

void NFSSlave::stat(const QUrl& url)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol(url)) {
        m_protocol->stat(cleanPath(url));
    }
    finishOperation();
}

void NFSSlave::mkdir(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol(url)) {
        m_protocol->mkdir(cleanPath(url), permissions);
    }
    finishOperation();
}

void NFSSlave::del(const QUrl& url, bool isfile)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol(url)) {
        m_protocol->del(cleanPath(url), isfile);
    }
    finishOperation();
}

void NFSSlave::chmod(const QUrl& url, int permissions)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol(url)) {
        m_protocol->chmod(cleanPath(url), permissions);
    }
    finishOperation();
}

void NFSSlave::rename(const QUrl& src, const QUrl& dest, KIO::JobFlags flags)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol(src) && verifyProtocol(dest)) {
        m_protocol->rename(cleanPath(src), cleanPath(dest), flags);
    }
    finishOperation();
}

void NFSSlave::copy(const QUrl& src, const QUrl& dest, int mode, KIO::JobFlags flags)
{
    qCDebug(LOG_KIO_NFS);

    if (verifyProtocol(src) && verifyProtocol(dest)) {
        m_protocol->copy(cleanPath(src), cleanPath(dest), mode, flags);
    }
    finishOperation();
}


// Perform initial URL and host checks before starting any operation.
// This means any KIO::SlaveBase action which is expected to end by
// calling either error() or finished().
bool NFSSlave::verifyProtocol(const QUrl &url)
{
    // The NFS protocol definition includes copyToFile=true and copyFromFile=true,
    // so the URL scheme here can also be "file".  No URL or protocol checking
    // is required in this case.
    if (url.scheme() != "nfs") return true;

    m_errorId = KIO::Error(0);				// ensure reset before starting
    m_errorText.clear();

    // A NFS URL must include a host name, if it does not then nothing
    // sensible can be done.  Doing the check here and returning immediately
    // avoids multiple calls of SlaveBase::error() as each protocol is tried
    // in NFSSlave::openConnection().

    const QString host = url.host();
    if (host.isEmpty())
    {
        // KIO::ERR_UNKNOWN_HOST with a blank host name results in the
        // error message "No hostname specified", but Konqueror does not
        // report it properly.  Return our own message.
        setError(KIO::ERR_SLAVE_DEFINED, i18n("The NFS protocol requires a server host name."));
        goto fail;
    }
    else
    {
        // There is a host name, so check that it can be resolved.  If it
        // can't, return an error now and don't bother trying the protocol.
        QHostInfo hostInfo = QHostInfo::fromName(host);
        if (hostInfo.error() != QHostInfo::NoError)
        {
            qCDebug(LOG_KIO_NFS) << "host lookup of" << host << "error" << hostInfo.errorString();
            setError(KIO::ERR_UNKNOWN_HOST, host);
            goto fail;
        }
    }

    if (m_protocol==nullptr)				// no protocol connection yet
    {
        openConnection();				// create and open connection
        if (m_protocol==nullptr)			// if that failed, then
        {						// no more can be done
            qCDebug(LOG_KIO_NFS) << "Could not resolve a compatible protocol version!";
            goto fail;
        }
    }
    else if (!m_protocol->isConnected())		// already have a protocol
    {
        m_protocol->openConnection();			// open its connection
    }

    if (m_protocol->isConnected()) return true;		// connection succeeded

fail:							// default error if none already
    setError(KIO::ERR_INTERNAL, i18n("Failed to initialise protocol"));
    return false;
}


// These two functions keep track of errors found during any operation,
// and return the error or finish the operation appropriately when
// the operation is complete.
//
// NFSProtocol and classes derived from it, and anything that they call,
// should call setError() instead of SlaveBase::error().  When the
// operation is complete, just return and do not call SlaveBase::finished().

// Record the error information, but do not call SlaveBase::error().
// If there has been an error, finishOperation() will report it when
// the protocol operation is complete.
void NFSSlave::setError(KIO::Error errid, const QString &text)
{
    if (m_errorId!=0)
    {
        qCDebug(LOG_KIO_NFS) << errid << "ignored due to previous error";
        return;
    }

    qCDebug(LOG_KIO_NFS) << errid << text;
    m_errorId = errid;
    m_errorText = text;
}


// An operation is complete.  If there has been an error, then report it.
void NFSSlave::finishOperation()
{
    if (m_errorId==0) {					// no error encountered
        SlaveBase::finished();
    } else {						// there was an error
        SlaveBase::error(m_errorId, m_errorText);
    }
}


NFSFileHandle::NFSFileHandle()
    : m_handle(nullptr),
      m_size(0),
      m_linkHandle(nullptr),
      m_linkSize(0),
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
    // See whether the path is an exported directory:  that is, a prefix
    // of but not identical to any of the server exports.  If it is, then
    // it can be virtually listed and some operations are forbidden.
    // For example, if the server exports "/export/nfs/dir" then the root,
    // "/export" and "/export/nfs" are considered to be exported directories,
    // but "/export/nfs/dir" is not because it needs a server mount in order
    // to be listed.
    //
    // This function looks similar to, but is not the same as, isValidPath()
    // below.  This tests for "is the given path a prefix of any exported
    // directory", but isValidPath() tests for "is any exported directory equal
    // to or a prefix of the given path".

    // The root is always considered to be exported.
    if (path.isEmpty() || path == "/" || QFileInfo(path).isRoot())
    {
        qCDebug(LOG_KIO_NFS) << path << "is root";
        return true;
    }

    const QString dirPath = path+QDir::separator();
    for (QStringList::const_iterator it = m_exportedDirs.constBegin();
         it != m_exportedDirs.constEnd(); ++it) {
        const QString &exportedDir = (*it);
        // We know that both 'path' and the contents of m_exportedDirs
        // have been cleaned of any trailing slashes.
        if (exportedDir.startsWith(dirPath))
        {
            qCDebug(LOG_KIO_NFS) << path << "is exported";
            return true;
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
    if (fh.isInvalid()) qDebug() << "not adding" << path << "with invalid NFSFileHandle";
    else m_handleCache.insert(path, fh);
}

NFSFileHandle NFSProtocol::getFileHandle(const QString& path)
{
    if (!isConnected()) {
        return NFSFileHandle();
    }

    if (m_exportedDirs.contains(path))
    {
        // All exported directories should have already been stored in
        // m_handleCache by the protocol's openConnection().  If any
        // exported directory could not be mounted, then it will be in
        // m_exportedDirs but not in m_handleCache.  There is nothing more
        // that can be done in this case.
        if (!m_handleCache.contains(path)) {
            m_slave->setError(KIO::ERR_CANNOT_MOUNT, path);
            return NFSFileHandle();
        }
    }

    if (!isValidPath(path)) {
        qCDebug(LOG_KIO_NFS) << path << "is not a valid path";
        m_slave->setError(KIO::ERR_CANNOT_ENTER_DIRECTORY, path);
        return NFSFileHandle();
    }

    // In theory the root ("/") is a valid path but matches here.
    // However, it should never be seen unless the NFS server is
    // exporting its entire filesystem (which is very unlikely).
    if (path.endsWith('/')) {
        qCWarning(LOG_KIO_NFS) << "Passed a path ending with '/'.  Fix the caller.";
    }

    // The handle may already be in the cache, check it now.
    // The exported dirs are always in the cache, unless there was a
    // problem mounting them which will have been checked above.
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
        addFileHandle(path, childFH);
    }

    return childFH;
}

void NFSProtocol::removeFileHandle(const QString& path)
{
    m_handleCache.remove(path);
}


bool NFSProtocol::isValidPath(const QString& path)
{
    // See whether the path is or below an exported directory:
    // that is, any of the server exports is identical to or a prefix
    // of the path.  If it does not start with an exported prefix,
    // then it is not a valid NFS file path on the server.

    // This function looks similar to, but is not the same as,
    // isExportedDir() above.  This tests for "is any exported directory
    // equal to or is a prefix of the given path", but isExportedDir()
    // tests for "is the given path a prefix of any exported
    // directory".

    // The root is always considered to be valid.
    if (path.isEmpty() || path == "/" || QFileInfo(path).isRoot())
    {
        return true;
    }

    for (QStringList::const_iterator it = m_exportedDirs.constBegin();
         it != m_exportedDirs.constEnd(); ++it)
    {
        const QString &exportedDir = (*it);
        // We know that both 'path' and the contents of m_exportedDirs
        // have been cleaned of any trailing slashes.
        if (path == exportedDir) return true;
        if (path.startsWith(exportedDir+QDir::separator())) return true;
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


KIO::Error NFSProtocol::openConnection(const QString& host, int prog, int vers, CLIENT*& client, int& sock)
{
    // NFSSlave::verifyProtocol() should already have checked that
    // the host name is not blank and is resolveable, so the two
    // KIO::ERR_UNKNOWN_HOST failures here should never happen.

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
    return KIO::Error(0);
}


bool NFSProtocol::checkForError(int clientStat, int nfsStat, const QString& text)
{
    if (clientStat != RPC_SUCCESS)
    {
        qCDebug(LOG_KIO_NFS) << "RPC error" << clientStat << text;
        m_slave->setError(KIO::ERR_INTERNAL_SERVER, i18n("RPC error %1", QString::number(clientStat)));
        return false;
    }

    if (nfsStat != NFS_OK)
    {
        qCDebug(LOG_KIO_NFS) << "NFS error" << nfsStat << text;
        switch (nfsStat) {
        case NFSERR_PERM:
            m_slave->setError(KIO::ERR_ACCESS_DENIED, text);
            break;
        case NFSERR_NOENT:
            m_slave->setError(KIO::ERR_DOES_NOT_EXIST, text);
            break;
        //does this mapping make sense ?
        case NFSERR_IO:
            m_slave->setError(KIO::ERR_INTERNAL_SERVER, text);
            break;
        //does this mapping make sense ?
        case NFSERR_NXIO:
            m_slave->setError(KIO::ERR_DOES_NOT_EXIST, text);
            break;
        case NFSERR_ACCES:
            m_slave->setError(KIO::ERR_ACCESS_DENIED, text);
            break;
        case NFSERR_EXIST:
            m_slave->setError(KIO::ERR_FILE_ALREADY_EXIST, text);
            break;
        //does this mapping make sense ?
        case NFSERR_NODEV:
            m_slave->setError(KIO::ERR_DOES_NOT_EXIST, text);
            break;
        case NFSERR_NOTDIR:
            m_slave->setError(KIO::ERR_IS_FILE, text);
            break;
        case NFSERR_ISDIR:
            m_slave->setError(KIO::ERR_IS_DIRECTORY, text);
            break;
        //does this mapping make sense ?
        case NFSERR_FBIG:
            m_slave->setError(KIO::ERR_INTERNAL_SERVER, text);
            break;
        //does this mapping make sense ?
        case NFSERR_NOSPC:
            m_slave->setError(KIO::ERR_DISK_FULL, text);
            break;
        case NFSERR_ROFS:
            m_slave->setError(KIO::ERR_WRITE_ACCESS_DENIED, text);
            break;
        case NFSERR_NAMETOOLONG:
            m_slave->setError(KIO::ERR_INTERNAL_SERVER, i18n("Filename too long"));
            break;
        case NFSERR_NOTEMPTY:
            m_slave->setError(KIO::ERR_CANNOT_RMDIR, text);
            break;
        //does this mapping make sense ?
        case NFSERR_DQUOT:
            m_slave->setError(KIO::ERR_INTERNAL_SERVER, i18n("Disk quota exceeded"));
            break;
        case NFSERR_STALE:
            m_slave->setError(KIO::ERR_DOES_NOT_EXIST, text);
            break;
        default:
            m_slave->setError(KIO::ERR_UNKNOWN, i18n("NFS error %1, %2", QString::number(nfsStat), text));
            break;
        }
        return false;
    }
    return true;
}


// This uses KIO::UDSEntry::fastInsert() and so must only be called with
// a blank UDSEntry or one where only UDS_NAME has been filled in.
void NFSProtocol::createVirtualDirEntry(UDSEntry& entry)
{
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    entry.fastInsert(KIO::UDSEntry::UDS_USER, QString::fromLatin1("root"));
    entry.fastInsert(KIO::UDSEntry::UDS_GROUP, QString::fromLatin1("root"));
    // Dummy size.
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
}
