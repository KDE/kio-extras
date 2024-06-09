/*
    SPDX-FileCopyrightText: 2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// Own
#include "ksaveioconfig.h"

// Qt
#ifdef WITH_DBUS
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#endif

// KDE
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <kio/ioworker_defaults.h>
class KSaveIOConfigPrivate
{
public:
    KSaveIOConfigPrivate();
    ~KSaveIOConfigPrivate();

    KConfig *config = nullptr;
    KConfig *http_config = nullptr;
};

Q_GLOBAL_STATIC(KSaveIOConfigPrivate, d)

KSaveIOConfigPrivate::KSaveIOConfigPrivate()
{
}

KSaveIOConfigPrivate::~KSaveIOConfigPrivate()
{
    delete config;
    delete http_config;
}

static KConfig *config()
{
    if (!d->config) {
        d->config = new KConfig(QStringLiteral("kioslaverc"), KConfig::NoGlobals);
        // KF6 TODO: rename to kioworkerrc here and elsewhere. See also T15956.
    }

    return d->config;
}

int KSaveIOConfig::proxyDisplayUrlFlags()
{
    KConfigGroup cfg(config(), QString());
    return cfg.readEntry("ProxyUrlDisplayFlags", 0);
}

void KSaveIOConfig::setProxyDisplayUrlFlags(int flags)
{
    KConfigGroup cfg(config(), QString());
    cfg.writeEntry("ProxyUrlDisplayFlags", flags);
    cfg.sync();
}

void KSaveIOConfig::setReadTimeout(int _timeout)
{
    KConfigGroup cfg(config(), QString());
    cfg.writeEntry("ReadTimeout", qMax(MIN_TIMEOUT_VALUE, _timeout));
    cfg.sync();
}

void KSaveIOConfig::setConnectTimeout(int _timeout)
{
    KConfigGroup cfg(config(), QString());
    cfg.writeEntry("ConnectTimeout", qMax(MIN_TIMEOUT_VALUE, _timeout));
    cfg.sync();
}

void KSaveIOConfig::setProxyConnectTimeout(int _timeout)
{
    KConfigGroup cfg(config(), QString());
    cfg.writeEntry("ProxyConnectTimeout", qMax(MIN_TIMEOUT_VALUE, _timeout));
    cfg.sync();
}

void KSaveIOConfig::setResponseTimeout(int _timeout)
{
    KConfigGroup cfg(config(), QString());
    cfg.writeEntry("ResponseTimeout", qMax(MIN_TIMEOUT_VALUE, _timeout));
    cfg.sync();
}

void KSaveIOConfig::setMarkPartial(bool _mode)
{
    KConfigGroup cfg(config(), QString());
    cfg.writeEntry("MarkPartial", _mode);
    cfg.sync();
}

void KSaveIOConfig::setMinimumKeepSize(int _size)
{
    KConfigGroup cfg(config(), QString());
    cfg.writeEntry("MinimumKeepSize", _size);
    cfg.sync();
}

void KSaveIOConfig::setUseReverseProxy(bool mode)
{
    KConfigGroup cfg(config(), QStringLiteral("Proxy Settings"));
    cfg.writeEntry("ReversedException", mode);
    cfg.sync();
}

void KSaveIOConfig::setProxyType(KSaveIOConfig::ProxyType type)
{
    KConfigGroup cfg(config(), QStringLiteral("Proxy Settings"));
    cfg.writeEntry("ProxyType", static_cast<int>(type));
    cfg.sync();
}

QString KSaveIOConfig::noProxyFor()
{
    KConfigGroup cfg(config(), QStringLiteral("Proxy Settings"));
    return cfg.readEntry("NoProxyFor");
}

void KSaveIOConfig::setNoProxyFor(const QString &_noproxy)
{
    KConfigGroup cfg(config(), QStringLiteral("Proxy Settings"));
    cfg.writeEntry("NoProxyFor", _noproxy);
    cfg.sync();
}

void KSaveIOConfig::setProxyFor(const QString &protocol, const QString &_proxy)
{
    KConfigGroup cfg(config(), QStringLiteral("Proxy Settings"));
    cfg.writeEntry(protocol.toLower() + QLatin1String("Proxy"), _proxy);
    cfg.sync();
}

void KSaveIOConfig::setProxyConfigScript(const QString &_url)
{
    KConfigGroup cfg(config(), QStringLiteral("Proxy Settings"));
    cfg.writeEntry("Proxy Config Script", _url);
    cfg.sync();
}

void KSaveIOConfig::updateRunningWorkers(QWidget *parent)
{
#ifdef WITH_DBUS
    // Inform all running KIO workers about the changes...
    // if we cannot update, KIO workers inform the end user...
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KIO/Scheduler"), QStringLiteral("org.kde.KIO.Scheduler"), QStringLiteral("reparseSlaveConfiguration"));
    message << QString();
    if (!QDBusConnection::sessionBus().send(message)) {
#endif
        KMessageBox::information(parent,
                                 i18n("You have to restart the running applications "
                                      "for these changes to take effect."),
                                 i18nc("@title:window", "Update Failed"));
#ifdef WITH_DBUS
    }
#endif
}
