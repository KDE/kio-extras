/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETDEVICE_P_H
#define NETDEVICE_P_H

// lib
#include "netdevice.h"
#include "netservice.h"
// Qt
#include <QSharedData>
#include <QString>
#include <QList>


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
    bool hasService( const QString& id ) const;

  public:
    void setName( const QString& name );
    void setHostName( const QString& hostName );
    void setIpAddress( const QString& ipAddress );
    void setType( NetDevice::Type type );
    void addService( const NetService& service );
    NetService removeService( const QString& id );

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
