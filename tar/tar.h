/*  This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef _TAR_H
#define _TAR_H

#include <sys/types.h>

#include <kio/global.h>
#include <kio/slavebase.h>

class ArchiveProtocol : public KIO::SlaveBase
{
public:
    ArchiveProtocol( const QCString &pool, const QCString &app );
    virtual ~ArchiveProtocol();

    virtual void listDir( const KURL & url );
    virtual void stat( const KURL & url );
    virtual void get( const KURL & url );

protected:
    void createUDSEntry( const KArchiveEntry * tarEntry, KIO::UDSEntry & entry );

    /**
     * \brief find, check and open the archive file
     * \param url The URL of the archive
     * \param path Path where the archive really is (returned value)
     * \param errNum KIO error number (undefined if the function returns true)
     * \return true if file was found, false if there was an error
     */
    bool checkNewFile( const KURL & url, QString & path, KIO::Error& errorNum );

    KArchive * m_archiveFile;
    QString m_archiveName;
    time_t m_mtime;
};

#endif
