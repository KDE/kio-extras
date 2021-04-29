/*
    icoutils.h - Extract Microsoft Window icons and images using icoutils package

    SPDX-FileCopyrightText: 2009-2010 Pali Rohár <pali.rohar@gmail.com>
    SPDX-FileCopyrightText: 2013 Andrius da Costa Ribas <andriusmao@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
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

bool loadIcoImageFromExe(QIODevice * inputDevice, QImage &image, int needWidth=512, int needHeight=512);
bool loadIcoImageFromExe(const QString &inputPath, QImage &image, int needWidth=512, int needHeight=512);
bool loadIcoImageFromExe(const QString &inputFileName, QIODevice *outputDevice);

bool loadIcoImage(QIODevice * inputDevice, QImage &image, int needWidth=512, int needHeight=512);
bool loadIcoImage(const QString &inputFileName, QImage &image, int needWidth=512, int needHeight=512);
bool loadIcoImage(QImageReader &reader, QImage &image, int needWidth, int needHeight);

}

#endif //ICO_UTILS_H
