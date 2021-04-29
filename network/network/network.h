/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORK_H
#define NETWORK_H

// lib
#include "molletnetwork_export.h"
// Qt
#include <QObject>

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
    Network();
    ~Network() override;

public:
    QList<NetDevice> deviceList() const;

Q_SIGNALS:
    void devicesAdded( const QList<NetDevice>& deviceList );
    void devicesRemoved( const QList<NetDevice>& deviceList );
    void servicesAdded( const QList<NetService>& serviceList );
    void servicesRemoved( const QList<NetService>& serviceList );

    void initDone();

private:
    Q_PRIVATE_SLOT( d, void onBuilderInit() )

private:
    NetworkPrivate* const d;
};

// void connect( Network& network, const char* signal, QObject* object, const char* slot );

}

#endif
