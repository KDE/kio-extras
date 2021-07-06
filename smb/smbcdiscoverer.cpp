/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
*/

#include <QCoreApplication>
#include <QUrlQuery>
#include <QScopeGuard>

#include "smbcdiscoverer.h"

static QEvent::Type LoopEvent = QEvent::User;

class SMBCServerDiscovery : public SMBCDiscovery
{
public:
    SMBCServerDiscovery(const UDSEntry &entry)
        : SMBCDiscovery(entry)
    {
        m_entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        m_entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));
        m_entry.fastInsert(KIO::UDSEntry::UDS_URL, url());
        m_entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("application/x-smb-server"));
        m_entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, QStringLiteral("network-server"));
    }

    QString url()
    {
        QUrl u("smb://");
        u.setHost(udsName());
        return u.url();
    }
};

class SMBCShareDiscovery : public SMBCDiscovery
{
public:
    SMBCShareDiscovery(const UDSEntry &entry)
        : SMBCDiscovery(entry)
    {
        m_entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        m_entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    }
};

class SMBCWorkgroupDiscovery : public SMBCDiscovery
{
public:
    SMBCWorkgroupDiscovery(const UDSEntry &entry)
        : SMBCDiscovery(entry)
    {
        m_entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        m_entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, (S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH));
        m_entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("application/x-smb-workgroup"));
        m_entry.fastInsert(KIO::UDSEntry::UDS_URL, url());
    }

    QString url()
    {
        QUrl u("smb://");
        u.setHost(udsName());
        if (!u.isValid()) {
            // In the event that the workgroup contains bad characters, put it in a query instead.
            // This is transparently handled by SMBUrl when we get this as input again.
            // Also see documentation there.
            // https://bugs.kde.org/show_bug.cgi?id=204423
            u.setHost(QString());
            QUrlQuery q;
            q.addQueryItem("kio-workgroup", udsName());
            u.setQuery(q);
        }
        return u.url();
    }
};

SMBCDiscovery::SMBCDiscovery(const UDSEntry &entry)
    : m_entry(entry)
      // cache the name, it may get accessed more than once
    , m_name(entry.stringValue(KIO::UDSEntry::UDS_NAME))
{
}

QString SMBCDiscovery::udsName() const
{
    return m_name;
}

KIO::UDSEntry SMBCDiscovery::toEntry() const
{
    return m_entry;
}

SMBCDiscoverer::SMBCDiscoverer(const SMBUrl &url, QEventLoop *loop, SMBSlave *slave)
    : m_url(url)
    , m_loop(loop)
    , m_slave(slave)
{
}

SMBCDiscoverer::~SMBCDiscoverer()
{
    if (m_dirFd > 0) {
        smbc_closedir(m_dirFd);
    }
}

void SMBCDiscoverer::start()
{
    queue();
}


bool SMBCDiscoverer::discoverNextFileInfo()
{
#ifdef HAVE_READDIRPLUS2
    // Readdirplus2 dir/file listing. Becomes noop when at end of data associated with dirfd.
    // If readdirplus2 isn't available the regular dirent listing is done.
    // readdirplus2 improves performance by giving us a stat without separate call (Samba>=4.12)
    struct stat st;
    const struct libsmb_file_info *fileInfo = smbc_readdirplus2(m_dirFd, &st);
    if (fileInfo) {
        const QString name = QString::fromUtf8(fileInfo->name);
        qCDebug(KIO_SMB_LOG) << "fileInfo" << "name:" << name;
        if (name == ".") {
            return true;
        } else if (name == "..") {
            m_dirWasRoot = false;
            return true;
        }
        UDSEntry entry;
        entry.reserve(5); // Minimal size. stat will set at least 4 fields.
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, name);

        m_url.addPath(name);
        m_slave->statToUDSEntry(m_url, st, entry); // won't produce useful error
        Q_EMIT newDiscovery(Discovery::Ptr(new SMBCDiscovery(entry)));
        m_url.cdUp();
        return true;
    }
#endif // HAVE_READDIRPLUS2
    return false;
}

