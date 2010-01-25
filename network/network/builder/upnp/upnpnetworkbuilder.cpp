/*
    This file is part of the Mollet network library, part of the KDE project.

    Copyright 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

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
// KUPnP
#include <devicebrowser.h>
#include <device.h>
// Qt
#include <QtCore/QStringList>

#include <KDebug>

namespace Mollet
{

UpnpNetworkBuilder::UpnpNetworkBuilder( NetworkPrivate* networkPrivate )
  : AbstractNetworkBuilder()
  , mNetworkPrivate( networkPrivate )
  , mDeviceBrowser( 0 )
{
kDebug();
}

void UpnpNetworkBuilder::registerNetSystemFactory( AbstractNetSystemFactory* netSystemFactory )
{
    UpnpNetSystemAble* upnpNetSystemAble = qobject_cast<UpnpNetSystemAble*>( netSystemFactory );

    if( upnpNetSystemAble )
        mNetSystemFactoryList.append( upnpNetSystemAble );
}

void UpnpNetworkBuilder::start()
{
    mDeviceBrowser = new UPnP::DeviceBrowser();

    connect( mDeviceBrowser, SIGNAL(deviceAdded( const UPnP::Device& )), SLOT(onUPnPDeviceAdded( const UPnP::Device& )) );
    connect( mDeviceBrowser, SIGNAL(deviceRemoved( const UPnP::Device& )), SLOT(onUPnPDeviceRemoved( const UPnP::Device& )) );

    const QList<UPnP::Device> devices = mDeviceBrowser->devices();

    addUPnPDevices( devices );

    // TODO: works already here, but is this a good design?
    emit initDone();
}

void UpnpNetworkBuilder::addUPnPDevices( const QList<UPnP::Device>& upnpDevices )
{
    QList<NetDevice> addedDevices;
    QList<NetService> addedServices;

    QList<NetDevice>& deviceList = mNetworkPrivate->deviceList();
    foreach( const UPnP::Device& upnpDevice, upnpDevices )
    {
        if( upnpDevice.hasParentDevice() )
            continue;

        const QString ipAddress = upnpDevice.ipAddress();

        NetDevicePrivate* d = 0;
        const NetDevice* deviceOfService;
        foreach( const NetDevice& device, deviceList )
        {
        const bool isSameAddress = ( device.ipAddress() == ipAddress );
kDebug()<<"existing device:"<<device.hostName()<<"at"<<device.ipAddress()<<"vs."<<ipAddress<<":"<<isSameAddress;
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
            const QString displayName = upnpDevice.displayName();

            const QString deviceName = displayName;
            d = new NetDevicePrivate( deviceName );
            d->setIpAddress( ipAddress );

            NetDevice device( d );
            addedDevices.append( device );
            deviceList.append( device );
            deviceOfService = &deviceList.last();
kDebug()<<"new device:"<<deviceName<<"at"<<ipAddress;
        }

        NetServicePrivate* netServicePrivate = 0;
        // do a priority based lookup who can build the object
        // TODO: priorisation
        foreach( const UpnpNetSystemAble* factory, mNetSystemFactoryList )
        {
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
kDebug()<<"new service:"<<netService.name()<<netService.url();

        // try guessing the device type by the services on it
        // TODO: move into  devicefactory
        const QString serviceType = upnpDevice.type();
        NetDevice::Type deviceTypeByService = NetDevice::Unknown;
        QString deviceName;
        if( serviceType == "InternetGatewayDevice1" )
            deviceTypeByService = NetDevice::Router;

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


void UpnpNetworkBuilder::removeUPnPDevices( const QList<UPnP::Device>& upnpDevices )
{
    QList<NetDevice> removedDevices;
    QList<NetService> removedServices;

    QList<NetDevice>& deviceList = mNetworkPrivate->deviceList();
    foreach( const UPnP::Device& upnpDevice, upnpDevices )
    {
        const QString ipAddress = upnpDevice.ipAddress();

        QMutableListIterator<NetDevice> it( deviceList );
        while( it.hasNext())
        {
            const NetDevice& device = it.next();
            if( device.ipAddress() == ipAddress )
            {
                NetDevicePrivate* d = device.dPtr();
                NetService netService = d->removeService( upnpDevice.displayName() );
                if( ! netService.isValid() )
                    break;

                removedServices.append( netService );

                // remove device on last service
                if( d->serviceList().count() == 0 )
                {
                    QList<NetDevice> removedDevices;
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


void UpnpNetworkBuilder::onUPnPDeviceAdded( const UPnP::Device& device )
{
    QList<UPnP::Device> devices;
    devices.append( device );
    addUPnPDevices( devices );
}

void UpnpNetworkBuilder::onUPnPDeviceRemoved( const UPnP::Device& device )
{
    QList<UPnP::Device> devices;
    devices.append( device );
    removeUPnPDevices( devices );
}

UpnpNetworkBuilder::~UpnpNetworkBuilder()
{
}

}
