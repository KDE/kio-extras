/*  This file is part of the KDE project

    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>
    SPDX-FileCopyrightText: 2014 Mathias Tillman <master.homer@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

#include <klocalizedstring.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <kio/global.h>

#include "kio_nfs_debug.h"
#include "nfsv2.h"
#include "nfsv3.h"

using namespace KIO;
using namespace std;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.nfs" FILE "nfs.json")
};

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
       m_usedirplus3(true),
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

    if (m_protocol != nullptr)
    {
        m_protocol->openConnection();
        return;
    }

    const KSharedConfig::Ptr cfg = KSharedConfig::openConfig("kionfsrc");

    const KConfigGroup grp1 = cfg->group("Default");        // default for all hosts
    int minproto = grp1.readEntry("minproto", 2);       // minimum NFS version to accept
    int maxproto = grp1.readEntry("maxproto", 4);       // maximum NFS version to try
    m_usedirplus3 = grp1.readEntry("usedirplus3", true);    // use READDIRPLUS3 for listing

    const KConfigGroup grp2 = cfg->group("Host "+m_host);
    if (grp2.exists())                      // look for host-specific settings
    {   // with default values from above
        minproto = grp2.readEntry("minproto", minproto);
        maxproto = grp2.readEntry("maxproto", maxproto);
        m_usedirplus3 = grp2.readEntry("usedirplus3", m_usedirplus3);
    }

    minproto = qBound(2, minproto, 4);              // enforce limits
    maxproto = qBound(minproto, maxproto, 4);
    qCDebug(LOG_KIO_NFS) << "configuration for" << m_host;
    qCDebug(LOG_KIO_NFS) << "minproto" << minproto << "maxproto" << maxproto << "usedirplus3" << m_usedirplus3;

    bool connectionError = false;

    int version = maxproto;
    while (version >= minproto)
    {
        qCDebug(LOG_KIO_NFS) << "Trying NFS version" << version;

        // Try to create an NFS protocol handler for that version
        switch (version)
        {
        case 4:     // TODO
            qCDebug(LOG_KIO_NFS) << "NFSv4 is not supported at this time";
            break;

        case 3:
            m_protocol = new NFSProtocolV3(this);
            break;

        case 2:
            m_protocol = new NFSProtocolV2(this);
            break;
        }

        if (m_protocol != nullptr)          // created protocol for that version
        {
            m_protocol->setHost(m_host, m_user);    // try to make initial connection
            if (m_protocol->isCompatible(connectionError)) break;
        }

        delete m_protocol;              // no point using that protocol
        --version;                  // try the next lower
        m_protocol = nullptr;               // try again with new protocol
    }

    if (m_protocol == nullptr)              // failed to find a protocol
    {
        if (!connectionError)               // but connection was possible
        {
            setError(KIO::ERR_SLAVE_DEFINED, i18n("Cannot find an NFS version that host '%1' supports", m_host));
        }
        else                        // connection failed
        {
            setError(KIO::ERR_CANNOT_CONNECT, m_host);
        }
    }
    else                        // usable protocol was created
    {
        m_protocol->openConnection();           // open the connection
    }
}


void NFSSlave::closeConnection()
{
    qCDebug(LOG_KIO_NFS);

    if (m_protocol != nullptr) {
        m_protocol->closeConnection();
    }
}


void NFSSlave::setHost(const QString& host, quint16 /*port*/, const QString &user, const QString& /*pass*/)
{
    qCDebug(LOG_KIO_NFS) << "host" << host << "user" << user;

    if (m_protocol != nullptr)
    {
        // New host or user? New protocol!
        if (host != m_host || user != m_user)
        {
            qCDebug(LOG_KIO_NFS) << "Deleting old protocol";
            delete m_protocol;
            m_protocol = nullptr;
        }
        else
        {
            // TODO: Doing this is pointless if nothing has changed
            m_protocol->setHost(host, user);
        }
    }

    m_host = host;
    m_user = user;
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
    m_errorId = KIO::Error(0);              // ensure reset before starting
    m_errorText.clear();

    // The NFS protocol definition includes copyToFile=true and copyFromFile=true,
    // so the URL scheme here can also be "file".  No URL or protocol checking
    // is required in this case.
    if (url.scheme() != "nfs") return true;

    if (!url.isValid())                 // also checks for empty
    {
        setError(KIO::ERR_MALFORMED_URL, url.toDisplayString());
        return (false);
    }

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
        return (false);
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
            return (false);
        }
    }

    if (m_protocol==nullptr)                // no protocol connection yet
    {
        openConnection();               // create and open connection
        if (m_protocol==nullptr)            // if that failed, then
        {   // no more can be done
            qCDebug(LOG_KIO_NFS) << "Could not resolve a compatible protocol version!";
            goto fail;
        }
    }
    else if (!m_protocol->isConnected())        // already have a protocol
    {
        m_protocol->openConnection();           // open its connection
    }

    if (m_protocol->isConnected()) return true;     // connection succeeded

