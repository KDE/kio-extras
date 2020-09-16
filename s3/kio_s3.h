/*
 *  Copyright (C) 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef KIO_S3_H
#define KIO_S3_H

#include "s3url.h"

#include <KIO/SlaveBase>

#include <QUrl>

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

    void listBuckets();
    void listBucket(const QString &bucketName);
    void listFolder(const S3Url &s3url);
    void listCwdEntry();
    QString contentType(const S3Url &s3url);

    QByteArray m_configProfileName;    // This must be passed to the S3Client objects to get the proper region from ~/.aws/config
    QStringList m_bucketNamesCache;
};

#endif // KIO_S3_H
