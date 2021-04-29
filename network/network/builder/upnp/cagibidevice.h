/*
    This file is part of the KUPnP library, part of the KDE project.

    SPDX-FileCopyrightText: 2009-2010 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef CAGIBIDEVICE_H
#define CAGIBIDEVICE_H

// Qt
#include <QExplicitlySharedDataPointer>

namespace Cagibi {
class Device;
}
class QString;
class QDBusArgument;
extern QDBusArgument& operator<<( QDBusArgument& argument,
                                  const Cagibi::Device& device );
extern const QDBusArgument& operator>>( const QDBusArgument& argument,
                                        Cagibi::Device& device );


namespace Cagibi
{
class DevicePrivate;

class Device
{
    friend QDBusArgument& ::operator<<( QDBusArgument& argument,
                                        const Cagibi::Device& device );
    friend const QDBusArgument& ::operator>>( const QDBusArgument& argument,
            Cagibi::Device& device );

protected:
    explicit Device( DevicePrivate* _d );

public:
    Device();
    Device( const Device& other );

    ~Device();

public:
    Device& operator=( const Device& other );

public:
    const QString& type() const;
    const QString& friendlyName() const;
    const QString& manufacturerName() const;
//     QString manufacturerUrl() const;
    const QString& modelDescription() const;
    const QString& modelName() const;
    const QString& modelNumber() const;
    const QString& serialNumber() const;
    const QString& udn() const;
//     QString upc() const;
    const QString& presentationUrl() const;
    const QString& ipAddress() const;
    int ipPortNumber() const;

    bool hasParentDevice() const;
    const QString& parentDeviceUdn() const;

protected:
    QExplicitlySharedDataPointer<DevicePrivate> d;
};

}

#endif
