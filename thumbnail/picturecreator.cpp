/*  This file is part of the KDE libraries
    Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                  2000 Malte Starostik <malte@kde.org>

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



#include <qpicture.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qimage.h>

#include "picturecreator.h"

extern "C"
{
    ThumbCreator *new_creator()
    {
        return new PictureCreator;
    }
}

bool PictureCreator::create(const QString &path, int width, int height, QImage &img)
{
    QPicture pict;
    if ( !pict.load(path,"svg") )
        return false;

    // render the HTML page on a bigger pixmap and use smoothScale,
    // looks better than directly scaling with the QPainter (malte)
    
    QRect rect = pict.boundingRect();
    double aspect = (double)rect.width() / (double)rect.height();
    
    QPixmap pix;
    if (width > 500 || height > 500 )
    {
        if (height > width)
            pix.resize(width, qRound(width / aspect));
        else
            pix.resize(qRound(height * aspect), height);
    }
    else
    {
        if (height > width)
            pix.resize(500, qRound(500 / aspect));
        else
            pix.resize(qRound(500 * aspect), 500);
    }
    // light-grey background, in case loadind the page failed
    pix.fill( QColor( 245, 245, 245 ) );

    int borderX = pix.width() / width,
        borderY = pix.height() / height;
    QRect rc(borderX, borderY, pix.width() - borderX * 2, pix.height() - borderY * 2);

    QPainter p;
    p.begin(&pix);
    p.setWindow(pict.boundingRect());
    p.drawPicture(0,0,pict);
    p.end();
    img = pix.convertToImage();
    return true;
}

ThumbCreator::Flags PictureCreator::flags() const
{
    return static_cast<Flags>(DrawFrame);
}
