/*
 * SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <KIO/ThumbnailCreator>

#include <QStringList>

class QIODevice;

class KArchiveDirectory;

class EbookCreator : public KIO::ThumbnailCreator
{
public:
    EbookCreator(QObject *parent, const QVariantList &args);
    ~EbookCreator() override;

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;

    KIO::ThumbnailResult createEpub(const QString &path);
    // either a QFile or a KZipFileEntry
    KIO::ThumbnailResult createFb2(QIODevice *device);

    static QStringList getEntryList(const KArchiveDirectory *dir, const QString &path);

};
