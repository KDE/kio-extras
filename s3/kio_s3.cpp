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

#include "kio_s3.h"
#include "s3debug.h"

#include <KLocalizedString>

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

S3Slave::S3Slave(const QByteArray &protocol, const QByteArray &pool_socket,
                      const QByteArray &app_socket):
    SlaveBase("s3", pool_socket, app_socket)
{
    Q_UNUSED(protocol)
    qCDebug(S3) << "kio_s3 ready";
}

S3Slave::~S3Slave()
{
    qCDebug(S3) << "kio_s3 ready";
}


void S3Slave::listDir(const QUrl &url)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::stat(const QUrl &url)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::mimetype(const QUrl &url)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::get(const QUrl &url)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::put(const QUrl &url, int, KIO::JobFlags flags)
{
    Q_UNUSED(url)
    Q_UNUSED(flags)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::copy(const QUrl &src, const QUrl &dest, int, KIO::JobFlags flags)
{
    Q_UNUSED(src)
    Q_UNUSED(dest)
    Q_UNUSED(flags)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::mkdir(const QUrl &url, int)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::del(const QUrl &url, bool)
{
    Q_UNUSED(url)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

void S3Slave::rename(const QUrl &src, const QUrl &dest, KIO::JobFlags flags)
{
    Q_UNUSED(src)
    Q_UNUSED(dest)
    Q_UNUSED(flags)
    qCDebug(S3) << "Not implemented yet.";
    error(KIO::ERR_UNSUPPORTED_ACTION, i18n("Not implemented yet."));
}

#include "kio_s3.moc"
