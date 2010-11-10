/*  This file is part of the KDE libraries
    Copyright (C) 2008 Andre Gemünd <scroogie@gmail.com>

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

#include "jpegcreator.h"

#include "jpegcreatorhelper.h"
#include <kdemacros.h>

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new JpegCreator;
    }
}

JpegCreator::JpegCreator()
{
}

bool JpegCreator::create(const QString &path, int width, int height, QImage &image)
{
    return JpegCreatorHelper::create(path, width, height, image);
}

ThumbCreator::Flags JpegCreator::flags() const
{
    return None;
}
