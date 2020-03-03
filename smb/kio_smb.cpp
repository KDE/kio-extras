/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE
//
// File:        Top level implementation file for kio_smb.cpp
//
// Abstract:    member function implementations for SMBSlave
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//
//---------------------------------------------------------------------------
//
// Copyright (c) 2000  Caldera Systems, Inc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program; see the file COPYING.  If not, please obtain
//     a copy from https://www.gnu.org/copyleft/gpl.html
//
/////////////////////////////////////////////////////////////////////////////

#include "kio_smb.h"
#include "smburl.h"
#include <QCoreApplication>
#include <QVersionNumber>

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.smb" FILE "smb.json")
};

bool needsEEXISTWorkaround()
{
    /* There is an issue with some libsmbclient versions that return EEXIST
     * return code from smbc_opendir() instead of EPERM when the user
     * tries to access a resource that requires login authetification.
     * We are working around the issue by treating EEXIST as a special case
     * of "invalid/unavailable credentials" if we detect that we are using
     * the affected versions of libsmbclient
     *
     * Upstream bug report: https://bugzilla.samba.org/show_bug.cgi?id=13050
     */
    static const QVersionNumber firstBrokenVer {4, 7, 0};
    static const QVersionNumber lastBrokenVer {4, 7, 6};

    const QVersionNumber currentVer = QVersionNumber::fromString(smbc_version());
    qCDebug(KIO_SMB_LOG) << "Using libsmbclient library version" << currentVer;

    if (currentVer >= firstBrokenVer && currentVer <= lastBrokenVer) {
        qCDebug(KIO_SMB_LOG) << "Detected broken libsmbclient version" << currentVer;
        return true;
    }

    return false;
}

SMBSlave::SMBSlave(const QByteArray &pool, const QByteArray &app)
    : SlaveBase("smb", pool, app)
    , m_openFd(-1)
    , m_enableEEXISTWorkaround(needsEEXISTWorkaround())
{
    m_initialized_smbc = false;

    // read in the default workgroup info...
    reparseConfiguration();

    // initialize the library...
    auth_initialize_smbc();
}

SMBSlave::~SMBSlave() = default;

void SMBSlave::virtual_hook(int id, void *data)
{
    switch (id) {
    case SlaveBase::GetFileSystemFreeSpace: {
        QUrl *url = static_cast<QUrl *>(data);
        fileSystemFreeSpace(*url);
    } break;
    case SlaveBase::Truncate: {
        auto length = static_cast<KIO::filesize_t *>(data);
        truncate(*length);
    } break;
    default: {
        SlaveBase::virtual_hook(id, data);
    } break;
    }
}

#include "kio_smb.moc"
