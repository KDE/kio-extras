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
#include "kio_smb_internal.h"
#include <QCoreApplication>
#include <QVersionNumber>

Q_LOGGING_CATEGORY(KIO_SMB, "kio_smb")

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
    static const QVersionNumber firstBrokenVer{4, 7, 0};
    static const QVersionNumber lastBrokenVer{4, 7, 6};

    const QVersionNumber currentVer = QVersionNumber::fromString(smbc_version());
    qCDebug(KIO_SMB) << "Using libsmbclient library version" << currentVer;

    if (currentVer >= firstBrokenVer && currentVer <= lastBrokenVer) {
        qCDebug(KIO_SMB) << "Detected broken libsmbclient version" << currentVer;
        return true;
    }

    return false;
}

//===========================================================================
SMBSlave::SMBSlave(const QByteArray& pool, const QByteArray& app)
    : SlaveBase( "smb", pool, app ),
      m_openFd(-1),
      m_enableEEXISTWorkaround(needsEEXISTWorkaround())
{
    m_initialized_smbc = false;

    //read in the default workgroup info...
    reparseConfiguration();

    //initialize the library...
    auth_initialize_smbc();
}


//===========================================================================
SMBSlave::~SMBSlave()
{
}

void SMBSlave::virtual_hook(int id, void *data) {
    switch(id) {
    case SlaveBase::GetFileSystemFreeSpace: {
        QUrl *url = static_cast<QUrl *>(data);
        fileSystemFreeSpace(*url);
    } break;
    default: {
        SlaveBase::virtual_hook(id, data);
    } break;
    }
}

//===========================================================================
int Q_DECL_EXPORT kdemain( int argc, char **argv )
{
    QCoreApplication app(argc, argv);
    if( argc != 4 )
    {
        qCDebug(KIO_SMB) << "Usage: kio_smb protocol domain-socket1 domain-socket2";
        return -1;
    }

    SMBSlave slave( argv[2], argv[3] );

    slave.dispatchLoop();

    return 0;
}

