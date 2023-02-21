/*
    This file is part of the KMTP framework, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kmtpstorageinterface.h"

#include <chrono>

#include "kmtpdeviceinterface.h"

using namespace std::chrono_literals;

KMTPStorageInterface::KMTPStorageInterface(const QString &dbusObjectPath, KMTPDeviceInterface *parent)
    : QObject(parent)
{
    m_dbusInterface = new org::kde::kmtp::Storage(QStringLiteral("org.kde.kiod5"),
            dbusObjectPath,
            QDBusConnection::sessionBus(),
            this);
    // Arbitrarily large number to prevent timeouts on file listing.
    // https://bugs.kde.org/show_bug.cgi?id=462059
    // https://github.com/libmtp/libmtp/issues/144
    m_dbusInterface->setTimeout(std::chrono::milliseconds(60min).count());

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

std::variant<QDBusObjectPath, QDBusError> KMTPStorageInterface::getFilesAndFolders2(const QString &path) const
{
    auto reply = m_dbusInterface->getFilesAndFolders2(path);
    reply.waitForFinished();
    if (reply.error().isValid()) {
        return reply.error();
    }
    return reply;
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
