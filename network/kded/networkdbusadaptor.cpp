/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "networkdbusadaptor.h"

// network
#include <networkdbus.h>
// Qt
#include <QDBusMetaType>


namespace Mollet
{

NetworkDBusAdaptor::NetworkDBusAdaptor( NetworkWatcher* parent )
    : QDBusAbstractAdaptor( parent )
{
    // TODO: best place to do this?
    qDBusRegisterMetaType<Mollet::NetDevice>();
    qDBusRegisterMetaType<Mollet::NetService>();
    qDBusRegisterMetaType<Mollet::NetDeviceList>();
    qDBusRegisterMetaType<Mollet::NetServiceList>();
}

NetDevice NetworkDBusAdaptor::deviceData( const QString& hostAddress )
{
    return parent()->deviceData( hostAddress );
}
NetService NetworkDBusAdaptor::serviceData( const QString& hostAddress, const QString& serviceName, const QString& serviceType )
{
    return parent()->serviceData( hostAddress, serviceName, serviceType );
}
NetDeviceList NetworkDBusAdaptor::deviceDataList()
{
    return parent()->deviceDataList();
}
NetServiceList NetworkDBusAdaptor::serviceDataList( const QString& hostAddress )
{
    return parent()->serviceDataList( hostAddress );
}

NetworkDBusAdaptor::~NetworkDBusAdaptor()
{
}

}
