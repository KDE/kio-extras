/*
    This file is part of the KMTP framework, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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

public Q_SLOTS:
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

Q_SIGNALS:
    void dataReady(const QByteArray &data);
    void copyProgress(qulonglong transferredBytes, qulonglong totalBytes);
    void copyFinished(int result);
};

#endif // KMTPSTORAGEINTERFACE_H