void SMBCDiscoverer::discoverNext()
{
    // Poor man's concurrency. smbc isn't thread safe so we'd hold up other
    // discoverers until we are done. While that will likely happen anyway
    // because smbc_opendir (usually?) blocks until it actually has all
    // the data to loop on, meaning the actual looping after open is fairly
    // fast. Even so, there's benefit in letting other discoverers do
    // their work in the meantime because they may do more atomic
    // requests that are async and can take a while due to network latency.
    // To get somewhat reasonable behavior we simulate an async smbc discovery
    // by posting loop events to the eventloop and each loop run we process
    // a single dirent.
    // This effectively unblocks the eventloop between iterations.
    // Once we are out of entries this discoverer is considered finished.

    // Always queue a new iteration when returning so we don't forget to.
    auto autoQueue = qScopeGuard([this] {
        queue();
    });

    if (m_dirFd == -1) {
        init();
        Q_ASSERT(m_dirFd || m_finished);
        return;
    }

    if (discoverNextFileInfo()) {
        return;
    }

    qCDebug(KIO_SMB_LOG) << "smbc_readdir ";
    struct smbc_dirent *dirp = smbc_readdir(m_dirFd);
    if (dirp == nullptr) {
        qCDebug(KIO_SMB_LOG) << "done with smbc";
        stop();
        return;
    }

    const QString name = QString::fromUtf8(dirp->name);
    // We cannot trust dirp->commentlen has it might be with or without the NUL character
    // See KDE bug #111430 and Samba bug #3030
    const QString comment = QString::fromUtf8(dirp->comment);

    qCDebug(KIO_SMB_LOG) << "dirent "
                         << "name:" << name
                         << "comment:" << comment
                         << "type:" << dirp->smbc_type;

    UDSEntry entry;
    // Minimal potential size. The actual size depends on this function,
    // possibly the stat function, and lastly the Discovery objects themselves.
    // The smallest will be a ShareDiscovery with 5 fields.
    entry.reserve(5);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, name);
    entry.fastInsert(KIO::UDSEntry::UDS_COMMENT, comment);
    // Ensure system shares are marked hidden.
    if (name.endsWith(QLatin1Char('$'))) {
        entry.fastInsert(KIO::UDSEntry::UDS_HIDDEN, 1);
    }

#if !defined(HAVE_READDIRPLUS2)
    // . and .. are always of the dir type so they are of no consequence outside
    // actual dir listing and that'd be done by readdirplus2 already
    if (name == ".") {
        // Skip the "." entry
        // Mind the way m_currentUrl is handled in the loop
    } else if (name == "..") {
        m_dirWasRoot = false;
    } else if (dirp->smbc_type == SMBC_FILE || dirp->smbc_type == SMBC_DIR) {
        // Set stat information
        m_url.addPath(name);
        const int statErr = m_slave->browse_stat_path(m_url, entry);
        if (statErr != 0) {
            // The entry can disappear in the time span between
            // listing and the stat call. There's nothing we or the user
            // can do about it. Log the incident and move on with listing.
            qCWarning(KIO_SMB_LOG) << "Failed to stat" << m_url << statErr;
        } else {
            Q_EMIT newDiscovery(Discovery::Ptr(new SMBCDiscovery(entry)));
        }
        m_url.cdUp();
    }
#endif // HAVE_READDIRPLUS2

    if (dirp->smbc_type == SMBC_SERVER) {
        Q_EMIT newDiscovery(Discovery::Ptr(new SMBCServerDiscovery(entry)));
    } else if (dirp->smbc_type == SMBC_FILE_SHARE) {
        Q_EMIT newDiscovery(Discovery::Ptr(new SMBCShareDiscovery(entry)));
    } else if (dirp->smbc_type == SMBC_WORKGROUP) {
        Q_EMIT newDiscovery(Discovery::Ptr(new SMBCWorkgroupDiscovery(entry)));
    } else {
        qCDebug(KIO_SMB_LOG) << "SMBC_UNKNOWN :" << name;
    }
}

void SMBCDiscoverer::customEvent(QEvent *event)
{
    if (event->type() == LoopEvent) {
        if (!m_finished) {
            discoverNext();
        }
        return;
    }
    QObject::customEvent(event);
}

void SMBCDiscoverer::stop()
{
    m_finished = true;
    Q_EMIT finished();
}

bool SMBCDiscoverer::isFinished() const
{
    return m_finished;
}

bool SMBCDiscoverer::dirWasRoot() const
{
    return m_dirWasRoot;
}

int SMBCDiscoverer::error() const
{
    return m_error;
}

void SMBCDiscoverer::init()
{
    Q_ASSERT(m_dirFd < 0);

    m_dirFd = smbc_opendir(m_url.toSmbcUrl());
    if (m_dirFd >= 0) {
        m_error = 0;
    } else {
        m_error = errno;
        stop();
    }

    qCDebug(KIO_SMB_LOG) << "open" << m_url.toSmbcUrl()
                         << "url-type:" << m_url.getType()
                         << "dirfd:" << m_dirFd
                         << "errNum:" << m_error;

    return;
}

void SMBCDiscoverer::queue()
{
    if (m_finished) {
        return;
    }

    // Queue low priority events. For server discovery (that is: other discoverers run as well)
    // we want the modern discoverers to be peferred. For other discoveries only
    // SMBC is running on the loop and so the priority has no negative impact.
    QCoreApplication::postEvent(this, new QEvent(LoopEvent), Qt::LowEventPriority);
}
