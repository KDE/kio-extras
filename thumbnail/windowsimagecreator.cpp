/*
    windowsimagecreator.cpp - Thumbnail Creator for Microsoft Windows Images

    SPDX-FileCopyrightText: 2009 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowsimagecreator.h"
#include "icoutils.h"

#include <QString>
#include <QImage>
#include <QImageReader>
#include <QMimeDatabase>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new WindowsImageCreator;
    }
}

bool WindowsImageCreator::create(const QString &path, int width, int height, QImage &img)
{
    if (IcoUtils::loadIcoImage(path, img, width, height)) {
        return true;
    }

    // Maybe it's an animated cursor
    if (QMimeDatabase().mimeTypeForFile(path).name() == QLatin1String("application/x-navi-animation")) {
        QImageReader reader(path, "ani");
        return reader.read(&img);
    }

    return false;

}
