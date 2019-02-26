/*  This file is part of the KDE libraries
    Copyright (C) 2019 Fabian Vogt <fabian@ritter-vogt.de>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <fcntl.h>
#include <unistd.h>

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QSize>
#include <QImage>

#include "sandboxedthumbnailerrunner.h"
#include "config-thumbnail.h"

SandboxedThumbnailerRunner::~SandboxedThumbnailerRunner()
{
    if (helperProcess.state() == QProcess::Running) {
        helperProcess.close();
        helperProcess.waitForFinished();
    }
}

bool SandboxedThumbnailerRunner::create(const QString &pluginName, const QString &path, QSize size, QImage &img, float sequenceIndex)
{
    if (!startHelper()) {
        return false;
    }

    QDataStream controlOut(&helperProcess), controlIn(&helperProcess);

    // Start creation
    controlOut << pluginName << path << size << sequenceIndex;

    bool readSuccessful = false, success = false;

    // Check whether it was created successfully
    readSuccessful = helperReadTransaction([&](QDataStream &controlIn) {
        controlIn >> success;
    });

    if (!readSuccessful || !success) {
        return false; // Failed to create thumbnail
    }

    // Read the thumbnail data
    QSize returnSize;
    int sizeInBytes;

    readSuccessful = helperReadTransaction([&](QDataStream &controlIn) {
        controlIn >> returnSize >> sizeInBytes;
    });

    if (!readSuccessful) {
        return false;
    }

    // Prepare a container for the image data
    QImage result(returnSize, QImage::Format_ARGB32);
    if(result.sizeInBytes() != sizeInBytes)
        return false;

    // Read the image data
    readSuccessful = helperReadTransaction([&](QDataStream &controlIn) {
        controlIn.readRawData(reinterpret_cast<char*>(result.bits()), result.sizeInBytes());
    });

    if (!readSuccessful) {
        return false;
    }

    img = result;
    return true;
}

bool SandboxedThumbnailerRunner::startHelper()
{
    if (helperProcess.state() == QProcess::Running) {
        return true; // Already running
    }

    // Start the helper
    helperProcess.setProgram(KTHUMBNAILHELPER_BIN);
    helperProcess.start(QIODevice::ReadWrite);
    helperProcess.setReadChannelMode(QProcess::ForwardedErrorChannel);
    if(!helperProcess.waitForStarted(1000)) {
        qWarning() << "Failed to start helper";
        return false;
    }

    // Do the handshake
    QDataStream controlOut(&helperProcess), controlIn(&helperProcess);

    // Do a handshake.
    int ourProtocolVersion = 1, theirProtocolVersion;

    controlOut << ourProtocolVersion;

    bool readSuccessful = helperReadTransaction([&](QDataStream &controlIn) {
        controlIn >> theirProtocolVersion;
    });

    if (!readSuccessful) {
        qWarning() << "Thumbnailhelper failed to complete startup";
        return false;
    }

    if (ourProtocolVersion != theirProtocolVersion) {
        qWarning() << "Thumbnailhelper protocol version mismatch!" << ourProtocolVersion << theirProtocolVersion;
        helperProcess.close();
        return false;
    }

    return true;
}
