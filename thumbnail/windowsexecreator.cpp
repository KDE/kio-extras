/*
    windowsexecreator.cpp - Thumbnail Creator for Microsoft Windows Executables

    SPDX-FileCopyrightText: 2009 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowsexecreator.h"
#include "icoutils.h"
#include "macros.h"

#include <QString>
#include <QImage>

EXPORT_THUMBNAILER_WITH_JSON(WindowsExeCreator, "windowsexethumbnail.json")

bool WindowsExeCreator::create(const QString &path, int width, int height, QImage &img)
{

    return IcoUtils::loadIcoImageFromExe(path, img, width, height);

}

#include "windowsexecreator.moc"
