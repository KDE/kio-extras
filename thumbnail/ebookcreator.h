/*
 * SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <kio/thumbcreator.h>

#include <QStringList>

class QIODevice;

class KArchiveDirectory;

class EbookCreator : public ThumbCreator
{
public:
    EbookCreator();
    ~EbookCreator() override;

    bool create(const QString &path, int width, int height, QImage &image) override;

    bool createEpub(const QString &path, QImage &image);
    // either a QFile or a KZipFileEntry
    bool createFb2(QIODevice *device, QImage &image);

    static QStringList getEntryList(const KArchiveDirectory *dir, const QString &path);

};
