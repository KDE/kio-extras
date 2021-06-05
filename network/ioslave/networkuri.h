/*
    This file is part of the network kioslave, part of the KDE project.

    SPDX-FileCopyrightText: 2009 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETWORKURI_H
#define NETWORKURI_H

// KF
#include <QUrl>
// // Qt
// #include <QDataStream>


// network://domain/hostname/servicename
class NetworkUri
{
//     friend QDataStream& operator<<( QDataStream& stream, const NetworkUri& networkUri );

public:
    enum Type { InvalidUrl, Domain, Device, Service };
public:
    explicit NetworkUri( const QUrl &url );

public:
    const QString& hostAddress() const;
    const QString& serviceName() const;
    const QString& serviceType() const;
    NetworkUri::Type type() const;

private:

private: // data
    QString mHostAddress;
    QString mServiceName;
    QString mServiceType;
};


inline NetworkUri::NetworkUri( const QUrl &url )
{
    mHostAddress = url.path().mid( 1 );
    const int slashIndex = mHostAddress.indexOf( QLatin1Char('/') );
    if( slashIndex != -1 )
    {
        // servicetype is currently appended as .type to the name
        const int serviceTypeIndex = mHostAddress.lastIndexOf( QLatin1Char('.') ) + 1;
        mServiceType = mHostAddress.mid( serviceTypeIndex );

        const int serviceNameLength = (serviceTypeIndex-1) - (slashIndex+1);
        mServiceName = mHostAddress.mid( slashIndex + 1, serviceNameLength );
        mHostAddress.resize( slashIndex );
    }
}

inline const QString& NetworkUri::hostAddress()    const {
    return mHostAddress;
}
inline const QString& NetworkUri::serviceName() const {
    return mServiceName;
}
inline const QString& NetworkUri::serviceType() const {
    return mServiceType;
}

inline NetworkUri::Type NetworkUri::type() const
{
    Type result =
        mHostAddress.isEmpty() ?    Domain :
        mServiceName.isEmpty() ? Device :
        /*else*/                 Service;

    return result;
}

/*
inline QDataStream& operator<<( QDataStream& stream, const NetworkUri& networkUri )
{
    stream << "NetworkUri(host:"<<networkUri.mHostAddress
           << ",service:"<<networkUri.mServiceName
           << ",type:"<<static_cast<int>(networkUri.type())<<")";
    return stream;
}
*/

#endif
