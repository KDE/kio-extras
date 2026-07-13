// SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "proxy.h"

#include "ksaveioconfig.h"

#include <KLocalizedQmlContext>
#include <KPluginFactory>

#include <QQmlEngine>
#include <QUrl>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(Proxy, "kcm_proxy.json")

Proxy::Proxy(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_settings(new ProxySettings(this))
{
    qmlRegisterUncreatableType<Proxy>("org.kde.private.kcms.proxy", 1, 0, "KCM", u"Cannot create instances of KCM"_s);
}

ProxySettings *Proxy::proxySettings()
{
    return m_settings;
}

void Proxy::save()
{
    KQuickManagedConfigModule::save();
    // TODO: Pass the window here once we have a way of getting it
    KSaveIOConfig::updateRunningWorkers();
}

QString Proxy::autoDetect(const QByteArrayList &envVars) const
{
    for (const auto &envVar : envVars) {
        const auto value = qgetenv(envVar.data());
        if (!value.isEmpty()) {
            return QString::fromUtf8(envVar);
        }
    }
    return {};
}

QString Proxy::autoDetectResolve(const QByteArrayList &envVars) const
{
    for (const auto &envVar : envVars) {
        const auto value = qgetenv(envVar.data());
        if (!value.isEmpty()) {
            return QString::fromUtf8(value);
        }
    }
    return {};
}

#include "proxy.moc"

#include "moc_proxy.cpp"
