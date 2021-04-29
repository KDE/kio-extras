/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef UPNPNETSYSTEMABLE_H
#define UPNPNETSYSTEMABLE_H

// Qt
#include <QtPlugin>

namespace Cagibi {
class Device;
}
class QString;


namespace Mollet
{
class NetServicePrivate;
class NetDevice;


class UpnpNetSystemAble
{
public:
    virtual ~UpnpNetSystemAble();

public: // API to be implemented
    virtual bool canCreateNetSystemFromUpnp( const Cagibi::Device& upnpDevice ) const = 0;
    virtual NetServicePrivate* createNetService( const Cagibi::Device& upnpDevice, const NetDevice& device ) const = 0;
    virtual QString upnpId( const Cagibi::Device& upnpDevice ) const = 0;
};


inline UpnpNetSystemAble::~UpnpNetSystemAble() {}

}

Q_DECLARE_INTERFACE( Mollet::UpnpNetSystemAble, "org.kde.mollet.upnpnetsystemable/1.0" )

#endif
