/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "networkwatcher.h"

// module
#include "networkdbusadaptor.h"
#include "kioslavenotifier.h"
// network
#include <network.h>
// KF
#include <KPluginFactory>
#include <KPluginLoader>
// Qt
#include <QDBusConnection>

namespace Mollet
{

K_PLUGIN_CLASS_WITH_JSON(NetworkWatcher, "networkwatcher.json")

NetworkWatcher::NetworkWatcher( QObject* parent, const QList<QVariant>& parameters )
    : KDEDModule( parent )
{
    Q_UNUSED( parameters )
    mNetwork = Network::network();

    new KioSlaveNotifier( mNetwork );

    new NetworkDBusAdaptor( this );
    QDBusConnection::sessionBus().registerService( QString::fromLatin1("org.kde.kded5") );
    QDBusConnection::sessionBus().registerObject( QString::fromLatin1("/modules/networkwatcher"), this );
}

// TODO: instead use networkuri and return QVariant for all these
NetDevice NetworkWatcher::deviceData( const QString& hostAddress )
{
    NetDevice result;

    const QList<NetDevice> deviceList = mNetwork->deviceList();
    for (const NetDevice& device : deviceList) {
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
    for (const NetDevice& device : deviceList) {
        if( device.hostAddress() == hostAddress )
        {
            const QList<NetService> serviceList = device.serviceList();

            for (const NetService& service : serviceList) {
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
    for (const NetDevice& device : deviceList) {
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

#include "networkwatcher.moc"
