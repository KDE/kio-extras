/*
    This file is part of the MTP KIOD module, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmtpd.h"

#include <QDBusConnection>
#include <QDebug>

#include <KPluginFactory>
#include <Solid/GenericInterface>
#include <Solid/DeviceNotifier>

#include "daemonadaptor.h"
#include "kiod_kmtpd_debug.h"
#include "mtpdevice.h"

K_PLUGIN_CLASS_WITH_JSON(KMTPd, "kmtpd.json")

KMTPd::KMTPd(QObject *parent, const QList<QVariant> &parameters)
    : KDEDModule(parent)
{
    Q_UNUSED(parameters)

    LIBMTP_Init();

    // search for already connected devices
    for (const Solid::Device &solidDevice : Solid::Device::listFromType(Solid::DeviceInterface::PortableMediaPlayer)) {
        checkDevice(solidDevice);
    }

    auto notifier = Solid::DeviceNotifier::instance();
    connect(notifier, &Solid::DeviceNotifier::deviceAdded, this, &KMTPd::deviceAdded);
    connect(notifier, &Solid::DeviceNotifier::deviceRemoved, this, &KMTPd::deviceRemoved);

    new DaemonAdaptor(this);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kiod5"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/modules/kmtpd"), this);
}

KMTPd::~KMTPd()
{
    // Release devices
    for (const MTPDevice *device : qAsConst(m_devices)) {
        deviceRemoved(device->udi());
    }
}

QString KMTPd::version() const
{
    return QStringLiteral(LIBMTP_VERSION_STRING);
}

void KMTPd::checkDevice(const Solid::Device &solidDevice)
{
    if (!deviceFromUdi(solidDevice.udi())) {
        qCDebug(LOG_KIOD_KMTPD) << "new device, getting raw devices";

        const Solid::GenericInterface *iface = solidDevice.as<Solid::GenericInterface>();
        if (!iface) {
            qCDebug(LOG_KIOD_KMTPD) << "Solid device " << solidDevice.udi() << " has NOT a Solid::GenericInterface";
            return;
        }

        const QMap<QString, QVariant> &properties = iface->allProperties();
        const quint32 solidBusNum = properties.value(QStringLiteral("BUSNUM")).toUInt();
        const quint32 solidDevNum = properties.value(QStringLiteral("DEVNUM")).toUInt();

        LIBMTP_raw_device_t *rawdevices = nullptr;
        int numrawdevices;
        LIBMTP_error_number_t err;

        err = LIBMTP_Detect_Raw_Devices(&rawdevices, &numrawdevices);
        switch (err) {
        case LIBMTP_ERROR_CONNECTING:
            qCWarning(LOG_KIOD_KMTPD) << "There has been an error connecting to the devices";
            break;
        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            qCWarning(LOG_KIOD_KMTPD) << "Encountered a Memory Allocation Error";
            break;
        case LIBMTP_ERROR_NONE: {
            qCDebug(LOG_KIOD_KMTPD) << "No Error, continuing";

            for (int i = 0; i < numrawdevices; i++) {
                LIBMTP_raw_device_t *rawDevice = &rawdevices[i];
                uint32_t rawBusNum = rawDevice->bus_location;
                uint32_t rawDevNum = rawDevice->devnum;

                if (rawBusNum == solidBusNum && rawDevNum == solidDevNum) {
                    qCDebug(LOG_KIOD_KMTPD) << "Found device matching the Solid description";

                    LIBMTP_mtpdevice_t *mtpDevice = LIBMTP_Open_Raw_Device_Uncached(rawDevice);
                    if (mtpDevice) {
                        MTPDevice *device = new MTPDevice(QStringLiteral("/modules/kmtpd/device%1").arg(m_devices.count()), mtpDevice, rawDevice, solidDevice.udi(), m_timeout);
                        m_devices.append(device);
                        Q_EMIT devicesChanged();
                    } else {
                        qCWarning(LOG_KIOD_KMTPD) << "LIBMTP_Open_Raw_Device_Uncached: Could not open MTP device";
                    }
                }
            }
        }
        break;
        case LIBMTP_ERROR_GENERAL:
        default:
            qCWarning(LOG_KIOD_KMTPD) << "Unknown connection error";
            break;
        }
        free(rawdevices);
    }
}

MTPDevice *KMTPd::deviceFromUdi(const QString &udi) const
{
    auto deviceIt = std::find_if(m_devices.constBegin(), m_devices.constEnd(), [udi] (const MTPDevice *device) {
        return device->udi() == udi;
    });

    return deviceIt == m_devices.constEnd() ? nullptr : *deviceIt;
}

QList<QDBusObjectPath> KMTPd::listDevices() const
{
    QList<QDBusObjectPath> list;
    for (const auto &device : m_devices) {
        list.append(QDBusObjectPath(device->dbusObjectName()));
    }

    return list;
}

void KMTPd::deviceAdded(const QString &udi)
{
    qCDebug(LOG_KIOD_KMTPD) << "New device attached with udi=" << udi << ". Checking if PortableMediaPlayer...";

    const Solid::Device device(udi);
    if (device.isDeviceInterface(Solid::DeviceInterface::PortableMediaPlayer)) {
        qCDebug(LOG_KIOD_KMTPD) << "SOLID: New Device with udi=" << udi;

        checkDevice(device);
    }
}

void KMTPd::deviceRemoved(const QString &udi)
{
    MTPDevice *device = deviceFromUdi(udi);
    if (device) {
        qCDebug(LOG_KIOD_KMTPD) << "SOLID: Device with udi=" << udi << " removed.";

        m_devices.removeOne(device);
        delete device;
        Q_EMIT devicesChanged();
    }
}

#include "kmtpd.moc"
