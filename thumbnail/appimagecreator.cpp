/*
 * Copyright (c) 2018 Kai Uwe Broulik <kde@broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "appimagecreator.h"

#include <QImage>
#include <QTemporaryFile>

#include <appimage/appimage.h>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new AppImageCreator;
    }
}

AppImageCreator::AppImageCreator() = default;
AppImageCreator::~AppImageCreator() = default;

bool AppImageCreator::create(const QString &path, int width, int height, QImage &image)
{
    // We just load the .DirIcon verbatim and let the PreviewJob figure out scaling to required size if needed
    Q_UNUSED(width);
    Q_UNUSED(height);

    QTemporaryFile file;
    if (!file.open()) {
        return false;
    }

    appimage_extract_file_following_symlinks(qUtf8Printable(path),
                                             ".DirIcon",
                                             qUtf8Printable(file.fileName()));

    if (!image.load(file.fileName())) {
        return false;
    }

    return true;
}

ThumbCreator::Flags AppImageCreator::flags() const
{
    return None;
}
