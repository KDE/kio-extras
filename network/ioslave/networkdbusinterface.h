/*
    This file is part of the network kioslave, part of the KDE project.

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

#ifndef NETWORKDBUSINTERFACE_H
#define NETWORKDBUSINTERFACE_H


// network
#include <networkdbus.h>
// Qt
#include <QtDBus/QDBusAbstractInterface>
#include <QtDBus/QDBusReply>


// TODO: see file networkdbus.h
class NetworkDBusInterface: public QDBusAbstractInterface
{
    Q_OBJECT

  public:
    NetworkDBusInterface( const QString& service, const QString& path, const QDBusConnection& connection, QObject* parent = 0 );
    virtual ~NetworkDBusInterface();

  public Q_SLOTS:
    QDBusReply<Mollet::NetDevice> deviceData( const QString& hostAddress );
    QDBusReply<Mollet::NetService> serviceData( const QString& hostAddress, const QString& serviceName, const QString& serviceType );
    QDBusReply<Mollet::NetDeviceList> deviceDataList();
    QDBusReply<Mollet::NetServiceList> serviceDataList( const QString& hostAddress );
};

// TODO: is QDBus::Block the right solution here?
inline QDBusReply<Mollet::NetDevice> NetworkDBusInterface::deviceData( const QString& hostAddress )
{
    QList<QVariant> argumentList;
    argumentList << qVariantFromValue(hostAddress);
    return callWithArgumentList( QDBus::Block, QString::fromLatin1("deviceData"), argumentList );
}
inline QDBusReply<Mollet::NetService> NetworkDBusInterface::serviceData( const QString& hostAddress, const QString& serviceName, const QString& serviceType )
{
    QList<QVariant> argumentList;
    argumentList << qVariantFromValue(hostAddress) << qVariantFromValue(serviceName) << qVariantFromValue(serviceType);
    return callWithArgumentList( QDBus::Block, QString::fromLatin1("serviceData"), argumentList );
}
inline QDBusReply<Mollet::NetDeviceList> NetworkDBusInterface::deviceDataList()
{
    return call( QString::fromLatin1("deviceDataList") );
}
inline QDBusReply<Mollet::NetServiceList> NetworkDBusInterface::serviceDataList( const QString& hostAddress )
{
    QList<QVariant> argumentList;
    argumentList << qVariantFromValue(hostAddress);
    return callWithArgumentList( QDBus::Block, QString::fromLatin1("serviceDataList"), argumentList );
}

#endif
