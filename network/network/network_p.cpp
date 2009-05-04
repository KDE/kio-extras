/*
    This file is part of the solid network library, part of the KDE project.

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

//
#include <config-slp.h>
// lib
#include "builder/dnssdnetworkbuilder.h"
#include "builder/simpleitemfactory.h"
#ifdef HAVE_SLP
#include "builder/slpnetworkbuilder.h"
#endif

#include <KDebug>

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
#ifdef HAVE_SLP
//     mSlpBuilder = new SlpNetworkBuilder( this );
#endif
    mNetworkBuilderList.append( dnssdBuilder );
    mNoOfInitBuilders = mNetworkBuilderList.count();

    foreach( AbstractNetworkBuilder* builder, mNetworkBuilderList )
    {
        foreach( AbstractNetSystemFactory* factory, mNetSystemFactoryList )
            builder->registerNetSystemFactory( factory );
        p->connect( builder, SIGNAL(initDone()), SLOT(onBuilderInit()) );
        builder->start();
    }
}

void NetworkPrivate::onBuilderInit()
{
    --mNoOfInitBuilders;
kDebug()<<mNoOfInitBuilders;
    if( mNoOfInitBuilders == 0 )
        emit p->initDone();
}

NetworkPrivate::~NetworkPrivate()
{
    qDeleteAll( mNetworkBuilderList );
    qDeleteAll( mNetSystemFactoryList );
}

}
