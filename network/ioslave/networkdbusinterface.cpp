/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "networkdbusinterface.h"

// Qt
#include <QDBusMetaType>


static const char NetworkDBusInterfaceName[] = "org.kde.network";

NetworkDBusInterface::NetworkDBusInterface( const QString& service, const QString& path, const QDBusConnection& connection, QObject* parent )
    : QDBusAbstractInterface( service, path, NetworkDBusInterfaceName, connection, parent )
{
    // TODO: best place to do this?
    qDBusRegisterMetaType<Mollet::NetDevice>();
    qDBusRegisterMetaType<Mollet::NetService>();
    qDBusRegisterMetaType<Mollet::NetDeviceList>();
    qDBusRegisterMetaType<Mollet::NetServiceList>();
}

NetworkDBusInterface::~NetworkDBusInterface()
{
}

