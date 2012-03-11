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
    virtual bool rewriteUrl(const KUrl &url, KUrl &newUrl);
    virtual void listDir(const KUrl &url);
    virtual void prepareUDSEntry(KIO::UDSEntry &entry, bool listing = false) const;
    virtual void stat(const KUrl& url);
    virtual void mimetype(const KUrl& url);
    virtual void del(const KUrl& url, bool isfile);
private:
    KDirWatch* m_recentDocWatch;
};

#endif
