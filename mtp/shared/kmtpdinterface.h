/*
    This file is part of the KMTP framework, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KMTPDINTERFACE_H
#define KMTPDINTERFACE_H

#include <QObject>
#include "daemoninterface.h"

class KMTPDeviceInterface;

/**
 * @brief The KMTPDeviceInterface class
 *
 * @note This interface should be a public API.
 */
class KMTPDInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit KMTPDInterface(QObject *parent = nullptr);

    bool isValid() const;

    KMTPDeviceInterface *deviceFromName(const QString &friendlyName) const;
    KMTPDeviceInterface *deviceFromUdi(const QString &udi) const;

    QVector<KMTPDeviceInterface *> devices() const;

    // D-Bus properties
    QString version() const;

private:
    void updateDevices();

    org::kde::kmtp::Daemon *m_dbusInterface;
    QVector<KMTPDeviceInterface *> m_devices;

public Q_SLOTS:
    // D-Bus methods
    QList<QDBusObjectPath> listDevices();

Q_SIGNALS:
    // D-Bus signals
    void devicesChanged();
};


#endif // KMTPDINTERFACE_H
