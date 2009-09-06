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

#ifndef NETWORK_P_H
#define NETWORK_P_H

// lib
#include "network.h"
#include "netdevice.h"


namespace Mollet
{

class AbstractNetworkBuilder;
class AbstractNetSystemFactory;

class NetworkPrivate
{
  public:
    explicit NetworkPrivate( Network* parent );
    virtual ~NetworkPrivate();

  public:
    const QList<NetDevice>& deviceList() const;
    QList<NetDevice>& deviceList();

  public:
    void init();
    void onBuilderInit();

  public:
    void emitDevicesAdded( const QList<NetDevice>& deviceList );
    void emitDevicesRemoved( const QList<NetDevice>& deviceList );
    void emitServicesAdded( const QList<NetService>& serviceList );
    void emitServicesRemoved( const QList<NetService>& serviceList );

  private: // data
    Network* p;
    QList<NetDevice> mDeviceList;
    QList<AbstractNetworkBuilder*> mNetworkBuilderList;
    QList<AbstractNetSystemFactory*> mNetSystemFactoryList;

    int mNoOfInitBuilders;
};


inline const QList<NetDevice>& NetworkPrivate::deviceList() const { return mDeviceList; }
inline QList<NetDevice>& NetworkPrivate::deviceList() { return mDeviceList; }

inline void NetworkPrivate::emitDevicesAdded( const QList<NetDevice>& deviceList ) { emit p->devicesAdded( deviceList ); }
inline void NetworkPrivate::emitDevicesRemoved( const QList<NetDevice>& deviceList ) { emit p->devicesRemoved( deviceList ); }
inline void NetworkPrivate::emitServicesAdded( const QList<NetService>& serviceList ) { emit p->servicesAdded( serviceList ); }
inline void NetworkPrivate::emitServicesRemoved( const QList<NetService>& serviceList ) { emit p->servicesRemoved( serviceList ); }

}

#endif
