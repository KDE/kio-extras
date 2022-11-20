#ifndef RECENTDOCUMENT_H
#define RECENTDOCUMENT_H

#include <KIO/ForwardingSlaveBase>

class KDirWatch;

class RecentDocuments : public KIO::ForwardingSlaveBase
{
public:
    RecentDocuments(const QByteArray &pool, const QByteArray &app);
    ~RecentDocuments() override;

protected:
    QString desktopFile(KIO::UDSEntry&) const;
    bool rewriteUrl(const QUrl &url, QUrl &newUrl) override;
    void listDir(const QUrl &url) override;
    void stat(const QUrl& url) override;
    void mimetype(const QUrl& url) override;

private:
    KDirWatch* m_recentDocWatch;
};

#endif
