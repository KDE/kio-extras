/*
    This file is part of the KMTP framework, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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

public Q_SLOTS:
    int setFriendlyName(const QString &friendlyName);

Q_SIGNALS:
    void friendlyNameChanged(const QString &friendlyName);
};


#endif // KMTPDEVICEINTERFACE_H
