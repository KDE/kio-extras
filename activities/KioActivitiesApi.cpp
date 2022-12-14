/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *   SPDX-FileCopyrightText: 2022 Alex Kuznetsov <alex@vxpro.io>
 * 
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KioActivities.h"
#include "KioActivitiesApi.h"

#include <QCoreApplication>

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QStandardPaths>

#include <KIO/Job>
#include <KLocalizedString>
#include <KUser>

#include <common/database/Database.h>
#include <utils/d_ptr_implementation.h>
#include <utils/qsqlquery_iterator.h>

#include <KActivities/Consumer>
#include <KActivities/Info>

#include <QProcess>

ActivitiesProtocolApi::ActivitiesProtocolApi()
{
}

ActivitiesProtocolApi::PathType ActivitiesProtocolApi::pathType(const QUrl &url, QString *activity, QString *filePath) const
{
    const auto fullPath = url.adjusted(QUrl::StripTrailingSlash).path();
    const auto path = QStringView(fullPath).mid(fullPath.startsWith(QLatin1Char('/')) ? 1 : 0);

    if (activity && !path.isEmpty()) {
        *activity = path.mid(0, path.indexOf(QStringLiteral("/"))).toString();
    }

    if (filePath) {
        auto strippedPath = path.mid(path.indexOf(QStringLiteral("/")) + 1);
        auto splitPosition = strippedPath.indexOf(QStringLiteral("/"));

        if (splitPosition == -1) {
            // if we have only one path segment
            *filePath = demangledPath(strippedPath.toString());

        } else {
            // if we have sub-paths
            auto head = strippedPath.mid(0, splitPosition);
            auto tail = strippedPath.mid(splitPosition);

            *filePath = demangledPath(head.toString()) + tail.toString();
        }
    }

    return path.length() == 0 ? RootItem : path.contains(QStringLiteral("/")) ? ActivityPathItem : ActivityRootItem;
}

void ActivitiesProtocolApi::syncActivities(KActivities::Consumer &activities)
{
    // We need to use the consumer in a synchronized way
    while (activities.serviceStatus() == KActivities::Consumer::Unknown) {
        QCoreApplication::processEvents();
    }
}

KIO::UDSEntry ActivitiesProtocolApi::activityEntry(const QString &activity)
{
    KIO::UDSEntry uds;
    uds.reserve(8);
    KActivities::Info activityInfo(activity);
    uds.fastInsert(KIO::UDSEntry::UDS_NAME, activity);
    uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, activityInfo.name());
    uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n("Activity"));
    uds.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, activityInfo.icon());
    uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
    uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, 0500);
    uds.fastInsert(KIO::UDSEntry::UDS_USER, KUser().loginName());
    return uds;
}

KIO::UDSEntry ActivitiesProtocolApi::filesystemEntry(const QString &path)
{
    KIO::UDSEntry uds;
    auto url = QUrl::fromLocalFile(path);

    if (KIO::StatJob *job = KIO::stat(url, KIO::HideProgressInfo)) {
        QScopedPointer<KIO::StatJob> sp(job);
        job->setAutoDelete(false);
        if (job->exec()) {
            uds = job->statResult();
        }
    }

    auto mangled = mangledPath(path);
    // QProcess::execute("kdialog", { "--passivepopup", mangled });

    uds.replace(KIO::UDSEntry::UDS_NAME, mangled);
    uds.replace(KIO::UDSEntry::UDS_DISPLAY_NAME, url.fileName());
    uds.replace(KIO::UDSEntry::UDS_TARGET_URL, url.url());
    uds.replace(KIO::UDSEntry::UDS_LOCAL_PATH, path);

    return uds;
}

QString ActivitiesProtocolApi::mangledPath(const QString &path) const
{
    // return QString::fromUtf8(QUrl::toPercentEncoding(path));
    return QString::fromLatin1(path.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString ActivitiesProtocolApi::demangledPath(const QString &mangled) const
{
    // return QUrl::fromPercentEncoding(mangled.toUtf8());
    return QString::fromUtf8(QByteArray::fromBase64(mangled.toLatin1(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

#include "KioActivitiesApi.moc"
