/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "networkthread.h"

// ioslave
#include "networkinitwatcher.h"
// network
#include "network.h"
#include "netdevice.h"
#include "netservice.h"
//Qt
#include <QCoreApplication>

#include <QDebug>

NetworkThread::NetworkThread()
    : QThread()
    , mNetwork( nullptr )
    , mContinue( true )
{
}

Mollet::Network* NetworkThread::network() const {
    return mNetwork;
}

void NetworkThread::pause()
{
    mMutex.lock();
    exit();
}


void NetworkThread::unpause()
{
    mMutex.unlock();
}

void NetworkThread::finish()
{
    mContinue = false;
    exit();
}


void NetworkThread::run()
{
    mNetwork = Mollet::Network::network();

    mMutex.lock();
    new NetworkInitWatcher( mNetwork, &mMutex );

    do
    {
        exec();
        mMutex.lock();
        mMutex.unlock();
    }
    while( mContinue );
}

NetworkThread::~NetworkThread()
{
}
