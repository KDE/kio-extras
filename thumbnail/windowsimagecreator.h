/*
    windowsimagecreator.h - Thumbnail Creator for Microsoft Windows Images

    SPDX-FileCopyrightText: 2009-2010 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WINDOWS_IMAGE_CREATOR_H
#define WINDOWS_IMAGE_CREATOR_H

#include <KIO/ThumbnailCreator>

class WindowsImageCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    WindowsImageCreator(QObject *parent, const QVariantList &args);
    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};

#endif // WINDOWS_IMAGE_CREATOR_H
