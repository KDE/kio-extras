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

#ifndef NETDEVICE_P_H
#define NETDEVICE_P_H

// lib
#include "netdevice.h"
#include "netservice.h"
// Qt
#include <QtCore/QSharedData>
#include <QtCore/QString>
#include <QtCore/QList>


namespace Mollet
{

class NetDevicePrivate : public QSharedData
{
  public:
    explicit NetDevicePrivate( const QString& name );
    virtual ~NetDevicePrivate();

  public:
    const QString& name() const;
    const QString& hostName() const;
    const QString& ipAddress() const;
    const QString& hostAddress() const;
    NetDevice::Type type() const;
    const QList<NetService>& serviceList() const;

  public:
    void setName( const QString& name );
    void setHostName( const QString& hostName );
    void setIpAddress( const QString& ipAddress );
    void setType( NetDevice::Type type );
    void addService( const NetService& service );
    NetService removeService( const QString& serviceName );

  private:
    QString mName;
    QString mHostName;
    QString mIpAddress;
    NetDevice::Type mType;
    QList<NetService> mServiceList;
};


inline const QString& NetDevicePrivate::name()      const { return mName; }
inline const QString& NetDevicePrivate::hostName()  const { return mHostName; }
inline const QString& NetDevicePrivate::ipAddress() const { return mIpAddress; }
inline NetDevice::Type NetDevicePrivate::type()     const { return mType; }
inline const QList<NetService>& NetDevicePrivate::serviceList() const { return mServiceList; }
inline const QString& NetDevicePrivate::hostAddress() const { return mHostName.isEmpty() ? mIpAddress : mHostName; }

inline void NetDevicePrivate::setName( const QString& name ) { mName = name; }
inline void NetDevicePrivate::setHostName( const QString& hostName ) { mHostName = hostName; }
inline void NetDevicePrivate::setIpAddress( const QString& ipAddress ) { mIpAddress = ipAddress; }
inline void NetDevicePrivate::setType( NetDevice::Type type ) { mType = type; }
inline void NetDevicePrivate::addService( const NetService& service ) { mServiceList.append( service ); }

}

#endif
