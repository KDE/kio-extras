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

#ifndef KMTPD_H
#define KMTPD_H

#include <KDEDModule>
#include <Solid/Device>

#include <QDBusObjectPath>

#include "mtpdevice.h"

Q_DECLARE_LOGGING_CATEGORY(LOG_KIOD_KMTPD)

class KMTPd : public KDEDModule
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit KMTPd(QObject *parent, const QList<QVariant> &parameters);
    ~KMTPd() override;

    QString version() const;

private:
    void checkDevice(const Solid::Device &solidDevice);
    MTPDevice *deviceFromUdi(const QString &udi) const;

    QList<MTPDevice *> m_devices;
    qint32 m_timeout;

public slots:
    // D-Bus methods
    QList<QDBusObjectPath> listDevices() const;

private slots:
    void deviceAdded(const QString &udi);
    void deviceRemoved(const QString &udi);

signals:
    // D-Bus signals
    void devicesChanged();
};

#endif // KMTPD_H
