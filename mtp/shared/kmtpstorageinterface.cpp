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
