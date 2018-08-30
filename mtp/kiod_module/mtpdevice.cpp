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

#include "mtpdevice.h"

#include <QDBusConnection>

#include <Solid/Device>
#include <Solid/GenericInterface>
#include <Solid/DeviceNotifier>

#include "mtpstorage.h"

// D-Bus adaptors
#include "deviceadaptor.h"

/**
 * Creates a Cached Device that has a predefined lifetime (default: 10000 msec)s
 * The lifetime is reset every time the device is accessed. After it expires it
 * will be released.
 *
 * @param device The LIBMTP_mtpdevice_t pointer to cache
 * @param udi The UDI of the new device to cache
 */
MTPDevice::MTPDevice(const QString &dbusObjectPath, LIBMTP_mtpdevice_t *device, LIBMTP_raw_device_t *rawdevice, const QString &udi, qint32 timeout, QObject *parent)
    : QObject(parent),
      m_dbusObjectName(dbusObjectPath),
      m_timeout(timeout),
      m_mtpdevice(device),
      m_rawdevice(*rawdevice),
      m_udi(udi)
{
    const char *deviceName = LIBMTP_Get_Friendlyname(device);
    const char *deviceModel = LIBMTP_Get_Modelname(device);

    // prefer friendly devicename over model
    if (!deviceName) {
        m_friendlyName = QString::fromUtf8(deviceModel);
    } else {
        m_friendlyName = QString::fromUtf8(deviceName);
    }

    qCDebug(LOG_KIOD_KMTPD) << "Created device " << m_friendlyName << "  with udi=" << udi << " and timeout " << timeout;

    new DeviceAdaptor(this);
    QDBusConnection::sessionBus().registerObject(m_dbusObjectName, this);

    int index = 0;
    for (LIBMTP_devicestorage_t *storage = device->storage; storage != nullptr; storage = storage->next) {
        m_storages.append(new MTPStorage(QStringLiteral("%1/storage%2").arg(m_dbusObjectName).arg(index++), storage, this));
    }
}

MTPDevice::~MTPDevice()
{
    qCDebug(LOG_KIOD_KMTPD) << "release device:" << m_friendlyName;
    LIBMTP_Release_Device(m_mtpdevice);
}

LIBMTP_mtpdevice_t *MTPDevice::getDevice()
{
    if (!m_mtpdevice->storage) {
        qCDebug(LOG_KIOD_KMTPD) << "no storage found: reopen mtpdevice";
        LIBMTP_Release_Device(m_mtpdevice);
        m_mtpdevice = LIBMTP_Open_Raw_Device_Uncached(&m_rawdevice);
    }

    return m_mtpdevice;
}

QString MTPDevice::dbusObjectName() const
{
    return m_dbusObjectName;
}

QString MTPDevice::udi() const
{
    return m_udi;
}

QString MTPDevice::friendlyName() const
{
    return m_friendlyName;
}

int MTPDevice::setFriendlyName(const QString &friendlyName)
{
    if (m_friendlyName == friendlyName) {
        return 1;
    }

    const int result = LIBMTP_Set_Friendlyname(m_mtpdevice, friendlyName.toUtf8().constData());
    if (!result) {
        m_friendlyName = friendlyName;
        emit friendlyNameChanged(m_friendlyName);

    }
    return result;
}

QList<QDBusObjectPath> MTPDevice::listStorages() const
{
    QList<QDBusObjectPath> list;
    list.reserve(m_storages.count());
    for (const MTPStorage *storage : m_storages) {
        list.append(QDBusObjectPath(storage->dbusObjectPath()));
    }
    return list;
}

#include "moc_mtpdevice.cpp"
