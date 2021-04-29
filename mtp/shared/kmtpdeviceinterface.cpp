/*
    This file is part of the KMTP framework, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kmtpdeviceinterface.h"
#include "kmtpstorageinterface.h"

KMTPDeviceInterface::KMTPDeviceInterface(const QString &dbusObjectPath, QObject *parent)
    : QObject(parent)
{
    m_dbusInterface = new org::kde::kmtp::Device(QStringLiteral("org.kde.kiod5"),
                                                 dbusObjectPath,
                                                 QDBusConnection::sessionBus(),
                                                 this);

    const auto storageNames = m_dbusInterface->listStorages().value();
    m_storages.reserve(storageNames.count());
    for (const QDBusObjectPath &storageName : storageNames) {
        m_storages.append(new KMTPStorageInterface(storageName.path(), this));
    }
}

QString KMTPDeviceInterface::udi() const
{
    return m_dbusInterface->udi();
}

QString KMTPDeviceInterface::friendlyName() const
{
    return m_dbusInterface->friendlyName();
}

QVector<KMTPStorageInterface *> KMTPDeviceInterface::storages() const
{
    return m_storages;
}

KMTPStorageInterface *KMTPDeviceInterface::storageFromDescription(const QString &description) const
{
    auto storageIt = std::find_if(m_storages.constBegin(), m_storages.constEnd(), [description] (KMTPStorageInterface *storage) {
        return storage->description() == description;
    });

    return storageIt == m_storages.constEnd() ? nullptr : *storageIt;
}

int KMTPDeviceInterface::setFriendlyName(const QString &friendlyName)
{
    return m_dbusInterface->setFriendlyName(friendlyName);
}

#include "moc_kmtpdeviceinterface.cpp"
