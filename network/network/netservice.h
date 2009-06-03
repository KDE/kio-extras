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

#ifndef NETSERVICE_H
#define NETSERVICE_H

// lib
#include "molletnetwork_export.h"
// KDE
#include <KSharedPtr>

namespace Mollet {
class NetDevice;
class NetService;
}
template < class T > class QList;
class QString;
class QDBusArgument;

extern MOLLETNETWORK_EXPORT QDBusArgument& operator<<( QDBusArgument& argument, const Mollet::NetService& service );
extern MOLLETNETWORK_EXPORT const QDBusArgument& operator>>( const QDBusArgument& argument, Mollet::NetService& service );


namespace Mollet
{

class NetServicePrivate;


class MOLLETNETWORK_EXPORT NetService
{
    friend class DNSSDNetworkBuilder;
    friend QDBusArgument& ::operator<<( QDBusArgument& argument, const NetService& service );
    friend const QDBusArgument& ::operator>>( const QDBusArgument& argument, NetService& service );

  public:
    NetService();
    NetService( const NetService& other );
    virtual ~NetService();

  public:
    QString name() const;
    QString iconName() const;
    QString type() const;
    NetDevice device() const;
    bool isValid() const;

    // TODO: not sure all services come down to one url
    QString url() const;

  public:
    NetService& operator =( const NetService& other );

  private:
    NetService( NetServicePrivate* _d );
    void setDPtr( NetServicePrivate* _d );
    NetServicePrivate* dPtr() const;

  private:
    KSharedPtr<NetServicePrivate> d;
};

typedef QList<NetService> NetServiceList;


inline  NetServicePrivate* NetService::dPtr() const { return const_cast<NetServicePrivate*>( d.data() ); }

}

#endif
