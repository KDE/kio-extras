// SPDX-License-Identifier: MIT

#ifndef __info_h__
#define __info_h__

#include "kio_info_debug.h"

#include <KIO/WorkerBase>

class InfoProtocol : public KIO::WorkerBase
{
public:
    InfoProtocol(const QByteArray &pool, const QByteArray &app);
    ~InfoProtocol() override = default;

    KIO::WorkerResult get(const QUrl &url) override;
    KIO::WorkerResult stat(const QUrl &url) override;
    KIO::WorkerResult mimetype(const QUrl &url) override;

protected:
    void decodeURL(const QUrl &url);
    void decodePath(QString path);

private:
    KIO::WorkerResult missingFilesReult() const;

private:
    QString m_page;
    QString m_node;

    QString m_perl;
    QString m_infoScript;
    QString m_infoConf;

    QStringList m_missingFiles;
};

#endif // __info_h__
