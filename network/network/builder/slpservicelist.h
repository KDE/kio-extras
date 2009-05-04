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

#ifndef SLPSERVICELIST_H
#define SLPSERVICELIST_H

// lib
#include "slpservice.h"
// Qt
#include <QtCore/QString>
#include <QtCore/QHash>


namespace Mollet
{

class SlpServiceList : public QHash<QString,SLPService>
{
  public:
    SlpServiceList();

  public:
    void resetState();
    void checkService( const QString& serviceUrl, const QString& name, const QString& attributeList );

  private:
};


inline  SlpServiceList::SlpServiceList() {}

inline void SlpServiceList::resetState()
{
    QHash<QString,SLPService>::Iterator it;
    const QHash<QString,SLPService>::Iterator endIt = end();
    for( it = begin(); it != endIt; ++it )
        it.value().resetState();
}

inline void SlpServiceList::checkService( const QString& serviceUrl, const QString& name, const QString& attributeList )
{
    QHash<QString,SLPService>::Iterator it = find( serviceUrl );
    if( it != end() )
        it.value().checkForChanges( attributeList, name );
    else
        insert( serviceUrl, SLPService(serviceUrl,attributeList,name) );
}

}

#endif
