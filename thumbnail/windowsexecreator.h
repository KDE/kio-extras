/*
    windowsexecreator.h - Thumbnail Creator for Microsoft Windows Executables

    SPDX-FileCopyrightText: 2009-2010 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

*/


#ifndef WINDOWS_EXE_CREATOR_H
#define WINDOWS_EXE_CREATOR_H

#include <KIO/ThumbnailCreator>
#include <KPluginMetaData>

class WindowsExeCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    WindowsExeCreator(QObject *parent, const QVariantList &args);

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};

#endif // WINDOWS_EXE_CREATOR_H
