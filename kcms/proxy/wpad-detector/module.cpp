// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include <chrono>

#include <QFutureWatcher>
#include <QNetworkInformation>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include <KConfigGroup>
#include <KDEDModule>
#include <KLibexec>
#include <KLocalizedString>
#include <KNotification>
#include <KPluginFactory>
#include <KSharedConfig>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

constexpr auto CONFIG_FILE = "kioslaverc"_L1;
constexpr auto GROUP = "Proxy Settings"_L1;
constexpr auto CHECK_WPAD = "CheckWpad";

class WpadDetectorModule : public KDEDModule
{
    Q_OBJECT
public:
    WpadDetectorModule(QObject *parent, const QList<QVariant> & /*args*/)
        : KDEDModule(parent)
    {
        // Delay the proxy check by quite a bit so we can be relatively certain that the network is "properly" up.
        // This in particular may also include VPNs coming up in addition to the interface itself, so we should be
        // fairly generous here.
        QTimer::singleShot(60s, this, [this] {
            connect(m_networkInformation, &QNetworkInformation::reachabilityChanged, this, &WpadDetectorModule::check);
            QTimer::singleShot(0s, this, &WpadDetectorModule::check);
        });
    }

    void check()
    {
        auto config = KSharedConfig::openConfig(CONFIG_FILE, KConfig::NoGlobals);
        auto group = config->group(GROUP);

        if (!group.readEntry(CHECK_WPAD, true)) {
            // Disabled
            return;
        }

        if (group.readEntry("ProxyType", 0) != 3) { // 3 = KSaveIOConfig::WpadProxy
            // Not wpad
            return;
        }

        const auto checker = [] {
            QProcess helper;
            helper.setProgram(KLibexec::path(u"wpad-detector-helper"_s));
            helper.start();
            return helper.waitForFinished(std::chrono::milliseconds(1s).count());
        };

        m_watcher = new QFutureWatcher<bool>(this);
        connect(m_watcher, &QFutureWatcher<bool>::finished, this, [this] {
            if (m_watcher->result()) {
                // All good.
                return;
            }

            // Took too long.

            if (m_notification) {
                return;
            }

            m_notification = KNotification::event(KNotification::Warning,
                                                  i18nc("@title", "Slow network performance detected"),
                                                  i18nc("@info",
                                                        "Automatic proxy discovery has been enabled, but is reducing the system's network performance. Check "
                                                        "your proxy settings to make sure that using this setting is really necessary."),
                                                  u"network-wired-activated-limited-symbolic"_s);
            connect(m_notification, &KNotification::closed, this, [this] {
                m_notification.clear();
            });

            auto open = m_notification->addAction(i18nc("@action:button", "Open Settings"));
            connect(open, &KNotificationAction::activated, this, [] {
                QProcess::startDetached(u"systemsettings"_s, {u"kcm_proxy"_s});
            });

            auto ignore = m_notification->addAction(i18nc("@action:button ignore notification", "Ignore Forever"));
            connect(ignore, &KNotificationAction::activated, this, [] {
                auto config = KSharedConfig::openConfig(CONFIG_FILE, KConfig::NoGlobals);
                config->group(GROUP).writeEntry(CHECK_WPAD, false);
            });

            m_notification->sendEvent();
        });
        m_watcher->setFuture(QtConcurrent::run(checker));
    }

private:
    QNetworkInformation *m_networkInformation = []() -> QNetworkInformation * {
        if (!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
            qDebug() << "Failed to load QNetworkInformation backend";
            return nullptr;
        }

        return QNetworkInformation::instance();
    }();
    QPointer<KNotification> m_notification;
    QFutureWatcher<bool> *m_watcher = nullptr;
};

K_PLUGIN_CLASS_WITH_JSON(WpadDetectorModule, "wpad-detector.json")

#include "module.moc"
