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
