/*  This file is part of the KDE libraries
    Copyright (C) 2008 Andre Gemünd <scroogie@gmail.com>
    Copyright (C) 2016 Alexander Volkov <a.volkov@rusbitech.ru>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "jpegcreator.h"
#include "jpegcreatorsettings5.h"

#include <QCheckBox>
#include <QImage>
#include <QImageReader>
#include <klocalizedstring.h>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new JpegCreator;
    }
}

JpegCreator::JpegCreator()
{
}

bool JpegCreator::create(const QString &path, int width, int height, QImage &image)
{
    QImageReader imageReader(path, "jpeg");

    const QSize imageSize = imageReader.size();
    if (imageSize.isValid() && (imageSize.width() > width || imageSize.height() > height)) {
        const QSize thumbnailSize = imageSize.scaled(width, height, Qt::KeepAspectRatio);
        imageReader.setScaledSize(thumbnailSize); // fast downscaling
    }
    imageReader.setQuality(75); // set quality so that the jpeg handler will use a high quality downscaler

    JpegCreatorSettings* settings = JpegCreatorSettings::self();
    settings->load();
    imageReader.setAutoTransform(settings->rotate());

    return imageReader.read(&image);
}

ThumbCreator::Flags JpegCreator::flags() const
{
    return None;
}

QWidget *JpegCreator::createConfigurationWidget()
{
    QCheckBox *rotateCheckBox = new QCheckBox(i18nc("@option:check", "Rotate the image automatically"));
    rotateCheckBox->setChecked(JpegCreatorSettings::rotate());
    return rotateCheckBox;
}

void JpegCreator::writeConfiguration(const QWidget *configurationWidget)
{
    const QCheckBox *rotateCheckBox = qobject_cast<const QCheckBox*>(configurationWidget);
    if (rotateCheckBox) {
        JpegCreatorSettings* settings = JpegCreatorSettings::self();
        settings->setRotate(rotateCheckBox->isChecked());
        settings->save();
    }
}
