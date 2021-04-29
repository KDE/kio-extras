/*
    This file is part of the KUPnP library, part of the KDE project.

    SPDX-FileCopyrightText: 2009-2010 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "cagibidevice.h"
#include "cagibidevice_p.h"


namespace Cagibi
{

Device::Device()
  : d( new DevicePrivate() )
{
}

Device::Device( DevicePrivate* _d )
  : d( _d )
{
}

Device::Device( const Device& other )
  : d( other.d )
{
}

const QString& Device::type() const { return d->type(); }
const QString& Device::friendlyName() const { return d->friendlyName(); }
const QString& Device::manufacturerName() const { return d->manufacturerName(); }
const QString& Device::modelDescription() const { return d->modelDescription(); }
const QString& Device::modelName() const { return d->modelName(); }
const QString& Device::modelNumber() const { return d->modelNumber(); }
const QString& Device::serialNumber() const { return d->serialNumber(); }
const QString& Device::udn() const { return d->udn(); }
const QString& Device::presentationUrl() const { return d->presentationUrl(); }
const QString& Device::ipAddress() const { return d->ipAddress(); }
int Device::ipPortNumber() const { return d->ipPortNumber(); }

bool Device::hasParentDevice() const { return d->hasParentDevice(); }
const QString& Device::parentDeviceUdn() const { return d->parentDeviceUdn(); }

Device& Device::operator=( const Device& other )
{
    d = other.d;
    return *this;
}

Device::~Device()
{
}

}
