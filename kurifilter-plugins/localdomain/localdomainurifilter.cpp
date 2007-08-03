/*
    localdomainfilter.cpp

    This file is part of the KDE project
    Copyright (C) 2002 Lubos Lunak <llunak@suse.cz>

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

#include "localdomainurifilter.h"

#include <k3process.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include <QtDBus/QtDBus>

#include <QRegExp>
#include <QFile>

#define HOSTPORT_PATTERN "[a-zA-Z0-9][a-zA-Z0-9+-]*(?:\\:[0-9]{1,5})?(?:/[\\w:@&=+$,-.!~*'()]*)*"

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * kdelibs/kio/tests/kurifiltertest
 */

LocalDomainUriFilter::LocalDomainUriFilter( QObject *parent, const QStringList & /*args*/ )
    : KUriFilterPlugin( "localdomainurifilter", parent ),
      last_time( 0 ),
      m_hostPortPattern( QLatin1String(HOSTPORT_PATTERN) )
{
    QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KUriFilterPlugin",
                                "configure", this, SLOT(configure()));
    configure();
}

bool LocalDomainUriFilter::filterUri( KUriFilterData& data ) const
{
    KUrl url = data.uri();
    QString cmd = url.url();

    kDebug() << "LocalDomainUriFilter::filterUri: " << url;

    if( m_hostPortPattern.exactMatch( cmd ) &&
        isLocalDomainHost( cmd ) )
    {
        cmd.prepend( QLatin1String("http://") );
        setFilteredUri( data, KUrl( cmd ) );
        setUriType( data, KUriFilterData::NET_PROTOCOL );

        kDebug() << "FilteredUri: " << data.uri();
        return true;
    }

    return false;
}

// if it's e.g. just 'www', try if it's a hostname in the local search domain
bool LocalDomainUriFilter::isLocalDomainHost( QString& cmd ) const
{
    // find() returns -1 when no match -> left()/truncate() are noops then
    QString host( cmd.left( cmd.indexOf( '/' ) ) );
    host.truncate( host.indexOf( ':' ) ); // Remove port number

    if( !(host == last_host && last_time > time( NULL ) - 5 ) ) {

        QString helper = KStandardDirs::findExe(QLatin1String( "klocaldomainurifilterhelper" ));
        if( helper.isEmpty())
            return last_result = false;

        m_fullname.clear();

        K3Process proc;
        proc << helper << host;
        connect( &proc, SIGNAL(receivedStdout(K3Process *, char *, int)),
                 SLOT(receiveOutput(K3Process *, char *, int)) );
        if( !proc.start( K3Process::NotifyOnExit, K3Process::Stdout ))
            return last_result = false;

        last_host = host;
        last_time = time( (time_t *)0 );

        last_result = proc.wait( 1 ) && proc.normalExit() && !proc.exitStatus();

        if( !m_fullname.isEmpty() )
            cmd.replace( 0, host.length(), m_fullname );
    }

    return last_result;
}

void LocalDomainUriFilter::receiveOutput( K3Process *, char *buf, int )
{
    m_fullname = QFile::decodeName( buf );
}

void LocalDomainUriFilter::configure()
{
    // nothing
}

K_EXPORT_COMPONENT_FACTORY( liblocaldomainurifilter,
                            KGenericFactory<LocalDomainUriFilter>( "kcmkurifilt" ) )

#include "localdomainurifilter.moc"
