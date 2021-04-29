/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DNSSDNETWORKBUILDER_H
#define DNSSDNETWORKBUILDER_H

// lib
#include "abstractnetworkbuilder.h"
#include "network.h"
#include "netdevice.h"
// KDE
#include <dnssd/remoteservice.h>
#include <DNSSD/ServiceTypeBrowser>
#include <DNSSD/ServiceBrowser>
// Qt
#include <QHash>

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
    ~DNSSDNetworkBuilder() override;

public: // AbstractNetworkBuilder API
    void registerNetSystemFactory( AbstractNetSystemFactory* netSystemFactory ) override;
    void start() override;

private Q_SLOTS:
    void addServiceType( const QString& serviceType );
    void addService( KDNSSD::RemoteService::Ptr service );
    void removeServiceType( const QString& serviceType );
    void removeService( KDNSSD::RemoteService::Ptr service );

    void onServiceTypeBrowserFinished();
    void onServiceBrowserFinished();

private: // data
    NetworkPrivate* mNetworkPrivate;

    KDNSSD::ServiceTypeBrowser* mServiceTypeBrowser;
    QHash<QString,KDNSSD::ServiceBrowser*> mServiceBrowserTable;

    QList<DNSSDNetSystemAble*> mNetSystemFactoryList;

    bool mIsInit;
    int mNoOfInitServiceTypes;
};

}

#endif
