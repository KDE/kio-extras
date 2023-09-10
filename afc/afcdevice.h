/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QVector>

#include <KIO/WorkerBase>

#include <libimobiledevice/afc.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

#include "afcclient.h"
#include "afcdiskusage.h"
#include "afcfilereader.h"

#include <KIO/UDSEntry>

class AfcApp;

struct LockdowndClientCleanup {
    static inline void cleanup(lockdownd_client_t lockdowndClient)
    {
        if (lockdowndClient) {
            lockdownd_client_free(lockdowndClient);
        }
    }
};
using ScopedLockdowndClientPtr = QScopedPointer<lockdownd_client_private, LockdowndClientCleanup>;

class AfcDevice
{
public:
    explicit AfcDevice(const QString &id);
    ~AfcDevice();

    idevice_t device() const;

    QString id() const;
    bool isValid() const;
    QString errorText() const;

    QString name() const;
    QString deviceClass() const;

    QString cacheLocation() const;

    KIO::WorkerResult client(const QString &appId, AfcClient::Ptr &client);

    AfcApp app(const QString &bundleId);
    KIO::WorkerResult apps(QVector<AfcApp> &apps);

    KIO::WorkerResult fetchAppIcon(AfcApp &app);
    // Fetches app icons for the list of apps provided.
    KIO::WorkerResult fetchAppIcons(QVector<AfcApp> &apps);

private:
    KIO::WorkerResult handshake();
    QString appIconCachePath(const QString &bundleId) const;

    idevice_t m_device = nullptr;

    ScopedLockdowndClientPtr m_lockdowndClient;
    bool m_handshakeSuccessful = false;

    afc_client_t m_afcClient = nullptr;

    QString m_id;
    QString m_name;
    QString m_deviceClass;

    QHash<QString, AfcApp> m_apps;

    // We can't have too many simultaneous house arrest connections
    // so store only the last requested client which in the majority case
    // should be the one where subsequent requests go to
    AfcClient::Ptr m_lastClient;
};
