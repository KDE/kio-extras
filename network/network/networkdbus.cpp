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

#include "networkdbus.h"

// network
#include "netdevice_p.h"
#include "netservice_p.h"
// Qt
#include <QtDBus/QDBusArgument>


// TODO: Attention, we currently do not stream any references to other items, just the direct data!

QDBusArgument& operator<<( QDBusArgument& argument, const Mollet::NetDevice& device )
{
    Mollet::NetDevicePrivate* devicePrivate = device.dPtr();

    argument.beginStructure();
    argument << devicePrivate->name();
    argument << devicePrivate->hostName();
    argument << (int) devicePrivate->type();
    argument.endStructure();

    return argument;
}
const QDBusArgument& operator>>( const QDBusArgument& argument, Mollet::NetDevice& device )
{
    QString name;
    QString hostName;
    int type;

    argument.beginStructure();
    argument >> name;
    argument >> hostName;
    argument >> type;
    argument.endStructure();

    Mollet::NetDevicePrivate* d = new Mollet::NetDevicePrivate( name, hostName );
    d->setType( (Mollet::NetDevice::Type)type );

    device.setDPtr( d );

    return argument;
}

QDBusArgument& operator<<( QDBusArgument& argument, const Mollet::NetService& service )
{
    Mollet::NetServicePrivate* servicePrivate = service.dPtr();

    argument.beginStructure();
    argument << servicePrivate->name();
    argument << servicePrivate->iconName();
    argument << servicePrivate->type();
    argument << servicePrivate->url();
    argument.endStructure();

    return argument;
}
const QDBusArgument& operator>>( const QDBusArgument& argument, Mollet::NetService& service )
{
    QString name;
    QString iconName;
    QString type;
    QString url;

    argument.beginStructure();
    argument >> name;
    argument >> iconName;
    argument >> type;
    argument >> url;
    argument.endStructure();

    Mollet::NetServicePrivate* d = new Mollet::NetServicePrivate( name, iconName, type, Mollet::NetDevice(), url );

    service.setDPtr( d );

    return argument;
}
