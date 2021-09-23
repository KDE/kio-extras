/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FILE_ITEM_LINKING_PLUGIN_P_H
#define FILE_ITEM_LINKING_PLUGIN_P_H

#include "FileItemLinkingPlugin.h"

#include <QPointer>

#include <KFileItemListProperties>

#include <KActivities/Consumer>
#include <KActivities/Info>

struct Action {
    QString title;
    QString icon;
    QString activity;
    bool link;
};
typedef QList<Action> ActionList;

class FileItemLinkingPlugin::Private : public QObject {
    Q_OBJECT

public:
    Private();

    QPointer<QAction> root;
    QMenu *rootMenu = nullptr;
    KFileItemListProperties items;

    QAction *basicAction(QWidget *parentWidget);

    KActivities::Consumer activities;

public Q_SLOTS:
    void activitiesServiceStatusChanged(KActivities::Consumer::ServiceStatus status);
    void rootActionHovered();
    void setActions(const ActionList &actions);

    void actionTriggered();
    void loadAllActions();

private:
    enum class Status {
        LoadingBlocked,
        ShouldLoad,
        Loaded
    };
    Status status = Status::LoadingBlocked;
};

class FileItemLinkingPluginActionStaticInit {
public:
    FileItemLinkingPluginActionStaticInit();
};

#endif // FILE_ITEM_LINKING_PLUGIN_P_H
