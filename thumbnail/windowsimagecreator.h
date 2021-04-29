/*
    windowsimagecreator.h - Thumbnail Creator for Microsoft Windows Images

    SPDX-FileCopyrightText: 2009-2010 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WINDOWS_IMAGE_CREATOR_H
#define WINDOWS_IMAGE_CREATOR_H

#include <kio/thumbcreator.h>

class WindowsImageCreator : public ThumbCreator
{
public:
    WindowsImageCreator() {}
    bool create(const QString &path, int width, int height, QImage &img) override;
};

#endif // WINDOWS_IMAGE_CREATOR_H
