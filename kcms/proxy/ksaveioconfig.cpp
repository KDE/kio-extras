/*
    SPDX-FileCopyrightText: 2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// Own
#include "ksaveioconfig.h"

// Qt
#ifdef WITH_DBUS
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#endif

// KDE
#include <kio/ioworker_defaults.h>

void KSaveIOConfig::updateRunningWorkers()
{
#ifdef WITH_DBUS
    // Inform all running KIO workers about the changes...
    // if we cannot update, KIO workers inform the end user...
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KIO/Scheduler"), QStringLiteral("org.kde.KIO.Scheduler"), QStringLiteral("reparseSlaveConfiguration"));
    message << QString();
    QDBusConnection::sessionBus().send(message);
#endif
}
