/*
    Cache for recent files accessed.
    Copyright (C) 2012  Philipp Schmidt <philschmidt@gmx.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef KIO_MTP_FILE_CACHE_H
#define KIO_MTP_FILE_CACHE_H

#define KIO_MTP 7000

#include <stdint.h>

#include <QDateTime>
#include <QHash>
#include <QPair>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(LOG_KIO_MTP)

/**
 * @class FileCache Implements a time based cache for file ids, mapping their path to their ID. Does _not_ store the device they are on.
 */
class FileCache : public QObject
{
Q_OBJECT

public:
    explicit FileCache(QObject *parent = nullptr);

    /**
     * Returns the ID of the item at the given path, else 0.
     * Automatically discards old items.
     *
     * @param path The Path to query the cache for
     * @return The ID of the Item if it exists, else 0
     */
    uint32_t queryPath(const QString &path, int timeToLive = 60);

    /**
     * Adds a Path to the Cache with the given id and ttl.
     *
     * @param path The path of the file/folder
     * @param id The file ID on the storage
     * @param timeToLive The time in seconds the entry should be valid
     */
    void addPath(const QString &path, uint32_t id, int timeToLive = 60);

    /**
     * Remove the given path from the cache, i.e. if it got deleted
     *
     * @param path The path that should be removed
     */
    void removePath(const QString &path);

// private slots:
//     void insertItem( const QString& path, QPair<QDateTime, uint32_t> item );
//     void removeItem( const QString& path );
//
// signals:
//     void s_insertItem( const QString& path, QPair<QDateTime, uint32_t> item );
//     void s_removeItem( const QString& path );

private:
    QHash<QString, QPair<QDateTime, uint32_t> > cache;
};

#endif // KIO_MTP_FILE_CACHE_H
