/*
    This file is part of the KMTP framework, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kmtpstorageinterface.h"
#include "kmtpdeviceinterface.h"

KMTPStorageInterface::KMTPStorageInterface(const QString &dbusObjectPath, KMTPDeviceInterface *parent)
    : QObject(parent)
{
    m_dbusInterface = new org::kde::kmtp::Storage(QStringLiteral("org.kde.kiod5"),
            dbusObjectPath,
            QDBusConnection::sessionBus(),
            this);
    m_dbusInterface->setTimeout(5 * 60 * 1000); // TODO: listing folders with a huge amount of files may take a while

    qDBusRegisterMetaType<KMTPFile>();
    qDBusRegisterMetaType<KMTPFileList>();

    connect(m_dbusInterface, &org::kde::kmtp::Storage::dataReady, this, &KMTPStorageInterface::dataReady);
    connect(m_dbusInterface, &org::kde::kmtp::Storage::copyProgress, this, &KMTPStorageInterface::copyProgress);
    connect(m_dbusInterface, &org::kde::kmtp::Storage::copyFinished, this, &KMTPStorageInterface::copyFinished);
}

QString KMTPStorageInterface::description() const
{
    return m_dbusInterface->description();
}

quint64 KMTPStorageInterface::maxCapacity() const
{
    return m_dbusInterface->maxCapacity();
}

quint64 KMTPStorageInterface::freeSpaceInBytes() const
{
    return m_dbusInterface->freeSpaceInBytes();
}

KMTPFileList KMTPStorageInterface::getFilesAndFolders(const QString &path, int &result) const
{
    return m_dbusInterface->getFilesAndFolders(path, result);
}

KMTPFile KMTPStorageInterface::getFileMetadata(const QString &path) const
{
    return m_dbusInterface->getFileMetadata(path);
}

int KMTPStorageInterface::getFileToHandler(const QString &path) const
{
    return m_dbusInterface->getFileToHandler(path);
}

int KMTPStorageInterface::getFileToFileDescriptor(const QDBusUnixFileDescriptor &descriptor, const QString &sourcePath) const
{
    return m_dbusInterface->getFileToFileDescriptor(descriptor, sourcePath).value();
}

int KMTPStorageInterface::sendFileFromFileDescriptor(const QDBusUnixFileDescriptor &descriptor, const QString &destinationPath) const
{
    return m_dbusInterface->sendFileFromFileDescriptor(descriptor, destinationPath);
}

int KMTPStorageInterface::setFileName(const QString &path, const QString &newName) const
{
    return m_dbusInterface->setFileName(path, newName);
}

quint32 KMTPStorageInterface::createFolder(const QString &path) const
{
    return m_dbusInterface->createFolder(path);
}

int KMTPStorageInterface::deleteObject(const QString &path) const
{
    return m_dbusInterface->deleteObject(path);
}

#include "moc_kmtpstorageinterface.cpp"
