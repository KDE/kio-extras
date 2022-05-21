/*
 *   SPDX-FileCopyrightText: 2022 Méven Car <meven.car@kdemail.net>
 *
 *   SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef FORGETFILEITEMACTION_H
#define FORGETFILEITEMACTION_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class QAction;
class QWidget;

class ForgetFileItemAction : public KAbstractFileItemActionPlugin
{
    Q_OBJECT

public:
    explicit ForgetFileItemAction(QObject *parent, const QVariantList &args);

    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override;

private:
    QAction *createAction(const QIcon &icon, const QString &name, QWidget *parent, const QList<QUrl> &urls, const QString &exec);
};

#endif // FORGETFILEITEMACTION_H
