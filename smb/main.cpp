/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2020-2022 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include <QCoreApplication>

#include "kio_smb.h"

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        qCDebug(KIO_SMB_LOG) << "Usage: kio_smb protocol domain-socket1 domain-socket2";
        return -1;
    }

    SMBWorker worker(argv[2], argv[3]);
    worker.dispatchLoop();

    return 0;
}
