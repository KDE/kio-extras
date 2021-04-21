/*
 *   Copyright (C) 2012 - 2016 by Ivan Cukic <ivan.cukic@kde.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License or (at your option) version 3 or any later version
 *   accepted by the membership of KDE e.V. (or its successor approved
 *   by the membership of KDE e.V.), which shall act as a proxy
 *   defined in Section 14 of version 3 of the license.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FileItemLinkingPlugin.h"
#include "FileItemLinkingPlugin_p.h"
#include "FileItemLinkingPluginActionLoader.h"

#include <kfileitemlistproperties.h>
#include <utils/d_ptr_implementation.h>
#include <utils/qsqlquery_iterator.h>

#include <QMenu>
#include <QCursor>
#include <QDebug>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlField>
#include <QSqlError>
#include <QSqlDriver>
#include <QStandardPaths>
#include <QDBusPendingCall>

#include <KPluginFactory>
#include <KLocalizedString>

#include <algorithm>

#include "common/dbus/common.h"

K_PLUGIN_CLASS_WITH_JSON(FileItemLinkingPlugin, "kactivitymanagerd_fileitem_linking_plugin.json")


// Private

FileItemLinkingPlugin::Private::Private()
{
    connect(&activities, &KActivities::Consumer::serviceStatusChanged,
            this, &Private::activitiesServiceStatusChanged);
}

void FileItemLinkingPlugin::Private::activitiesServiceStatusChanged(
    KActivities::Consumer::ServiceStatus status)
{
    if (status != KActivities::Consumer::Unknown) {
        loadAllActions();
    }
}

void FileItemLinkingPlugin::Private::rootActionHovered()
{
    shouldLoad = true;
    loadAllActions();
}

void FileItemLinkingPlugin::Private::actionTriggered()
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
        service.asyncCall(
            link ? "LinkResourceToActivity" : "UnlinkResourceFromActivity",
            QString(),
            item.toLocalFile(),
            activity);
    }
}

QAction *FileItemLinkingPlugin::Private::basicAction(QWidget *parentWidget)
{
    root = new QAction(QIcon::fromTheme("activities"),
                       i18n("Activities"), parentWidget);

    rootMenu = new QMenu(parentWidget);
    rootMenu->addAction(new QAction(i18n("Loading..."), this));

    connect(root, &QAction::hovered,
            this, &Private::rootActionHovered);


    root->setMenu(rootMenu);

    return root;
}

void FileItemLinkingPlugin::Private::loadAllActions()
{
    if (!shouldLoad
            || activities.serviceStatus() == KActivities::Consumer::Unknown) {
        return;
    }

    if (activities.serviceStatus() == KActivities::Consumer::NotRunning) {
        Action action = { };
        action.title = i18n("The Activity Manager is not running");

        setActions({ action });

    } else if (!loaded) {
        auto loader = FileItemLinkingPluginActionLoader::create(items);

        static FileItemLinkingPluginActionStaticInit init;

        connect(loader, &FileItemLinkingPluginActionLoader::result,
                this, &Private::setActions,
                Qt::QueuedConnection);

        loader->start();

        loaded = true; // ignore that the thread may not be finished at this time
    }
}

void FileItemLinkingPlugin::Private::setActions(const ActionList &actions)
{
    if (!rootMenu) {
        return;
    }

    for (auto action: rootMenu->actions()) {
        rootMenu->removeAction(action);
        action->deleteLater();
    }

    for (const auto& actionInfo: actions) {
        if (actionInfo.icon != "-") {
            auto action = new QAction(nullptr);

            action->setText(actionInfo.title);
            action->setIcon(QIcon::fromTheme(actionInfo.icon));
            action->setProperty("activity", actionInfo.activity);
            action->setProperty("link", actionInfo.link);

            rootMenu->addAction(action);

            connect(action, &QAction::triggered,
                    this, &Private::actionTriggered);

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
}

FileItemLinkingPlugin::~FileItemLinkingPlugin()
{
    d->setActions({});
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

    d->loaded = false;
    d->items = fileItemInfos;

    return { d->basicAction(parentWidget) };
}

#include "FileItemLinkingPlugin.moc"

