/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef UPNPNETWORKBUILDER_H
#define UPNPNETWORKBUILDER_H

// lib
#include <abstractnetworkbuilder.h>
#include <network.h>
#include <netdevice.h>
// Qt
#include <QMetaType>
#include <QHash>

namespace Cagibi {
class Device;
}

class QDBusInterface;
class QDBusPendingCallWatcher;

typedef QHash<QString,QString> DeviceTypeMap;
Q_DECLARE_METATYPE( DeviceTypeMap )


namespace Mollet
{
class UpnpNetSystemAble;


class UpnpNetworkBuilder : public AbstractNetworkBuilder
{
    Q_OBJECT

public:
    explicit UpnpNetworkBuilder( NetworkPrivate* networkPrivate );
    ~UpnpNetworkBuilder() override;

public: // AbstractNetworkBuilder API
    void registerNetSystemFactory( AbstractNetSystemFactory* netSystemFactory ) override;
    void start() override;
    //TODO: void stop(); ? why needed, what to do?

protected:
    void addUPnPDevices( const QList<Cagibi::Device>& devices );
    void removeUPnPDevices( const QList<Cagibi::Device>& devices );

private Q_SLOTS:
    void startBrowse();

    void onDevicesAdded( const DeviceTypeMap& deviceTypeMap );
    void onDevicesRemoved( const DeviceTypeMap& deviceTypeMap );
    void onAddedDeviceDetails( const Cagibi::Device& device );
    void onCagibiServiceOwnerChanged( const QString& serviceName,
                                      const QString& oldOwner, const QString& newOwner );

    void onAllDevicesCallFinished( QDBusPendingCallWatcher* allDevicesCallWatcher );

private:
    void queryCurrentDevices();

private: // data
    NetworkPrivate* mNetworkPrivate;

    QList<UpnpNetSystemAble*> mNetSystemFactoryList;

    QHash<QString,Cagibi::Device> mActiveDevices;

    QDBusInterface* mCagibiDeviceListDBusProxy;
};

}

#endif
