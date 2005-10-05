/*  This file is part of the KDE libraries
    Copyright (C) 2003 Fredrik Höglund <fredrik@kde.org>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <qimage.h>
#include <qfile.h>

#include <kdemacros.h>

#include "cursorcreator.h"

#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new CursorCreator;
    }
}

bool CursorCreator::create( const QString &path, int width, int height, QImage &img )
{
    XcursorImage *cursor = XcursorFilenameLoadImage(
		    QFile::encodeName( path ).data(),
		    width > height ? height : width );

    if ( cursor ) {
    	img = QImage( reinterpret_cast<uchar *>( cursor->pixels ),
			cursor->width, cursor->height, 32,
			NULL, 0, QImage::BigEndian );
	img.setAlphaBuffer( true );

	// Convert the image to non-premultiplied alpha
	Q_UINT32 *pixels = reinterpret_cast<Q_UINT32 *>( img.bits() );
	for ( int i = 0; i < (img.width() * img.height()); i++ ) {
		float alpha = qAlpha( pixels[i] ) / 255.0;
		if ( alpha > 0.0 && alpha < 1.0 )
			pixels[i] = qRgba( int( qRed(pixels[i]) / alpha ),
			                   int( qGreen(pixels[i]) / alpha ),
			                   int( qBlue(pixels[i]) / alpha ),
			                   qAlpha(pixels[i]) );
	}

	// Create a deep copy of the image so the image data is preserved
	img = img.copy();
	XcursorImageDestroy( cursor );
    	return true;
    }

    return false;
}

