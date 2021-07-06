/*
    This file is part of the MTP KIOD module, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MTPDEVICE_H
#define MTPDEVICE_H

#include <libmtp.h>

#include <QDBusObjectPath>

class MTPStorage;

/**
 * @brief This D-Bus interface is used to access a single MTP device.
 */
class MTPDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString udi READ udi CONSTANT)
    Q_PROPERTY(QString friendlyName READ friendlyName NOTIFY friendlyNameChanged)

public:
    explicit MTPDevice(const QString &dbusObjectPath,
                       LIBMTP_mtpdevice_t *device,
                       LIBMTP_raw_device_t *rawdevice,
                       const QString &udi,
                       qint32 timeout,
                       QObject *parent = nullptr);
    ~MTPDevice() override;

    LIBMTP_mtpdevice_t *getDevice();
    QString dbusObjectName() const;

    // D-Bus properties
    QString udi() const;
    QString friendlyName() const;

private:
    const QString m_dbusObjectName;
    QList<MTPStorage *> m_storages;
    qint32 m_timeout;

    LIBMTP_mtpdevice_t *m_mtpdevice;
    LIBMTP_raw_device_t m_rawdevice;

    QString m_udi;
    QString m_friendlyName;

public Q_SLOTS:
    // D-Bus methods

    int setFriendlyName(const QString &friendlyName);
    QList<QDBusObjectPath> listStorages() const;

Q_SIGNALS:
    void friendlyNameChanged(const QString &friendlyName);
};

#endif // MTPDEVICE_H
