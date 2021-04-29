/*
 * This file is part of the KDE KIO-extras Project
 * SPDX-FileCopyrightText: 2016 Anthony Fieroni <bvbfan@abv.bg>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *
 */

#include "filenamesearchmodule.h"

#include <QDBusConnection>
#include <QUrl>

#include <KPluginFactory>

FileNameSearchModule::FileNameSearchModule(QObject* parent, const QVariantList&)
    : KDEDModule(parent)
    , m_dirNotify(QString(), QString(), QDBusConnection::sessionBus())
{
    connect(&m_dirNotify, &OrgKdeKDirNotifyInterface::enteredDirectory,
            this, &FileNameSearchModule::registerSearchUrl);
    connect(&m_dirNotify, &OrgKdeKDirNotifyInterface::leftDirectory,
            this, &FileNameSearchModule::unregisterSearchUrl);
    connect(&m_dirNotify, &OrgKdeKDirNotifyInterface::FilesAdded,
            this, &FileNameSearchModule::slotFilesAdded);
    connect(&m_dirNotify, &OrgKdeKDirNotifyInterface::FilesChanged,
            this, &FileNameSearchModule::slotFilesChanged);
    connect(&m_dirNotify, &OrgKdeKDirNotifyInterface::FilesRemoved,
            this, &FileNameSearchModule::slotFilesRemoved);
}

void FileNameSearchModule::registerSearchUrl(const QString &urlString)
{
    if (urlString.startsWith(QStringLiteral("filenamesearch"))) {
        m_searchUrls << QUrl(urlString);
    }
}

void FileNameSearchModule::unregisterSearchUrl(const QString &urlString)
{
    if (urlString.startsWith(QStringLiteral("filenamesearch"))) {
        m_searchUrls.removeAll(QUrl(urlString));
    }
}

void FileNameSearchModule::slotFilesAdded(const QString &urlString)
{
    const QUrl url(urlString);
    if (!url.isLocalFile()) {
        return;
    }
    const QString urlPath = url.path();
    for (const QUrl &dirUrl : qAsConst(m_searchUrls)) {
        if (urlPath.startsWith(dirUrl.path())) {
            org::kde::KDirNotify::emitFilesAdded(dirUrl);
        }
    }
}

void FileNameSearchModule::slotFilesChanged(const QStringList &files)
{
    QList<QUrl> fileList;
    for (const QString &file : files) {
        QUrl url(file);
        if (!url.isLocalFile()) {
            continue;
        }
        const QString urlPath = url.path();
        for (const QUrl &dirUrl : qAsConst(m_searchUrls)) {
            if (urlPath.startsWith(dirUrl.path())) {
                url.setScheme(QStringLiteral("filenamesearch"));
                fileList << url;
            }
        }
    }
    if (!fileList.isEmpty()) {
        org::kde::KDirNotify::emitFilesChanged(fileList);
    }
}

void FileNameSearchModule::slotFilesRemoved(const QStringList &files)
{
    QList<QUrl> fileList;
    for (const QString &file : files) {
        QUrl url(file);
        if (!url.isLocalFile()) {
            continue;
        }
        const QString urlPath = url.path();
        for (const QUrl &dirUrl : qAsConst(m_searchUrls)) {
            if (urlPath.startsWith(dirUrl.path())) {
                url.setScheme(QStringLiteral("filenamesearch"));
                fileList << url;
            }
        }
    }
    if (!fileList.isEmpty()) {
        org::kde::KDirNotify::emitFilesRemoved(fileList);
    }
}

K_PLUGIN_CLASS_WITH_JSON(FileNameSearchModule, "filenamesearchmodule.json")

#include "filenamesearchmodule.moc"
