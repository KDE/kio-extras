// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>

#include <KPluginFactory>
#include <KDEDModule>
#include <KDirNotify>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDBusConnection>
#include <QProcess>
#include <QTimer>

#include <smburl.h>
#include <smb-logsettings.h>
#include "config.h"

class Notifier : public QObject
{
    Q_OBJECT
public:
    explicit Notifier(const QString &url, QObject *parent)
        : QObject(parent)
        , m_url(url)
    {
    }

    ~Notifier() override
    {
        if (m_proc) {
            m_proc->disconnect(); // no need for a finished signal
            m_proc->terminate();
            m_proc->waitForFinished(1000); // we'll want to proceed to kill fairly quickly
            m_proc->kill();
        }
    }

    // Update last event on this notifier.
    // Notifiers that haven't seen activity may get dropped should we run out of capacity.
    void poke()
    {
        m_lastEntry = QDateTime::currentDateTimeUtc();
    }

    bool operator<(const Notifier &other) const
    {
        return m_lastEntry < other.m_lastEntry;
    }

Q_SIGNALS:
    void finished(const QString &url);

public Q_SLOTS:
    void start()
    {
        ++m_startCounter;
        // libsmbclient isn't properly thread safe and attaching a notification request to a context
        // is fully blocking. So notify is blockig the current thread an we can't start more threads
        // with more contexts to watch multiple directories in-process.
        // To bypass this limitation we'll spawn separated notifier processes for each directory
        // we want to notify on.
        // https://bugzilla.samba.org/show_bug.cgi?id=11413
        m_proc = new QProcess(this);
        m_proc->setProcessChannelMode(QProcess::ForwardedChannels);
        m_proc->setProgram(QStringLiteral(KDE_INSTALL_FULL_LIBEXECDIR_KF5 "/smbnotifier"));
        m_proc->setArguments({m_url});
        connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &Notifier::maybeRestart);
        m_proc->start();
    }

private Q_SLOTS:
    void maybeRestart(int code, QProcess::ExitStatus status)
    {
        if (code == 0 || status != QProcess::NormalExit || m_startCounter >= m_startCounterLimit) {
            Q_EMIT finished(m_url);
            return;
        }
        m_proc->deleteLater();
        m_proc = nullptr;
        // Try to restart if it error'd out. Notifying requires authentication, if credentials
        // weren't cached by the time we attempted to register the notifier an error will
        // occur and the child exits !0.
        QTimer::singleShot(10000, this, &Notifier::start);
    }

private:
    static const int m_startCounterLimit = 4;
    int m_startCounter = 0;
    const QString m_url;
    QDateTime m_lastEntry { QDateTime::currentDateTimeUtc() };
    QProcess *m_proc = nullptr;
};

class Watcher : public QObject
{
    Q_OBJECT
public:
    explicit Watcher(QObject *parent = nullptr)
        : QObject(parent)
    {
        connect(&m_interface, &OrgKdeKDirNotifyInterface::enteredDirectory,
                this, &Watcher::watchDirectory);
        connect(&m_interface, &OrgKdeKDirNotifyInterface::leftDirectory,
                this, &Watcher::unwatchDirectory);
    }

private Q_SLOTS:
    void watchDirectory(const QString &url)
    {
        if (!isInterestingUrl(url)) {
            return;
        }
        auto existingNotifier = m_watches.value(url, nullptr);
        if (existingNotifier) {
            existingNotifier->poke();
            return;
        }
        while (m_watches.count() >= m_capacity) {
            makeSpace();
        }

        // TODO: we could keep track of all potential urls regardless of active notification.
        //   Then closing some tabs in dolphin could lead to more watches freeing up and
        //   us being able to use the free slots for still active urls.

        auto notifier = new Notifier(url, this);
        connect(notifier, &Notifier::finished, this, &Watcher::unwatchDirectory);
        notifier->start();

        m_watches[url] = notifier;
        qCDebug(KIO_SMB_LOG) << "entered" << url << m_watches;
    }

    void unwatchDirectory(const QString &url)
    {
        if (!m_watches.contains(url)) {
            return;
        }
        auto notifier = m_watches.take(url);
        notifier->deleteLater();
        qCDebug(KIO_SMB_LOG) << "leftDirectory" << url << m_watches;
    }

private:
    inline bool isInterestingUrl(const QString &str)
    {
        SMBUrl url { QUrl(str) };
        switch (url.getType()) {
        case SMBURLTYPE_UNKNOWN:
        case SMBURLTYPE_ENTIRE_NETWORK:
        case SMBURLTYPE_WORKGROUP_OR_SERVER:
            return false;
        case SMBURLTYPE_SHARE_OR_PATH:
            return true;
        }
        qCWarning(KIO_SMB_LOG) << "Unexpected url type" << url.getType() << url;
        Q_UNREACHABLE();
        return false;
    }

    void makeSpace()
    {
        auto oldestIt = m_watches.cbegin();
        for (auto it = m_watches.cbegin(); it != m_watches.cend(); ++it) {
            if (*it.value() < *oldestIt.value()) {
                oldestIt = it;
            }
        }
        unwatchDirectory(oldestIt.key());
        qCDebug(KIO_SMB_LOG) << "made space:" << m_watches;
    }

    // Cap the amount of notifiers we can run. Each notifier weighs about 1MiB in private heap
    // depending on the linked/loaded libraries behind KIO so in the interest of staying lightweight
    // we'll want to put a limit on active notifiers even when the user has a bazillion open
    // tabs in dolphin or something. On top of that there's a shared weight of ~3MiB on a plasma
    // session from the actual shared libraries.
    // Further optimizing the notifier would require moving all KIO and qdbus linkage out of
    // the notifier and have a socket pair with this process. The gains are sub 0.5MiB though
    // so given the added complexity I'll deem it unreasonable for now.
    // The better improvement would be to make smbc actually thread safe so we can get rid of the
    // subprocess overhead entirely (and by extension the private heaps of static library objects).
    static const int m_capacity = 10;
    OrgKdeKDirNotifyInterface m_interface { QString(), QString(), QDBusConnection::sessionBus() };
    QHash<QString, Notifier *> m_watches; // watcher is parent of procs
};

class SMBWatcherModule : public KDEDModule
{
    Q_OBJECT
public:
    explicit SMBWatcherModule(QObject *parent, const QVariantList &args)
        : KDEDModule(parent)
    {
        Q_UNUSED(args);
    }

private:
    Watcher m_watcher;
};

K_PLUGIN_FACTORY_WITH_JSON(SMBWatcherModuleFactory,
                           "kded_smbwatcher.json",
                           registerPlugin<SMBWatcherModule>();)

#include "watcher.moc"
