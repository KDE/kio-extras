/*
    windowsexecreator.cpp - Thumbnail Creator for Microsoft Windows Executables

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

#include "windowsexecreator.h"
#include "icoutils.h"

#include <QString>
#include <QImage>
#include <QTemporaryFile>

#include <kdemacros.h>

extern "C"
{
	KDE_EXPORT ThumbCreator *new_creator() {
		return new WindowsExeCreator;
	}
}

bool WindowsExeCreator::create(const QString &path, int width, int height, QImage &img) {

	QTemporaryFile icoTempFile, pngTempFile;

	if ( ! icoTempFile.open() )
		return false;

	if ( ! pngTempFile.open() )
		return false;

	if ( ! IcoUtils::convertExeToIco(path, icoTempFile.fileName()) )
		return false;

	if ( ! IcoUtils::convertIcoToPng(icoTempFile.fileName(), pngTempFile.fileName()) )
		return false;

	if ( ! img.load(pngTempFile.fileName()) )
		return false;

	return true;

}

