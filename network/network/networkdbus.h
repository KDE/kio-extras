/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORKDBUS_H
#define NETWORKDBUS_H

// lib
#include "molletnetwork_export.h"
#include "netdevice.h"
#include "netservice.h"
// Qt
#include <QMetaType>


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
