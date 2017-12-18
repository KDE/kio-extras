/*
    Cache for recently used devices.
    Copyright (C) 2012  Philipp Schmidt <philschmidt@gmx.net>

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

#include "devicecache.h"
#include "kio_mtp_helpers.h"

// #include <libudev.h>
// #include <fcntl.h>

#include <Solid/Device>
#include <Solid/GenericInterface>
#include <Solid/DeviceNotifier>

/**
 * Creates a Cached Device that has a predefined lifetime (default: 10000 msec)s
 * The lifetime is reset every time the device is accessed. After it expires it
 * will be released.
 *
 * @param device The LIBMTP_mtpdevice_t pointer to cache
 * @param udi The UDI of the new device to cache
 */
CachedDevice::CachedDevice(LIBMTP_mtpdevice_t *device, LIBMTP_raw_device_t *rawdevice, const QString udi, qint32 timeout)
{
    this->timeout = timeout;
    this->mtpdevice = device;
    this->rawdevice = *rawdevice;
    this->udi = udi;

    char *deviceName = LIBMTP_Get_Friendlyname(device);
    char *deviceModel = LIBMTP_Get_Modelname(device);

    // prefer friendly devicename over model
    if (!deviceName) {
        name = QString::fromUtf8(deviceModel);
    } else {
        name = QString::fromUtf8(deviceName);
    }

    qCDebug(LOG_KIO_MTP) << "Created device " << name << "  with udi=" << udi << " and timeout " << timeout;
}

CachedDevice::~CachedDevice()
{
    LIBMTP_Release_Device(mtpdevice);
}

LIBMTP_mtpdevice_t *CachedDevice::getDevice()
{
    LIBMTP_mtpdevice_t *device = mtpdevice;
    if (!device->storage) {
        qCDebug(LOG_KIO_MTP) << "reopen mtpdevice if we have no storage found";
        LIBMTP_Release_Device(mtpdevice);
        mtpdevice = LIBMTP_Open_Raw_Device_Uncached(&rawdevice);
    }

    return mtpdevice;
}

const QString CachedDevice::getName()
{
    return name;
}
const QString CachedDevice::getUdi()
{
    return udi;
}

DeviceCache::DeviceCache(qint32 timeout, QObject *parent)
    : QEventLoop(parent)
{
    this->timeout = timeout;

    notifier = Solid::DeviceNotifier::instance();

    connect(notifier, SIGNAL(deviceAdded(QString)), this, SLOT(deviceAdded(QString)));
    connect(notifier, SIGNAL(deviceRemoved(QString)), this, SLOT(deviceRemoved(QString)));

    foreach(Solid::Device solidDevice, Solid::Device::listFromType(Solid::DeviceInterface::PortableMediaPlayer, QString())) {
        checkDevice(solidDevice);
    }
}

DeviceCache::~DeviceCache()
{
    processEvents();

    // Release devices
    foreach(QString udi, udiCache.keys()) {
        deviceRemoved(udi);
    }
}

void DeviceCache::checkDevice(Solid::Device solidDevice)
{
    if (!udiCache.contains(solidDevice.udi())) {
        qCDebug(LOG_KIO_MTP) << "new device, getting raw devices";

        Solid::GenericInterface *iface = solidDevice.as<Solid::GenericInterface>();
        if (!iface) {
            qCDebug( LOG_KIO_MTP ) << "Solid device " << solidDevice.udi() << " has NOT a Solid::GenericInterface";
            return;
        }

        const QMap<QString, QVariant> &properties = iface->allProperties();
        const uint32_t solidBusNum = properties.value ( QLatin1String ( "BUSNUM" ) ).toUInt();
        const uint32_t solidDevNum = properties.value ( QLatin1String ( "DEVNUM" ) ).toUInt();

        LIBMTP_raw_device_t *rawdevices = nullptr;
        int numrawdevices;
        LIBMTP_error_number_t err;


        err = LIBMTP_Detect_Raw_Devices(&rawdevices, &numrawdevices);
        switch (err) {
        case LIBMTP_ERROR_CONNECTING:
            qCWarning(LOG_KIO_MTP) << "There has been an error connecting to the devices";
            break;
        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            qCWarning(LOG_KIO_MTP) << "Encountered a Memory Allocation Error";
            break;
        case LIBMTP_ERROR_NONE: {
            qCDebug(LOG_KIO_MTP) << "No Error, continuing";

            for (int i = 0; i < numrawdevices; i++) {
                LIBMTP_raw_device_t *rawDevice = &rawdevices[i];
                uint32_t rawBusNum = rawDevice->bus_location;
                uint32_t rawDevNum = rawDevice->devnum;

                if (rawBusNum == solidBusNum && rawDevNum == solidDevNum) {
                    qCDebug(LOG_KIO_MTP) << "Found device matching the Solid description";

                    LIBMTP_mtpdevice_t *mtpDevice = LIBMTP_Open_Raw_Device_Uncached(rawDevice);

                    if (udiCache.find(solidDevice.udi()) == udiCache.end()) {
                        CachedDevice *cDev = new CachedDevice(mtpDevice, rawDevice, solidDevice.udi(), timeout);
                        udiCache.insert(solidDevice.udi(), cDev);
                        nameCache.insert(cDev->getName(), cDev);
                    }
                }
            }
        }
        break;
        case LIBMTP_ERROR_GENERAL:
        default:
            qCWarning(LOG_KIO_MTP) << "Unknown connection error";
            break;
        }
        free(rawdevices);
    }
}

void DeviceCache::deviceAdded(const QString &udi)
{
    qCDebug(LOG_KIO_MTP) << "New device attached with udi=" << udi << ". Checking if PortableMediaPlayer...";

    Solid::Device device(udi);
    if (device.isDeviceInterface(Solid::DeviceInterface::PortableMediaPlayer)) {
        qCDebug(LOG_KIO_MTP) << "SOLID: New Device with udi=" << udi << "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||";

        checkDevice(device);
    }
}

void DeviceCache::deviceRemoved(const QString &udi)
{
    if (udiCache.contains(udi)) {
        qCDebug(LOG_KIO_MTP) << "SOLID: Device with udi=" << udi << " removed. ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||";

        CachedDevice *cDev = udiCache.value(udi);

        udiCache.remove(cDev->getUdi());
        nameCache.remove(cDev->getName());
        delete cDev;
    }
}

QHash<QString, CachedDevice *> DeviceCache::getAll()
{
    qCDebug(LOG_KIO_MTP) << "getAll()";

    processEvents();

    return nameCache;
}

bool DeviceCache::contains(QString string, bool isUdi)
{
    processEvents();

    if (isUdi) {
        return udiCache.find(string) != udiCache.end();
    } else {
        return nameCache.find(string) != nameCache.end();
    }
}

CachedDevice *DeviceCache::get(const QString &string, bool isUdi)
{
    processEvents();

    if (isUdi) {
        return udiCache.value(string);
    } else {
        return nameCache.value(string);
    }
}

int DeviceCache::size()
{
    processEvents();

    return nameCache.size();
}

#include "devicecache.moc"
