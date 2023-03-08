/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2008 Andre Gemünd <scroogie@gmail.com>
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _JPEGCREATOR_H_
#define _JPEGCREATOR_H_

#include <KIO/ThumbnailCreator>

class QTransform;

class JpegCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    JpegCreator(QObject *parent, const QVariantList &args);
    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;

private:
    KIO::ThumbnailResult exifThumbnail(const KIO::ThumbnailRequest &request) const;
    KIO::ThumbnailResult imageReaderThumbnail(const KIO::ThumbnailRequest &request) const;
};

#endif
