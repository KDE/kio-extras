/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009, 2011 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef NETSERVICE_P_H
#define NETSERVICE_P_H

// lib
#include "netdevice.h"
#include "netservice.h"
// Qt
#include <QSharedData>
#include <QString>


namespace Mollet
{

class NetServicePrivate : public QSharedData
{
  public:
    explicit NetServicePrivate( const QString& name, const QString& iconName, const QString& type,
                                const NetDevice& device, const QString& url, const QString& id );
    virtual ~NetServicePrivate();

  public:
    const QString& name() const;
    const QString& iconName() const;
    const QString& type() const;
    const NetDevice& device() const;
    const QString& url() const;
    const QString& id() const;

  private:
    QString mName;
    QString mIconName;
    QString mType;
    NetDevice mDevice;
    QString mUrl;
    QString mId;
};


inline const QString& NetServicePrivate::name()     const { return mName; }
inline const QString& NetServicePrivate::iconName() const { return mIconName; }
inline const QString& NetServicePrivate::type()     const { return mType; }
inline const NetDevice& NetServicePrivate::device() const { return mDevice; }
inline const QString& NetServicePrivate::url()      const { return mUrl; }
inline const QString& NetServicePrivate::id()       const { return mId; }

}

#endif
