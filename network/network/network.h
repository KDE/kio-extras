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

#ifndef NETWORK_H
#define NETWORK_H

// lib
#include "molletnetwork_export.h"
// Qt
#include <QtCore/QObject>

namespace Mollet {
class NetDevice;
class NetService;
}
template < class T > class QList;


namespace Mollet
{
class NetworkPrivate;

class MOLLETNETWORK_EXPORT Network : public QObject
{
    Q_OBJECT

    friend class NetworkPrivate;

  public:
    static Network* network();

  public:
    virtual ~Network();

  public:
    QList<NetDevice> deviceList() const;

  Q_SIGNALS:
    void devicesAdded( const QList<NetDevice>& deviceList );
    void devicesRemoved( const QList<NetDevice>& deviceList );
    void servicesAdded( const QList<NetService>& serviceList );
    void servicesRemoved( const QList<NetService>& serviceList );

    void initDone();

  private:
    Network();
    Q_PRIVATE_SLOT( d, void onBuilderInit() )

  private:
    NetworkPrivate* const d;
};

// void connect( Network& network, const char* signal, QObject* object, const char* slot );

}

#endif
