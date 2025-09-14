/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _IMAGECREATOR_H_
#define _IMAGECREATOR_H_

#include <QtGlobal>

#include <KIO/ThumbnailCreator>

class ImageCreator : public KIO::DynamicThumbnailCreator
{
    Q_OBJECT
public:
    ImageCreator(QObject *parent, const QVariantList &args);
    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
    QStringList supportedMimeTypes() override;
};

#endif
