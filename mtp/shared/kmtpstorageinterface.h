/*
    This file is part of the KMTP framework, part of the KDE project.

    Copyright (C) 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KMTPSTORAGEINTERFACE_H
#define KMTPSTORAGEINTERFACE_H

#include "storageinterface.h"
#include "kmtpfile.h"

class KMTPDeviceInterface;

/**
 * @brief The KMTPStorageInterface class
 *
 * @note This interface should be a public API.
 */
class KMTPStorageInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(quint64 maxCapacity READ maxCapacity)
    Q_PROPERTY(quint64 freeSpaceInBytes READ freeSpaceInBytes)

public:
    explicit KMTPStorageInterface(const QString &dbusObjectPath, KMTPDeviceInterface *parent);

    QString description() const;
    quint64 maxCapacity() const;
    quint64 freeSpaceInBytes() const;

private:
    org::kde::kmtp::Storage *m_dbusInterface;

public slots:
    // file management
    KMTPFileList getFilesAndFolders(const QString &path, int &result) const;
    KMTPFile getFileMetadata(const QString &path) const;

    int getFileToHandler(const QString &path) const;
    int getFileToFileDescriptor(const QDBusUnixFileDescriptor &descriptor, const QString &sourcePath) const;

    int sendFileFromFileDescriptor(const QDBusUnixFileDescriptor &descriptor, const QString &destinationPath) const;

    int setFileName(const QString &path, const QString &newName) const;

    // folder management
    quint32 createFolder(const QString &path) const;

    // object management
    int deleteObject(const QString &path) const;

signals:
    void dataReady(const QByteArray &data);
    void copyProgress(qulonglong transferredBytes, qulonglong totalBytes);
    void copyFinished(int result);
};

#endif // KMTPSTORAGEINTERFACE_H
