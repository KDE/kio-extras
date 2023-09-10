/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KIO/Global>
#include <KIO/WorkerBase>

#include <QMap>
#include <QMutex>
#include <QScopedPointer>
#include <QString>

#include <libimobiledevice/libimobiledevice.h>

#include <memory>

#include "afcclient.h"

using namespace KIO;

class AfcDevice;
class AfcFile;
class AfcUrl;

using Result = KIO::WorkerResult;

class AfcWorker : public KIO::WorkerBase
{
public:
    explicit AfcWorker(const QByteArray &poolSocket, const QByteArray &appSocket);
    ~AfcWorker() override;

    void onDeviceEvent(const idevice_event_t *event);

    Q_REQUIRED_RESULT Result init();

    Q_REQUIRED_RESULT Result listDir(const QUrl &url) override;

    Q_REQUIRED_RESULT Result stat(const QUrl &url) override;

    Q_REQUIRED_RESULT Result get(const QUrl &url) override;
    Q_REQUIRED_RESULT Result put(const QUrl &url, int permissions, KIO::JobFlags flags) override;

    Q_REQUIRED_RESULT Result open(const QUrl &url, QIODevice::OpenMode mode) override;
    Q_REQUIRED_RESULT Result read(KIO::filesize_t bytesRequested) override;
    Q_REQUIRED_RESULT Result seek(KIO::filesize_t offset) override;
    Q_REQUIRED_RESULT Result truncate(KIO::filesize_t length) override;
    Q_REQUIRED_RESULT Result write(const QByteArray &data) override;
    Q_REQUIRED_RESULT Result close() override;

    Q_REQUIRED_RESULT Result copy(const QUrl &src, const QUrl &dest, int permissions, JobFlags flags) override;
    Q_REQUIRED_RESULT Result del(const QUrl &url, bool isFile) override;
    Q_REQUIRED_RESULT Result rename(const QUrl &url, const QUrl &dest, KIO::JobFlags flags) override;
    Q_REQUIRED_RESULT Result symlink(const QString &target, const QUrl &dest, KIO::JobFlags flags) override;
    Q_REQUIRED_RESULT Result mkdir(const QUrl &url, int permissions) override;
    Q_REQUIRED_RESULT Result setModificationTime(const QUrl &url, const QDateTime &mtime) override;

    Q_REQUIRED_RESULT Result fileSystemFreeSpace(const QUrl &url) override;

private:
    void updateDeviceList();
    bool addDevice(const QString &id);
    void removeDevice(const QString &id);

    Q_REQUIRED_RESULT Result clientForUrl(const AfcUrl &afcUrl, AfcClient::Ptr &client) const;
    QString deviceIdForFriendlyUrl(const AfcUrl &afcUrl) const;

    QUrl resolveSolidUrl(const QUrl &url) const;
    bool redirectIfSolidUrl(const QUrl &url);

    UDSEntry overviewEntry(const QString &fileName = QString()) const;
    UDSEntry deviceEntry(const AfcDevice *device, const QString &fileName = QString(), bool asLink = false) const;
    UDSEntry appsOverviewEntry(const AfcDevice *device, const QString &fileName = QString()) const;

    void guessMimeType(AfcFile &file, const QString &path);

    QMutex m_mutex;

    QMap<QString /*udid*/, AfcDevice *> m_devices;
    QMap<QString /*pretty name*/, QString /*udid*/> m_friendlyNames;

    std::unique_ptr<AfcFile> m_openFile;
};
