/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2008 Andre Gemünd <scroogie@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _JPEGCREATOR_H_
#define _JPEGCREATOR_H_

#include <kio/thumbcreator.h>

class QTransform;

class JpegCreator : public ThumbCreator
{
public:
    JpegCreator();
    bool create(const QString &path, int, int, QImage &img) override;
};

#endif
