#ifndef __kio_man_h__
#define __kio_man_h__


#include <qstring.h>
#include <qcstring.h>
#include <qstringlist.h>
#include <qdict.h>


#include <kio/global.h>
#include <kio/slavebase.h>


class MANProtocol : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:

    MANProtocol(const QCString &pool_socket, const QCString &app_socket);
    virtual ~MANProtocol();

    virtual void get(const KURL& url);
    virtual void stat(const KURL& url);

    virtual void mimetype(const KURL &url);

    void outputError(const QString& errmsg);

    void showMainIndex();
    void showIndex(const QString& section);

    static MANProtocol *self();

    QCString findPage(const QString& section, const QString &title) const;

private:
    void addToBuffer(const char *buffer, int buflen);
    void initCache(const QString& section);
    QString pageName(const QString& page) const;

    QDict<char> *_cache;

    ssize_t m_unzippedLength;
    ssize_t m_unzippedBufferSize;
    char *m_unzippedData;

    static MANProtocol *_self;

};


#endif
