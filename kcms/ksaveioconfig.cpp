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
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <kio/ioworker_defaults.h>

void KSaveIOConfig::updateRunningWorkers(QWidget *parent)
{
#ifdef WITH_DBUS
    // Inform all running KIO workers about the changes...
    // if we cannot update, KIO workers inform the end user...
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KIO/Scheduler"), QStringLiteral("org.kde.KIO.Scheduler"), QStringLiteral("reparseSlaveConfiguration"));
    message << QString();
    if (!QDBusConnection::sessionBus().send(message)) {
#endif
        KMessageBox::information(parent,
                                 i18n("You have to restart the running applications "
                                      "for these changes to take effect."),
                                 i18nc("@title:window", "Update Failed"));
#ifdef WITH_DBUS
    }
#endif
}
