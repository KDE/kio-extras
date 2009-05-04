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

#ifndef NETSERVICE_P_H
#define NETSERVICE_P_H

// lib
#include "netdevice.h"
#include "netservice.h"
// Qt
#include <QtCore/QSharedData>
#include <QtCore/QString>


namespace Mollet
{

class NetServicePrivate : public QSharedData
{
  public:
    explicit NetServicePrivate( const QString& name, const QString& iconName, const QString& type,
                                const NetDevice& device, const QString& url );
    virtual ~NetServicePrivate();

  public:
    const QString& name() const;
    const QString& iconName() const;
    const QString& type() const;
    const NetDevice& device() const;
    const QString& url() const;

  private:
    QString mName;
    QString mIconName;
    QString mType;
    NetDevice mDevice;
    QString mUrl;
};


inline const QString& NetServicePrivate::name()     const { return mName; }
inline const QString& NetServicePrivate::iconName() const { return mIconName; }
inline const QString& NetServicePrivate::type()     const { return mType; }
inline const NetDevice& NetServicePrivate::device() const { return mDevice; }
inline const QString& NetServicePrivate::url()      const { return mUrl; }

}

#endif
