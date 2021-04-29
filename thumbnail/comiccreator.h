/**
 * This file is part of the KDE libraries
 *
 * Comic Book Thumbnailer for KDE 4 v0.1
 * Creates cover page previews for comic-book files (.cbr/z/t).
 * SPDX-FileCopyrightText: 2009 Harsh J <harsh@harshj.com>
 *
 * Some code borrowed from Okular's comicbook generators,
 * by Tobias Koenig <tokoe@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

// comiccreator.h

#ifndef COMIC_CREATOR_H
#define COMIC_CREATOR_H

#include <kio/thumbcreator.h>

#include <QByteArray>
#include <QStringList>
#include <QImage>

class KArchiveDirectory;
class QEventLoop;

class ComicCreator : public QObject, public ThumbCreator
{
    Q_OBJECT
public:
    ComicCreator();
    bool create(const QString& path, int width, int height, QImage& img) override;

private:
    enum Type {
        ZIP,
        TAR,
        RAR,
        SEVENZIP
    };
    void filterImages(QStringList& entries);
    int  runProcess(const QString& processPath, const QStringList& args);

    // For "zip" and "tar" type files.
    // Uses KDE's internal archive classes.
    QImage extractArchiveImage(const QString& path, const ComicCreator::Type);
    void getArchiveFileList(QStringList& entries, const QString& prefix,
                            const KArchiveDirectory* dir);

    // For "rar" type files.
    // Uses the non-free 'unrar' executable, if available.
    QImage extractRARImage(const QString& path);
    QString unrarPath() const;
    QStringList getRARFileList(const QString& path, const QString& unrarPath);

private:
    QByteArray m_stdOut;
};

#endif
