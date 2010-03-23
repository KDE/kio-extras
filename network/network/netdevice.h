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

#ifndef NETDEVICE_H
#define NETDEVICE_H

// lib
#include "molletnetwork_export.h"
// KDE
#include <KSharedPtr>

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
    NetDevice( NetDevicePrivate* _d );
    void setDPtr( NetDevicePrivate* _d );
    NetDevicePrivate* dPtr() const;

  private:
    KSharedPtr<NetDevicePrivate> d;
};

typedef QList<NetDevice> NetDeviceList;


inline  NetDevicePrivate* NetDevice::dPtr() const { return const_cast<NetDevicePrivate*>( d.data() ); }

}

#endif
