/*
    This file is part of the network kioslave, part of the KDE project.

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

#include "networkwatcher.h"

// module
#include "networkdbusadaptor.h"
#include "kioslavenotifier.h"
// network
#include <network.h>


namespace Mollet
{

NetworkWatcher::NetworkWatcher( QObject* parent, const QList<QVariant>& parameters )
  : KDEDModule( parent )
{
Q_UNUSED( parameters )
    mNetwork = Network::network();

    new KioSlaveNotifier( mNetwork );

    new NetworkDBusAdaptor( this );
    QDBusConnection::sessionBus().registerService( QString::fromLatin1("org.kde.kded") );
    QDBusConnection::sessionBus().registerObject( QString::fromLatin1("/modules/networkwatcher"), this );
}

// TODO: instead use networkuri and return QVariant for all these
NetDevice NetworkWatcher::deviceData( const QString& hostAddress )
{
    NetDevice result;

    const QList<NetDevice> deviceList = mNetwork->deviceList();
    foreach( const NetDevice& device, deviceList )
    {
        if( device.hostAddress() == hostAddress )
        {
            result = device;
            break;
        }
    }

    return result;
}

NetService NetworkWatcher::serviceData( const QString& hostAddress, const QString& serviceName, const QString& serviceType )
{
    NetService result;

    const QList<NetDevice> deviceList = mNetwork->deviceList();
    foreach( const NetDevice& device, deviceList )
    {
        if( device.hostAddress() == hostAddress )
        {
            const QList<NetService> serviceList = device.serviceList();

            foreach( const NetService& service, serviceList )
            {
                if( service.name() == serviceName && service.type() == serviceType )
                {
                    result = service;
                    break;
                }
            }
            break;
        }
    }

    return result;
}

NetDeviceList NetworkWatcher::deviceDataList()
{
    return mNetwork->deviceList();
}

NetServiceList NetworkWatcher::serviceDataList( const QString& hostAddress )
{
    NetServiceList result;

    const QList<NetDevice> deviceList = mNetwork->deviceList();
    foreach( const NetDevice& device, deviceList )
    {
        if( device.hostAddress() == hostAddress )
        {
            result = device.serviceList();
            break;
        }
    }

    return result;
}

NetworkWatcher::~NetworkWatcher()
{
}

}
