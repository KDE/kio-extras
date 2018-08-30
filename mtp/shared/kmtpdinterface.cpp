/*
    This file is part of the KMTP framework, part of the KDE project.

    Copyright (C) 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#include "kmtpdinterface.h"
#include "kmtpdeviceinterface.h"

KMTPDInterface::KMTPDInterface(QObject *parent)
    : QObject(parent)
{
    // connect to the KDE MTP daemon over D-Bus
    m_dbusInterface = new org::kde::kmtp::Daemon(QStringLiteral("org.kde.kiod5"),
                                                 QStringLiteral("/modules/kmtpd"),
                                                 QDBusConnection::sessionBus());

    updateDevices();
}

bool KMTPDInterface::isValid() const
{
    return m_dbusInterface->isValid();
}

KMTPDeviceInterface *KMTPDInterface::deviceFromName(const QString &friendlyName) const
{
    auto deviceIt = std::find_if(m_devices.constBegin(), m_devices.constEnd(), [friendlyName] (const KMTPDeviceInterface *device) {
        return device->friendlyName() == friendlyName;
    });

    return deviceIt == m_devices.constEnd() ? nullptr : *deviceIt;
}

KMTPDeviceInterface *KMTPDInterface::deviceFromUdi(const QString &udi) const
{
    auto deviceIt = std::find_if(m_devices.constBegin(), m_devices.constEnd(), [udi] (const KMTPDeviceInterface *device) {
        return device->udi() == udi;
    });

    return deviceIt == m_devices.constEnd() ? nullptr : *deviceIt;
}

QVector<KMTPDeviceInterface *> KMTPDInterface::devices() const
{
    return m_devices;
}

QString KMTPDInterface::version() const
{
    return m_dbusInterface->version();
}

void KMTPDInterface::updateDevices()
{
    qDeleteAll(m_devices);
    m_devices.clear();
    const auto deviceNames = m_dbusInterface->listDevices().value();
    for (const QDBusObjectPath &deviceName : deviceNames) {
        m_devices.append(new KMTPDeviceInterface(deviceName.path(), this));
    }
}

QList<QDBusObjectPath> KMTPDInterface::listDevices()
{
    return m_dbusInterface->listDevices();
}

#include "moc_kmtpdinterface.cpp"
