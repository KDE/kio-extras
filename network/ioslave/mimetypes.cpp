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

#include "mimetypes.h"

// Qt
#include <QtCore/QString>


const char Mimetypes::NetworkMimetype[] = "inode/vnd.kde.network";

// keep in sync with Mollet::NetDevice::Type
const char* const Mimetypes::DeviceMimetype[] =
{
    "inode/vnd.kde.device.unknown",
    "inode/vnd.kde.device.scanner",
    "inode/vnd.kde.device.printer",
    "inode/vnd.kde.device.server",
    "inode/vnd.kde.device.router",
    "inode/vnd.kde.device.workstation"
};


struct MimetypePair {
    const char* typeName;
    const char* mimetype;
};


// keep in sync with network.xml
static const char* const SimpleServiceMimetype[] =
{
    "ftp",
    "sftp-ssh",
    "ftps",
    "nfs",
    "afpovertcp",
    "smb",
    "webdav",
    "webdavs",

    "svn",
    "rsync",

    "pop3",
    "imap",

    "ssh",
    "telnet",
    "rfb",
    "rdp",

    "http",
    "ntp",
    "ldap",

    "xmpp-server",
    "presence",
    "lobby",
    "giver",
    "sip",
    "h323",
    "skype",

    "ipp",
    "printer",
    "pdl-datastream",

    "plasma",

    "kbattleship",
    "lskat",
    "kfourinline",
    "ksirk",

    "pulse-server",
    "pulse-source",
    "pulse-sink",
    "udisks-ssh",
    "libvirt",
    "airmouse",

    "postgresql",
    "couchdb_location",

    "realplayfavs",
    "acrobat-server",
    "adobe-vc",
    "ggz",

    "pgpkey-ldap",
    "pgpkey-hkp",
    "pgpkey-https",

    "maemo-inf",

    "airport",
    "daap",
    "dacp",
    "eppc",
    "net-assistant",
    "odisk",
    "raop",
    "touch-able",
    "workstation",
    "sleep-proxy",
    "nssocketport",
    "home-sharing",
    "appletv-itunes",
    "appletv-pair",

    "upnp.BasicDevice1",
    "upnp.WLANAccessPointDevice1",
    "upnp.InternetGatewayDevice1",
    "upnp.PrinterBasic1",
    "upnp.PrinterEnhanced1",
    "upnp.Scanner1",
    "upnp.MediaServer1",
    "upnp.MediaRenderer1",
    "upnp.MediaServer2",
    "upnp.MediaRenderer2",
    "upnp.MediaServer3",
    "upnp.SolarProtectionBlind1",
    "upnp.DigitalSecurityCamera1",
    "upnp.HVAC1",
    "upnp.LightingControls1",
    "upnp.RemoteUIClientDevice1",
    "upnp.RemoteUIServerDevice1",
    "upnp.RAClient1",
    "upnp.RAServer1",
    "upnp.RADiscoveryAgent1",
    "upnp.LANDevice1",
    "upnp.WANDevice1",
    "upnp.WANConnectionDevice1",
    "upnp.WFADevice1",
    "upnp.Unknown"
};
static const int SimpleServiceMimetypeCount = sizeof(SimpleServiceMimetype) / sizeof(SimpleServiceMimetype[0]);

QString Mimetypes::mimetypeForServiceType( const QString& serviceTypeName )
{
    QString subType = "unknown";
    for( int i=0; i<SimpleServiceMimetypeCount; ++i )
    {
        if( serviceTypeName == QLatin1String(SimpleServiceMimetype[i]) )
        {
            subType = serviceTypeName;
            break;
        }
    }
    return "inode/vnd.kde.service." + subType;
}
