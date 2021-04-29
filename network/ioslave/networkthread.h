/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

// Qt
#include <QThread>
#include <QMutex>

namespace Mollet {
class Network;
}


class NetworkThread : public QThread
{
public:
    NetworkThread();
    ~NetworkThread() override;

public:
    Mollet::Network* network() const;

public:
    void pause();
    void unpause();
    void finish();

protected: // QThread API
    void run() override;

private:
    QMutex mMutex;
    Mollet::Network* mNetwork;

    bool mContinue;
};

#endif
