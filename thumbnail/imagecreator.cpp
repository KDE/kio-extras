/*  This file is part of the KDE libraries
    Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                  2000 Malte Starostik <malte.starostik@t-online.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

// $Id$

#include <assert.h>

#include <qpixmap.h>
#include <qimage.h>

#include <kdebug.h>

#include "imagecreator.h"

bool ImageCreator::create(const QString &path, int extent, QPixmap &pix)
{
    // create image preview
    if ( pix.load( path ) )
    {
        int w = pix.width(), h = pix.height();
        // scale to pixie size
        if(w > extent || h > extent)
        {
            if(w > h)
            {
                h = (int)( (double)( h * extent ) / w );
                if ( h == 0 ) h = 1;
                w = extent;
                ASSERT( h <= extent );
            }
            else
            {
                w = (int)( (double)( w * extent ) / h );
                if ( w == 0 ) w = 1;
                h = extent;
                ASSERT( w <= extent );
            }
            QImage img(pix.convertToImage().smoothScale( w, h ));
            if ( img.width() != w || img.height() != h )
            {
                // Resizing failed. Aborting.
                kdWarning() << "Resizing of " << path << " failed. Aborting. " << endl;
                return false;
            }
            pix.convertFromImage( img );
        }
        return true;
    }
    return false;
}

