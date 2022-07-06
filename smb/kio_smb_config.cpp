/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "kio_smb.h"
#include "smburl.h"
#include <KConfig>
#include <KConfigGroup>

#include <QTextCodec>

void SMBWorker::reparseConfiguration()
{
    m_context.authenticator()->loadConfiguration();
}
