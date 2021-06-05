/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORKSLAVE_H
#define NETWORKSLAVE_H

// KF
#include <KIO/SlaveBase>

class NetworkDBusInterface;
class NetworkUri;
namespace Mollet {
class NetDevice;
class NetService;
}
namespace KIO {
class UDSEntry;
}

class NetworkSlave : public KIO::SlaveBase
{
public:
    NetworkSlave( const QByteArray& name, const QByteArray& poolSocket, const QByteArray& programSocket );
    ~NetworkSlave() override;

public: // KIO::SlaveBase API
    void mimetype( const QUrl& url ) override;
    void get( const QUrl& url ) override;
    void stat( const QUrl& url ) override;
    void listDir( const QUrl& url ) override;

private:
    void feedEntryAsNetwork( KIO::UDSEntry* entry );
    void feedEntryAsDevice( KIO::UDSEntry* entry, const Mollet::NetDevice& deviceData );
    void feedEntryAsService( KIO::UDSEntry* entry, const Mollet::NetService& serviceData );

    void reportError( const NetworkUri& networkUri, int errorId );

private: // data
    NetworkDBusInterface* mNetworkDBusProxy;
};

#endif
