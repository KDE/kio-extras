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

#include "jpegrotatedcreator.h"

#include "jpegcreatorhelper.h"

#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>

#include <QtCore/QFile>
#include <QtGui/QImage>

#include <cstdio>

extern "C"
{
    #include <jpeglib.h>

    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new JpegRotatedCreator;
    }
}

JpegRotatedCreator::JpegRotatedCreator()
{
}

bool JpegRotatedCreator::create(const QString &path, int width, int height, QImage &image)
{
    QImage img;
    if (!JpegCreatorHelper::create(path, width, height, img)) {
        return false;
    }

    const QByteArray name = QFile::encodeName(path);
    try {
        Exiv2::Image::AutoPtr exivImg = Exiv2::ImageFactory::open(name.constData());
        if (exivImg.get()) {
            exivImg->readMetadata();
            Exiv2::ExifData exifData = exivImg->exifData();
            if (!exifData.empty()) {
                Exiv2::ExifKey key("Exif.Image.Orientation");
                Exiv2::ExifData::iterator it = exifData.findKey(key);
                if (it != exifData.end()) {
                    int orient = it->toLong();
                    image = img.transformed(orientationMatrix(orient));
                    return true;
                }
            }
        }
    } catch (...) {
        // Apparently libexiv changed its API at some point, a different exception is thrown
        // depending on the version. an ifdef could make it work, but since we just ignore the exception
        // there is no point in doing that
    }
    image = img;
    return true;
}

ThumbCreator::Flags JpegRotatedCreator::flags() const
{
    return None;
}

QTransform JpegRotatedCreator::orientationMatrix(int exivOrientation) const
{
    // Check (e.g.) man jpegexiforient for an explanation
    switch (exivOrientation) {
    case 2:
        return QTransform(-1, 0, 0, 1, 0, 0);
    case 3:
        return QTransform(-1, 0, 0, -1, 0, 0);
    case 4:
        return QTransform(1, 0, 0, -1, 0, 0);
    case 5:
        return QTransform(0, 1, 1, 0, 0, 0);
    case 6:
        return QTransform(0, 1, -1, 0, 0, 0);
    case 7:
        return QTransform(0, -1, -1, 0, 0, 0);
    case 8:
        return QTransform(0, -1, 1, 0, 0, 0);
    case 1:
    default:
        return QTransform(1, 0, 0, 1, 0, 0);
    }
}

