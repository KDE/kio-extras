/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cursorcreator.h"

#include <QImage>
#include <QFile>

#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
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
                      cursor->width, cursor->height, QImage::Format_ARGB32_Premultiplied );

        // Create a deep copy of the image so the image data is preserved
        img = img.copy();
        XcursorImageDestroy( cursor );
        return true;
    }

    return false;
}

