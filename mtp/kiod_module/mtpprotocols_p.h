/*
    This file is part of the MTP KIOD module, part of the KDE project.

    SPDX-FileCopyrightText: 2026 Kun Ma <mk01022025@outlook.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MTPPROTOCOLS_P_H
#define MTPPROTOCOLS_P_H

#include <QString>
#include <QStringList>

namespace KMTP
{
inline QString mtpPlayerPredicate()
{
    return QStringLiteral("PortableMediaPlayer.supportedProtocols == 'mtp'");
}

inline bool supportsMtpProtocol(const QStringList &supportedProtocols)
{
    return supportedProtocols.contains(QStringLiteral("mtp"));
}
}

#endif // MTPPROTOCOLS_P_H
