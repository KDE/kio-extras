/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *   SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KioActivitiesApi.h"
#include "KioActivities.h"

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
#include "KioActivitiesApi.h"

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.activities" FILE "activities.json")
};

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    // necessary to use other kio slaves
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        fprintf(stderr, "Usage: kio_activities protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }
    // start the slave
    ActivitiesProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();
    return 0;
}


ActivitiesProtocol::ActivitiesProtocol(const QByteArray &poolSocket,
                                       const QByteArray &appSocket)
    : KIO::ForwardingWorkerBase("activities", poolSocket, appSocket)
{
}

ActivitiesProtocol::~ActivitiesProtocol() = default;

bool ActivitiesProtocol::rewriteUrl(const QUrl &url, QUrl &newUrl)
{
    QString activity, path;
    switch (d->pathType(url, &activity, &path)) {
    case ActivitiesProtocolApi::RootItem:
    case ActivitiesProtocolApi::ActivityRootItem:
        if (activity == "current") {
            KActivities::Consumer activities;
            d->syncActivities(activities);
            newUrl = QUrl(QStringLiteral("activities:/") + activities.currentActivity());
            return true;
        }
        return false;

    case ActivitiesProtocolApi::ActivityPathItem: {
        // auto demangled = d->demangledPath(path);
        // QProcess::execute("kdialog",
        //                   { "--passivepopup",
        //                     path.midRef(1).toString() + "\n" + demangled });

        newUrl = QUrl::fromLocalFile(path);
        return true;
    }

    default:
        return true;
    }
}

KIO::WorkerResult ActivitiesProtocol::listDir(const QUrl &url)
{
    KActivities::Consumer activities;
    d->syncActivities(activities);

    QString activity, path;
    switch (d->pathType(url, &activity, &path)) {
    case ActivitiesProtocolApi::RootItem: {
        KIO::UDSEntryList udslist;

        KIO::UDSEntry uds;
        uds.reserve(9);
        uds.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("current"));
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, i18n("Current activity"));
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n("Activity"));
        uds.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, QStringLiteral("activities"));
        uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
        uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, 0500);
        uds.fastInsert(KIO::UDSEntry::UDS_USER, KUser().loginName());
        uds.fastInsert(KIO::UDSEntry::UDS_TARGET_URL, QStringLiteral("activities:/") + activities.currentActivity());
        udslist << uds;

        for (const auto &activity : activities.activities()) {
            udslist << d->activityEntry(activity);
        }

        listEntries(udslist);
        return KIO::WorkerResult::pass();
    }

    case ActivitiesProtocolApi::ActivityRootItem: {
        KIO::UDSEntryList udslist;

        auto database = Common::Database::instance(Common::Database::ResourcesDatabase, Common::Database::ReadOnly);

        if (!database) {
            return KIO::WorkerResult::pass();
        }

        if (activity == "current") {
            activity = activities.currentActivity();
        }

        static const auto queryString = QStringLiteral(
            "SELECT targettedResource "
            "FROM ResourceLink "
            "WHERE usedActivity = '%1' "
            "AND initiatingAgent = \":global\" ");

        auto query = database->execQuery(queryString.arg(activity));

        for (const auto &result : query) {
            auto path = result[0].toString();

            if (!QFile(path).exists())
                continue;

            KIO::UDSEntry uds;

            udslist << d->filesystemEntry(path);
        }

        listEntries(udslist);
        return KIO::WorkerResult::pass();
    }

    case ActivitiesProtocolApi::ActivityPathItem:
        return ForwardingWorkerBase::listDir(QUrl::fromLocalFile(path));
    }

    Q_UNREACHABLE();
    return KIO::WorkerResult::fail();
}


KIO::WorkerResult ActivitiesProtocol::stat(const QUrl& url)
{
    QString activity;

    switch (d->pathType(url, &activity)) {
    case ActivitiesProtocolApi::RootItem: {
        QString dirName = i18n("Activities");
        KIO::UDSEntry uds;
        uds.reserve(6);
        uds.fastInsert(KIO::UDSEntry::UDS_NAME, dirName);
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, dirName);
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_TYPE, dirName);
        uds.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, QStringLiteral("activities"));
        uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));

        statEntry(uds);

        return KIO::WorkerResult::pass();
    }

    case ActivitiesProtocolApi::ActivityRootItem: {
        KActivities::Consumer activities;
        d->syncActivities(activities);

        if (activity == "current") {
            activity = activities.currentActivity();
        }

        statEntry(d->activityEntry(activity));

        return KIO::WorkerResult::pass();
    }

    case ActivitiesProtocolApi::ActivityPathItem:
        return ForwardingWorkerBase::stat(url);
    }

    Q_UNREACHABLE();
    return KIO::WorkerResult::fail();
}

KIO::WorkerResult ActivitiesProtocol::mimetype(const QUrl& url)
{
    switch (d->pathType(url)) {
    case ActivitiesProtocolApi::RootItem:
    case ActivitiesProtocolApi::ActivityRootItem:
        mimeType(QStringLiteral("inode/directory"));
        return KIO::WorkerResult::pass();

    case ActivitiesProtocolApi::ActivityPathItem:
        return ForwardingWorkerBase::mimetype(url);
    }

    Q_UNREACHABLE();
    return KIO::WorkerResult::fail();
}

#include "KioActivities.moc"
