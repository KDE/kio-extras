/*
    windowsexecreator.h - Thumbnail Creator for Microsoft Windows Executables

    SPDX-FileCopyrightText: 2009-2010 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

*/


#ifndef WINDOWS_EXE_CREATOR_H
#define WINDOWS_EXE_CREATOR_H

#include <kio/thumbcreator.h>

class WindowsExeCreator : public ThumbCreator
{
public:
    WindowsExeCreator() {}
    bool create(const QString &path, int width, int height, QImage &img) override;
};

#endif // WINDOWS_EXE_CREATOR_H
