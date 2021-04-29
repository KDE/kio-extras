/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "netservice.h"
#include "netservice_p.h"

// lib
#include "netdevice.h"
// Qt
#include <QGlobalStatic>


namespace Mollet
{

// 
Q_GLOBAL_STATIC_WITH_ARGS(QSharedPointer<NetServicePrivate>,
    defaultEmptyNetServicePrivate,
    ( new NetServicePrivate(QString(),QString(),QString(),NetDevice(),QString(),QString()) ))


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
    d.reset(_d);
}

NetService::~NetService()
{
}

}
