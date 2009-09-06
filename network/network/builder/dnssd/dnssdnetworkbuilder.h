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

#ifndef DNSSDNETWORKBUILDER_H
#define DNSSDNETWORKBUILDER_H

// lib
#include "abstractnetworkbuilder.h"
#include "network.h"
#include "netdevice.h"
// KDE
#include <DNSSD/RemoteService>
// Qt
#include <QtCore/QHash>

namespace Mollet {
class DNSSDNetSystemAble;
}

namespace DNSSD {
class ServiceBrowser;
class ServiceTypeBrowser;
}


namespace Mollet
{

class DNSSDNetworkBuilder : public AbstractNetworkBuilder
{
    Q_OBJECT

  public:
    explicit DNSSDNetworkBuilder( NetworkPrivate* networkPrivate );
    virtual ~DNSSDNetworkBuilder();

  public: // AbstractNetworkBuilder API
    virtual void registerNetSystemFactory( AbstractNetSystemFactory* netSystemFactory );
    virtual void start();

  private Q_SLOTS:
    void addServiceType( const QString& serviceType );
    void addService( DNSSD::RemoteService::Ptr service );
    void removeServiceType( const QString& serviceType );
    void removeService( DNSSD::RemoteService::Ptr service );

    void onServiceTypeBrowserFinished();
    void onServiceBrowserFinished();

  private: // data
    NetworkPrivate* mNetworkPrivate;

    DNSSD::ServiceTypeBrowser* mServiceTypeBrowser;
    QHash<QString,DNSSD::ServiceBrowser*> mServiceBrowserTable;

    QList<DNSSDNetSystemAble*> mNetSystemFactoryList;

    bool mIsInit;
    int mNoOfInitServiceTypes;
};

}

#endif
