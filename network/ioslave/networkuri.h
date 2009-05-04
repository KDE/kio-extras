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

#ifndef NETWORKURI_H
#define NETWORKURI_H

// KDE
#include <KUrl>
// // Qt
// #include <QtCore/QDataStream>


// network://domain/hostname/servicename
class NetworkUri
{
//     friend QDataStream& operator<<( QDataStream& stream, const NetworkUri& networkUri );

  public:
    enum Type { InvalidUrl, Domain, Device, Service };
  public:
    explicit NetworkUri( const KUrl& url );

  public:
    const QString& hostName() const;
    const QString& serviceName() const;
    const QString& serviceType() const;
    NetworkUri::Type type() const;

  private:

  private: // data
    QString mHostName;
    QString mServiceName;
    QString mServiceType;
};


inline NetworkUri::NetworkUri( const KUrl& url )
{
    mHostName = url.path().mid( 1 );
    const int slashIndex = mHostName.indexOf( '/' );
    if( slashIndex != -1 )
    {
        // servicetype is currently appended as .type to the name
        const int serviceTypeIndex = mHostName.lastIndexOf( '.' );
        mServiceType = mHostName.mid( serviceTypeIndex );

        const int serviceNameLength = (serviceTypeIndex-1) - (slashIndex+1);
        mServiceName = mHostName.mid( slashIndex + 1, serviceNameLength );
        mHostName.resize( slashIndex );
    }
}

inline const QString& NetworkUri::hostName()    const { return mHostName; }
inline const QString& NetworkUri::serviceName() const { return mServiceName; }
inline const QString& NetworkUri::serviceType() const { return mServiceType; }

inline NetworkUri::Type NetworkUri::type() const
{
    Type result =
        mHostName.isEmpty() ?    Domain :
        mServiceName.isEmpty() ? Device :
        /*else*/                 Service;

    return result;
}

/*
inline QDataStream& operator<<( QDataStream& stream, const NetworkUri& networkUri )
{
    stream << "NetworkUri(host:"<<networkUri.mHostName
           << ",service:"<<networkUri.mServiceName
           << ",type:"<<static_cast<int>(networkUri.type())<<")";
    return stream;
}
*/

#endif
