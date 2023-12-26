/*
 *   SPDX-FileCopyrightText: 2019 Méven Car <meven.car@kdemail.net>
 *
 *   SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "recentlyused.h"
#include "recentlyused-logsettings.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QUrl>
#include <QUrlQuery>

#include <KIO/StatJob>
#include <KLocalizedString>

#include <PlasmaActivities/Stats/Cleaning>
#include <PlasmaActivities/Stats/ResultModel>
#include <PlasmaActivities/Stats/Terms>

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
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.recentlyused" FILE "recentlyused.json")
};

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    // necessary to use other kio workers
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kio_recentlyused"));
    if (argc != 4) {
        fprintf(stderr, "Usage: kio_recentlyused protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }
    // start the worker
    RecentlyUsed worker(argv[2], argv[3]);
    worker.dispatchLoop();
    return 0;
}

static bool isRootUrl(const QUrl &url)
{
    const QString path = url.adjusted(QUrl::StripTrailingSlash).path();
    return path.isEmpty() || path == QLatin1String("/");
}

RecentlyUsed::RecentlyUsed(const QByteArray &pool, const QByteArray &app)
    : WorkerBase("recentlyused", pool, app)
{
}

RecentlyUsed::~RecentlyUsed()
{
}

int queryLimit(QUrl url)
{
    const auto urlQuery = QUrlQuery(url);
    // limit parameter
    if (urlQuery.hasQueryItem(QStringLiteral("limit"))) {
        const auto limitValue = urlQuery.queryItemValue(QStringLiteral("limit"));
        bool parseOk;
        const auto limitInt = limitValue.toInt(&parseOk);
        if (parseOk) {
            return limitInt;
        }
    }
    return 30;
}

ResultModel *runQuery(const QUrl &url, int limit)
{
    qCDebug(KIO_RECENTLYUSED_LOG) << "runQuery for url" << url.toString();

    auto query = UsedResources | Limit(30);

    // Parse url query parameter
    const auto urlQuery = QUrlQuery(url);

    const auto path = url.path();
    if (path.startsWith(QStringLiteral("/locations"))) {
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

KIO::UDSEntry RecentlyUsed::udsEntryFromResource(int row, const QString &resource, const QString &mimeType, const QString &agent, int lastUpdateTime)
{
    qCDebug(KIO_RECENTLYUSED_LOG) << "udsEntryFromResource" << resource;

    // the query only returns files and folders
    QUrl resourceUrl = QUrl::fromUserInput(resource);

    KIO::UDSEntry uds;
    KIO::StatJob *job = KIO::stat(resourceUrl, KIO::HideProgressInfo);

    // we do not want to wait for the event loop to delete the job
    QScopedPointer<KIO::StatJob> sp(job);
    sp->setAutoDelete(false);
    if (sp->exec()) {
        uds = sp->statResult();
    } else {
        // not found / not existing anymore
        return uds;
    }
    // replace name with a technical unique name
    const auto name = uds.stringValue(KIO::UDSEntry::UDS_NAME);
    uds.replace(KIO::UDSEntry::UDS_NAME, QStringLiteral("%1-%2").arg(name).arg(row));
    uds.reserve(uds.count() + 5);
    if (name.isEmpty()) {
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, resource);
    } else {
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, name);
    }
    uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, mimeType);
    uds.fastInsert(KIO::UDSEntry::UDS_TARGET_URL, resourceUrl.toString());
    if (resourceUrl.isLocalFile()) {
        uds.fastInsert(KIO::UDSEntry::UDS_LOCAL_PATH, resource);
    }
    if (!uds.contains(KIO::UDSEntry::UDS_ACCESS_TIME)) {
        // default access time
        uds.fastInsert(KIO::UDSEntry::UDS_ACCESS_TIME, lastUpdateTime);
    }
    uds.fastInsert(KIO::UDSEntry::UDS_EXTRA, agent);
    return uds;
}

KIO::WorkerResult RecentlyUsed::listDir(const QUrl &url)
{
    // / /files and /locations
    const auto path = url.path();
    if (path == QStringLiteral("/") || path == QStringLiteral("/files") || path == QStringLiteral("/locations")) {
        KIO::UDSEntryList udslist;

        // add "." to transmit permissions for current directory
        KIO::UDSEntry uds;
        uds.reserve(4);
        uds.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
        uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
#ifdef Q_OS_WIN
        uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, _S_IREAD);
#else
        uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR);
#endif
        udslist << uds;

        int limit = queryLimit(url);
        // query twice the limit size to be able to pass not existing files
        const auto model = runQuery(url, limit * 2);

        bool canFetchMore = true;
        int row = 0;

        while (canFetchMore) {
            for (; udslist.count() != limit + 1 && row < model->rowCount(); ++row) {
                const QModelIndex index = model->index(row, 0);
                const QString resource = model->data(index, ResultModel::ResourceRole).toString();
                const QString mimeType = model->data(index, ResultModel::MimeType).toString();
                const int lastUpdate = model->data(index, ResultModel::LastUpdateRole).toInt();
                const QString agent = model->data(index, ResultModel::Agent).toString();

                const auto entry = udsEntryFromResource(row, resource, mimeType, agent, lastUpdate);
                if (entry.count() > 0) {
                    udslist << entry;
                }
            }
            canFetchMore = udslist.count() != limit + 1 && model->canFetchMore(QModelIndex());
            if (canFetchMore) {
                model->fetchMore(QModelIndex());
            }
        }

        listEntries(udslist);

        return KIO::WorkerResult::pass();
    }

    // subdirs

    // parse the technical id: filename-id, id being an index in the model
    const auto splitted = QStringView(url.fileName()).split(QLatin1Char('-'));
    if (splitted.count() < 2) {
        return KIO::WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
    }
    bool ok;
    int id = splitted.last().toInt(&ok);
    if (!ok) {
        return KIO::WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
    }

    // query twice the limit size to be able to pass not existing files
    const auto model = runQuery(url, queryLimit(url) * 2);
    const auto index = model->index(id, 0);

    if (!index.isValid()) {
        return KIO::WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
    }

    const QString resource = model->data(index, ResultModel::ResourceRole).toString();
    qCDebug(KIO_RECENTLYUSED_LOG) << "redirection to " << resource << url;
    redirection(QUrl::fromUserInput(resource));
    return KIO::WorkerResult::pass();
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
    uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, _S_IREAD);
#else
    uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR);
#endif
    return uds;
}

KIO::WorkerResult RecentlyUsed::stat(const QUrl &url)
{
    qCDebug(KIO_RECENTLYUSED_LOG) << "stating"
                                  << " " << url;

    if (isRootUrl(url)) {
        //
        // stat the root path
        //

        const QString dirName = i18n("Recent Documents");

        statEntry(udsEntryForRoot(dirName, QStringLiteral("document-open-recent")));
        return KIO::WorkerResult::pass();
    }

    const auto path = url.path();
    if (path == QStringLiteral("/files")) {
        const QString dirName = i18n("Recent Files");
        statEntry(udsEntryForRoot(dirName, QStringLiteral("document-open-recent")));
    } else if (path == QStringLiteral("/locations")) {
        const QString dirName = i18n("Recent Locations");
        statEntry(udsEntryForRoot(dirName, QStringLiteral("folder-open-recent")));
    } else {
        // only / /files and /locations paths are supported
        return KIO::WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
    }
    return KIO::WorkerResult::pass();
}

KIO::WorkerResult RecentlyUsed::mimetype(const QUrl &url)
{
    // the root url is always a folder
    if (isRootUrl(url)) {
        mimeType(QStringLiteral("inode/directory"));
        return KIO::WorkerResult::pass();
    }

    // only the root path is supported
    return KIO::WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
}

KIO::WorkerResult RecentlyUsed::special(const QByteArray &data)
{
    int id;
    QDataStream stream(data);
    stream >> id;

    switch (id) {
    case 1: { // Forget
        QList<QUrl> urls;
        stream >> urls;

        QList<QString> paths;
        for (const auto &url : qAsConst(urls)) {
            if (url.isLocalFile() || url.scheme().isEmpty()) {
                paths.append(url.path());
            } else {
                paths.append(url.toString());
            }
        }

        Query query = UsedResources | Limit(paths.size());
        query.setUrlFilters(Url(paths));
        query.setAgents(Agent::any());
        query.setActivities(Activity::any());

        ResultModel model(query);
        model.forgetResources(paths);

        break;
    }
    default:
        break;
    }

    return KIO::WorkerResult::pass();
}

// needed for JSON file embedding
#include "recentlyused.moc"
