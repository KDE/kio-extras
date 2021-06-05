/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kioslavenotifier.h"

// module
#include "kioslavenotifieradaptor.h"
// kioslave
#include <networkuri.h>
// network
#include <network.h>
#include <netdevice.h>
#include <netservice.h>
// KF
#include <kdirnotify.h>
// Qt
#include <QStringList>

#include <QDebug>


namespace Mollet
{

static inline QString idFrom( const NetworkUri& networkUri )
{
    return networkUri.hostAddress().isEmpty() ?    QString() :
           networkUri.serviceName().isEmpty() ? networkUri.hostAddress() :
           /*else*/                             networkUri.hostAddress()+QLatin1Char('/')+networkUri.serviceName();
}

static inline QString dirIdFor( const NetDevice& device )
{
    Q_UNUSED( device )
    return QString();
}
static inline QString pathFor( const NetDevice& device )
{
    return device.hostAddress();
}

static inline QString dirIdFor( const NetService& service )
{
    return service.device().hostAddress();
}

static inline QString pathFor( const NetService& service )
{
    return service.device().hostAddress() + QLatin1Char('/') + service.name()+QLatin1Char('.')+service.type();
}


KioSlaveNotifier::KioSlaveNotifier( Network* network, QObject* parent )
    : QObject( parent )
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    const QString allServices;
    const QString allPaths;
    const QString interface = QLatin1String( "org.kde.KDirNotify" );
    sessionBus.connect( allServices, allPaths, interface, QLatin1String("enteredDirectory"),
                        this, SLOT(onDirectoryEntered(QString)) );
    sessionBus.connect( allServices, allPaths, interface, QLatin1String("leftDirectory"),
                        this, SLOT(onDirectoryLeft(QString)) );

    new KioSlaveNotifierAdaptor( this );

    connect( network, SIGNAL(devicesAdded(QList<NetDevice>)), SLOT(onDevicesAdded(QList<NetDevice>)) );
    connect( network, SIGNAL(devicesRemoved(QList<NetDevice>)), SLOT(onDevicesRemoved(QList<NetDevice>)) );
    connect( network, SIGNAL(servicesAdded(QList<NetService>)), SLOT(onServicesAdded(QList<NetService>)) );
    connect( network, SIGNAL(servicesRemoved(QList<NetService>)), SLOT(onServicesRemoved(QList<NetService>)) );
}

QStringList KioSlaveNotifier::watchedDirectories() const
{
    return mWatchedDirs.keys();
}


void KioSlaveNotifier::onDirectoryEntered( const QString& directory )
{
    if( !directory.startsWith(QLatin1String("network:/")) )
        return;

    const NetworkUri networkUri(( QUrl(directory) ));
    const QString id = idFrom( networkUri );

    QHash<QString, int>::Iterator it = mWatchedDirs.find( id );

    if( it == mWatchedDirs.end() )
    {
        mWatchedDirs.insert( id, 1 );
    }
    else
        (*it)++;
}


void KioSlaveNotifier::onDirectoryLeft( const QString& directory )
{
    if( !directory.startsWith(QLatin1String("network:/")) )
        return;

    const NetworkUri networkUri(( QUrl(directory) ));
    const QString id = idFrom( networkUri );

    QHash<QString, int>::Iterator it = mWatchedDirs.find( id );

    if( it == mWatchedDirs.end() )
        return;

    if( *it == 1 )
        mWatchedDirs.erase( it );
    else
        (*it)--;
}


void KioSlaveNotifier::notifyAboutAdded( const QString& dirId )
{
    QHash<QString, int>::Iterator it = mWatchedDirs.find( dirId );
    if( it != mWatchedDirs.end() )
    {
        const QUrl url( QLatin1String("network:/") + dirId );
        org::kde::KDirNotify::emitFilesAdded( url );
    }
}

void KioSlaveNotifier::notifyAboutRemoved( const QString& dirId, const QString& itemPath )
{
    QHash<QString, int>::Iterator it = mWatchedDirs.find( dirId );
    if( it != mWatchedDirs.end() )
    {
        QList<QUrl> itemUrls;
        itemUrls.append( QUrl( QStringLiteral("network:/") + itemPath ) );
        org::kde::KDirNotify::emitFilesRemoved( itemUrls );
    }
}


void KioSlaveNotifier::onDevicesAdded( const QList<NetDevice>& deviceList )
{
    for (const NetDevice& device : deviceList) {
        const QString id = dirIdFor( device );
        notifyAboutAdded( id );
    }
}

void KioSlaveNotifier::onDevicesRemoved( const QList<NetDevice>& deviceList )
{
    for (const NetDevice& device : deviceList) {
        const QString dirId = dirIdFor( device );
        const QString itemPath = pathFor( device );
        notifyAboutRemoved( dirId, itemPath );
    }
}


void KioSlaveNotifier::onServicesAdded( const QList<NetService>& serviceList )
{
    for (const NetService& service : serviceList) {
        const QString id = dirIdFor( service );
        notifyAboutAdded( id );
    }
}


void KioSlaveNotifier::onServicesRemoved( const QList<NetService>& serviceList )
{
    for (const NetService& service : serviceList) {
        const QString dirId = dirIdFor( service );
        const QString itemPath = pathFor( service );
        notifyAboutRemoved( dirId, itemPath );
    }
}

KioSlaveNotifier::~KioSlaveNotifier()
{
}

}
