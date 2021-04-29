/*
 * This file is part of the KDE KIO-extras Project
 * SPDX-FileCopyrightText: 2016 Anthony Fieroni <bvbfan@abv.bg>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 */

#ifndef KDED_FILENAME_SEARCH_MODULE_H_
#define KDED_FILENAME_SEARCH_MODULE_H_

#include <KDEDModule>
#include <kdirnotify.h>
#include <QUrl>

class FileNameSearchModule : public KDEDModule
{
    Q_OBJECT

public:
    FileNameSearchModule(QObject *parent, const QVariantList&);

private Q_SLOTS:
    void registerSearchUrl(const QString &urlString);
    void unregisterSearchUrl(const QString &urlString);
    void slotFilesAdded(const QString &urlString);
    void slotFilesChanged(const QStringList &files);
    void slotFilesRemoved(const QStringList &files);

private:
    QList<QUrl> m_searchUrls;
    org::kde::KDirNotify m_dirNotify;
};

#endif
