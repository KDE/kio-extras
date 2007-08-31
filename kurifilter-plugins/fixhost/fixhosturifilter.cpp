/*
    fixhostfilter.cpp

    This file is part of the KDE project
    Copyright (C) 2007 Lubos Lunak <llunak@suse.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <config-runtime.h>

#include "fixhosturifilter.h"

#include <kdebug.h>
#include <kurl.h>
#include <k3resolver.h>

using namespace KNetwork;

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * kdelibs/kio/tests/kurifiltertest
 */
 
FixHostUriFilter::FixHostUriFilter( QObject *parent, const QVariantList & /*args*/ )
    : KUriFilterPlugin( "fixhosturifilter", parent )
{
}

bool FixHostUriFilter::filterUri( KUriFilterData& data ) const
{
    KUrl url = data.uri();
    QString cmd = url.url();
    
    kDebug() << "FixHostUriFilter::filterUri: " << url;

    KUrl url2 = url;
    url2.setHost( "www." + url.host());

    if(( url.protocol() == "http" || url.protocol() == "https" )    
        && !url.host().startsWith( "www." ) && !exists( url ) && exists( url2 ))
    {
        setFilteredUri( data, url2 );
        setUriType( data, KUriFilterData::NET_PROTOCOL );
        
        kDebug() << "FilteredUri: " << data.uri();
        return true;
    }

    return false;
}

bool FixHostUriFilter::exists( const KUrl& url )
{
    KResolver resolver( url.host());
    resolver.setFamily( KResolver::InetFamily );
    return( resolver.start() && resolver.wait( 5000 ) && resolver.error() == KResolver::NoError );
}

K_PLUGIN_FACTORY(FixHostUriFilterFactory, registerPlugin<FixHostUriFilter>();)
K_EXPORT_PLUGIN(FixHostUriFilterFactory("kcmkurifilt"))

#include "fixhosturifilter.moc"
