/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE
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

#include <QCoreApplication>

#include "kio_smb.h"

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        qCDebug(KIO_SMB_LOG) << "Usage: kio_smb protocol domain-socket1 domain-socket2";
        return -1;
    }

    SMBSlave slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}

