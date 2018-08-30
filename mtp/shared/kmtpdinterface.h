/*
    This file is part of the KMTP framework, part of the KDE project.

    Copyright (C) 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
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

public slots:
    // D-Bus methods
    QList<QDBusObjectPath> listDevices();

signals:
    // D-Bus signals
    void devicesChanged();
};


#endif // KMTPDINTERFACE_H
