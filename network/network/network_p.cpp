/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
