/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KIO_ARCHIVE_H
#define KIO_ARCHIVE_H

#include "kio_archivebase.h"

class ArchiveProtocol : public ArchiveProtocolBase
{
public:
    ArchiveProtocol( const QByteArray &proto, const QByteArray &pool, const QByteArray &app );
    ~ArchiveProtocol() override = default;

    KArchive *createArchive( const QString & proto, const QString & archiveFile ) override;
};

#endif // KIO_ARCHIVE_H
