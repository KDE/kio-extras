/*
 *   SPDX-FileCopyrightText: 2019 Méven Car <meven.car@kdemail.net>
 *
 *   SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "recentlyused-logsettings.h"
#include "recentlyused.h"

#include <QCoreApplication>
#include <QUrl>
#include <QUrlQuery>

#include <KIO/Job>
#include <KLocalizedString>

#include <KActivities/Stats/Cleaning>
#include <KActivities/Stats/ResultModel>
#include <KActivities/Stats/Terms>

#ifdef Q_OS_WIN
#include <sys/stat.h>
#endif

namespace KAStats = KActivities::Stats;

using namespace KAStats;
using namespace KAStats::Terms;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.recentlyused" FILE "recentlyused.json")
};

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    // necessary to use other kio slaves
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kio_recentlyused"));
    if (argc != 4) {
        fprintf(stderr, "Usage: kio_recentlyused protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }
    // start the slave
    RecentlyUsed slave(argv[2], argv[3]);
    slave.dispatchLoop();
    return 0;
}

static bool isRootUrl(const QUrl &url)
{
    const QString path = url.adjusted(QUrl::StripTrailingSlash).path();
    return path.isEmpty() || path == QLatin1String("/");
}

RecentlyUsed::RecentlyUsed(const QByteArray &pool, const QByteArray &app)
    : SlaveBase("recentlyused", pool, app)
{
}

RecentlyUsed::~RecentlyUsed()
{
}

ResultModel *runQuery(const QUrl &url)
{
    qCDebug(KIO_RECENTLYUSED_LOG) << "runQuery for url" << url.toString();

    auto query = UsedResources
                 | Limit(30);

    // Parse url query parameter
    const auto urlQuery = QUrlQuery(url);

    const auto path = url.path();
    if (path == QStringLiteral("/locations")) {
        query.setTypes(Type::directories());
    } else {
        if (urlQuery.hasQueryItem(QStringLiteral("type"))) {
            // handles type parameter aka mimetype
            const auto typeValue = urlQuery.queryItemValue(QStringLiteral("type"));
            const auto types = typeValue.split(QLatin1Char(','));
            query.setTypes(types);
        } else if (path == QStringLiteral("/files")) {
            query.setTypes(Type::files());
        }
    }

    // limit parameter
    if (urlQuery.hasQueryItem(QStringLiteral("limit"))) {
        const auto limitValue = urlQuery.queryItemValue(QStringLiteral("limit"));
        bool parseOk;
        const auto limitInt = limitValue.toInt(&parseOk);
        if (parseOk) {
            query.setLimit(limitInt);
        }
    }

    // activity parameter, filter using the uuid of the activity
    if (urlQuery.hasQueryItem(QStringLiteral("activity"))) {
        const auto activityValue = urlQuery.queryItemValue(QStringLiteral("activity"));
        if (activityValue == QStringLiteral("any")) {
            query.setActivities(Activity::any());
        } else {
            query.setActivities(activityValue);
        }
    } else {
        query.setActivities(Activity::current());
    }

    // date parameter, filter using the date when an event occurred on a resource
    if (urlQuery.hasQueryItem(QStringLiteral("date"))) {
        const auto dateValue = urlQuery.queryItemValue(QStringLiteral("date"));
        if (dateValue == QStringLiteral("today")) {
            query.setDate(Date::today());
        } else if (dateValue == QStringLiteral("yesterday")) {
            query.setDate(Date::yesterday());
        } else {
            query.setDate(Date::fromString(dateValue));
        }
    }

    // agent parameter, filter using the application name that used the resource
    if (urlQuery.hasQueryItem(QStringLiteral("agent"))) {
        const auto agentValue = urlQuery.queryItemValue(QStringLiteral("agent"));
        const auto agents = agentValue.split(QLatin1Char(','));
        query.setAgents(agents);
    } else {
        query.setAgents(Agent::any());
    }

    // path parameter for exact path match or folders, supports wildcard pattern matching
    if (urlQuery.hasQueryItem(QStringLiteral("path"))) {
        const auto pathValue = urlQuery.queryItemValue(QStringLiteral("path"));
        query.setUrlFilters(pathValue);
    } else {
        // only files are supported for now, because of limited support in udsEntryFromResource
        query.setUrlFilters(Url::file());
    }

    // see KActivities::Stats::Terms::Order
    if (urlQuery.hasQueryItem(QStringLiteral("order"))) {
        const auto orderValue = urlQuery.queryItemValue(QStringLiteral("order"));

        if (orderValue == QStringLiteral("HighScoredFirst")) {
            query.setOrdering(Order::HighScoredFirst);
        } else if (orderValue == QStringLiteral("RecentlyCreatedFirst")) {
            query.setOrdering(Order::RecentlyCreatedFirst);
        } else if (orderValue == QStringLiteral("OrderByUrl")) {
            query.setOrdering(Order::OrderByUrl);
        } else if (orderValue == QStringLiteral("OrderByTitle")) {
            query.setOrdering(Order::OrderByTitle);
        } else {
            query.setOrdering(Order::RecentlyUsedFirst);
        }
    } else {
        query.setOrdering(Order::RecentlyUsedFirst);
    }

    return new ResultModel(query);
}

KIO::UDSEntry RecentlyUsed::udsEntryFromResource(const QString &resource)
{
    qCDebug(KIO_RECENTLYUSED_LOG) << "udsEntryFromResource" << resource;

    // the query only returns files and folders
    QUrl resourceUrl = QUrl::fromLocalFile(resource);

    KIO::UDSEntry uds;
    KIO::StatJob *job = KIO::stat(resourceUrl, KIO::HideProgressInfo);

    // we do not want to wait for the event loop to delete the job
    QScopedPointer<KIO::StatJob> sp(job);
    job->setAutoDelete(false);
    if (job->exec()) {
        uds = job->statResult();
    }
    uds.fastInsert(KIO::UDSEntry::UDS_URL, resourceUrl.toString());
    return uds;
}

void RecentlyUsed::listDir(const QUrl &url)
{
    if (!isRootUrl(url)) {
        const auto path = url.path();
        if (path != QStringLiteral("/files") && path != QStringLiteral("/locations") ) {
            error(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
            return;
        }
    }

    auto model = runQuery(url);

    KIO::UDSEntryList udslist;
    udslist.reserve(model->rowCount());

    for (int r = 0; r < model->rowCount(); ++r) {
        QModelIndex index = model->index(r, 0);
        QString resource = model->data(index, ResultModel::ResourceRole).toString();

        udslist << udsEntryFromResource(resource);
    }

    listEntries(udslist);
    finished();
}

KIO::UDSEntry RecentlyUsed::udsEntryForRoot(const QString &dirName, const QString &iconName)
{
    KIO::UDSEntry uds;
    uds.reserve(7);
    uds.fastInsert(KIO::UDSEntry::UDS_NAME, dirName);
    uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, dirName);
    uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_TYPE, dirName);
    uds.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, iconName);
    uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
#ifdef Q_OS_WIN
    uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, _S_IREAD );
#else
    uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR );
#endif
    return uds;
}

void RecentlyUsed::stat(const QUrl &url)
{
    qCDebug(KIO_RECENTLYUSED_LOG) << "stating" << " " << url;

    if (isRootUrl(url)) {
        //
        // stat the root path
        //

        const QString dirName = i18n("Recent Documents");

        statEntry(udsEntryForRoot(dirName, QStringLiteral("document-open-recent")));
        finished();
    } else {

        const auto path = url.path();
        if (path == QStringLiteral("/files")) {
            const QString dirName = i18n("Recent Files");
            statEntry(udsEntryForRoot(dirName, QStringLiteral("document-open-recent")));
            finished();
        } else if (path == QStringLiteral("/locations")) {
            const QString dirName = i18n("Recent Locations");
            statEntry(udsEntryForRoot(dirName, QStringLiteral("folder-open-recent")));
            finished();
        } else {
            // only / /files and /locations paths are supported
            error(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
        }
    }
}

void RecentlyUsed::mimetype(const QUrl &url)
{
    // the root url is always a folder
    if (isRootUrl(url)) {
        mimeType(QStringLiteral("inode/directory"));
        finished();
    } else {
        // only the root path is supported
        error(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
    }
}

// needed for JSON file embedding
#include "recentlyused.moc"
