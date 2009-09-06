/*
    This file is part of the solid network library, part of the KDE project.

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

#ifndef SLPSERVICE_H
#define SLPSERVICE_H

// Qt
#include <QtCore/QString>
#include <QtCore/QHash>


namespace Mollet
{

class SLPService
{
  public:
    enum State { New, Changed, Unchanged, Unknown };

  public:
    explicit SLPService( const QString& serviceUrl, const QString& attributesString, const QString& name );

  public:
    SLPService::State state() const;
    const QString& serviceUrl() const;
    const QString& attributesString() const;
    const QString& name() const;

  public:
    void resetState();
    void checkForChanges( const QString& attributesString, const QString& name );

  private:
    State mState;
    QString mServiceUrl;
    QString mAttributesString; // TODO make QByteArray
    QString mName; // by mdns
};


inline SLPService::SLPService( const QString& serviceUrl, const QString& attributesString, const QString& name )
 : mState( New ), mServiceUrl( serviceUrl ), mAttributesString( attributesString ), mName( name ) {}

inline SLPService::State SLPService::state() const { return mState; }
inline const QString& SLPService::serviceUrl() const { return mServiceUrl; }
inline const QString& SLPService::attributesString() const { return mAttributesString; }
inline const QString& SLPService::name() const { return mName; }

inline void SLPService::resetState() { mState = Unknown; }
inline void SLPService::checkForChanges( const QString& attributesString, const QString& name )
{
    State newState = Unchanged;
    if( mAttributesString != attributesString )
    {
        mAttributesString = attributesString;
        newState = Changed;
    }
    if( mName != name )
    {
        mName = name;
        newState = Changed;
    }
    mState = newState;
}


inline uint qHash( const SLPService& service ) { return qHash( service.attributesString() ); }

}

#endif
