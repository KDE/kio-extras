#include <KDirWatch>
#include <KGlobal>
#include <KGlobalSettings>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KStandardDirs>
#include <QUrl>
#include <KDirNotify>
#include <KRecentDocument>

#include "recentdocumentsnotifier.h"

K_PLUGIN_FACTORY(RecentDocumentsFactory, registerPlugin<RecentDocumentsNotifier>();)
K_EXPORT_PLUGIN(RecentDocumentsFactory("kio_recentdocuments"))


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
    if (path.endsWith(".desktop")) {
        // Emitting FilesAdded forces a re-read of the dir
        QUrl url("recentdocuments:/");
        QFileInfo info(path);
        url.setPath(QStringLiteral("/") + info.fileName());
        org::kde::KDirNotify::emitFilesAdded(url);
    }
}

#include "recentdocumentsnotifier.moc"
