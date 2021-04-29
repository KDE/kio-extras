/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef ABSTRACTNETSYSTEMFACTORY_H
#define ABSTRACTNETSYSTEMFACTORY_H

// Qt
#include <QObject>


namespace Mollet
{

class AbstractNetSystemFactory : public QObject
{
    Q_OBJECT

public:
    ~AbstractNetSystemFactory() override;

public: // API to be implemented
};


inline AbstractNetSystemFactory::~AbstractNetSystemFactory() {}

}

#endif
