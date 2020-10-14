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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "imagecreator.h"

#include <QImageReader>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new ImageCreator;
    }
}

bool ImageCreator::create(const QString &path, int, int, QImage &img)
{
    // create image preview
    QImageReader ir(path);
    ir.setDecideFormatFromContent(true);
    img = ir.read();
    if (img.isNull())
        return false;
    if (img.depth() != 32)
        img = img.convertToFormat(img.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    return true;
}
