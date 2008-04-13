/*  This file is part of the KDE libraries
    Copyright (c) 2006 Pascal Létourneau <pascal.letourneau@kdemail.net>

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

#include "svgcreator.h"

#include <QImage>
#include <QPainter>

#include <kdemacros.h>
#include <ksvgrenderer.h>

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new SvgCreator;
    }
}

bool SvgCreator::create(const QString &path, int w, int h, QImage &img)
{
    KSvgRenderer r(path);
    if ( !r.isValid() )
        return false;

    // render using the correct ratio
    const double ratio = static_cast<double>(r.defaultSize().height()) /
                         static_cast<double>(r.defaultSize().width());
    if (w < h)
        h = qRound(ratio * w);
    else
        w = qRound(h / ratio);

    QImage i(w, h, QImage::Format_ARGB32_Premultiplied);
    i.fill(0);
    QPainter p(&i);
    r.render(&p);
    img = i;
    return true;
}

ThumbCreator::Flags SvgCreator::flags() const
{
    return None;
}
