/*
    SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kio_s3.h"
#include "s3debug.h"

#include <QCoreApplication>

class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.s3" FILE "s3.json")
};

extern "C"
int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QLatin1String("kio_s3"));

    if (argc != 4) {
        fprintf(stderr, "Usage: kio_s3 protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    S3Slave slave(argv[1], argv[2], argv[3]);
    qCDebug(S3) << "Starting kio_s3...";
    slave.dispatchLoop();

    return 0;
}

S3Slave::S3Slave(const QByteArray &protocol, const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase("s3", pool_socket, app_socket)
{
    Q_UNUSED(protocol)
    qCDebug(S3) << "kio_s3 ready.";
}

S3Slave::~S3Slave()
{
    qCDebug(S3) << "kio_s3 ended.";
}

void S3Slave::listDir(const QUrl &url)
{
    finalize(d->listDir(url));
}

void S3Slave::stat(const QUrl &url)
{
    finalize(d->stat(url));
}

void S3Slave::mimetype(const QUrl &url)
{
    finalize(d->mimetype(url));
}

void S3Slave::get(const QUrl &url)
{
    finalize(d->get(url));
}

void S3Slave::put(const QUrl &url, int permissions, KIO::JobFlags flags)
{
    finalize(d->put(url, permissions, flags));
}

void S3Slave::copy(const QUrl &src, const QUrl &dest, int permissions, KIO::JobFlags flags)
{
    finalize(d->copy(src, dest, permissions, flags));
}

void S3Slave::mkdir(const QUrl &url, int permissions)
{
    finalize(d->mkdir(url, permissions));
}

void S3Slave::del(const QUrl &url, bool isfile)
{
    finalize(d->del(url, isfile));
}

void S3Slave::rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags)
{
    finalize(d->rename(src, dest, flags));
}

void S3Slave::finalize(const S3Backend::Result &result)
{
    if (result.exitCode > 0) {
        error(result.exitCode, result.errorMessage);
    } else {
        finished();
    }
}

#include "kio_s3.moc"
