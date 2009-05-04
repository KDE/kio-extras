/*
    This file is part of the solid network library, part of the KDE project.

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

#ifndef ABSTRACTNETWORKBUILDER_H
#define ABSTRACTNETWORKBUILDER_H

// Qt
#include <QtCore/QObject>

namespace Mollet
{
class AbstractNetSystemFactory;


class AbstractNetworkBuilder : public QObject
{
    Q_OBJECT

  public:
    virtual ~AbstractNetworkBuilder();

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
