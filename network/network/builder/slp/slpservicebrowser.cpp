/*
    This file is part of the Mollet network library, part of the KDE project.

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

#include "slpservicebrowser.h"

// Qt
#include <QtCore/QStringList>

#include <KDebug>

#ifndef Q_OS_WIN
extern "C"
{
    /* let it still work, even someone installed a non-SuSE openslp */
    SLPEXP const char* SLPAPI SLPGetMDNSName( SLPHandle hSLP, const char* pcURL ) __attribute__ ((weak));
}
#endif

namespace Mollet
{

static const char DefaultScope[] =         "";
static const char AllNamingAuthorities[] = '*';
static const char AllServiceFilter[] =     "";
static const char AllAttributesFilter[] =  "";
static const int updateIntervalMSecs = 7000;

// crasht
// zum einen darf man eine slphandle nur für einen api call nutzen, idee unten geht also nicht.


SlpServiceBrowser::SlpServiceBrowser()
{
    SLPError result = SLPOpen( 0, SLP_FALSE, &mSLP );

    if( result != SLP_OK )
        // TODO: emit error somewhere
        return;

    scan();
    startTimer( updateIntervalMSecs );
}

void SlpServiceBrowser::scan()
{
    mCurrentServiceList.resetState();

    // list services
    SLPFindSrvTypes( mSLP, AllNamingAuthorities, DefaultScope, onServiceTypesFound, this );

    // detect changes
    QList<SLPService> newServices;
    QList<SLPService> changedServices;
    QList<SLPService> unknownServices;

    QHash<QString,SLPService>::Iterator it = mCurrentServiceList.begin();
    while( it != mCurrentServiceList.end() )
    {
        const SLPService::State state = it->state();
        switch( state )
        {
        case SLPService::New:
            newServices.append( *it );
            ++it;
            break;
        case SLPService::Changed:
            changedServices.append( *it );
            ++it;
            break;
        case SLPService::Unknown:
            unknownServices.append( *it );
            it = mCurrentServiceList.erase( it );
            break;
        case SLPService::Unchanged:
        default:
            ++it;
        }
    }

    if( ! newServices.isEmpty() )
        emit servicesAdded( newServices );
    if( ! changedServices.isEmpty() )
        emit servicesChanged( changedServices );
    if( ! unknownServices.isEmpty() )
        emit servicesRemoved( unknownServices );
}

void SlpServiceBrowser::timerEvent( QTimerEvent* event )
{
Q_UNUSED( event )
    scan();
}


inline bool splitSlpServiceUrl( QString* serviceType, QString* siteNParameter, const char* serviceUrl )
{
Q_UNUSED( serviceType )
Q_UNUSED( siteNParameter )

    bool result;

    QString url( serviceUrl );
    if( url.startsWith(QLatin1String("service:")) )
        url.remove( 0, 8 );

    result = false;//( matcher->indexIn(url) >= 0 );
    // TODO: when can a service url not match?
    if ( result )
    {
//         *serviceType = matcher->cap( 1 );
//         *siteNParameter = matcher->cap( 2 );
    }

    return result;
}

SLPBoolean SlpServiceBrowser::onServiceTypesFound( SLPHandle hslp, const char* serviceTypes, SLPError errorCode, void* builder )
{
kDebug();

    if( errorCode == SLP_LAST_CALL )
        return SLP_FALSE;

    // problems to ignore for now
    if ( !serviceTypes || !*serviceTypes || errorCode != SLP_OK )
        return SLP_TRUE;

    // optimize by operating on original data as much as possible
    const QStringList serviceTypeList = QString::fromLatin1( serviceTypes ).split( ',' );

    foreach( const QString& serviceType, serviceTypeList )
    {
kDebug()<<serviceType;
        //  "service:"<abstract-type.naming-authority>":"<concrete-type>
continue;
        SLPFindSrvs( hslp, serviceType.toLatin1(), DefaultScope, AllServiceFilter, onServiceFound, builder );
    }

    return SLP_TRUE;
}

SLPBoolean SlpServiceBrowser::onServiceFound( SLPHandle hslp, const char* serviceUrl, unsigned short lifetime, SLPError errorCode, void* builder )
{
Q_UNUSED( lifetime )

    if( errorCode == SLP_LAST_CALL )
        return SLP_FALSE;

    // problems to ignore for now
    if ( !serviceUrl || !*serviceUrl || errorCode != SLP_OK )
        return SLP_TRUE;

    QString serviceType;
    QString siteNParameter;
//     const bool success = ::splitSlpServiceUrl( &serviceType, &siteNParameter, serviceUrl );

//     if( success )
    {
// #ifndef Q_OS_WIN
//         const QString mdnsName = ( SLPGetMDNSName ) ? QString::fromUtf8(SLPGetMDNSName(hslp, serviceUrl)) : QString();
// #endif

        QString attributeList;
        SLPFindAttrs( hslp, serviceUrl, DefaultScope, AllAttributesFilter, onAttributesFound, &attributeList );

        const QLatin1String name( serviceUrl );
        static_cast<SlpServiceBrowser*>( builder )->mCurrentServiceList.checkService( serviceUrl, name, attributeList );

    }

    return SLP_TRUE;
}

SLPBoolean SlpServiceBrowser::onAttributesFound( SLPHandle hslp, const char* attributeList, SLPError errorCode, void* _attributeList )
{
Q_UNUSED( hslp )

    if( errorCode != SLP_OK )
        return SLP_FALSE;

    // TODO: find out how long attrlist is valid and if we could not just use a const char* pointer until onServiceFound
    *static_cast<QString*>( _attributeList ) = QString::fromUtf8( attributeList ); //right for mdns at least
#if 0
parse only on change in scan()
    // TODO: speed up by not resetting l but tracking the current index
    while ( !l.isEmpty() )
    {
        if ( l[0] == '(' )
        {
            const int indexOfBracket = l.indexOf( ')' );
            QString m = l.mid( 1, indexOfBracket-1 );
            const int indexOfEquals = m.indexOf( '=' );
//             attributeHtml += m.left( indexOfEquals );
            m.remove( 0, indexOfEquals+1 );
//             attributeHtml += m.split( ',' ).join("<br>");

            l.remove( 0, indexOfBracket+2 );
        }
        else
        {
            const int indexOfComma = l.indexOf( ',' );
            QString m = l.left( indexOfComma );

            l.remove( 0, indexOfComma+1 );
        }
    }

//     static_cast<SlpServiceBrowser*>( builder )->reportAttributes();
#endif
    // signal no further info needed, as attributes were just asked for one service
    return SLP_FALSE;
}


SlpServiceBrowser::~SlpServiceBrowser()
{
    SLPClose( mSLP );
}

}
