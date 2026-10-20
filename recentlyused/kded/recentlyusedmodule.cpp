/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "recentlyusedmodule.h"

#include <QDBusConnection>
#include <QUrl>

#include <KPluginFactory>

#include <PlasmaActivities/Stats/ResultWatcher>
#include <PlasmaActivities/Stats/Terms>

using namespace KActivities::Stats;
using namespace KActivities::Stats::Terms;
using namespace std::chrono_literals;

namespace
{
// The folders that the places panel and the file dialogs open. A view of these can start before the
// module is loaded, so it misses their enteredDirectory.
const QStringList s_placeUrls = {
    QStringLiteral("recentlyused:/"),
    QStringLiteral("recentlyused:/files"),
    QStringLiteral("recentlyused:/locations"),
};
}

RecentlyUsedModule::RecentlyUsedModule(QObject *parent, const QVariantList &)
    : KDEDModule(parent)
    , m_dirNotify(QString(), QString(), QDBusConnection::sessionBus())
{
    connect(&m_dirNotify, &OrgKdeKDirNotifyInterface::enteredDirectory, this, &RecentlyUsedModule::registerUrl);
    connect(&m_dirNotify, &OrgKdeKDirNotifyInterface::leftDirectory, this, &RecentlyUsedModule::unregisterUrl);

    m_notifyTimer.setSingleShot(true);
    m_notifyTimer.setInterval(500ms);
    connect(&m_notifyTimer, &QTimer::timeout, this, &RecentlyUsedModule::notifyViews);

    m_watcher = new ResultWatcher(UsedResources | Agent::any() | Type::any() | Activity::any(), this);
    auto scheduleNotify = [this]() {
        m_notifyTimer.start();
    };
    connect(m_watcher, &ResultWatcher::resultScoreUpdated, this, scheduleNotify);
    connect(m_watcher, &ResultWatcher::resultRemoved, this, scheduleNotify);
    connect(m_watcher, &ResultWatcher::resultLinked, this, scheduleNotify);
    connect(m_watcher, &ResultWatcher::resultUnlinked, this, scheduleNotify);
    connect(m_watcher, &ResultWatcher::resultsInvalidated, this, scheduleNotify);
}

RecentlyUsedModule::~RecentlyUsedModule() = default;

void RecentlyUsedModule::registerUrl(const QString &urlString)
{
    if (urlString.startsWith(QLatin1String("recentlyused:")) && !s_placeUrls.contains(urlString)) {
        m_shownUrls.insert(urlString);
    }
}

void RecentlyUsedModule::unregisterUrl(const QString &urlString)
{
    m_shownUrls.remove(urlString);
}

void RecentlyUsedModule::notifyViews()
{
    for (const QString &urlString : s_placeUrls) {
        org::kde::KDirNotify::emitFilesAdded(QUrl(urlString));
    }
    for (const QString &urlString : std::as_const(m_shownUrls)) {
        org::kde::KDirNotify::emitFilesAdded(QUrl(urlString));
    }
}

K_PLUGIN_CLASS_WITH_JSON(RecentlyUsedModule, "recentlyusedmodule.json")

#include "moc_recentlyusedmodule.cpp"
#include "recentlyusedmodule.moc"
