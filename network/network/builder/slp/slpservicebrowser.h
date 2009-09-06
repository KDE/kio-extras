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

#ifndef SLPSERVICEBROWSER_H
#define SLPSERVICEBROWSER_H

// lib
#include "slpservicelist.h"
// SLP
#include <slp.h>
// Qt
#include <QtCore/QObject>

class QStringList;
template <class T> class QList;


namespace Mollet
{

class SlpServiceBrowser : public QObject
{
    Q_OBJECT

  private:
    static SLPBoolean onAttributesFound( SLPHandle hslp, const char* attributeList, SLPError errorCode, void* builder );
    static SLPBoolean onServiceFound( SLPHandle hslp, const char* serviceUrl, unsigned short lifetime, SLPError errorCode, void* builder );
    static SLPBoolean onServiceTypesFound( SLPHandle hslp, const char* serviceTypes, SLPError errorCode, void* _attributeList );

  public:
    explicit SlpServiceBrowser();
    virtual ~SlpServiceBrowser();

  Q_SIGNALS:
    void servicesAdded( const QList<SLPService>& services );
    void servicesChanged( const QList<SLPService>& services );
    void servicesRemoved( const QList<SLPService>& services );

  protected Q_SLOTS: // Object API
    virtual void timerEvent( QTimerEvent* event );

  private Q_SLOTS:
    void scan();

  private:
    SlpServiceList mCurrentServiceList;

    SLPHandle mSLP;
    // used to store the attributelist from the onAttributesFound call
    QString mAttributeList;
};

}

#endif
