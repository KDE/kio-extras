/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETDEVICE_H
#define NETDEVICE_H

// lib
#include "molletnetwork_export.h"
// Qt
#include <QSharedPointer>

namespace Mollet {
class NetService;
class NetDevice;
}
template < class T > class QList;
class QString;
class QDBusArgument;

extern MOLLETNETWORK_EXPORT QDBusArgument& operator<<( QDBusArgument& argument, const Mollet::NetDevice& device );
extern MOLLETNETWORK_EXPORT const QDBusArgument& operator>>( const QDBusArgument& argument, Mollet::NetDevice& device );


namespace Mollet
{

class NetDevicePrivate;


class MOLLETNETWORK_EXPORT NetDevice
{
    friend class DNSSDNetworkBuilder;
    friend class UpnpNetworkBuilder;
    friend QDBusArgument& ::operator<<( QDBusArgument& argument, const NetDevice& device );
    friend const QDBusArgument& ::operator>>( const QDBusArgument& argument, NetDevice& device );

public:
    // later has priority
    enum Type { Unknown = 0, Scanner, Printer, FileServer, Router, Workstation };
    static QString iconName( Type type );

public:
    NetDevice();
    NetDevice( const NetDevice& other );
    virtual ~NetDevice();

public:
    QString name() const;
    QString hostName() const;
    /// if hostName is not set, use ipAddress to identify device
    QString ipAddress() const;
    /// returns hostName if set, otherwise ipAddress TODO: find better name
    QString hostAddress() const;
    Type type() const;
    QList<NetService> serviceList() const;

public:
    NetDevice& operator =( const NetDevice& other );

private:
    explicit NetDevice( NetDevicePrivate* _d );
    void setDPtr( NetDevicePrivate* _d );
    NetDevicePrivate* dPtr() const;

private:
    QSharedPointer<NetDevicePrivate> d;
};

typedef QList<NetDevice> NetDeviceList;


inline  NetDevicePrivate* NetDevice::dPtr() const {
    return const_cast<NetDevicePrivate*>( d.data() );
}

}

#endif
