/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
