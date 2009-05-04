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

#ifndef NETWORKINITWATCHER_H
#define NETWORKINITWATCHER_H

// network
#include "network.h"
//Qt
#include <QtCore/QObject>
#include <QtCore/QMutex>

#include <KDebug>

using namespace Mollet;


class NetworkInitWatcher : public QObject
{
    Q_OBJECT

  public:
    NetworkInitWatcher( Network* network, QMutex* mutex );
    virtual ~NetworkInitWatcher();

  public Q_SLOTS:
    void onNetworkInitDone();

  private:
    QMutex* mMutex;
};


inline NetworkInitWatcher::NetworkInitWatcher( Network* network, QMutex* mutex )
  : mMutex( mutex )
{
    connect( network, SIGNAL(initDone()), SLOT(onNetworkInitDone()) );
}
inline NetworkInitWatcher::~NetworkInitWatcher()
{
kDebug();
}

inline void NetworkInitWatcher::onNetworkInitDone()
{
kDebug()<<"before unlock";
    mMutex->unlock();
kDebug()<<"after unlock";
    deleteLater();
kDebug()<<"after deleteLater";
}

#endif
