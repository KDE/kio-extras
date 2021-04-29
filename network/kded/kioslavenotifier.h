/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KIOSLAVENOTIFIER_H
#define KIOSLAVENOTIFIER_H

// Qt
#include <QHash>
#include <QObject>

namespace Mollet
{

class Network;
class NetDevice;
class NetService;


class KioSlaveNotifier : public QObject
{
    Q_OBJECT

public:
    explicit KioSlaveNotifier( Network* network, QObject* parent = nullptr );
    ~KioSlaveNotifier() override;

public: // for debugging, remove also from adaptor.xml
    QStringList watchedDirectories() const;

public Q_SLOTS:
    void onDirectoryEntered( const QString& directory );
    void onDirectoryLeft( const QString& directory );

private:
    void notifyAboutAdded( const QString& dirId );
    void notifyAboutRemoved( const QString& dirId, const QString& itemPath );

private Q_SLOTS:
    void onDevicesAdded( const QList<NetDevice>& deviceList );
    void onDevicesRemoved( const QList<NetDevice>& deviceList );
    void onServicesAdded( const QList<NetService>& serviceList );
    void onServicesRemoved( const QList<NetService>& serviceList );

private:
    QHash<QString, int> mWatchedDirs;
};

}

#endif
