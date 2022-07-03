/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2008 Andre Gemünd <scroogie@gmail.com>
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _JPEGCREATOR_H_
#define _JPEGCREATOR_H_

#include <KIO/ThumbCreator>

class QTransform;

class JpegCreator : public ThumbCreator
{
public:
    JpegCreator();
    bool create(const QString &path, int, int, QImage &img) override;
private:
    bool exifThumbnail(const QString &path, QImage &image) const;
    bool imageReaderThumbnail(const QString &path, int width, int height, QImage &image) const;
};

#endif
