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

#include "netdevice.h"
#include "netdevice_p.h"

// KDE
#include <KGlobal>

namespace Mollet
{

K_GLOBAL_STATIC_WITH_ARGS(KSharedPtr< NetDevicePrivate >, dummyNetDevicePrivate, ( new NetDevicePrivate(QString()) ))


QString NetDevice::iconName( Type type )
{
    static const char* const IconName[] =
    {
    /*"unknown"*/"network-server", "scanner", "printer", "network-server-database", "network-server", "computer"
    };

    return IconName[type];
}

NetDevice::NetDevice()
  : d( *dummyNetDevicePrivate )
{
}

NetDevice::NetDevice( NetDevicePrivate* _d )
  : d( _d )
{
}

NetDevice::NetDevice( const NetDevice& other )
  : d( other.d )
{
}

QString NetDevice::name() const     { return d->name(); }
QString NetDevice::hostName() const { return d->hostName(); }
QString NetDevice::ipAddress() const { return d->ipAddress(); }
QString NetDevice::hostAddress() const { return d->hostAddress(); }
NetDevice::Type NetDevice::type() const  { return d->type(); }
QList<NetService> NetDevice::serviceList() const { return d->serviceList(); }


NetDevice& NetDevice::operator =( const NetDevice& other )
{
    d = other.d;
    return *this;
}

void NetDevice::setDPtr( NetDevicePrivate* _d )
{
    d = _d;
}

NetDevice::~NetDevice()
{
}

}
