/*
    This file is part of the KUPnP library, part of the KDE project.

    Copyright 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy 
    defined in Section 14 of version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// lib
#include <devicebrowser.h>
#include <device.h>
#include <service.h>
// KDE
#include <KApplication>
#include <KCmdLineArgs>
#include <KAboutData>
// Qt
#include <QtCore/QTextStream>
#include <QtCore/QTimer>


int main( int argc, char* argv[] )
{
    KAboutData aboutData( "kupnpbrowser", "KUpnpBrowser", ki18n(""), "", ki18n("") );
    KCmdLineArgs::init( argc, argv, &aboutData );
    KApplication programCore;

    UPnP::DeviceBrowser deviceBrowser;
    const QList<UPnP::Device> devices = deviceBrowser.devices();

    QTextStream out( stdout );
    foreach( const UPnP::Device& device, devices )
    {
        const QList<UPnP::Service> services = device.services();

        out << device.displayName() << " ("<<device.type()<<")"<<endl;
        foreach( const UPnP::Service& service, services )
            out << "* " << service.displayName() << " ("<<service.type()<<")"<<endl;
    }

    // run loop at least once
    QTimer::singleShot( 0, &programCore, SLOT(quit()) );
    return programCore.exec();
}
