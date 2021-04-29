/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "netdevice.h"
#include "netdevice_p.h"

namespace Mollet
{

Q_GLOBAL_STATIC_WITH_ARGS(QSharedPointer<NetDevicePrivate>, dummyNetDevicePrivate, ( new NetDevicePrivate(QString()) ))


QString NetDevice::iconName( Type type )
{
    static const char* const IconName[] =
    {
        /*"unknown"*/"network-server", "scanner", "printer", "network-server-database", "network-server", "computer"
    };

    return QLatin1String(IconName[type]);
}

NetDevice::NetDevice()
    : d( *dummyNetDevicePrivate )
{
}

NetDevice::NetDevice( NetDevicePrivate* _d )
    : d( _d )
{
}

NetDevice::NetDevice( const NetDevice& other )
    : d( other.d )
{
}

QString NetDevice::name() const     {
    return d->name();
}
QString NetDevice::hostName() const {
    return d->hostName();
}
QString NetDevice::ipAddress() const {
    return d->ipAddress();
}
QString NetDevice::hostAddress() const {
    return d->hostAddress();
}
NetDevice::Type NetDevice::type() const  {
    return d->type();
}
QList<NetService> NetDevice::serviceList() const {
    return d->serviceList();
}


NetDevice& NetDevice::operator =( const NetDevice& other )
{
    d = other.d;
    return *this;
}

void NetDevice::setDPtr( NetDevicePrivate* _d )
{
    d.reset(_d);
}

NetDevice::~NetDevice()
{
}

}
