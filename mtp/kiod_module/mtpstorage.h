/*
    This file is part of the MTP KIOD module, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MTPSTORAGE_H
#define MTPSTORAGE_H

#include <QObject>

#include <libmtp.h>
#include <kmtpfile.h>

class MTPDevice;

/**
 * @brief This D-Bus interface is used to access a single MTP storage.
 *
 * This includes storage management like file-, folder- and object-access.
 *
 * As a performance optimization to reduce hardware interaction,
 * a time based cache for file ids, mapping their path to their ID is used.
 */
class MTPStorage : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(quint64 maxCapacity READ maxCapacity)
    Q_PROPERTY(quint64 freeSpaceInBytes READ freeSpaceInBytes)

public:
    explicit MTPStorage(const QString &dbusObjectPath, const LIBMTP_devicestorage_t *mtpStorage, MTPDevice *parent);

    QString dbusObjectPath() const;

    // D-Bus properties
    QString description() const;
    quint64 maxCapacity() const;
    quint64 freeSpaceInBytes();

private:
    Q_DISABLE_COPY(MTPStorage)

    void setStorageProperties(const LIBMTP_devicestorage_t *storage);
    void updateStorageInfo();

    LIBMTP_mtpdevice_t *getDevice() const;

    /**
     * @brief Get the correct file/folder from the device.
     *
     * @param path
     * @return
     */
    KMTPFile getFileFromPath(const QString &path);

    /**
     * @brief Get all children files/folders of @a parentId and cache them.
     *
     * @param path      parent path, used for caching
     * @param parentId
     * @return
     */
    KMTPFileList getFilesAndFoldersCached(const QString &path, quint32 parentId);

    /**
     * @brief Returns the ID of the item at the given path, else 0.
     *
     * Automatically discards old items.
     *
     * @param path The Path to query the cache for
     * @return The ID of the Item if it exists, else 0
     */
    quint32 queryPath(const QString &path, int timeToLive = 60);

    /**
     * @brief Adds a Path to the Cache with the given id and ttl.
     *
     * @param path The path of the file/folder
     * @param id The file ID on the storage
     * @param timeToLive The time in seconds the entry should be valid
     */
    void addPath(const QString &path, quint32 id, int timeToLive = 60);

    /**
     * @brief Remove the given path from the cache, i.e. if it got deleted
     *
     * @param path The path that should be removed
     */
    void removePath(const QString &path);

    const QString m_dbusObjectPath;

    // LIBMTP_devicestorage_t properties
    quint32 m_id;                   /**< Unique ID for this storage */
    quint64 m_maxCapacity;          /**< Maximum capability */
    quint64 m_freeSpaceInBytes;     /**< Free space in bytes */
    QString m_description;          /**< A brief description of this storage */

    QHash<QString, QPair<QDateTime, uint32_t> > m_cache;

public Q_SLOTS:
    // D-Bus methods

    // file management
    KMTPFileList getFilesAndFolders(const QString &path, int &result);
    KMTPFile getFileMetadata(const QString &path);

    int getFileToHandler(const QString &path);
    int getFileToFileDescriptor(const QDBusUnixFileDescriptor &descriptor, const QString &sourcePath);

    int sendFileFromFileDescriptor(const QDBusUnixFileDescriptor &descriptor, const QString &destinationPath);
    int setFileName(const QString &path, const QString &newName);

    // folder management
    quint32 createFolder(const QString &path);

    // object management
    int deleteObject(const QString &path);

Q_SIGNALS:
    // D-Bus signals
    void dataReady(const QByteArray &data);
    void copyProgress(qulonglong transferredBytes, qulonglong totalBytes);
    void copyFinished(int result);
};

#endif // MTPSTORAGE_H
