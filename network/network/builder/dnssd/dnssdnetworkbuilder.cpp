/*
    This file is part of the Mollet network library, part of the KDE project.

    Copyright 2009-2010 Friedrich W. H. Kossebau <kossebau@kde.org>

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

#include "dnssdnetworkbuilder.h"

// lib
#include "dnssdnetsystemable.h"
#include "abstractnetsystemfactory.h"
#include "network_p.h"
#include "netdevice_p.h"
// KDE
#include <DNSSD/ServiceTypeBrowser>
#include <DNSSD/ServiceBrowser>
// Qt
#include <QHostAddress>
#include <QMutableListIterator>

#include <QDebug>


namespace Mollet
{

DNSSDNetworkBuilder::DNSSDNetworkBuilder( NetworkPrivate* networkPrivate )
  : AbstractNetworkBuilder()
  , mNetworkPrivate( networkPrivate )
  , mServiceTypeBrowser( nullptr )
{
}


void DNSSDNetworkBuilder::registerNetSystemFactory( AbstractNetSystemFactory* netSystemFactory )
{
    DNSSDNetSystemAble* dnssdNetSystemAble = qobject_cast<DNSSDNetSystemAble*>( netSystemFactory );

    if( dnssdNetSystemAble )
        mNetSystemFactoryList.append( dnssdNetSystemAble );
}

void DNSSDNetworkBuilder::start()
{
    mIsInit = true;
    mNoOfInitServiceTypes = 0;

    mServiceTypeBrowser = new KDNSSD::ServiceTypeBrowser();
    connect(mServiceTypeBrowser, &KDNSSD::ServiceTypeBrowser::serviceTypeAdded, this, &DNSSDNetworkBuilder::addServiceType);
    connect(mServiceTypeBrowser, &KDNSSD::ServiceTypeBrowser::serviceTypeRemoved, this, &DNSSDNetworkBuilder::removeServiceType);
    connect(mServiceTypeBrowser, &KDNSSD::ServiceTypeBrowser::finished, this, &DNSSDNetworkBuilder::onServiceTypeBrowserFinished);
// TODO: add a signal network initialized to Network, so is cleared when first usable
    mServiceTypeBrowser->startBrowse();
}

void DNSSDNetworkBuilder::addServiceType( const QString& serviceType )
{
//qDebug()<<serviceType<<mServiceBrowserTable.contains(serviceType);
    if( mServiceBrowserTable.contains(serviceType))
        return;

// //qDebug()<<serviceType;
    KDNSSD::ServiceBrowser* serviceBrowser = new KDNSSD::ServiceBrowser( serviceType, true );
    connect(serviceBrowser, &KDNSSD::ServiceBrowser::serviceAdded, this, &DNSSDNetworkBuilder::addService);
    connect(serviceBrowser, &KDNSSD::ServiceBrowser::serviceRemoved, this, &DNSSDNetworkBuilder::removeService);

    if( mIsInit )
    {
        ++mNoOfInitServiceTypes;
        connect(serviceBrowser, &KDNSSD::ServiceBrowser::finished, this, &DNSSDNetworkBuilder::onServiceBrowserFinished);
    }

    mServiceBrowserTable[serviceType] = serviceBrowser;
    serviceBrowser->startBrowse();
}

void DNSSDNetworkBuilder::removeServiceType( const QString& serviceType )
{
//qDebug()<<serviceType<<mServiceBrowserTable.contains(serviceType);
    // for now we keep the service browser (aren't that many) because otherwise
    // the serviceRemoved calls won't reach us.
    // we could also go through all the devices and remove the services manually as a fix
    return;
}

void DNSSDNetworkBuilder::addService( KDNSSD::RemoteService::Ptr service )
{
    QList<NetDevice>& deviceList = mNetworkPrivate->deviceList();

    QString hostName = service->hostName();
    // TODO: this blocks. and the ip address should be delivered from DNS-SD with resolve
    const QHostAddress hostAddress = KDNSSD::ServiceBrowser::resolveHostName( hostName );
    const QString ipAddress = hostAddress.toString();
    // forget domain name if just ip address
    if( hostName == ipAddress )
        hostName.clear();

    // device TODO: only search for if we can create the service?
    NetDevicePrivate* d = nullptr;
    const NetDevice* deviceOfService = nullptr;
    for (const NetDevice& device : qAsConst(deviceList)) {
        const QString deviceHostName = device.hostName();
        const bool useIpAddress = ( deviceHostName.isEmpty() || hostName.isEmpty() );
        const bool isSameAddress = useIpAddress ?
            ( device.ipAddress() == ipAddress ) :
            ( deviceHostName == hostName );
//qDebug()<<"existing device:"<<deviceHostName<<"at"<<device.ipAddress()<<"vs."<<hostName<<"at"<<ipAddress<<":"<<isSameAddress;

        if( isSameAddress )
        {
            d = device.dPtr();
            // workaround: KDNSSD currently (4.7.0) emits two signals per service
            // just relying on service->serviceName() is fragile, but matches
            // current approach in removeService(...)
            QString id;
            const QString serviceType = service->type();
            for (const DNSSDNetSystemAble* factory : qAsConst(mNetSystemFactoryList)) {
                if( factory->canCreateNetSystemFromDNSSD(serviceType) )
                {
                    id = factory->dnssdId( service );
                    break;
                }
            }
            if( d->hasService(id) )
                return;

            deviceOfService = &device;
            break;
        }
    }
    if( !d )
    {
        const QString deviceName = hostName.left( hostName.indexOf(QLatin1Char('.')) );
        d = new NetDevicePrivate( deviceName );
        d->setHostName( hostName );
        d->setIpAddress( ipAddress );
        NetDevice device( d );
        deviceList.append( device );
        deviceOfService = &deviceList.last();

        QList<NetDevice> newDevices;
        newDevices.append( device );
        // TODO: the new service will be announced two times, once with the new device and once alone.
        // what to do about that? which order? okay? for now just do not attach services before. find usecases.
        mNetworkPrivate->emitDevicesAdded( newDevices );
//qDebug()<<"new device:"<<deviceName<<"at"<<hostName<<"by"<<service->type();
    }
    else
    {
        if( d->hostName().isEmpty() && ! hostName.isEmpty() )
            d->setHostName( hostName );
    }


    const QString serviceType = service->type();

    NetServicePrivate* netServicePrivate = nullptr;
    // do a priority based lookup who can build the object
    // TODO: priorisation
    for (const DNSSDNetSystemAble* factory : qAsConst(mNetSystemFactoryList)) {
        if( factory->canCreateNetSystemFromDNSSD(serviceType) )
        {
            // TODO: here we should rather see if this service already exists
            netServicePrivate = factory->createNetService( service, *deviceOfService );
            break;
        }
    }
    // TODO: create dummy service
//     if( ! netServicePrivate )
//         netServicePrivate = new UnknownService;

    NetService netService( netServicePrivate );
    d->addService( netService );

    // try guessing the device type by the services on it
    // TODO: move into  devicefactory
    NetDevice::Type deviceTypeByService = NetDevice::Unknown;
    QString deviceName;
    if( serviceType == QLatin1String("_workstation._tcp") )
    {
        deviceTypeByService = NetDevice::Workstation;
        deviceName = service->serviceName().left( service->serviceName().lastIndexOf(QLatin1Char('[')) ).trimmed();
    }
    else if( serviceType == QLatin1String("_net-assistant._udp") )
    {
        deviceTypeByService = NetDevice::Workstation;
        deviceName = service->serviceName();
    }
    else if( serviceType == QLatin1String("_airport._tcp") )
        deviceTypeByService = NetDevice::Router;
    else if( serviceType == QLatin1String("_ipp._tcp")
             || serviceType == QLatin1String("_printer._tcp")
             || serviceType == QLatin1String("_pdl-datastream._tcp") )
    {
        deviceTypeByService = NetDevice::Printer;
        deviceName = service->serviceName();
    }

    if( deviceTypeByService != NetDevice::Unknown )
    {
        if( deviceTypeByService > d->type() )
        {
            d->setType( deviceTypeByService );
            if( ! deviceName.isEmpty() )
                d->setName( deviceName );
        }
    }

    QList<NetService> newServices;
    newServices.append( netService );
    mNetworkPrivate->emitServicesAdded( newServices );
}

// TODO: each builder should refcount its services, so a shared one does not get removed to early
// (might disappear for one sd because of master breakdown)
void DNSSDNetworkBuilder::removeService( KDNSSD::RemoteService::Ptr service )
{
    QList<NetDevice>& deviceList = mNetworkPrivate->deviceList();

    const QString hostName = service->hostName();

    // device
    QMutableListIterator<NetDevice> it( deviceList );
    while( it.hasNext())
    {
        const NetDevice& device = it.next();
        if( device.hostName() == hostName )
        {
// //qDebug()<<hostName;
            QString id;
            const QString serviceType = service->type();
            for (const DNSSDNetSystemAble* factory : qAsConst(mNetSystemFactoryList)) {
                if( factory->canCreateNetSystemFromDNSSD(serviceType) )
                {
                    id = factory->dnssdId( service );
                    break;
                }
            }
            NetDevicePrivate* d = device.dPtr();
            NetService netService = d->removeService( id );
            if( !netService.isValid() )
                break;

            QList<NetService> removedServices;
            removedServices.append( netService );
            mNetworkPrivate->emitServicesRemoved( removedServices );

            // remove device on last service
            if( d->serviceList().count() == 0 )
            {
                QList<NetDevice> removedDevices;
                removedDevices.append( device );
                // remove only after taking copy from reference into removed list
                it.remove();

                mNetworkPrivate->emitDevicesRemoved( removedDevices );
            }
            break;
        }
    }
}

void DNSSDNetworkBuilder::onServiceTypeBrowserFinished()
{
// //qDebug();
    if( mIsInit )
    {
        mIsInit = false;
        if( mNoOfInitServiceTypes == 0 )
            emit initDone();
    }
}

void DNSSDNetworkBuilder::onServiceBrowserFinished()
{
    --mNoOfInitServiceTypes;
// //qDebug()<<"mIsInit="<<mIsInit<<"mNoOfInitServiceTypes="<<mNoOfInitServiceTypes;
    // only check for countdown after end of new service types
    if( !mIsInit && mNoOfInitServiceTypes == 0 )
        emit initDone();
}

DNSSDNetworkBuilder::~DNSSDNetworkBuilder()
{
    qDeleteAll(mServiceBrowserTable);
    delete mServiceTypeBrowser;
}

}
