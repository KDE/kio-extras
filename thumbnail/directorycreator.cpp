/*
    SPDX-FileCopyrightText: 2023 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "directorycreator.h"

K_PLUGIN_CLASS_WITH_JSON(DirectoryCreator, "directorythumbnail.json")

DirectoryCreator::DirectoryCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult DirectoryCreator::create(const KIO::ThumbnailRequest & /*request*/)
{
    // The actual implementation for directory thumbnails is in thumbnail.cpp
    // This class is only a dummy to produce a KPluginMetaData so that the directory thumbnailer is listed by PreviewJob::availablePlugins()

    return KIO::ThumbnailResult::fail();
}

#include "directorycreator.moc"

#include "moc_directorycreator.cpp"
