#ifndef RECENTDOCUMENTSNOTIFIER_H
#define RECENTDOCUMENTSNOTIFIER_H

#include <KDEDModule>

class KDirWatch;

class RecentDocumentsNotifier : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.RecentDocumentsNotifier")

public:
    RecentDocumentsNotifier(QObject* parent, const QList<QVariant>&);

private Q_SLOTS:
    void dirty(const QString &path);

private:
    KDirWatch *dirWatch;
};

#endif
