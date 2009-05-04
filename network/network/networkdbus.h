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

#ifndef NETWORKDBUS_H
#define NETWORKDBUS_H

// lib
#include "molletnetwork_export.h"
#include "netdevice.h"
#include "netservice.h"
// Qt
#include <QtCore/QMetaType>


// TODO: copying single items out of an interlinked structure does not work
// classical problem with different address spaces.
// instead just the item data is transferred for now. Perhaps this should become another data structure
// which uses ids as reference to the linked data. Needs more use cases to get a better picture.
// also needed: xml description for introspection and automagical creation of adaptor and interface


MOLLETNETWORK_EXPORT QDBusArgument& operator<<( QDBusArgument& argument, const Mollet::NetDevice& device );
MOLLETNETWORK_EXPORT const QDBusArgument& operator>>( const QDBusArgument& argument, Mollet::NetDevice& device );

MOLLETNETWORK_EXPORT QDBusArgument& operator<<( QDBusArgument& argument, const Mollet::NetService& service );
MOLLETNETWORK_EXPORT const QDBusArgument& operator>>( const QDBusArgument& argument, Mollet::NetService& service );

Q_DECLARE_METATYPE(Mollet::NetDevice)
Q_DECLARE_METATYPE(Mollet::NetService)
Q_DECLARE_METATYPE(Mollet::NetDeviceList)
Q_DECLARE_METATYPE(Mollet::NetServiceList)

#endif
