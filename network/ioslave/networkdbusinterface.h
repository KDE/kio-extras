/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORKDBUSINTERFACE_H
#define NETWORKDBUSINTERFACE_H


// network
#include <networkdbus.h>
// Qt
#include <QDBusAbstractInterface>
#include <QDBusReply>


// TODO: see file networkdbus.h
class NetworkDBusInterface: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    NetworkDBusInterface( const QString& service, const QString& path, const QDBusConnection& connection, QObject* parent = nullptr );
    ~NetworkDBusInterface() override;

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
    argumentList << QVariant::fromValue(hostAddress);
    return callWithArgumentList( QDBus::Block, QString::fromLatin1("deviceData"), argumentList );
}
inline QDBusReply<Mollet::NetService> NetworkDBusInterface::serviceData( const QString& hostAddress, const QString& serviceName, const QString& serviceType )
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(hostAddress) << QVariant::fromValue(serviceName) << QVariant::fromValue(serviceType);
    return callWithArgumentList( QDBus::Block, QString::fromLatin1("serviceData"), argumentList );
}
inline QDBusReply<Mollet::NetDeviceList> NetworkDBusInterface::deviceDataList()
{
    return call( QString::fromLatin1("deviceDataList") );
}
inline QDBusReply<Mollet::NetServiceList> NetworkDBusInterface::serviceDataList( const QString& hostAddress )
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(hostAddress);
    return callWithArgumentList( QDBus::Block, QString::fromLatin1("serviceDataList"), argumentList );
}

#endif
