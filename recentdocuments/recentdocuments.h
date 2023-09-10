#ifndef RECENTDOCUMENT_H
#define RECENTDOCUMENT_H

#include <KIO/ForwardingWorkerBase>

class KDirWatch;

class RecentDocuments : public KIO::ForwardingWorkerBase
{
public:
    RecentDocuments(const QByteArray &pool, const QByteArray &app);
    ~RecentDocuments() override;

public:
    KIO::WorkerResult listDir(const QUrl &url) override;
    KIO::WorkerResult stat(const QUrl &url) override;
    KIO::WorkerResult mimetype(const QUrl &url) override;

protected:
    QString desktopFile(KIO::UDSEntry &) const;

    bool rewriteUrl(const QUrl &url, QUrl &newUrl) override;

private:
    KDirWatch *m_recentDocWatch;
};

#endif
