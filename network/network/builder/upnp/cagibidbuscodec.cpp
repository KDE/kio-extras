/*
    This file is part of the KUPnP library, part of the KDE project.

    Copyright 2010-2011 Friedrich W. H. Kossebau <kossebau@kde.org>

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

#include "cagibidbuscodec.h"

// network
#include "cagibidevice_p.h"
// Qt
#include <QtDBus/QDBusArgument>

#include <QtCore/QDebug>

static const QString type = QLatin1String( "deviceType" );
static const QString friendlyName = QLatin1String( "friendlyName" );
static const QString manufacturerName = QLatin1String( "manufacturer" );
static const QString modelDescription = QLatin1String( "modelDescription" );
static const QString modelName = QLatin1String( "modelName" );
static const QString modelNumber = QLatin1String( "modelNumber" );
static const QString serialNumber = QLatin1String( "serialNumber" );
static const QString udn = QLatin1String( "UDN" );
static const QString presentationUrl = QLatin1String( "presentationURL" );
static const QString ipAddress = QLatin1String( "ipAddress" );
static const QString portNumber = QLatin1String( "ipPortNumber" );
static const QString parentDeviceUdn = QLatin1String( "parentDeviceUDN" );


QDBusArgument& operator<<( QDBusArgument& argument, const Cagibi::Device& device )
{
    Q_UNUSED( device )
    // not used in code at the moment, so do not stream anything
    // QtDBus marshaller still uses it to calculate the D-Bus argument signature
    argument.beginMap( QVariant::String, QVariant::String );
    argument.endMap();

    return argument;
}

const QDBusArgument& operator>>( const QDBusArgument& argument,
                                 Cagibi::Device& device )
{
    Cagibi::DevicePrivate* const devicePrivate = device.d.data();

    argument.beginMap();

    QString key;
    QString value;
    while( ! argument.atEnd() )
    {
         argument.beginMapEntry();
         argument >> key;

         if( key == type )
         {
            QString type;
            argument >> type;
            const QStringList typeParts = type.split( QLatin1Char(':') );
qDebug()<<type;
            if( typeParts.size() >=5 )
                devicePrivate->mType = typeParts[3]+typeParts[4];
         }
         else if( key == friendlyName )
             argument >> devicePrivate->mFriendlyName;
         else if( key == manufacturerName )
             argument >> devicePrivate->mManufacturerName;
//     const QString& manufacturerUrl() const;
         else if( key == modelDescription )
             argument >> devicePrivate->mModelDescription;
         else if( key == modelName )
             argument >> devicePrivate->mModelName;
         else if( key == modelNumber )
             argument >> devicePrivate->mModelNumber;
         else if( key == serialNumber )
             argument >> devicePrivate->mSerialNumber;
         else if( key == udn )
             argument >> devicePrivate->mUdn;
//     const QString upc() const;
         else if( key == presentationUrl )
             argument >> devicePrivate->mPresentationUrl;
         else if( key == ipAddress )
             argument >> devicePrivate->mIpAddress;
         else if( key == portNumber )
             argument >> devicePrivate->mIpPortNumber;
         else if( key == parentDeviceUdn )
             argument >> devicePrivate->mParentDeviceUdn;
//     const QList<Icon>& icons() const;
//     const QList<Service>& services() const;
//     const QList<Device>& devices() const;
         // unknown key
         else
             argument >> value;
         argument.endMapEntry();
     }

    argument.endMap();

    return argument;
}
