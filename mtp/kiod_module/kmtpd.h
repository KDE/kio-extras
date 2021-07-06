/*
    This file is part of the MTP KIOD module, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KMTPD_H
#define KMTPD_H

#include <KDEDModule>
#include <Solid/Device>

#include <QDBusObjectPath>

class MTPDevice;

class KMTPd : public KDEDModule
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit KMTPd(QObject *parent, const QList<QVariant> &parameters);
    ~KMTPd() override;

    QString version() const;

private:
    void checkDevice(const Solid::Device &solidDevice);
    MTPDevice *deviceFromUdi(const QString &udi) const;

    QList<MTPDevice *> m_devices;
    qint32 m_timeout;

public Q_SLOTS:
    // D-Bus methods
    QList<QDBusObjectPath> listDevices() const;

private Q_SLOTS:
    void deviceAdded(const QString &udi);
    void deviceRemoved(const QString &udi);

Q_SIGNALS:
    // D-Bus signals
    void devicesChanged();
};

#endif // KMTPD_H
