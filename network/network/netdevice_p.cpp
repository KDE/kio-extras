/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "netdevice_p.h"

// library
#include "netservice_p.h"
// Qt
#include <QMutableListIterator>


namespace Mollet
{

NetDevicePrivate::NetDevicePrivate( const QString& name )
    : mName( name )
    , mType( NetDevice::Unknown )
{
}

bool NetDevicePrivate::hasService( const QString& id ) const
{
    bool result = false;

    for (const NetService& service : mServiceList) {
        const NetServicePrivate* const d = service.dPtr();
        if( d->id() == id )
        {
            result = true;
            break;
        }
    }

    return result;
}

NetService NetDevicePrivate::removeService( const QString& id )
{
    NetService result;
    QMutableListIterator<NetService> it( mServiceList );
    while( it.hasNext())
    {
        const NetService& service = it.next();
        const NetServicePrivate* const d = service.dPtr();
        if( d->id() == id )
        {
            result = service;
            it.remove();
            break;
        }
    }
    return result;
}

NetDevicePrivate::~NetDevicePrivate()
{
}

}
