/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "network.h"
#include "network_p.h"

// Qt
#include <QGlobalStatic>
#include <QList>


namespace Mollet
{

Q_GLOBAL_STATIC( Network, networkSingleton )

Network* Network::network()
{
    return networkSingleton;
}

Network::Network()
    : d( new NetworkPrivate(this) )
{
    d->init();
}

QList<NetDevice> Network::deviceList() const {
    return d->deviceList();
}

Network::~Network()
{
}

}

#include "moc_network.cpp"
