#include <KDirWatch>
#include <KGlobal>
#include <KGlobalSettings>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KStandardDirs>
#include <KUrl>
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
    connect(dirWatch, SIGNAL(created(QString)), this, SLOT(dirty(QString)));
    connect(dirWatch, SIGNAL(deleted(QString)), this, SLOT(dirty(QString)));
    connect(dirWatch, SIGNAL(dirty(QString)), this, SLOT(dirty(QString)));
}

void RecentDocumentsNotifier::dirty(const QString &path)
{
    if (path.endsWith(".desktop")) {
        // Emitting FilesAdded forces a re-read of the dir
        KUrl url("recentdocuments:/");
        QFileInfo info(path);
        url.addPath(info.fileName());
        org::kde::KDirNotify::emitFilesAdded(url.url());
    }
}

#include "recentdocumentsnotifier.moc"
