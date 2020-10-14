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

#include "network_p.h"

// lib
#include "builder/dnssd/dnssdnetworkbuilder.h"
#include "builder/upnp/upnpnetworkbuilder.h"
#include "builder/simpleitemfactory.h"

#include <QDebug>

namespace Mollet
{

NetworkPrivate::NetworkPrivate( Network* parent )
  : p( parent )
{
}

void NetworkPrivate::init()
{
    SimpleItemFactory* simpleItemFactory = new SimpleItemFactory();
    mNetSystemFactoryList.append( simpleItemFactory );

    DNSSDNetworkBuilder* dnssdBuilder = new DNSSDNetworkBuilder( this );
    UpnpNetworkBuilder* upnpBuilder = new UpnpNetworkBuilder( this );
    mNetworkBuilderList.append( dnssdBuilder );
    mNetworkBuilderList.append( upnpBuilder );
    mNoOfInitBuilders = mNetworkBuilderList.count();

    for (AbstractNetworkBuilder* builder : qAsConst(mNetworkBuilderList)) {
        for (AbstractNetSystemFactory* factory : qAsConst(mNetSystemFactoryList)) {
            builder->registerNetSystemFactory( factory );
        }
        p->connect( builder, SIGNAL(initDone()), SLOT(onBuilderInit()) );
        builder->start();
    }
}

void NetworkPrivate::onBuilderInit()
{
    --mNoOfInitBuilders;
    if( mNoOfInitBuilders == 0 )
        emit p->initDone();
}

NetworkPrivate::~NetworkPrivate()
{
    qDeleteAll( mNetworkBuilderList );
    qDeleteAll( mNetSystemFactoryList );
}

}
