/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef SIMPLEITEMFACTORY_H
#define SIMPLEITEMFACTORY_H

// lib
#include <dnssd/dnssdnetsystemable.h>
#include <upnp/upnpnetsystemable.h>
#include <abstractnetsystemfactory.h>


namespace Mollet
{

class SimpleItemFactory : public AbstractNetSystemFactory,
    public DNSSDNetSystemAble,
    public UpnpNetSystemAble
{
    Q_OBJECT
    Q_INTERFACES(
        Mollet::DNSSDNetSystemAble
        Mollet::UpnpNetSystemAble
    )

public:
    SimpleItemFactory();
    ~SimpleItemFactory() override;

public: // DNSSDNetSystemAble API
    bool canCreateNetSystemFromDNSSD( const QString& serviceType ) const override;
    NetServicePrivate* createNetService( const KDNSSD::RemoteService::Ptr& service, const NetDevice& device ) const override;
    QString dnssdId( const KDNSSD::RemoteService::Ptr& dnssdService ) const override;

public: // UpnpNetSystemAble API
    bool canCreateNetSystemFromUpnp( const Cagibi::Device& upnpDevice ) const override;
    NetServicePrivate* createNetService( const Cagibi::Device& upnpDevice, const NetDevice& device ) const override;
    QString upnpId( const Cagibi::Device& upnpDevice ) const override;

private:
};

}

#endif