fail:                           // default error if none already
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
    if (m_errorId==0) {                 // no error encountered
        SlaveBase::finished();
    } else {                        // there was an error
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
    if (fh.isInvalid()) qCDebug(LOG_KIO_NFS) << "not adding" << path << "with invalid NFSFileHandle";
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


bool NFSProtocol::isValidLink(const QString &parentDir, const QString &linkDest)
{
    qCDebug(LOG_KIO_NFS) << "checking" << linkDest << "in" << parentDir;

    if (linkDest.isEmpty()) return false;       // ensure link is absolute
    const QString absDest = QFileInfo(parentDir, linkDest).absoluteFilePath();

    // The link target may not be valid on the NFS server (i.e. it may
    // point outside of the exported directories).  Check for this before
    // calling getFileHandle() for the target of the link, as otherwise
    // the isValidPath() check in getFileHandle() will set the error
    // ERR_CANNOT_ENTER_DIRECTORY which will be taken as the result of
    // the NFS operation.  This is not an error condition if just checking
    // the target of a link, so do the same check here but ignore any error.
    if (!isValidPath(absDest))
    {
        qCDebug(LOG_KIO_NFS) << "target" << absDest << "is invalid";
        return false;
    }

    // It is now safe to call getFileHandle() on the link target.
    return (!getFileHandle(absDest).isInvalid());
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
        server_addr.sin_addr.s_addr = inet_addr(host.toLatin1().constData());
    } else {
        struct hostent* hp = gethostbyname(host.toLatin1().constData());
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

    uid_t uid = geteuid();
    if (!m_currentUser.isEmpty())
    {
        bool ok;
        uid_t num = m_currentUser.toUInt(&ok);
        if (ok) uid = num;
        else
        {
            const struct passwd *pwd = getpwnam(m_currentUser.toLocal8Bit().constData());
            if (pwd != nullptr) uid = pwd->pw_uid;
        }
    }

    client->cl_auth = authunix_create(hostName.toUtf8().data(), uid, getegid(), 0, nullptr);
    return KIO::Error(0);
}


bool NFSProtocol::checkForError(int clientStat, int nfsStat, const QString& text)
{
    if (clientStat!=RPC_SUCCESS)
    {
        const char *errstr = clnt_sperrno(static_cast<clnt_stat>(clientStat));
        qCDebug(LOG_KIO_NFS) << "RPC error" << clientStat << errstr << "on" << text;
        m_slave->setError(KIO::ERR_INTERNAL_SERVER,
                          i18n("RPC error %1, %2", QString::number(clientStat), errstr));
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


// Perform checks and, if so indicated, list a virtual (exported)
// directory that will not actually involve accessing the NFS server.
// Return the directory path, or a null string if there is a problem
// or if the URL refers to a virtual directory which has been listed.

QString NFSProtocol::listDirInternal(const QUrl &url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    // Is the path part of an exported (virtual) directory?
    if (isExportedDir(path))
    {
        qCDebug(LOG_KIO_NFS) << "Listing virtual dir" << path;

        QString dirPrefix = path;
        if (dirPrefix!="/") dirPrefix += QDir::separator();

        QStringList virtualList;
        const QStringList exportedDirs = getExportedDirs();
        for (QStringList::const_iterator it = exportedDirs.constBegin(); it != exportedDirs.constEnd(); ++it)
        {
            // When an export is multiple levels deep (for example "/export/nfs/dir"
            // where "/export" is being listed), we only want to display one level
            // ("nfs") at a time.  Find all of the exported directories that are
            // below the 'dirPrefix', and list the first (or only) path component
            // of each.

            QString name = (*it);           // this exported directory
            if (!name.startsWith(dirPrefix)) continue;  // not below this prefix

            name = name.mid(dirPrefix.length());    // remainder after the prefix

            const int idx = name.indexOf(QDir::separator());
            if (idx!=-1) name = name.left(idx);     // take first path component

            if (!virtualList.contains(name)) {
                qCDebug(LOG_KIO_NFS) << "Found exported" << name;
                virtualList.append(name);
            }
        }

        KIO::UDSEntry entry;
        createVirtualDirEntry(entry);
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, ".");
        entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, "folder-network");
        m_slave->listEntry(entry);

        for (QStringList::const_iterator it = virtualList.constBegin(); it != virtualList.constEnd(); ++it)
        {
            const QString &name = (*it);        // this listed directory

            entry.replace(KIO::UDSEntry::UDS_NAME, name);
            if (isExportedDir(dirPrefix+name)) entry.replace(KIO::UDSEntry::UDS_ICON_NAME, "folder-network");
            else entry.replace(KIO::UDSEntry::UDS_ICON_NAME, "folder");

            m_slave->listEntry(entry);
        }

        return (QString());             // listed, no more to do
    }

    // For the listing we now actually need to access the NFS server.
    // We should always be connected at this point, but better safe than sorry!
    if (!isConnected()) return (QString());

    return (path);                  // the path to list
}


// Perform checks and, if so indicated, return information for
// a virtual (exported) directory.  Return the entry path, or a
// null string if there is a problem or if the URL refers to a
// virtual directory which has been processed.

QString NFSProtocol::statInternal(const QUrl &url)
{
    qCDebug(LOG_KIO_NFS) << url;

    const QString path(url.path());
    if (path.isEmpty())
    {
        // Displaying a location with an empty path (e.g. "nfs://server")
        // seems to confuse Konqueror, it shows the directory but will not
        // descend into any subdirectories.  The same location with a root
        // path ("nfs://server/") works, so redirect to that.
        QUrl redir = url.resolved(QUrl("/"));
        qDebug() << "root with empty path, redirecting to" << redir;
        slave()->redirection(redir);
        return (QString());
    }

    // We can't stat an exported directory on the NFS server,
    // but we know that it must be a directory.
    if (isExportedDir(path))
    {
        KIO::UDSEntry entry;
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, ".");
        entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, "folder-network");
        createVirtualDirEntry(entry);

        slave()->statEntry(entry);
        return (QString());
    }

    return (path);                  // the path to stat
}


