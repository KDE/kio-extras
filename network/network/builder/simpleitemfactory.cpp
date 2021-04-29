/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009-2010 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "simpleitemfactory.h"

// lib
#include "netservice_p.h"
// #include "service.h"
#include "upnp/cagibidevice.h"

// Qt
#include <QIcon>

// KDE
#include <QUrl>

#include <QDebug>

namespace Mollet
{

struct DNSSDServiceDatum {
    const char* dnssdTypeName;
    const char* typeName;
    const char* fallbackIconName;
    bool isFilesystem;
    const char* protocol; // KDE can forward to it
    const char* pathField;
    const char* iconField;
    const char* userField;
    const char* passwordField;

    void feedUrl( QUrl* url, const KDNSSD::RemoteService* remoteService ) const;
};

static const DNSSDServiceDatum DNSSDServiceData[] =
{
    // file services
    { "_ftp._tcp",         "ftp",        "folder-remote",  true, "ftp",     "path", nullptr, "u", "p" },
    { "_sftp-ssh._tcp",    "sftp-ssh",   "folder-remote",  true, "sftp",    nullptr,      nullptr, "u", "p" },
    { "_ftps._tcp",        "ftps",       "folder-remote",  true, "ftps",    "path", nullptr, "u", "p" },
    { "_nfs._tcp",         "nfs",        "folder-remote",  true, "nfs",     "path", nullptr, nullptr, nullptr },
    { "_afpovertcp._tcp",  "afpovertcp", "folder-remote",  true, "afp",     "path", nullptr, nullptr, nullptr },
    { "_smb._tcp",         "smb",        "folder-remote",  true, "smb",     "path", nullptr, "u", "p" },
    { "_webdav._tcp",      "webdav",     "folder-remote",  true, "webdav",  "path", nullptr, "u", "p" },
    { "_webdavs._tcp",     "webdavs",    "folder-remote",  true, "webdavs", "path", nullptr, "u", "p" },

    { "_svn._tcp",    "svn",   "folder-sync",  true, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_rsync._tcp",  "rsync", "folder-sync",  true, nullptr, nullptr, nullptr, nullptr, nullptr },

    // email
    { "_imap._tcp",   "imap",   "email",  false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_pop3._tcp",   "pop3",   "email",  false, "pop3", nullptr, nullptr, nullptr, nullptr },

    // shell services
    { "_ssh._tcp",    "ssh",    "terminal",  false, "ssh",    nullptr,      nullptr, "u", "p" },
    { "_telnet._tcp", "telnet", "terminal",  false, "telnet", nullptr,      nullptr, "u", "p" },
    { "_rfb._tcp",    "rfb",    "krfb",      false, "vnc",    "path", nullptr, "u", "p" },
    { "_rdp._tcp",    "rdp",    "krfb",      false, "rdp", nullptr, nullptr, nullptr, nullptr },

    // other standard services
    { "_http._tcp",   "http",   "folder-html",            false, "http", "path", nullptr, "u", "p" },
    { "_ntp._udp",    "ntp",    "xclock",                 false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_ldap._tcp",   "ldap",   "user-group-properties",  false, "ldap", nullptr, nullptr, nullptr, nullptr },

    // user2user (chat, collaboration)
    { "_xmpp-server._tcp", "xmpp-server", "xchat",         false, "jabber", nullptr, nullptr, nullptr, nullptr },
    { "_presence._tcp",    "presence",    "im-user",       false, "presence", nullptr, nullptr, nullptr, nullptr },
    { "_lobby._tcp",       "lobby",       "document-edit", false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_giver._tcp",       "giver",       "folder-remote", false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_sip._udp",         "sip",         "phone",         false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_h323._tcp",        "h323",        "phone",         false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_skype._tcp",       "skype",       "phone",         false, nullptr, nullptr, nullptr, nullptr, nullptr },

    // printing
    { "_ipp._tcp",            "ipp",            "printer", false, "ipp",  "path", nullptr, "u", "p" },
    { "_printer._tcp",        "printer",        "printer", false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_pdl-datastream._tcp", "pdl-datastream", "printer", false, nullptr, nullptr, nullptr, nullptr, nullptr },

    // KDE workspace
    { "_plasma._tcp",       "plasma",      "plasma",       false, "plasma", "name", "icon", nullptr, nullptr },

    // KDE games
    { "_kbattleship._tcp",  "kbattleship", "kbattleship",  false, "kbattleship", nullptr, nullptr, nullptr, nullptr },
    { "_lskat._tcp",        "lskat",       "lskat",        false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_kfourinline._tcp",  "kfourinline", "kfourinline",  false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_ksirk._tcp",        "ksirk",       "ksirk",        false, nullptr, nullptr, nullptr, nullptr, nullptr },

    // hardware
    { "_pulse-server._tcp","pulse-server","audio-card",          false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_pulse-source._tcp","pulse-source","audio-input-line",    false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_pulse-sink._tcp",  "pulse-sink",  "speaker",             false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_udisks-ssh._tcp",  "udisks-ssh",  "drive-harddisk",      false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_libvirt._tcp",     "libvirt",     "computer",            false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_airmouse._tcp",    "airmouse",    "input-mouse",         false, nullptr, nullptr, nullptr, nullptr, nullptr },

    // database
    { "_postgresql._tcp",       "postgresql",       "server-database",  false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_couchdb_location._tcp", "couchdb_location", "server-database",  false, nullptr, nullptr, nullptr, nullptr, nullptr },

    // else
    { "_realplayfavs._tcp","realplayfavs","favorites",           false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_acrobatSRV._tcp",  "acrobat-server","application-pdf",   false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_adobe-vc._tcp",    "adobe-vc",    "services",   false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_ggz._tcp",         "ggz",         "applications-games",  false, "ggz", nullptr, nullptr, nullptr, nullptr },

    { "_pgpkey-ldap._tcp",  "pgpkey-ldap",  "application-pgp-keys",  false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_pgpkey-hkp._tcp",   "pgpkey-hkp",   "application-pgp-keys",  false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_pgpkey-https._tcp", "pgpkey-https", "application-pgp-keys",  true, "https", "path", nullptr, nullptr, nullptr },

    // Maemo
    { "_maemo-inf._tcp",    "maemo-inf",    "pda",  false, nullptr, nullptr, nullptr, nullptr, nullptr },
    // TODO: _maemo-inf._tcp seems to be not a service, just some about info, use to identify device and hide

    // Apple
    { "_airport._tcp",       "airport",       "network-wireless",  false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_daap._tcp",          "daap",          "folder-sound",      false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_dacp._tcp",          "dacp",          "folder-sound",      false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_eppc._tcp",          "eppc",          "network-connect",   false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_net-assistant._udp", "net-assistant", "services",          false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_odisk._tcp",         "odisk",         "media-optical",     false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_raop._tcp",          "raop",          "speaker",           false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_touch-able._tcp",    "touch-able",    "input-tablet",      false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_workstation._tcp",   "workstation",   "network-workgroup", false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_sleep-proxy._udp",   "sleep-proxy",   "services",          false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_nssocketport._tcp",  "nssocketport",  "services",          false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_home-sharing._tcp",  "home-sharing",  "services",          false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_appletv-itunes._tcp","appletv-itunes","services",          false, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "_appletv-pair._tcp",  "appletv-pair",  "services",          false, nullptr, nullptr, nullptr, nullptr, nullptr }
};
//     result["_ssh._tcp"]=      DNSSDNetServiceBuilder(i18n("Remote disk (fish)"),     "fish",   "service/ftp", QString(), "u", "p");
// network-server-database icon
// see             // SubEthaEdit 2
//                 Defined TXT keys: txtvers=1, name=<Full Name>, userid=<User ID>, version=2

static const int DNSSDServiceDataSize = sizeof( DNSSDServiceData ) / sizeof( DNSSDServiceData[0] );

static const DNSSDServiceDatum UnknownServiceDatum = { "", "unknown", "unknown", false, nullptr, nullptr, nullptr, nullptr, nullptr };

// TODO:
// * find out how ws (webservices, _ws._tcp), upnp (_upnp._tcp) are exactly meant to be used
// * learn about uddi
// * see how the apple specific protocols can be used for devicetype identification (printer, scanner)


void DNSSDServiceDatum::feedUrl( QUrl* url, const KDNSSD::RemoteService* remoteService ) const
{
    const QMap<QString,QByteArray> serviceTextData = remoteService->textData();

    url->setScheme( QString::fromLatin1(protocol) );
    if( userField )
        url->setUserName( QLatin1String(serviceTextData.value(QLatin1String(userField)).constData()) );
    if( passwordField )
        url->setPassword( QLatin1String(serviceTextData.value(QLatin1String(passwordField)).constData()) );
    if( pathField )
        url->setPath( QLatin1String(serviceTextData.value(QLatin1String(pathField)).constData()) );

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

QString SimpleItemFactory::dnssdId( const KDNSSD::RemoteService::Ptr& dnssdService ) const
{
    return dnssdService->type() + QLatin1Char('_') + dnssdService->serviceName();
}

NetServicePrivate* SimpleItemFactory::createNetService( const KDNSSD::RemoteService::Ptr& dnssdService, const NetDevice& device ) const
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

    QUrl url;
    if( serviceDatum->protocol )
        serviceDatum->feedUrl( &url, dnssdService.data() );

    const bool isUnknown = ( serviceDatum == &UnknownServiceDatum );
    const QString typeName = isUnknown ?
                             dnssdServiceType.mid( 1, dnssdServiceType.lastIndexOf(QLatin1Char('.'))-1 ) :
                             QString::fromLatin1( serviceDatum->typeName );

    QString iconName = QString::fromLatin1(serviceDatum->fallbackIconName);
    if ( serviceDatum->iconField ) {
        const QMap<QString,QByteArray> serviceTextData = dnssdService->textData();
        QString serviceIconName = QString::fromUtf8(serviceTextData[QString::fromLatin1(serviceDatum->iconField)]);
        if ( QIcon::hasThemeIcon(serviceIconName) ) {
            iconName = serviceIconName;
        }
    }

    result = new NetServicePrivate( dnssdService->serviceName(), iconName,
                                    typeName, device, url.url(), SimpleItemFactory::dnssdId(dnssdService) );

    return result;
}

bool SimpleItemFactory::canCreateNetSystemFromUpnp( const Cagibi::Device& upnpDevice ) const
{
    Q_UNUSED( upnpDevice )
    return true;
}

QString SimpleItemFactory::upnpId( const Cagibi::Device& upnpDevice ) const
{
    return upnpDevice.udn();
}

// TODO: add KIcon with specialized KIconLoader (fetches Icons via D-Bus)
NetServicePrivate* SimpleItemFactory::createNetService( const Cagibi::Device& upnpDevice, const NetDevice& device ) const
{
    NetServicePrivate* result;

    QString url = upnpDevice.presentationUrl();
    if( url.isEmpty() )
    {
        url = QString::fromLatin1( "upnp://" );
        url.append( upnpDevice.udn() );
    }
    result = new NetServicePrivate( upnpDevice.friendlyName(),
                                    QString::fromLatin1("unknown"),
                                    QString::fromLatin1("upnp.")+upnpDevice.type(), device, url, SimpleItemFactory::upnpId(upnpDevice) );

    return result;
}

SimpleItemFactory::~SimpleItemFactory() {}

}
