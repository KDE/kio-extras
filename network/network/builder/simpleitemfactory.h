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
