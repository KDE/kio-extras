#ifndef RECENTDOCUMENT_H
#define RECENTDOCUMENT_H

#include <KIO/ForwardingSlaveBase>

class KDirWatch;

class RecentDocuments : public KIO::ForwardingSlaveBase
{
public:
    RecentDocuments(const QByteArray &pool, const QByteArray &app);
    virtual ~RecentDocuments();

protected:
    QString desktopFile(KIO::UDSEntry&) const;
    virtual bool rewriteUrl(const QUrl &url, QUrl &newUrl);
    virtual void listDir(const QUrl &url);
    virtual void prepareUDSEntry(KIO::UDSEntry &entry, bool listing = false) const;
    virtual void stat(const QUrl& url);
    virtual void mimetype(const QUrl& url);
    virtual void del(const QUrl& url, bool isfile);
private:
    KDirWatch* m_recentDocWatch;
};

#endif
