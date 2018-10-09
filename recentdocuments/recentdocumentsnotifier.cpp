#include "recentdocumentsnotifier.h"

#include <KDirWatch>
#include <KPluginFactory>
#include <QUrl>
#include <KDirNotify>
#include <KRecentDocument>
#include <QFileInfo>

K_PLUGIN_FACTORY_WITH_JSON(RecentDocumentsFactory,
                           "recentdocumentsnotifier.json",
                           registerPlugin<RecentDocumentsNotifier>();)

RecentDocumentsNotifier::RecentDocumentsNotifier(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    dirWatch = new KDirWatch(this);
    dirWatch->addDir(KRecentDocument::recentDocumentDirectory(), KDirWatch::WatchFiles);
    connect(dirWatch, &KDirWatch::created, this, &RecentDocumentsNotifier::dirty);
    connect(dirWatch, &KDirWatch::deleted, this, &RecentDocumentsNotifier::dirty);
    connect(dirWatch, &KDirWatch::dirty, this, &RecentDocumentsNotifier::dirty);
}

void RecentDocumentsNotifier::dirty(const QString &path)
{
    if (path.endsWith(QLatin1String(".desktop"))) {
        // Emitting FilesAdded forces a re-read of the dir
        QUrl url("recentdocuments:/");
        QFileInfo info(path);
        url.setPath(QStringLiteral("/") + info.fileName());
        org::kde::KDirNotify::emitFilesAdded(url);
    }
}

#include "recentdocumentsnotifier.moc"
