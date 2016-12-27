/*
 * This file is part of the KDE KIO-extras Project
 * Copyright (C) 2016 Anthony Fieroni <bvbfan@abv.bg>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
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
    for (const QUrl &dirUrl : m_searchUrls) {
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
        for (const QUrl &dirUrl : m_searchUrls) {
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
        for (const QUrl &dirUrl : m_searchUrls) {
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

K_PLUGIN_FACTORY_WITH_JSON(Factory,
                           "filenamesearchmodule.json",
                           registerPlugin<FileNameSearchModule>();)

#include "filenamesearchmodule.moc"
