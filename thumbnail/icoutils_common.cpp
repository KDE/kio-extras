/*
    icoutils_common.cpp - Extract Microsoft Window icons and images using icoutils package

    Copyright (c) 2009-2010 by Pali Rohár <pali.rohar@gmail.com>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include "icoutils.h"

#include <QList>
#include <QString>
#include <QTemporaryFile>
#include <QImage>
#include <QImageReader>

#define abs(n) ( ( n < 0 ) ? -n : n )

bool IcoUtils::loadIcoImageFromExe(QIODevice * inputDevice, QImage &image, int needWidth, int needHeight, const qint32 iconNumber)
{

    QTemporaryFile inputFile;

    if ( ! inputFile.open() )
         return false;

    QByteArray data = inputDevice->readAll();

    if ( inputFile.write(data) == -1 )
        return false;

    return IcoUtils::loadIcoImageFromExe(inputFile.fileName(), image, needWidth, needHeight, iconNumber);

}

bool IcoUtils::loadIcoImageFromExe(const QString &inputFileName, QImage &image, int needWidth, int needHeight, const qint32 iconNumber)
{

    QTemporaryFile outputFile;

    if ( ! outputFile.open() )
        return false;

    if ( ! IcoUtils::loadIcoImageFromExe(inputFileName, outputFile.fileName(), iconNumber) )
         return false;

    return IcoUtils::loadIcoImage(outputFile.fileName(), image, needWidth, needHeight);

}

bool IcoUtils::loadIcoImage(QImageReader &reader, QImage &image, int needWidth, int needHeight)
{

    if ( ! reader.canRead() )
        return false;

    QList <QImage> icons;
    do icons << reader.read();
    while ( reader.jumpToNextImage() );

    if ( icons.empty() )
        return false;

    int min_w = 1024;
    int min_h = 1024;
    int index = icons.size() - 1;


    // we loop in reverse order because QtIcoHandler converts all images to 32-bit depth, and resources are ordered from lower depth to higher depth
    for ( int i_index = icons.size() - 1; i_index >= 0 ; --i_index )
    {

        const QImage &icon = icons.at(i_index);
        int i_width = icon.width();
        int i_height = icon.height();
        int i_w = abs(i_width - needWidth);
        int i_h = abs(i_height - needHeight);

        if ( i_w < min_w || ( i_w == min_w && i_h < min_h ) )
        {

            min_w = i_w;
            min_h = i_h;
            index = i_index;

        }

    }

    image = icons.at(index);
    return true;

}

bool IcoUtils::loadIcoImage(QIODevice * inputDevice, QImage &image, int needWidth, int needHeight)
{

    QImageReader reader(inputDevice, "ico");
    return IcoUtils::loadIcoImage(reader, image, needWidth, needHeight);

}

bool IcoUtils::loadIcoImage(const QString &inputFileName, QImage &image, int needWidth, int needHeight)
{

    QImageReader reader(inputFileName, "ico");
    return IcoUtils::loadIcoImage(reader, image, needWidth, needHeight);

}
