/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORKDBUSADAPTOR_H
#define NETWORKDBUSADAPTOR_H

// kded
#include "networkwatcher.h"
//
#include <networkdbus.h>
// Qt
#include <QObject>
#include <QDBusAbstractAdaptor>


namespace Mollet
{

// TODO: see file networkdbus.h
class NetworkDBusAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.network")

public:
    explicit NetworkDBusAdaptor( NetworkWatcher* parent );
    ~NetworkDBusAdaptor() override;

public:
    NetworkWatcher* parent() const;

public Q_SLOTS:
    Mollet::NetDevice deviceData( const QString& hostAddress );
    Mollet::NetService serviceData( const QString& hostAddress, const QString& serviceName, const QString& serviceType );
    Mollet::NetDeviceList deviceDataList();
    Mollet::NetServiceList serviceDataList( const QString& hostAddress );
};


inline NetworkWatcher* NetworkDBusAdaptor::parent() const {
    return static_cast<NetworkWatcher*>( QObject::parent() );
}

}

#endif
