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

#ifndef KMTPDEVICEINTERFACE_H
#define KMTPDEVICEINTERFACE_H

#include <QObject>

#include "deviceinterface.h"

class KMTPStorageInterface;

/**
 * @brief The KMTPDeviceInterface class
 *
 * @note This interface should be a public API.
 */
class KMTPDeviceInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString udi READ udi CONSTANT)
    Q_PROPERTY(QString friendlyName READ friendlyName NOTIFY friendlyNameChanged)

public:
    explicit KMTPDeviceInterface(const QString &dbusObjectPath, QObject *parent);

    QString udi() const;
    QString friendlyName() const;

    QVector<KMTPStorageInterface *> storages() const;
    KMTPStorageInterface *storageFromDescription(const QString &description) const;

private:
    org::kde::kmtp::Device *m_dbusInterface;
    QVector<KMTPStorageInterface *> m_storages;

public slots:
    int setFriendlyName(const QString &friendlyName);

signals:
    void friendlyNameChanged(const QString &friendlyName);
};


#endif // KMTPDEVICEINTERFACE_H
