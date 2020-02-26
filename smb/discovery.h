/*
    Copyright 2019 Harald Sitter <sitter@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3 of
    the License or any later version accepted by the membership of
    KDE e.V. (or its successor approved by the membership of KDE
    e.V.), which shall act as a proxy defined in Section 14 of
    version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <KIO/UDSEntry>
#include <QSharedPointer>

class Discovery
{
public:
    typedef QSharedPointer<Discovery> Ptr;

    Discovery();
    virtual ~Discovery();
    virtual KIO::UDSEntry toEntry() const = 0;
};

class Discoverer
{
public:
    Discoverer();
    virtual ~Discoverer();

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isFinished() const = 0;

    // Implement as signal!
    virtual void newDiscovery(Discovery::Ptr discovery) = 0;
    virtual void finished() = 0;
};

#endif // DISCOVERY_H