// Set the host to be accessed.  If the host has changed a new
// host connection is required, so close the current one.
//
// Due the way that NFSSlave::setHost() is implemented, if the
// host name changes then the protocol will always be deleted
// and recreated.  So in reality this function does nothing useful.

void NFSProtocol::setHost(const QString &host, const QString &user)
{
    qCDebug(LOG_KIO_NFS) << "host" << host << "user" << user;

    if (host.isEmpty())                 // must have a host name
    {
        m_slave->setError(KIO::ERR_UNKNOWN_HOST, host);
        return;
    }
    // nothing to do if no change
    if (host == m_currentHost && user == m_currentUser) return;
    closeConnection();                  // close the existing connection
    m_currentHost = host;               // set the new host name
    m_currentUser = user;               // set the new user name
}


// This function and completeInvalidUDSEntry() must use KIO::UDSEntry::replace()
// because they may be called with a UDSEntry that has already been partially
// filled in by NFSProtocol::createVirtualDirEntry().

void NFSProtocol::completeUDSEntry(KIO::UDSEntry &entry, uid_t uid, gid_t gid)
{
    QString str;

    if (!m_usercache.contains(uid))
    {
        const struct passwd *user = getpwuid(uid);
        if (user!=nullptr)
        {
            m_usercache.insert(uid, QString::fromLatin1(user->pw_name));
            str = user->pw_name;
        }
        else str = QString::number(uid);
    }
    else str = m_usercache.value(uid);
    entry.replace(KIO::UDSEntry::UDS_USER, str);

    if (!m_groupcache.contains(gid))
    {
        const struct group *grp = getgrgid(gid);
        if (grp!=nullptr)
        {
            m_groupcache.insert(gid, QString::fromLatin1(grp->gr_name));
            str = grp->gr_name;
        }
        else str = QString::number(gid);
    }
    else str = m_groupcache.value(gid);
    entry.replace(KIO::UDSEntry::UDS_GROUP, str);
}


void NFSProtocol::completeInvalidUDSEntry(KIO::UDSEntry &entry)
{
    entry.replace(KIO::UDSEntry::UDS_SIZE, 0);      // dummy size
    entry.replace(KIO::UDSEntry::UDS_FILE_TYPE, S_IFMT - 1);
    entry.replace(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    // The UDS_USER and UDS_GROUP must be string values.  It would be possible
    // to look up appropriate values as in completeUDSEntry() above, but it seems
    // pointless to go to that trouble for an unusable invalid entry.
    entry.replace(KIO::UDSEntry::UDS_USER, QString::fromLatin1("root"));
    entry.replace(KIO::UDSEntry::UDS_GROUP, QString::fromLatin1("root"));
}


// This uses KIO::UDSEntry::fastInsert() and so must only be called with
// a blank UDSEntry or one where only UDS_NAME has been filled in.
void NFSProtocol::createVirtualDirEntry(UDSEntry& entry)
{
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);   // dummy size
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    entry.fastInsert(KIO::UDSEntry::UDS_USER, QString::fromLatin1("root"));
    entry.fastInsert(KIO::UDSEntry::UDS_GROUP, QString::fromLatin1("root"));
}

#include "kio_nfs.moc"
