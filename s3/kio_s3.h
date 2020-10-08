/*
    SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KIO_S3_H
#define KIO_S3_H

#include "s3backend.h"

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
    void put(const QUrl &url, int permissions, KIO::JobFlags flags) override;
    void copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags) override;
    void mkdir(const QUrl &url, int permissions) override;
    void del(const QUrl &url, bool isfile) override;
    void rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags) override;

private:
    Q_DISABLE_COPY(S3Slave)

    void finalize(const S3Backend::Result &result);

    QScopedPointer<S3Backend> d { new S3Backend(this) };
};

#endif // KIO_S3_H
