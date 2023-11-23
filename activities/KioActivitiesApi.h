/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *   SPDX-FileCopyrightText: 2022 Alex Kuznetsov <alex@vxpro.io>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KIO_ACTIVITIES_API_H
#define KIO_ACTIVITIES_API_H

#include <KIO/UDSEntry>
#include <PlasmaActivities/Consumer>
#include <QString>

class ActivitiesProtocolApi
{
public:
    ActivitiesProtocolApi();

    enum PathType { RootItem, ActivityRootItem, ActivityPathItem };

    PathType pathType(const QUrl &url, QString *activity = nullptr, QString *filePath = nullptr) const;

    void syncActivities(KActivities::Consumer &activities);

    KIO::UDSEntry activityEntry(const QString &activity);

    KIO::UDSEntry filesystemEntry(const QString &path);

    QString mangledPath(const QString &path) const;

    QString demangledPath(const QString &mangled) const;
};

#endif // KIO_ACTIVITIES_API_H
