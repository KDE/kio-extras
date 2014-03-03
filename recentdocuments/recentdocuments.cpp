#include <QCoreApplication>
#include <QDBusInterface>
#include <QFileInfo>

#include <KDebug>
#include <KComponentData>
#include <KRecentDocument>
#include <KDirWatch>
#include <KDesktopFile>
#include <KStandardDirs>
#include <KIO/Job>
#include <KIO/NetAccess>

#include <stdio.h>

#include "recentdocuments.h"

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    // necessary to use other kio slaves
    QCoreApplication app(argc, argv);
    KComponentData("kio_recentdocuments", "kio_recentdocuments");
    KGlobal::locale();
    if (argc != 4) {
        fprintf(stderr, "Usage: kio_recentdocuments protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }
    // start the slave
    RecentDocuments slave(argv[2], argv[3]);
    slave.dispatchLoop();
    return 0;
}

bool isRootUrl(const KUrl& url)
{
    const QString path = url.path(KUrl::RemoveTrailingSlash);
    return(!url.hasQuery() &&
           (path.isEmpty() || path == QLatin1String("/")));
}

RecentDocuments::RecentDocuments(const QByteArray& pool, const QByteArray& app):
        ForwardingSlaveBase("recentdocuments", pool, app)
{
    QDBusInterface kded("org.kde.kded", "/kded", "org.kde.kded");
    kded.call("loadModule", "recentdocumentsnotifier");
}

RecentDocuments::~RecentDocuments()
{

}

bool RecentDocuments::rewriteUrl(const KUrl& url, KUrl& newUrl)
{
    if (isRootUrl(url)) {
        return false;
    } else {
        QString desktopFilePath = QString("%1/%2.desktop").arg(KRecentDocument::recentDocumentDirectory()).arg(url.path(KUrl::RemoveTrailingSlash));
        if (KDesktopFile::isDesktopFile(desktopFilePath)) {
            KDesktopFile file(desktopFilePath);
            if (file.hasLinkType())
                newUrl = KUrl(file.readUrl());
        }

        return !newUrl.isEmpty();
    }
    return false;
}

void RecentDocuments::listDir(const KUrl& url)
{
    if (isRootUrl(url)) {
        // flush
        listEntry(KIO::UDSEntry(), true);

        QStringList list = KRecentDocument::recentDocuments();
        KIO::UDSEntryList udslist;
        QSet<QString> urlSet;
        Q_FOREACH(const QString & entry, list) {
            if (KDesktopFile::isDesktopFile(entry)) {
                QFileInfo info(entry);
                KDesktopFile file(entry);

                KUrl urlInside(file.readUrl());
                QString prettyUrl = urlInside.prettyUrl();
                if (urlInside.protocol() == "recentdocuments" || urlSet.contains(prettyUrl))
                    continue;

                KIO::UDSEntry uds;
                if (urlInside.isLocalFile()) {
                    KIO::StatJob* job = KIO::stat(urlInside, KIO::HideProgressInfo);
                    // we do not want to wait for the event loop to delete the job
                    QScopedPointer<KIO::StatJob> sp(job);
                    job->setAutoDelete(false);
                    if (KIO::NetAccess::synchronousRun(job, 0)) {
                        uds = job->statResult();
                    }
                }

                urlSet.insert(prettyUrl);
                uds.insert(KIO::UDSEntry::UDS_NAME, info.completeBaseName());

                if (urlInside.isLocalFile()) {
                    uds.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, urlInside.toLocalFile());
                    uds.insert(KIO::UDSEntry::UDS_LOCAL_PATH, urlInside.path());
                } else {
                    uds.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, prettyUrl);
                    uds.insert(KIO::UDSEntry::UDS_ICON_NAME, file.readIcon());
                }
                uds.insert(KIO::UDSEntry::UDS_TARGET_URL, prettyUrl);
                udslist << uds;
            }
        }
        listEntries(udslist);

        listEntry(KIO::UDSEntry(), true);
        finished();
    }
    else
        error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
}

void RecentDocuments::prepareUDSEntry(KIO::UDSEntry& entry, bool listing) const
{
    ForwardingSlaveBase::prepareUDSEntry(entry, listing);
}

QString RecentDocuments::desktopFile(KIO::UDSEntry& entry) const
{
    const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
    if (name == "." || name == "..")
        return QString();

    KUrl url = processedUrl();
    url.addPath(name);

    if (KDesktopFile::isDesktopFile(url.toLocalFile()))
        return url.toLocalFile();

    return QString();
}

void RecentDocuments::stat(const KUrl& url)
{
    if (isRootUrl(url)) {
        kDebug() << "Stat root" << url;
        //
        // stat the root path
        //
        QString dirName = i18n("Recent Documents");
        KIO::UDSEntry uds;
        uds.insert(KIO::UDSEntry::UDS_NAME, dirName);
        uds.insert(KIO::UDSEntry::UDS_DISPLAY_NAME, dirName);
        uds.insert(KIO::UDSEntry::UDS_DISPLAY_TYPE, dirName);
        uds.insert(KIO::UDSEntry::UDS_ICON_NAME, QString::fromLatin1("document-open-recent"));
        uds.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        uds.insert(KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("inode/directory"));

        statEntry(uds);
        finished();
    }
    // results are forwarded
    else {
        kDebug() << "Stat forward" << url;
        ForwardingSlaveBase::stat(url);
    }
}

void RecentDocuments::mimetype(const KUrl& url)
{
    kDebug() << url;

    // the root url is always a folder
    if (isRootUrl(url)) {
        mimeType(QString::fromLatin1("inode/directory"));
        finished();
    }
    // results are forwarded
    else {
        ForwardingSlaveBase::mimetype(url);
    }
}

void RecentDocuments::del(const KUrl& url, bool isFile)
{
    ForwardingSlaveBase::del(url, isFile);
}
