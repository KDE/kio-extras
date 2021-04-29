/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FILE_ITEM_LINKING_PLUGIN_H
#define FILE_ITEM_LINKING_PLUGIN_H

#include <KAbstractFileItemActionPlugin>

#include <QList>
#include <QAction>
#include <QVariant>

#include <utils/d_ptr.h>

/**
 * FileItemLinkingPlugin
 */
class FileItemLinkingPlugin : public KAbstractFileItemActionPlugin {
public:
    FileItemLinkingPlugin(QObject *parent, const QVariantList &);
    ~FileItemLinkingPlugin() override;

    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos,
                             QWidget *parentWidget) override;

private:
    D_PTR;
};

#endif // FILE_ITEM_LINKING_PLUGIN_H
