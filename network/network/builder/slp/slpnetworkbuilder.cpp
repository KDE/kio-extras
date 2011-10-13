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

#include "slpnetworkbuilder.h"

// lib
#include "slpservicebrowser.h"
// Qt
#include <QtCore/QList>

#include <KDebug>


namespace Mollet
{

SlpNetworkBuilder::SlpNetworkBuilder( NetworkPrivate* networkPrivate )
  : mNetworkPrivate( networkPrivate )
{
    mSlpServiceBrowser = new SlpServiceBrowser();

    connect( mSlpServiceBrowser, SIGNAL(servicesAdded(QList<SLPService>)),
             SLOT(onServicesAdded(QList<SLPService>)) );
    connect( mSlpServiceBrowser, SIGNAL(servicesChanged(QList<SLPService>)),
             SLOT(onServicesChanged(QList<SLPService>)) );
    connect( mSlpServiceBrowser, SIGNAL(servicesRemoved(QList<SLPService>)),
             SLOT(onServicesRemoved(QList<SLPService>)) );
}


void SlpNetworkBuilder::onServicesAdded( const QList<SLPService>& services )
{
kDebug()<<services.count()<<services[0].serviceUrl();
}

void SlpNetworkBuilder::onServicesChanged( const QList<SLPService>& services )
{
kDebug()<<services.count()<<services[0].serviceUrl();
}

void SlpNetworkBuilder::onServicesRemoved( const QList<SLPService>& services )
{
kDebug()<<services.count()<<services[0].serviceUrl();
}


SlpNetworkBuilder::~SlpNetworkBuilder()
{
    delete mSlpServiceBrowser;
}

}
