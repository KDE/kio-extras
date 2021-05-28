/*
 *   SPDX-FileCopyrightText: 2012-2016 Ivan Cukic <ivan.cukic@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KioActivities.h"

#include <QCoreApplication>

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlField>
#include <QSqlError>
#include <QSqlDriver>
#include <QByteArray>

#include <KLocalizedString>
#include <KUser>
#include <kio/job.h>

#include <utils/d_ptr_implementation.h>
#include <utils/qsqlquery_iterator.h>
#include <common/database/Database.h>

#include <KActivities/Info>
#include <KActivities/Consumer>

#include <QProcess>

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.activities" FILE "activities.json")
};

class ActivitiesProtocol::Private {
public:
    Private()
    {
    }

    enum PathType {
        RootItem,
        ActivityRootItem,
        ActivityPathItem
    };

    PathType pathType(const QUrl &url, QString *activity = nullptr,
                      QString *filePath = nullptr) const
    {
        const auto fullPath = url.adjusted(QUrl::StripTrailingSlash).path();
        const auto path = fullPath.midRef(fullPath.startsWith('/') ? 1 : 0);

        if (activity) {
            *activity = path.mid(0, path.indexOf("/") - 1).toString();
        }

        if (filePath) {
            auto strippedPath = path.mid(path.indexOf("/") + 1);
            auto splitPosition = strippedPath.indexOf("/");

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

        return path.length() == 0 ? RootItem
               : path.contains("/") ? ActivityPathItem
               : ActivityRootItem;
    }

    void syncActivities(KActivities::Consumer &activities)
    {
        // We need to use the consumer in a synchronized way
        while (activities.serviceStatus() == KActivities::Consumer::Unknown) {
            QCoreApplication::processEvents();
        }
    }

    KIO::UDSEntry activityEntry(const QString &activity)
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

    KIO::UDSEntry filesystemEntry(const QString &path)
    {
        KIO::UDSEntry uds;
        auto url = QUrl::fromLocalFile(path);

        if (KIO::StatJob* job = KIO::stat(url, KIO::HideProgressInfo)) {
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

    QString mangledPath(const QString &path) const
    {
        // return QString::fromUtf8(QUrl::toPercentEncoding(path));
        return QString::fromLatin1(path.toUtf8().toBase64(
                                       QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    }

    QString demangledPath(const QString &mangled) const
    {
        // return QUrl::fromPercentEncoding(mangled.toUtf8());
        return QString::fromUtf8(QByteArray::fromBase64(mangled.toLatin1(),
                                 QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    }

    // KActivities::Consumer activities;
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
    : KIO::ForwardingSlaveBase("activities", poolSocket, appSocket)
{
}

ActivitiesProtocol::~ActivitiesProtocol()
{
}

bool ActivitiesProtocol::rewriteUrl(const QUrl &url, QUrl &newUrl)
{
    QString activity, path;
    switch (d->pathType(url, &activity, &path)) {
    case Private::RootItem:
    case Private::ActivityRootItem:
        if (activity == "current") {
            KActivities::Consumer activities;
            d->syncActivities(activities);
            newUrl = QUrl(QStringLiteral("activities:/")
                          + activities.currentActivity());
            return true;
        }
        return false;

    case Private::ActivityPathItem:
    {
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

void ActivitiesProtocol::listDir(const QUrl &url)
{
    KActivities::Consumer activities;
    d->syncActivities(activities);

    QString activity, path;
    switch (d->pathType(url, &activity, &path)) {
    case Private::RootItem:
    {
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

        for (const auto& activity: activities.activities()) {
            udslist << d->activityEntry(activity);
        }

        listEntries(udslist);
        finished();
        break;
    }

    case Private::ActivityRootItem:
    {
        KIO::UDSEntryList udslist;

        auto database = Common::Database::instance(
                            Common::Database::ResourcesDatabase,
                            Common::Database::ReadOnly);

        if (!database) {
            finished();
            break;
        }

        if (activity == "current") {
            activity = activities.currentActivity();
        }

        static const auto queryString = QStringLiteral(
                                            "SELECT targettedResource "
                                            "FROM ResourceLink "
                                            "WHERE usedActivity = '%1' "
                                            "AND initiatingAgent = \":global\" "
                                        );

        auto query = database->execQuery(queryString.arg(activity));

        for (const auto& result: query) {
            auto path = result[0].toString();

            if (!QFile(path).exists()) continue;

            KIO::UDSEntry uds;

            udslist << d->filesystemEntry(path);
        }

        listEntries(udslist);
        finished();
        break;
    }

    case Private::ActivityPathItem:
        ForwardingSlaveBase::listDir(QUrl::fromLocalFile(path));
        break;
    }
}

void ActivitiesProtocol::prepareUDSEntry(KIO::UDSEntry &entry, bool listing) const
{
    ForwardingSlaveBase::prepareUDSEntry(entry, listing);
}

void ActivitiesProtocol::stat(const QUrl& url)
{
    QString activity;

    switch (d->pathType(url, &activity)) {
    case Private::RootItem:
    {
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
        finished();
        break;
    }

    case Private::ActivityRootItem:
    {
        KActivities::Consumer activities;
        d->syncActivities(activities);

        if (activity == "current") {
            activity = activities.currentActivity();
        }

        statEntry(d->activityEntry(activity));
        finished();
        break;
    }

    case Private::ActivityPathItem:
        ForwardingSlaveBase::stat(url);
        break;
    }
}

void ActivitiesProtocol::mimetype(const QUrl& url)
{
    switch (d->pathType(url)) {
    case Private::RootItem:
    case Private::ActivityRootItem:
        mimeType(QStringLiteral("inode/directory"));
        finished();
        break;

    case Private::ActivityPathItem:
        ForwardingSlaveBase::mimetype(url);
        break;
    }

}

void ActivitiesProtocol::del(const QUrl& url, bool isfile)
{
    Q_UNUSED(url);
    Q_UNUSED(isfile);
}

#include "KioActivities.moc"
