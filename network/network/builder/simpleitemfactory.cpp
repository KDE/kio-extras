/*
    This file is part of the Mollet network library, part of the KDE project.

    Copyright 2009-2010 Friedrich W. H. Kossebau <kossebau@kde.org>

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

#include "simpleitemfactory.h"

// lib
#include "netservice_p.h"
#include "service.h"
#include "device.h"
// KDE
#include <KUrl>


namespace Mollet
{

struct DNSSDServiceDatum {
    const char* dnssdTypeName;
    const char* typeName;
    const char* iconName;
    bool isFilesystem;
    const char* protocol; // KDE can forward to it
    const char* pathField;
    const char* userField;
    const char* passwordField;

    void feedUrl( KUrl* url, const DNSSD::RemoteService* remoteService ) const;
};

static const DNSSDServiceDatum DNSSDServiceData[] =
{
    // file services
    { "_ftp._tcp",         "ftp",        "folder-remote",  true, "ftp",     "path", "u", "p" },
    { "_nfs._tcp",         "nfs",        "folder-remote",  true, "nfs",     "path", 0, 0 },
    { "_afpovertcp._tcp",  "afpovertcp", "folder-remote",  true, "afp",     "path", 0, 0 },
    { "_smb._tcp",         "smb",        "folder-remote",  true, "smb",     "path", "u", "p" },
    { "_webdav._tcp",      "webdav",     "folder-remote",  true, "webdav",  "path", "u", "p" },
    { "_webdavs._tcp",     "webdavs",    "folder-remote",  true, "webdavs", "path", "u", "p" },
    { "_sftp-ssh._tcp",    "sftp-ssh",   "folder-remote",  true, "sftp",    0,      "u", "p" },
    { "_ftps._tcp",        "ftps",       "folder-remote",  true, "ftps",    "path", "u", "p" },

    { "_svn._tcp",    "svn",   "folder-sync",  true, 0, 0, 0, 0 },
    { "_rsync._tcp",  "rsync", "folder-sync",  true, 0, 0, 0, 0 },

    // email
    { "_imap._tcp",   "imap",   "email",  false, 0, 0, 0, 0 },
    { "_pop3._tcp",   "pop3",   "email",  false, "pop3", 0, 0, 0 },

    //  shell services
    { "_ssh._tcp",    "ssh",    "terminal",  false, "ssh",    0,      "u", "p" },
    { "_telnet._tcp", "telnet", "terminal",  false, "telnet", 0,      "u", "p" },
    { "_rfb._tcp",    "rfb",    "krfb",      false, "vnc",    "path", "u", "p" },
    { "_rdp._tcp",    "rdp",    "krfb",      false, "rdp", 0, 0, 0 },

    // other standard services
    { "_ipp._tcp",    "ipp",    "printer",                false, "ipp",  "path", "u", "p" },
    { "_http._tcp",   "http",   "folder-html",            false, "http", "path", "u", "p" },
    { "_ntp._udp",    "ntp",    "xclock",                 false, 0, 0, 0, 0 },
    { "_ldap._tcp",   "ldap",   "user-group-properties",  false, "ldap", 0, 0, 0 },
    { "_pgpkey-ldap._tcp",  "pgpkey-hkp",   "application-pgp-keys",  false, 0, 0, 0, 0 },
    { "_pgpkey-ldap._tcp",  "pgpkey-hkp",   "application-pgp-keys",  false, 0, 0, 0, 0 },
    { "_pgpkey-hkp._tcp",   "pgpkey-hkp",   "application-pgp-keys",  false, 0, 0, 0, 0 },
    { "_pgpkey-https._tcp", "pgpkey-https", "application-pgp-keys",  true, "https", "path", 0, 0 },
    { "_presence._tcp",     "presence",     "im-user",               false, "presence", 0, 0, 0 },
    { "_printer._tcp",        "printer",        "printer", false, 0, 0, 0, 0 },
    { "_pdl-datastream._tcp", "pdl-datastream", "printer", false, 0, 0, 0, 0 },

    // Apple
    { "_airport._tcp",       "airport",       "network-wireless",  false, 0, 0, 0, 0 },
    { "_daap._tcp",          "daap",          "folder-sound",      false, 0, 0, 0, 0 },
    { "_eppc._tcp",          "eppc",          "network-connect",   false, 0, 0, 0, 0 },
    { "_net-assistant._udp", "net-assistant", "services",          false, 0, 0, 0, 0 },
    { "_odisk._tcp",         "odisk",         "media-optical",     false, 0, 0, 0, 0 },
    { "_raop._tcp",          "raop",          "speaker",           false, 0, 0, 0, 0 },
    { "_touch-able._tcp",    "touch-able",    "input-tablet",      false, 0, 0, 0, 0 },
    { "_workstation._tcp",   "workstation",   "network-workgroup", false, 0, 0, 0, 0 },
    { "_sleep-proxy._udp",   "sleep-proxy",   "services",          false, 0, 0, 0, 0 },
    { "_nssocketport._tcp",  "nssocketport",  "services",          false, 0, 0, 0, 0 },
    { "_home-sharing._tcp",  "home-sharing",  "services",          false, 0, 0, 0, 0 },

    // KDE workspace
    { "_plasma._tcp",       "plasma",      "plasma",       false, "plasma", 0, 0, 0 },

    // KDE games
    // TODO: http as protocol is a fake, but the KIO system currently only supports known protocols, so we pretend it is http
    { "_kbattleship._tcp",  "kbattleship", "kbattleship",  false, "kbattleship", 0, 0, 0 },
    { "_lskat._tcp",        "lskat",       "lskat",        false, "http", 0, 0, 0 },
    { "_kfourinline._tcp",  "kfourinline", "kfourinline",  false, "http", 0, 0, 0 },
    { "_ksirk._tcp",        "ksirk",       "ksirk",        false, "http", 0, 0, 0 },

    // hardware
    { "_pulse-server._tcp","pulse-server","audio-card",          false, 0, 0, 0, 0 },
    { "_pulse-source._tcp","pulse-source","audio-input-line",    false, 0, 0, 0, 0 },
    { "_pulse-sink._tcp",  "pulse-sink",  "speaker",             false, 0, 0, 0, 0 },
    { "_libvirt._tcp",     "libvirt",     "computer",            false, 0, 0, 0, 0 },
    { "_airmouse._tcp",    "airmouse",    "input-mouse",         false, 0, 0, 0, 0 },

    // else
    { "_postgresql._tcp",  "postgresql",  "server-database",     false, 0, 0, 0, 0 },
    { "_xmpp-server._tcp", "xmpp-server", "xchat",               false, "jabber", 0, 0, 0 },
    { "_lobby._tcp",       "lobby",       "document-edit",       false, 0, 0, 0, 0 },
    { "_giver._tcp",       "giver",       "folder-remote",       false, 0, 0, 0, 0 },
    { "_realplayfavs._tcp","realplayfavs","favorites",           false, 0, 0, 0, 0 },
    { "_acrobatSRV._tcp",  "acrobat-server","application-pdf",   false, 0, 0, 0, 0 },
    { "_adobe-vc._tcp",    "adobe-vc",    "services",   false, 0, 0, 0, 0 },
    { "_ggz._tcp",         "ggz",         "applications-games",  false, "ggz", 0, 0, 0 }
};
//     result["_ssh._tcp"]=      DNSSDNetServiceBuilder(i18n("Remote disk (fish)"),     "fish",   "service/ftp", QString(), "u", "p");
// network-server-database icon
// see             // SubEthaEdit 2
//                 Defined TXT keys: txtvers=1, name=<Full Name>, userid=<User ID>, version=2

static const int DNSSDServiceDataSize = sizeof( DNSSDServiceData ) / sizeof( DNSSDServiceData[0] );

static const DNSSDServiceDatum UnknownServiceDatum = { "", "unknown", "unknown", false, 0, 0, 0, 0 };

// TODO:
// * find out how ws (webservices, _ws._tcp), upnp (_upnp._tcp) are exactly meant to be used
// * learn about uddi
// * see how the apple specific protocols can be used for devicetype identification (printer, scanner)


void DNSSDServiceDatum::feedUrl( KUrl* url, const DNSSD::RemoteService* remoteService ) const
{
    const QMap<QString,QByteArray> serviceTextData = remoteService->textData();

    url->setProtocol( QString::fromLatin1(protocol) );
    if( userField )
        url->setUser( serviceTextData[QString::fromLatin1(userField)] );
    if( passwordField )
        url->setPass( serviceTextData[QString::fromLatin1(passwordField)] );
    if( pathField )
        url->setPath( serviceTextData[QString::fromLatin1(pathField)] );

    url->setHost( remoteService->hostName() );
    url->setPort( remoteService->port() );
}


SimpleItemFactory::SimpleItemFactory()
  : AbstractNetSystemFactory()
{
}


bool SimpleItemFactory::canCreateNetSystemFromDNSSD( const QString& serviceType ) const
{
Q_UNUSED( serviceType )
    return true;
}


NetServicePrivate* SimpleItemFactory::createNetService( const DNSSD::RemoteService::Ptr& dnssdService, const NetDevice& device ) const
{
    NetServicePrivate* result;

    const QString dnssdServiceType = dnssdService->type();

    const DNSSDServiceDatum* serviceDatum = &UnknownServiceDatum;
    for( int i = 0; i< DNSSDServiceDataSize; ++i )
    {
        const DNSSDServiceDatum* datum = &DNSSDServiceData[i];
        if( dnssdServiceType == QLatin1String(datum->dnssdTypeName) )
        {
            serviceDatum = datum;
            break;
        }
    }

    KUrl url;
    if( serviceDatum->protocol )
        serviceDatum->feedUrl( &url, dnssdService.data() );

    const bool isUnknown = ( serviceDatum == &UnknownServiceDatum );
    const QString typeName = isUnknown ?
        dnssdServiceType.mid( 1, dnssdServiceType.lastIndexOf('.')-1 ) :
        QString::fromLatin1( serviceDatum->typeName );
    result = new NetServicePrivate( dnssdService->serviceName(), QString::fromLatin1(serviceDatum->iconName),
        typeName, device, url.url() );

    return result;
}

bool SimpleItemFactory::canCreateNetSystemFromUpnp( const UPnP::Device& upnpDevice ) const
{
Q_UNUSED( upnpDevice )
    return true;
}
// TODO: add KIcon with specialiced KIconLoader (fetches Icons via D-Bus)
NetServicePrivate* SimpleItemFactory::createNetService( const UPnP::Device& upnpDevice, const NetDevice& device ) const
{
    NetServicePrivate* result;

    QString url = upnpDevice.presentationUrl();
    if( url.isEmpty() )
    {
        url = QString::fromLatin1( "upnp://" );
        url.append( upnpDevice.udn() );
    }
    result = new NetServicePrivate( upnpDevice.displayName(), QString::fromLatin1("unknown"),
        "upnp."+upnpDevice.type(), device, url );

    return result;
}

SimpleItemFactory::~SimpleItemFactory() {}

}
