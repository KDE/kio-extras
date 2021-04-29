/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DNSSDNETSYSTEMABLE_H
#define DNSSDNETSYSTEMABLE_H

// KDE
#include <dnssd/remoteservice.h>
// Qt
#include <QtPlugin>

namespace Mollet {
class NetServicePrivate;
class NetDevice;
}
class QString;


namespace Mollet
{

class DNSSDNetSystemAble
{
public:
    virtual ~DNSSDNetSystemAble();

public: // API to be implemented
    virtual bool canCreateNetSystemFromDNSSD( const QString& serviceType ) const = 0;
    virtual NetServicePrivate* createNetService( const KDNSSD::RemoteService::Ptr& service, const NetDevice& device ) const = 0;
    virtual QString dnssdId( const KDNSSD::RemoteService::Ptr& dnssdService ) const = 0;
};


inline DNSSDNetSystemAble::~DNSSDNetSystemAble() {}

}

Q_DECLARE_INTERFACE( Mollet::DNSSDNetSystemAble, "org.kde.mollet.dnssdnetsystemable/1.0" )

#endif
