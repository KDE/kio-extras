/*
    This file is part of the network kioslave, part of the KDE project.

    Copyright 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

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

#include "networkslave.h"

// ioslave
#include "networkuri.h"
#include "mimetypes.h"
#include "networkdbusinterface.h"
// network
#include <netdevice.h>
#include <netservice.h>
// KDE
// Qt
#include <QtCore/QEventLoop>

#include <KDebug>

// static const char NetworkIconName[] = "network-workgroup";


NetworkSlave::NetworkSlave( const QByteArray& name, const QByteArray& poolSocket, const QByteArray& programSocket )
  : SlaveBase( name, poolSocket, programSocket )
{
kDebug();
    mNetworkDBusProxy = new NetworkDBusInterface( "org.kde.network", "/", QDBusConnection::sessionBus() );
}

void NetworkSlave::get( const KUrl& url )
{
    const NetworkUri networkUri( url );

    bool successfulGetting = false;

    const NetworkUri::Type type = networkUri.type();
kDebug()<<"type="<<networkUri.type()<<"host="<<networkUri.hostAddress()<<"service="<<networkUri.serviceName()<<"stype="<<networkUri.serviceType();
    if( type == NetworkUri::Service )
    {
        const QString hostAddress =    networkUri.hostAddress();
        const QString serviceName = networkUri.serviceName();
        const QString serviceType = networkUri.serviceType();
        QDBusReply<Mollet::NetService> reply = mNetworkDBusProxy->serviceData( hostAddress, serviceName, serviceType );

kDebug()<<reply.isValid();
        if( reply.isValid() ) // TODO: find how a not found service can be expressed in the reply
        {
            Mollet::NetService serviceData = reply.value();
            if( serviceData.isValid() )
            {
                const QString url = serviceData.url();
                redirection( url );
                finished();
                successfulGetting = true;
            }
        }
    }

    if( ! successfulGetting )
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
}

void NetworkSlave::mimetype( const KUrl& url )
{
    const NetworkUri networkUri( url );

    bool successfulMimetyping = false;
    NetworkUri::Type type = networkUri.type();
kDebug()<<"type="<<networkUri.type()<<"host="<<networkUri.hostAddress()<<"service="<<networkUri.serviceName()<<"stype="<<networkUri.serviceType();

    if( type == NetworkUri::Domain )
    {
        mimeType( Mimetypes::NetworkMimetype );
        finished();
        successfulMimetyping = true;
    }
    else
    {
        const QString hostAddress = networkUri.hostAddress();
        if( type == NetworkUri::Device )
        {
            QDBusReply<Mollet::NetDevice> reply = mNetworkDBusProxy->deviceData( hostAddress );

kDebug()<<reply.isValid();
            if( reply.isValid() ) // TODO: find how a not found service can be expressed in the reply
            {
                Mollet::NetDevice deviceData = reply.value();

                mimeType( Mimetypes::DeviceMimetype[deviceData.type()] );
                finished();
                successfulMimetyping = true;
            }
        }
        else if( type == NetworkUri::Service )
        {
            const QString serviceName = networkUri.serviceName();
            const QString serviceType = networkUri.serviceType();
            QDBusReply<Mollet::NetService> reply = mNetworkDBusProxy->serviceData( hostAddress, serviceName, serviceType );

kDebug()<<reply.isValid();
            if( reply.isValid() ) // TODO: find how a not found service can be expressed in the reply
            {
                Mollet::NetService serviceData = reply.value();
                if( serviceData.isValid() )
                {
                    const QString url = serviceData.url();
                    redirection( url );
                    //mimeType( Mimetypes::mimetypeForServiceType(serviceData.type()) );
                    finished();
                    successfulMimetyping = true;
                }
            }
        }
    }

    if( !successfulMimetyping )
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
}


void NetworkSlave::stat( const KUrl& url )
{
    const NetworkUri networkUri( url );

    bool successfulStating = false;
    NetworkUri::Type type = networkUri.type();
kDebug()<<"type="<<networkUri.type()<<"host="<<networkUri.hostAddress()<<"service="<<networkUri.serviceName()<<"stype="<<networkUri.serviceType();

    if( type == NetworkUri::Domain )
    {
        KIO::UDSEntry entry;
        feedEntryAsNetwork( &entry );
        statEntry( entry );
        finished();
        successfulStating = true;
    }
    else
    {
        const QString hostAddress = networkUri.hostAddress();
        if( type == NetworkUri::Device )
        {
            QDBusReply<Mollet::NetDevice> reply = mNetworkDBusProxy->deviceData( hostAddress );

kDebug()<<reply.isValid();
            if( reply.isValid() ) // TODO: find how a not found service can be expressed in the reply
            {
                Mollet::NetDevice deviceData = reply.value();

                KIO::UDSEntry entry;
                feedEntryAsDevice( &entry, deviceData );
                statEntry( entry );
                finished();
                successfulStating = true;
            }
        }
        else if( type == NetworkUri::Service )
        {
            const QString serviceName = networkUri.serviceName();
            const QString serviceType = networkUri.serviceType();
            QDBusReply<Mollet::NetService> reply = mNetworkDBusProxy->serviceData( hostAddress, serviceName, serviceType );

kDebug()<<reply.isValid();
            if( reply.isValid() ) // TODO: find how a not found service can be expressed in the reply
            {
                Mollet::NetService serviceData = reply.value();
                if( serviceData.isValid() )
                {
                    const QString url = serviceData.url();
                    redirection( url );
                    //KIO::UDSEntry entry;
                    //feedEntryAsService( &entry, serviceData );
                    //statEntry( entry );
                    finished();
                    successfulStating = true;
                }
            }
        }
    }

    if( !successfulStating )
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
}

void NetworkSlave::listDir( const KUrl& url )
{
    const NetworkUri networkUri( url );

    bool successfulListing = false;
    NetworkUri::Type networkUriType = networkUri.type();
kDebug()<<"type="<<networkUri.type()<<"host="<<networkUri.hostAddress()<<"service="<<networkUri.serviceName()<<"stype="<<networkUri.serviceType();

    if( networkUriType != NetworkUri::InvalidUrl )
    {
        if( networkUriType == NetworkUri::Domain )
        {
            QDBusReply<Mollet::NetDeviceList> reply = mNetworkDBusProxy->deviceDataList();

kDebug()<<reply.isValid();
            if( reply.isValid() ) // TODO: find how a not found service can be expressed in the reply
            {
                Mollet::NetDeviceList deviceDataList = reply.value();

                foreach( const Mollet::NetDevice& deviceData, deviceDataList )
                {
                    KIO::UDSEntry entry;
                    feedEntryAsDevice( &entry, deviceData );
                    listEntry( entry, false );
                }
                KIO::UDSEntry entry;
                listEntry( entry, true );
                finished();
                successfulListing = true;
            }
        }
        else
        {
            const QString hostAddress = networkUri.hostAddress();
            if( networkUriType == NetworkUri::Device )
            {
                QDBusReply<Mollet::NetServiceList> reply = mNetworkDBusProxy->serviceDataList( hostAddress );

kDebug()<<reply.isValid();
                if( reply.isValid() ) // TODO: find how a not found service can be expressed in the reply
                {
                    Mollet::NetServiceList serviceDataList = reply.value();

                    foreach( const Mollet::NetService& serviceData, serviceDataList )
                    {
                        KIO::UDSEntry entry;
                        feedEntryAsService( &entry, serviceData );
                        listEntry( entry, false );
                    }
                    KIO::UDSEntry entry;
                    listEntry( entry, true );
                    finished();
                    successfulListing = true;
                }
            }
            else if( networkUriType == NetworkUri::Service )
            {
                const QString serviceName = networkUri.serviceName();
                const QString serviceType = networkUri.serviceType();
                QDBusReply<Mollet::NetService> reply = mNetworkDBusProxy->serviceData( hostAddress, serviceName, serviceType );

kDebug()<<reply.isValid();
                if( reply.isValid() ) // TODO: find how a not found service can be expressed in the reply
                {
                    Mollet::NetService serviceData = reply.value();
                    if( serviceData.isValid() )
                    {
                        const QString url = serviceData.url();
                        redirection( url );
                        finished();
                        successfulListing = true;
                    }
                }
            }
        }
    }

    if( ! successfulListing )
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
}


void NetworkSlave::feedEntryAsNetwork( KIO::UDSEntry* entry )
{
    entry->insert( KIO::UDSEntry::UDS_FILE_TYPE,    S_IFDIR );
//     entry->insert( KIO::UDSEntry::UDS_ICON_NAME,    NetworkIconName );
    entry->insert( KIO::UDSEntry::UDS_MIME_TYPE,    Mimetypes::NetworkMimetype );

}

void NetworkSlave::feedEntryAsDevice( KIO::UDSEntry* entry, const Mollet::NetDevice& deviceData )
{
    entry->insert( KIO::UDSEntry::UDS_NAME,         deviceData.hostAddress() );
    entry->insert( KIO::UDSEntry::UDS_DISPLAY_NAME, deviceData.name() );
    entry->insert( KIO::UDSEntry::UDS_FILE_TYPE,    S_IFDIR );
//     entry->insert( KIO::UDSEntry::UDS_ICON_NAME,    NetDevice::iconName(deviceData.type()) );
    entry->insert( KIO::UDSEntry::UDS_MIME_TYPE,    Mimetypes::DeviceMimetype[deviceData.type()] );

}

void NetworkSlave::feedEntryAsService( KIO::UDSEntry* entry, const Mollet::NetService& serviceData )
{
    entry->insert( KIO::UDSEntry::UDS_NAME,         serviceData.name()+'.'+serviceData.type() );
    entry->insert( KIO::UDSEntry::UDS_DISPLAY_NAME, serviceData.name() );
    entry->insert( KIO::UDSEntry::UDS_FILE_TYPE,    S_IFLNK );
    entry->insert( KIO::UDSEntry::UDS_ACCESS,       S_IRWXU|S_IRWXG|S_IRWXO );
//     entry->insert( KIO::UDSEntry::UDS_ICON_NAME,    serviceData.iconName() );
    entry->insert( KIO::UDSEntry::UDS_MIME_TYPE,    Mimetypes::mimetypeForServiceType(serviceData.type()) );
    if( !serviceData.url().isEmpty() )
        entry->insert( KIO::UDSEntry::UDS_TARGET_URL, serviceData.url() );
}

void NetworkSlave::reportError( const NetworkUri& networkUri, int errorId )
{
Q_UNUSED( networkUri )
Q_UNUSED( errorId )
}

NetworkSlave::~NetworkSlave()
{
    delete mNetworkDBusProxy;
}
