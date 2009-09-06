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

#include "netservice.h"
#include "netservice_p.h"

// lib
#include "netdevice.h"
// KDE
#include <KGlobal>


namespace Mollet
{

// 
K_GLOBAL_STATIC_WITH_ARGS(KSharedPtr< NetServicePrivate >,
    defaultEmptyNetServicePrivate,
    ( new NetServicePrivate(QString(),QString(),QString(),NetDevice(),QString()) ))


NetService::NetService()
  : d( *defaultEmptyNetServicePrivate )
{
}

NetService::NetService( NetServicePrivate* _d )
  : d( _d )
{
}

NetService::NetService( const NetService& other )
  : d( other.d )
{
}

QString NetService::name() const { return d->name(); }
QString NetService::iconName() const { return d->iconName(); }
QString NetService::type() const { return d->type(); }
NetDevice NetService::device() const { return d->device(); }
bool NetService::isValid() const { return !d->type().isEmpty(); } // was !d.isNull()
QString NetService::url() const { return d->url(); }


NetService& NetService::operator =( const NetService& other )
{
    d = other.d;
    return *this;
}

void NetService::setDPtr( NetServicePrivate* _d )
{
    d = _d;
}

NetService::~NetService()
{
}

}
