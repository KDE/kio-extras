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

#ifndef UPNPNETWORKBUILDER_H
#define UPNPNETWORKBUILDER_H

// lib
#include <abstractnetworkbuilder.h>
#include <network.h>
#include <netdevice.h>
// Qt
#include <QtCore/QMetaType>
#include <QtCore/QHash>

namespace Cagibi { class Device; }

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
    virtual ~UpnpNetworkBuilder();

  public: // AbstractNetworkBuilder API
    virtual void registerNetSystemFactory( AbstractNetSystemFactory* netSystemFactory );
    virtual void start();
    //TODO: void stop(); ? why needed, what to do?

  protected:
    void addUPnPDevices( const QList<Cagibi::Device>& devices );
    void removeUPnPDevices( const QList<Cagibi::Device>& devices );

  private Q_SLOTS:
    void onDevicesAdded( const DeviceTypeMap& deviceTypeMap );
    void onDevicesRemoved( const DeviceTypeMap& deviceTypeMap );
    void onAddedDeviceDetails( const Cagibi::Device& device );

    void onAllDevicesCallFinished( QDBusPendingCallWatcher* allDevicesCallWatcher );

  private: // data
    NetworkPrivate* mNetworkPrivate;

    QList<UpnpNetSystemAble*> mNetSystemFactoryList;

    QHash<QString,Cagibi::Device> mActiveDevices;

    QDBusInterface* mDBusCagibiProxy;
};

}

#endif
