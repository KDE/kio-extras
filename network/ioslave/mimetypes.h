/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef MIMETYPES_H
#define MIMETYPES_H

class QString;


class Mimetypes
{
public:
    static const char NetworkMimetype[];
    static const char* const DeviceMimetype[];

    static QString mimetypeForServiceType( const QString& serviceTypeName );
};

#endif
