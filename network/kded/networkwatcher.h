/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORKWATCHER_H
#define NETWORKWATCHER_H

// KDE
#include <KDEDModule>

namespace Mollet
{
class Network;
class NetDevice;
class NetService;
typedef QList<NetDevice> NetDeviceList;
typedef QList<NetService> NetServiceList;


class NetworkWatcher : public KDEDModule
{
    Q_OBJECT

public:
    NetworkWatcher( QObject* parent, const QList<QVariant>& parameters );
    ~NetworkWatcher() override;

public:
    Mollet::NetDevice deviceData( const QString& hostAddress );
    Mollet::NetService serviceData( const QString& hostAddress, const QString& serviceName, const QString& serviceType );
    Mollet::NetDeviceList deviceDataList();
    Mollet::NetServiceList serviceDataList( const QString& hostAddress );

private:
    Network* mNetwork;
};

}

#endif
