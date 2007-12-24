/*
    localdomainfilter.cpp

    This file is part of the KDE project
    Copyright (C) 2002 Lubos Lunak <llunak@suse.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "localdomainurifilter.h"

#include <KProcess>
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

LocalDomainUriFilter::LocalDomainUriFilter( QObject *parent, const QVariantList & /*args*/ )
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
    const KUrl url = data.uri();
    QString cmd = url.url();

    kDebug() << url;

    if( m_hostPortPattern.exactMatch( cmd ) &&
        isLocalDomainHost( cmd ) )
    {
        cmd.prepend( QLatin1String("http://") );
        setFilteredUri( data, KUrl( cmd ) );
        setUriType( data, KUriFilterData::NetProtocol );

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

        KProcess proc;
        proc << helper << host;
        proc.start();
        if( !proc.waitForStarted( 1000 ) )
            return last_result = false;

        last_host = host;
        last_time = time( (time_t *)0 );

        last_result = proc.waitForFinished( 1000 ) && proc.exitCode() == QProcess::NormalExit;

        const QString fullname = QFile::decodeName( proc.readAllStandardOutput() );

        if( !fullname.isEmpty() )
            cmd.replace( 0, host.length(), fullname );
    }

    return last_result;
}

void LocalDomainUriFilter::configure()
{
    // nothing
}

K_PLUGIN_FACTORY(LocalDomainUriFilterFactory, registerPlugin<LocalDomainUriFilter>();)
K_EXPORT_PLUGIN(LocalDomainUriFilterFactory("kcmkurifilt"))

#include "localdomainurifilter.moc"
