/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FileItemLinkingPlugin.h"
#include "FileItemLinkingPluginActionLoader.h"

#include <KFileItemListProperties>
#include <utils/qsqlquery_iterator.h>

#include <QCursor>
#include <QDBusPendingCall>
#include <QDebug>
#include <QFileInfo>
#include <QMenu>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KPluginFactory>

#include <algorithm>

#include "common/dbus/common.h"

K_PLUGIN_CLASS_WITH_JSON(FileItemLinkingPlugin, "kactivitymanagerd_fileitem_linking_plugin.json")

void FileItemLinkingPlugin::activitiesServiceStatusChanged(KActivities::Consumer::ServiceStatus status)
{
    if (status != KActivities::Consumer::Unknown) {
        loadAllActions();
    }
}

void FileItemLinkingPlugin::rootActionHovered()
{
    if (status != Status::LoadingBlocked) {
        return;
    }
    status = Status::ShouldLoad;
    loadAllActions();
}

void FileItemLinkingPlugin::actionTriggered()
{
    QAction *action = dynamic_cast<QAction *>(sender());

    if (!action) {
        return;
    }

    bool link = action->property("link").toBool();
    QString activity = action->property("activity").toString();

    KAMD_DBUS_DECL_INTERFACE(service, "Resources/Linking", "ResourcesLinking");

    const auto urlList = items.urlList();
    for (const auto &item : urlList) {
        service.asyncCall(link ? "LinkResourceToActivity" : "UnlinkResourceFromActivity", QString(), item.toLocalFile(), activity);
    }
}

QAction *FileItemLinkingPlugin::basicAction(QWidget *parentWidget)
{
    if (root) {
        return root;
    }

    // If we are showing Loading... text, this means the submenu
    // is not opened yet, so activities should not be loaded
    status = Status::LoadingBlocked;

    root = new QAction(QIcon::fromTheme("activities"), i18n("Activities"), parentWidget);

    rootMenu = new QMenu(parentWidget);
    rootMenu->addAction(new QAction(i18n("Loading..."), this));

    connect(root, &QAction::hovered, this, &FileItemLinkingPlugin::rootActionHovered);

    root->setMenu(rootMenu);

    return root;
}

void FileItemLinkingPlugin::loadAllActions()
{
    if (status != Status::ShouldLoad || activities.serviceStatus() == KActivities::Consumer::Unknown) {
        return;
    }

    if (activities.serviceStatus() == KActivities::Consumer::NotRunning) {
        Action action = {};
        action.title = i18n("The Activity Manager is not running");

        setActions({action});

    } else if (status != Status::Loaded) {
        status = Status::Loaded; // loading is async, we don't want to slin two threads

        auto loader = FileItemLinkingPluginActionLoader::create(items);

        static FileItemLinkingPluginActionStaticInit init;

        connect(loader, &FileItemLinkingPluginActionLoader::result, this, &FileItemLinkingPlugin::setActions, Qt::QueuedConnection);

        loader->start();
    }
}

void FileItemLinkingPlugin::setActions(const ActionList &actions)
{
    if (!rootMenu) {
        return;
    }

    for (auto action : rootMenu->actions()) {
        rootMenu->removeAction(action);
        action->deleteLater();
    }

    for (const auto &actionInfo : actions) {
        if (actionInfo.icon != "-") {
            auto action = new QAction(nullptr);

            action->setText(actionInfo.title);
            action->setIcon(QIcon::fromTheme(actionInfo.icon));
            action->setProperty("activity", actionInfo.activity);
            action->setProperty("link", actionInfo.link);

            rootMenu->addAction(action);

            connect(action, &QAction::triggered, this, &FileItemLinkingPlugin::actionTriggered);

        } else {
            auto action = new QAction(actionInfo.title, nullptr);
            action->setSeparator(true);

            rootMenu->addAction(action);
        }
    }
}

FileItemLinkingPluginActionStaticInit::FileItemLinkingPluginActionStaticInit()
{
    qRegisterMetaType<Action>("Action");
    qRegisterMetaType<ActionList>("ActionList");
}

// Main class

FileItemLinkingPlugin::FileItemLinkingPlugin(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
{
    connect(&activities, &KActivities::Consumer::serviceStatusChanged, this, &FileItemLinkingPlugin::activitiesServiceStatusChanged);
}

FileItemLinkingPlugin::~FileItemLinkingPlugin()
{
    setActions({});
}

QList<QAction *> FileItemLinkingPlugin::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    // We can only link local files, don't show the menu if there are none
    // KFileItemListProperties::isLocal() is for *all* being local, we just want *any* local
    const auto urlList = fileItemInfos.urlList();
    const bool hasLocalUrl = std::any_of(urlList.begin(), urlList.end(), [](const QUrl &url) {
        return url.isLocalFile();
    });

    if (!hasLocalUrl) {
        return {};
    }

    items = fileItemInfos;

    return {basicAction(parentWidget)};
}

#include "FileItemLinkingPlugin.moc"
#include "moc_FileItemLinkingPlugin.cpp"
