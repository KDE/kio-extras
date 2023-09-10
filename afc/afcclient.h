/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <libimobiledevice/afc.h>
#include <libimobiledevice/house_arrest.h>

#include <QSharedPointer>

#include <KIO/Global>
#include <KIO/Job>
#include <KIO/UDSEntry>
#include <KIO/WorkerBase>

class AfcDevice;

class AfcClient
{
public:
    explicit AfcClient(AfcDevice *devce);
    ~AfcClient();

    using Ptr = QSharedPointer<AfcClient>;

    KIO::WorkerResult init(lockdownd_client_t lockdowndClient, const QString &appId);

    AfcDevice *device() const;
    afc_client_t internalClient() const;

    QString appId() const;

    KIO::WorkerResult entry(const QString &path, KIO::UDSEntry &entry);
    KIO::WorkerResult entryList(const QString &path, QStringList &entryList);

    KIO::WorkerResult del(const QString &path);
    KIO::WorkerResult delRecursively(const QString &path);
    KIO::WorkerResult rename(const QString &src, const QString &dest, KIO::JobFlags flags);
    KIO::WorkerResult symlink(const QString &target, const QString &dest, KIO::JobFlags flags);
    KIO::WorkerResult mkdir(const QString &path);
    KIO::WorkerResult setModificationTime(const QString &path, const QDateTime &mtime);

private:
    AfcDevice *m_device = nullptr;
    QString m_appId;

    afc_client_t m_client = nullptr;
    house_arrest_client_t m_houseArrestClient = nullptr;

    Q_DISABLE_COPY(AfcClient)
};
