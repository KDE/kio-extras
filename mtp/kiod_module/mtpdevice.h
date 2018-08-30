/*
    This file is part of the MTP KIOD module, part of the KDE project.

    Copyright (C) 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MTPDEVICE_H
#define MTPDEVICE_H

#include <libmtp.h>

#include <QDBusObjectPath>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(LOG_KIOD_KMTPD)

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

public slots:
    // D-Bus methods

    int setFriendlyName(const QString &friendlyName);
    QList<QDBusObjectPath> listStorages() const;

signals:
    void friendlyNameChanged(const QString &friendlyName);
};

#endif // MTPDEVICE_H
