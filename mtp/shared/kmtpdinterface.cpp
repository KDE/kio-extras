/*
    This file is part of the KMTP framework, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kmtpdinterface.h"
#include "kmtpdeviceinterface.h"

KMTPDInterface::KMTPDInterface(QObject *parent)
    : QObject(parent)
{
    // connect to the KDE MTP daemon over D-Bus
    m_dbusInterface = new org::kde::kmtp::Daemon(QStringLiteral("org.kde.kmtpd5"),
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
