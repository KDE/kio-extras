/*
    This file is part of the Mollet network library, part of the KDE project.

    SPDX-FileCopyrightText: 2009, 2011 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "netservice_p.h"


namespace Mollet
{

NetServicePrivate::NetServicePrivate( const QString& name, const QString& iconName, const QString& type,
                                      const NetDevice& device, const QString& url, const QString& id )
    : mName( name )
    , mIconName( iconName )
    , mType( type )
    , mDevice( device )
    , mUrl( url )
    , mId( id )
{
}


NetServicePrivate::~NetServicePrivate()
{
}

}
