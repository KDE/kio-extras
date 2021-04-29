/*
    This file is part of the KUPnP library, part of the KDE project.

    SPDX-FileCopyrightText: 2010 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef CAGIBIDBUSCODEC_H
#define CAGIBIDBUSCODEC_H

// lib
#include "cagibidevice.h"
// Qt
#include <QMetaType>
#include <QVector>

class QDBusArgument;
QDBusArgument& operator<<( QDBusArgument& argument,
                           const Cagibi::Device& device );
const QDBusArgument& operator>>( const QDBusArgument& argument,
                                 Cagibi::Device& device );

Q_DECLARE_METATYPE( Cagibi::Device )

#endif
