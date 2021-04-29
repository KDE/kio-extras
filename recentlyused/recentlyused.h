/*
 *   SPDX-FileCopyrightText: 2019 Méven Car <meven.car@kdemail.net>
 *
 *   SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef RECENTLYUSED_H
#define RECENTLYUSED_H

#include <kio/slavebase.h>

/**
 * Implements recentlyused:/ ioslave
 * It uses KActivitiesStats as a backend (as kickoff/kicker do) to retrieve recently accessed
 * files or folders.
 * It supports filtering on mimetype (option type), path, date of access or date of access range, activity and agent (meaning application).
 * There are also order and limit options.
 *
 * There only three path allowed : / /files and /locations.
 * / does not do anything special
 * /files returns only files whose mimetype is known to the KActivity backend
 * /locations returns only folders
 * When /locations is used the option ?type, described below, cannot be used.
 *
 * It supports options to filter what is returned through url parameters:
 *
 * ?activity=[activity UUID|any]
 *
 *   Allows to filter the resources based on the activity they were used in.
 *   Defaults to the current user activity.
 *   any value means include resources from any activity.
 *   Example: recentlyused:/?activity=428fa590-1920-4b3c-a7e1-1842e6164707
 *
 * ?date=[ISODATE[,ISODATE]|today|yesterday]
 *
 *   Allows to filter on event date of resources
 *   today and yesterday returns resource that had an event today or
 *   yesterday respectively.
 *   ISODATE must be of the form YYYY-MM-DD
 *   If a secondary date is passed, the filtering occurs on the date range starting
 *   at the first date passed and ending at the second date passed inclusively
 *   Example: recentlyused:/?date=2019-07-30
 *
 * ?agent=[agentName1[,agentName*]*
 *
 *   Filter on the name or names of the application that used the resource.
 *   Defaults to no agent filtering
 *   Example: recentlyused:/?agent=kate,dolphin
 *
 * ?path=path
 *
 *   Filters resources based on the path of the file or directory.
 *   Path can contain '*', defaults to no filtering.
 *   Example: recentlyused:/?path=/home/meven/projects/*
 *
 * ?type=mimetype[,mimetype]*
 *
 *   Filters resources based on the mimetype of files.
 *   Defaults to no filtering
 *   Example: recentlyused:/?type=video/*,audio/*
 *
 * ?limit=number
 *
 *   Specify the number of resources to return.
 *   Defaults to 30
 *   Example: recentlyused:/?limit=10
 *
 * ?order=(HighScoredFirst|RecentlyCreatedFirst|OrderByUrl|OrderByTitle)
 *   Allow to modify the order of the returned resources.
 *   Defaults to RecentlyUsedFirst
 *   See KActivities::Stats::Terms::Order for detail
 *   Example: recentlyused:/?limit=HighScoredFirst
 *
 * Examples:
 *
 * - recentlyused:/?type=video/*,audio/*&order=HighScoredFirst : recently used video or audio files ordered by their scoring descending
 * - recentlyused:/?url=/home/meven/kde/src/*&type=text/plain : recently used text files located in a subdir of /home/meven/kde/src/
 *
 * @brief The RecentlyUsed implements an ioslave to access recently used files or directories
 */
class RecentlyUsed : public QObject, public KIO::SlaveBase
{
    Q_OBJECT
public:
    RecentlyUsed(const QByteArray &pool, const QByteArray &app);
    ~RecentlyUsed() override;

protected:
    void listDir(const QUrl &url) override;
    void stat(const QUrl &url) override;
    void mimetype(const QUrl &url) override;

private:
    KIO::UDSEntry udsEntryFromResource(const QString &resource);
    KIO::UDSEntry udsEntryForRoot(const QString &dirName, const QString &iconName);
};

#endif
