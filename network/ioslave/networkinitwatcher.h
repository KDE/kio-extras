/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORKINITWATCHER_H
#define NETWORKINITWATCHER_H

// network
#include "network.h"
//Qt
#include <QObject>
#include <QMutex>
#include <QDebug>

using namespace Mollet;


class NetworkInitWatcher : public QObject
{
    Q_OBJECT

public:
    NetworkInitWatcher( Network* network, QMutex* mutex );
    ~NetworkInitWatcher() override;

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
    qDebug();
}

inline void NetworkInitWatcher::onNetworkInitDone()
{
    qDebug()<<"before unlock";
    mMutex->unlock();
    qDebug()<<"after unlock";
    deleteLater();
    qDebug()<<"after deleteLater";
}

#endif
