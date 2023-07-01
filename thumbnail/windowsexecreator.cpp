/*
    windowsexecreator.cpp - Thumbnail Creator for Microsoft Windows Executables

    SPDX-FileCopyrightText: 2009 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowsexecreator.h"
#include "icoutils.h"

#include <QString>
#include <QImage>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(WindowsExeCreator, "windowsexethumbnail.json")

WindowsExeCreator::WindowsExeCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult WindowsExeCreator::create(const KIO::ThumbnailRequest &request)
{
    QImage img;
    IcoUtils::loadIcoImageFromExe(request.url().toLocalFile(), img, request.targetSize().width(), request.targetSize().height());
    return !img.isNull() ? KIO::ThumbnailResult::pass(img) : KIO::ThumbnailResult::fail();
}

#include "moc_windowsexecreator.cpp"
#include "windowsexecreator.moc"
