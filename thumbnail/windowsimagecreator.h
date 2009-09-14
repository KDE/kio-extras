/*
    windowsimagecreator.h - Thumbnail Creator for Microsoft Windows Images

    Copyright (c) 2009 by Pali Rohár <pali.rohar@gmail.com>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef WINDOWS_IMAGE_CREATOR_H
#define WINDOWS_IMAGE_CREATOR_H

#include <kio/thumbcreator.h>

class WindowsImageCreator : public ThumbCreator
{
	public:
		WindowsImageCreator() {}
		bool create(const QString &path, int width, int height, QImage &img);
};

#endif // WINDOWS_IMAGE_CREATOR_H

