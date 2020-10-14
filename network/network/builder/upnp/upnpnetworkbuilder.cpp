/*
    This file is part of the Mollet network library, part of the KDE project.

    Copyright 2009-2011 Friedrich W. H. Kossebau <kossebau@kde.org>

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

#include "upnpnetworkbuilder.h"

// lib
#include "upnpnetsystemable.h"
#include "abstractnetsystemfactory.h"
#include "network_p.h"
#include "netdevice_p.h"
#include "netservice_p.h"
#include "cagibidevice.h"
#include "cagibidbuscodec.h"
// Qt
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QStringList>

#include <QDebug>

namespace Mollet
{
static const char cagibiServiceName[] =          "org.kde.Cagibi";
static const char cagibiDeviceListObjectPath[] = "/org/kde/Cagibi/DeviceList";
static const char cagibiDeviceListInterface[] =  "org.kde.Cagibi.DeviceList";

UpnpNetworkBuilder::UpnpNetworkBuilder( NetworkPrivate* networkPrivate )
  : AbstractNetworkBuilder()
  , mNetworkPrivate( networkPrivate )
  , mCagibiDeviceListDBusProxy( nullptr )
{
}

void UpnpNetworkBuilder::registerNetSystemFactory( AbstractNetSystemFactory* netSystemFactory )
{
    UpnpNetSystemAble* upnpNetSystemAble = qobject_cast<UpnpNetSystemAble*>( netSystemFactory );

    if( upnpNetSystemAble )
        mNetSystemFactoryList.append( upnpNetSystemAble );
}

void UpnpNetworkBuilder::start()
{
    QMetaObject::invokeMethod( this, "startBrowse", Qt::QueuedConnection );
}

void UpnpNetworkBuilder::startBrowse()
{
    qDBusRegisterMetaType<DeviceTypeMap>();
    qDBusRegisterMetaType<Cagibi::Device>();

    QDBusConnection dbusConnection = QDBusConnection::systemBus();

    const QString serviceName = QLatin1String( cagibiServiceName );
    const QString deviceListObjectPath =  QLatin1String( cagibiDeviceListObjectPath );
    const QString deviceListInterface =   QLatin1String( cagibiDeviceListInterface );

    // install service watcher
    QDBusServiceWatcher* cagibiServiceWatcher =
        new QDBusServiceWatcher( serviceName,
                                 dbusConnection,
                                 QDBusServiceWatcher::WatchForOwnerChange,
                                 this );
    connect( cagibiServiceWatcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
             SLOT(onCagibiServiceOwnerChanged(QString,QString,QString)) );

    // create devicelist proxy
    mCagibiDeviceListDBusProxy =
        new QDBusInterface( serviceName,
                            deviceListObjectPath,
                            deviceListInterface,
                            dbusConnection, this );
    connect( mCagibiDeviceListDBusProxy, SIGNAL(devicesAdded(DeviceTypeMap)),
             SLOT(onDevicesAdded(DeviceTypeMap)) );
    connect( mCagibiDeviceListDBusProxy, SIGNAL(devicesRemoved(DeviceTypeMap)),
             SLOT(onDevicesRemoved(DeviceTypeMap)) );

    // query current devicelist
    queryCurrentDevices();

    emit initDone();
}

void UpnpNetworkBuilder::queryCurrentDevices()
{
    // query current devicelist
    QDBusPendingCall allDevicesCall =
        mCagibiDeviceListDBusProxy->asyncCall( QLatin1String("allDevices") );

    QDBusPendingCallWatcher* allDevicesCallWatcher =
        new QDBusPendingCallWatcher( allDevicesCall, this );
    connect( allDevicesCallWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
             SLOT(onAllDevicesCallFinished(QDBusPendingCallWatcher*)) );
}

void UpnpNetworkBuilder::onAllDevicesCallFinished( QDBusPendingCallWatcher* allDevicesCallWatcher )
{
    QDBusReply<DeviceTypeMap> reply = *allDevicesCallWatcher;

    if( reply.isValid() )
    {
//qDebug() << "Connected to Cagibi, listing of UPnP devices/services started.";
        const DeviceTypeMap deviceTypeMap = reply;
        onDevicesAdded( deviceTypeMap );
    }
    else
    {
        //qDebug() << "Could not connect to Cagibi, no listing of UPnP devices/services.";
        //qDebug() << "Error: " << reply.error().name();
    }

    delete allDevicesCallWatcher;
}

void UpnpNetworkBuilder::addUPnPDevices( const QList<Cagibi::Device>& upnpDevices )
{
    QList<NetDevice> addedDevices;
    QList<NetService> addedServices;

    QList<NetDevice>& deviceList = mNetworkPrivate->deviceList();
    for (const Cagibi::Device& upnpDevice : upnpDevices) {
        if( upnpDevice.hasParentDevice() )
            continue;

        const QString ipAddress = upnpDevice.ipAddress();

        NetDevicePrivate* d = nullptr;
        const NetDevice* deviceOfService = nullptr;
        for (const NetDevice& device : qAsConst(deviceList)) {
        const bool isSameAddress = ( device.ipAddress() == ipAddress );
//qDebug()<<"existing device:"<<device.hostName()<<"at"<<device.ipAddress()<<"vs."<<ipAddress<<":"<<isSameAddress;
            // TODO: lookup hostname and try to use that
            if( isSameAddress )
            {
                d = device.dPtr();
                deviceOfService = &device;
                break;
            }
        }
        if( ! d )
        {
            const QString displayName = upnpDevice.friendlyName();

            const QString deviceName = displayName;
            d = new NetDevicePrivate( deviceName );
            d->setIpAddress( ipAddress );

            NetDevice device( d );
            addedDevices.append( device );
            deviceList.append( device );
            deviceOfService = &deviceList.last();
//qDebug()<<"new device:"<<deviceName<<"at"<<ipAddress;
        }

        NetServicePrivate* netServicePrivate = nullptr;
        // do a priority based lookup who can build the object
        // TODO: priorisation
        for (const UpnpNetSystemAble* factory : qAsConst(mNetSystemFactoryList)) {
            if( factory->canCreateNetSystemFromUpnp(upnpDevice) )
            {
                // TODO: here we should rather see if this service already exists
                netServicePrivate = factory->createNetService( upnpDevice, *deviceOfService );
                break;
            }
        }

        NetService netService( netServicePrivate );
        d->addService( netService );

        addedServices.append( netService );
//qDebug()<<"new service:"<<netService.name()<<netService.url();

        // try guessing the device type by the services on it
        // TODO: move into  devicefactory
        const QString serviceType = upnpDevice.type();
        NetDevice::Type deviceTypeByService = NetDevice::Unknown;
        QString deviceName;
        if( serviceType == QLatin1String("InternetGatewayDevice1") )
            deviceTypeByService = NetDevice::Router;
        else if( serviceType == QLatin1String("PrinterBasic1")
             || serviceType == QLatin1String("PrinterEnhanced1") )
            deviceTypeByService = NetDevice::Printer;
        else if( serviceType == QLatin1String("Scanner1") )
            deviceTypeByService = NetDevice::Scanner;

        if( deviceTypeByService != NetDevice::Unknown )
        {
            if( deviceTypeByService > d->type() )
            {
                d->setType( deviceTypeByService );
                if( ! deviceName.isEmpty() )
                    d->setName( deviceName );
            }
        }
    }

    if( ! addedDevices.isEmpty() )
        mNetworkPrivate->emitDevicesAdded( addedDevices );
    if( ! addedServices.isEmpty() )
        mNetworkPrivate->emitServicesAdded( addedServices );
}


void UpnpNetworkBuilder::removeUPnPDevices( const QList<Cagibi::Device>& upnpDevices )
{
    QList<NetDevice> removedDevices;
    QList<NetService> removedServices;

    QList<NetDevice>& deviceList = mNetworkPrivate->deviceList();
    for (const Cagibi::Device& upnpDevice : upnpDevices) {
        const QString ipAddress = upnpDevice.ipAddress();

        QMutableListIterator<NetDevice> it( deviceList );
        while( it.hasNext())
        {
            const NetDevice& device = it.next();
            if( device.ipAddress() == ipAddress )
            {
                QString id;
                for (const UpnpNetSystemAble* factory : qAsConst(mNetSystemFactoryList)) {
                    if( factory->canCreateNetSystemFromUpnp(upnpDevice) )
                    {
                        id = factory->upnpId( upnpDevice );
                        break;
                    }
                }
                NetDevicePrivate* d = device.dPtr();
                NetService netService = d->removeService( id );
                if( ! netService.isValid() )
                    break;

                removedServices.append( netService );

                // remove device on last service
                if( d->serviceList().count() == 0 )
                {
                    removedDevices.append( device );
                    // remove only after taking copy from reference into removed list
                    it.remove();
                }
                break;
            }
        }
    }
   if( ! removedServices.isEmpty() )
        mNetworkPrivate->emitServicesRemoved( removedServices );
   if( ! removedDevices.isEmpty() )
        mNetworkPrivate->emitDevicesRemoved( removedDevices );
}


void UpnpNetworkBuilder::onDevicesAdded( const DeviceTypeMap& deviceTypeMap )
{
    DeviceTypeMap::ConstIterator it = deviceTypeMap.constBegin();
    DeviceTypeMap::ConstIterator end = deviceTypeMap.constEnd();
    for( ; it != end; ++it )
    {
        const QString udn = it.key();
        QList<QVariant> args;
        args << udn;
        mCagibiDeviceListDBusProxy->callWithCallback(
            QLatin1String("deviceDetails"), args,
            this, SLOT(onAddedDeviceDetails(Cagibi::Device)), nullptr );
    }
}

void UpnpNetworkBuilder::onDevicesRemoved( const DeviceTypeMap& deviceTypeMap )
{
    QList<Cagibi::Device> upnpDevices;

    DeviceTypeMap::ConstIterator it = deviceTypeMap.constBegin();
    DeviceTypeMap::ConstIterator end = deviceTypeMap.constEnd();
    for( ; it != end; ++it )
    {
        QHash<QString,Cagibi::Device>::Iterator adIt =
            mActiveDevices.find( it.key() );
        if( adIt != mActiveDevices.end() )
        {
//qDebug()<<"removing UPnP device" << adIt.value().friendlyName();
            upnpDevices.append( adIt.value() );
            mActiveDevices.erase( adIt );
        }
    }

    removeUPnPDevices( upnpDevices );
}

void UpnpNetworkBuilder::onAddedDeviceDetails( const Cagibi::Device& device )
{
    // currently only root devices are shown
    if( device.hasParentDevice() )
        return;

    mActiveDevices.insert( device.udn(), device );

    QList<Cagibi::Device> devices;
    devices.append( device );
    addUPnPDevices( devices );
}

void UpnpNetworkBuilder::onCagibiServiceOwnerChanged( const QString& serviceName,
                                                      const QString& oldOwner,
                                                      const QString& newOwner )
{
    Q_UNUSED(serviceName);
    Q_UNUSED(newOwner);

    // old service disappeared?
    if( ! oldOwner.isEmpty() )
    {
//qDebug()<<"Cagibi disappeared, removing all UPnP devices";

        // remove all registered UPnP devices
        QList<Cagibi::Device> upnpDevices = mActiveDevices.values();
        mActiveDevices.clear();

        removeUPnPDevices( upnpDevices );
    }

    if( ! newOwner.isEmpty() )
        queryCurrentDevices();
}

UpnpNetworkBuilder::~UpnpNetworkBuilder()
{
}

}
