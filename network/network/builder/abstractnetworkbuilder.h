/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef ABSTRACTNETWORKBUILDER_H
#define ABSTRACTNETWORKBUILDER_H

// Qt
#include <QObject>

namespace Mollet
{
class AbstractNetSystemFactory;


class AbstractNetworkBuilder : public QObject
{
    Q_OBJECT

public:
    ~AbstractNetworkBuilder() override;

public: // API to be implemented
    // build initial table synch
//     virtual void init() = 0;
    // build initial table asynch
//     virtual void startInit() = 0;

    // TODO: how to handle previously found, but unknown (or now-underpriorized) services? ignore and wait for rescan?
    virtual void registerNetSystemFactory( AbstractNetSystemFactory* netSystemFactory ) = 0;
    // temporary solution: have all factories set, then call start
    virtual void start() = 0;


    // drop all knowledge and restart
//     virtual void rescan()
//     virtual void startRescan()

Q_SIGNALS:
    /** initial structure build */
    void initDone();
};


inline AbstractNetworkBuilder::~AbstractNetworkBuilder() {}

}

#endif
