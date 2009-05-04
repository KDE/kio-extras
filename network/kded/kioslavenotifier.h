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

#ifndef KIOSLAVENOTIFIER_H
#define KIOSLAVENOTIFIER_H

// Qt
#include <QtCore/QHash>
#include <QtCore/QObject>

namespace Mollet
{

class Network;
class NetDevice;
class NetService;


class KioSlaveNotifier : public QObject
{
    Q_OBJECT

  public:
    explicit KioSlaveNotifier( Network* network, QObject* parent = 0 );
    virtual ~KioSlaveNotifier();

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
