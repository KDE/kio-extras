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
