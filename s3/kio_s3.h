/*
    SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KIO_S3_H
#define KIO_S3_H

#include "s3url.h"

#include <KIO/SlaveBase>

#include <QUrl>

#include <aws/s3/S3Client.h>

class S3Slave : public KIO::SlaveBase
{
public:

    S3Slave(const QByteArray &protocol,
            const QByteArray &pool_socket,
            const QByteArray &app_socket);
    ~S3Slave() override;

    void listDir(const QUrl &url) override;
    void stat(const QUrl &url) override;
    void mimetype(const QUrl &url) override;
    void get(const QUrl &url) override;
    void put(const QUrl &url, int, KIO::JobFlags flags) override;
    void copy(const QUrl &src, const QUrl &dest, int, KIO::JobFlags flags) override;
    void mkdir(const QUrl &url, int) override;
    void del(const QUrl &url, bool) override;
    void rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;

private:
    Q_DISABLE_COPY(S3Slave)

    enum CwdAccess {
        ReadOnlyCwd,
        WritableCwd
    };

    void listBuckets();
    void listBucket(const QString &bucketName);
    void listKey(const S3Url &s3url);
    void listCwdEntry(CwdAccess access = WritableCwd);
    bool deletePrefix(const Aws::S3::S3Client &client, const S3Url &s3url, const QString &prefix);
    QString contentType(const S3Url &s3url);

    QByteArray m_configProfileName;    // This must be passed to the S3Client objects to get the proper region from ~/.aws/config
};

#endif // KIO_S3_H
