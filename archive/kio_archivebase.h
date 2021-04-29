/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KIO_ARCHIVEBASE_H
#define KIO_ARCHIVEBASE_H

#include <sys/types.h>

#include <kio/global.h>
#include <kio/slavebase.h>

#include "libkioarchive_export.h"

class KArchive;
class KArchiveEntry;
class KArchiveDirectory;

class LIBKIOARCHIVE_EXPORT ArchiveProtocolBase : public KIO::SlaveBase
{
public:
    ArchiveProtocolBase( const QByteArray &proto, const QByteArray &pool, const QByteArray &app );
    ~ArchiveProtocolBase() override;

    void listDir( const QUrl & url ) override;
    void stat( const QUrl & url ) override;
    void get( const QUrl & url ) override;

private:
    void createRootUDSEntry( KIO::UDSEntry & entry );
    void createUDSEntry( const KArchiveEntry * tarEntry, KIO::UDSEntry & entry );

    virtual KArchive *createArchive( const QString & proto, const QString & archiveFile ) = 0;

    KIO::StatDetails getStatDetails();
    uint             computeArchiveDirSize(const KArchiveDirectory *dir);

    /**
     * \brief find, check and open the archive file
     * \param url The URL of the archive
     * \param path Path where the archive really is (returned value)
     * \param errNum KIO error number (undefined if the function returns true)
     * \return true if file was found, false if there was an error
     */
    bool checkNewFile( const QUrl & url, QString & path, KIO::Error& errorNum );

    KArchive * m_archiveFile;
    QString m_archiveName;
    QString m_user, m_group;
    time_t m_mtime;
};

#endif // KIO_ARCHIVEBASE_H
