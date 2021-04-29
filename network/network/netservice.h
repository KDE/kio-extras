/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETSERVICE_H
#define NETSERVICE_H

// lib
#include "molletnetwork_export.h"
// Qt
#include <QSharedPointer>

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
    friend class UpnpNetworkBuilder;
    friend class NetDevicePrivate;
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
    explicit NetService( NetServicePrivate* _d );
    void setDPtr( NetServicePrivate* _d );
    NetServicePrivate* dPtr() const;

private:
    QSharedPointer<NetServicePrivate> d;
};

typedef QList<NetService> NetServiceList;


inline  NetServicePrivate* NetService::dPtr() const {
    return const_cast<NetServicePrivate*>( d.data() );
}

}

#endif
