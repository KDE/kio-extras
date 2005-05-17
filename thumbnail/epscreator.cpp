/*  This file is part of the KDE libraries
    Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                  2000 Malte Starostik <malte@kde.org>
    Copyright (C) 2005 Nicolas GOUTTE <goutte@kde.org>

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


#include <qimage.h>

#include <kimageio.h>
#include <kdebug.h>

#include "epscreator.h"

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        KImageIO::registerFormats();
        return new EpsCreator;
    }
}

bool EpsCreator::create(const QString &path, int width, int height, QImage &img)
{
    kdDebug() << "EPS CREATOR:" << path << endl; 
    QImageIO io( path, 0 );
    
    QCString param;
    param.sprintf( "%i:%i", width, height );
    
    io.setParameters( param );
    
    // create image preview
    if ( !io.read() )
	return false;
	
    // KImageIO's EPSimport is low colour, so always convert
    img = io.image().convertDepth( 32 );
    return !img.isNull();
}

ThumbCreator::Flags EpsCreator::flags() const
{
    return None;
}
