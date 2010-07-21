/*
    fixhosturifilter.h

    This file is part of the KDE project
    Copyright (C) 2007 Lubos Lunak <llunak@suse.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _FIXHOSTURIFILTER_H_
#define _FIXHOSTURIFILTER_H_

#include <time.h>

#include <kgenericfactory.h>
#include <kurifilter.h>

#include<QtCore/QEventLoop>
#include<QtNetwork/QHostInfo>

/*
 This filter tries to automatically prepend www. to http URLs that
 need it.
*/

class FixHostUriFilter : public KUriFilterPlugin
{
    Q_OBJECT

    public:
        FixHostUriFilter( QObject* parent, const QVariantList& args );
        virtual bool filterUri( KUriFilterData &data ) const;
    private slots:
        void lookedUp( const QHostInfo &hostInfo );
    private:
        bool exists( const KUrl& url ) const;

        mutable QEventLoop *m_eventLoop;
        mutable int m_lookupId;
        mutable bool m_hostExists;
};

#endif
