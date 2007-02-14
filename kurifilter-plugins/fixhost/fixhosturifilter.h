/*
    fixhosturifilter.h

    This file is part of the KDE project
    Copyright (C) 2007 Lubos Lunak <llunak@suse.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _FIXHOSTURIFILTER_H_
#define _FIXHOSTURIFILTER_H_

#include <time.h>

#include <kgenericfactory.h>
#include <kurifilter.h>

/*
 This filter tries to automatically prepend www. to http URLs that
 need it.
*/

class FixHostUriFilter : public KUriFilterPlugin
{
    Q_OBJECT

    public:
        FixHostUriFilter( QObject* parent, const QStringList& args );
        virtual bool filterUri( KUriFilterData &data ) const;
    private:
        static bool exists( const KUrl& url );
};

#endif
