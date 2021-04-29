/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FILE_ITEM_LINKING_PLUGIN_ACTION_LOADER_H
#define FILE_ITEM_LINKING_PLUGIN_ACTION_LOADER_H

#include "FileItemLinkingPlugin.h"
#include "FileItemLinkingPlugin_p.h"

#include <QThread>

#include <KFileItemListProperties>

#include <KActivities/Consumer>

class FileItemLinkingPluginActionLoader: public QThread {
    Q_OBJECT

public:
    static FileItemLinkingPluginActionLoader* create(const KFileItemListProperties &items);

    void run() override;

    Action createAction(const QString &activity, bool link,
                        const QString &title = QString(),
                        const QString &icon = QString()) const;
    Action createSeparator(const QString &title) const;

Q_SIGNALS:
    void result(const ActionList &actions);

private:
    FileItemLinkingPluginActionLoader(const KFileItemListProperties &items);
    KFileItemListProperties items;
    KActivities::Consumer activities;
};

#endif // FILE_ITEM_LINKING_PLUGIN_ACTION_LOADER_H
