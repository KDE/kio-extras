/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FILE_ITEM_LINKING_PLUGIN_H
#define FILE_ITEM_LINKING_PLUGIN_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

#include <PlasmaActivities/Consumer>
#include <PlasmaActivities/Info>

#include <QAction>
#include <QList>
#include <QPointer>
#include <QVariant>

struct Action {
    QString title;
    QString icon;
    QString activity;
    bool link;
};
typedef QList<Action> ActionList;

class FileItemLinkingPluginActionStaticInit
{
public:
    FileItemLinkingPluginActionStaticInit();
};

/**
 * FileItemLinkingPlugin
 */
class FileItemLinkingPlugin : public KAbstractFileItemActionPlugin
{
    Q_OBJECT
public:
    FileItemLinkingPlugin(QObject *parent, const QVariantList &);
    ~FileItemLinkingPlugin() override;

    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override;

private:
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

#endif // FILE_ITEM_LINKING_PLUGIN_H
