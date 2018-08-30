/*
    icoutils.h - Extract Microsoft Window icons and images using icoutils package

    Copyright (c) 2009-2010 by Pali Rohár <pali.rohar@gmail.com>
    Copyright (c) 2013 by Andrius da Costa Ribas <andriusmao@gmail.com>

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

#include <QtGlobal>

class QString;
class QIODevice;
class QImage;
class QImageReader;

namespace IcoUtils
{

    bool loadIcoImageFromExe(QIODevice * inputDevice, QImage &image, int needWidth=512, int needHeight=512, const qint32 iconNumber=0);
    bool loadIcoImageFromExe(const QString &inputPath, QImage &image, int needWidth=512, int needHeight=512, const qint32 iconNumber=0);
    bool loadIcoImageFromExe(const QString &inputFileName, QIODevice *outputDevice, const qint32 iconNumber);

    bool loadIcoImage(QIODevice * inputDevice, QImage &image, int needWidth=512, int needHeight=512);
    bool loadIcoImage(const QString &inputFileName, QImage &image, int needWidth=512, int needHeight=512);
    bool loadIcoImage(QImageReader &reader, QImage &image, int needWidth, int needHeight);

}

#endif //ICO_UTILS_H
