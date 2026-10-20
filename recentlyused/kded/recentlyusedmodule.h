/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <KDEDModule>
#include <KDirNotify>

#include <QSet>
#include <QTimer>

namespace KActivities::Stats
{
class ResultWatcher;
}

/*!
 * Tells the views of recentlyused:/ that their content changed when the activity manager records a use,
 * so that they list it again.
 */
class RecentlyUsedModule : public KDEDModule
{
    Q_OBJECT

public:
    RecentlyUsedModule(QObject *parent, const QVariantList &);
    ~RecentlyUsedModule() override;

private:
    void registerUrl(const QString &urlString);
    void unregisterUrl(const QString &urlString);
    void notifyViews();

    org::kde::KDirNotify m_dirNotify;
    KActivities::Stats::ResultWatcher *m_watcher = nullptr;
    // Groups the uses recorded in a burst into one refresh
    QTimer m_notifyTimer;
    // The recentlyused: urls that views show, with their query, other than the ones of the places
    QSet<QString> m_shownUrls;
};
