/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
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
    virtual QString udsName() const = 0;
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
