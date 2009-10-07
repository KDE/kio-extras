/*
    icoutils.h - Extract Microsoft Window icons and images using icoutils package

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

#ifndef ICO_UTILS_H
#define ICO_UTILS_H

class QString;

namespace IcoUtils {

	bool convertExeToIco(const QString &inputPath, const QString &outputPath);
	bool convertIcoToPng(const QString &inputPath, const QString &outputPath, int needWidth=512, int needHeight=512);

}

#endif //ICO_UTILS_H

