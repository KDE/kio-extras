/**
 * This file is part of the KDE libraries
 *
 * Comic Book Thumbnailer for KDE 4 v0.1
 * Creates cover page previews for comic-book files (.cbr/z/t).
 * Copyright (c) 2009 Harsh J <harsh@harshj.com>
 *
 * Some code borrowed from Okular's comicbook generators,
 * by Tobias Koenig <tokoe@kde.org>
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

// comiccreator.h

#ifndef COMIC_CREATOR_H
#define COMIC_CREATOR_H

#include <kio/thumbcreator.h>

#include <QtCore/QByteArray>
#include <QtCore/QStringList>
#include <QtGui/QImage>
#include <QtCore/QScopedPointer>

#if defined(Q_OS_WIN)
    #include <QtCore/QProcess>
#else
    #include <kptyprocess.h>
#endif

class KArchiveDirectory;
class QEventLoop;

class ComicCreator : public QObject, public ThumbCreator
{
    Q_OBJECT
    public:
        ComicCreator();
        virtual bool create(const QString& path, int width, int height, QImage& img);
        virtual Flags flags() const;

    private:
        enum Type {
            ZIP,
            TAR,
            RAR
        };
        void filterImages(QStringList& entries);
        int  startProcess(const QString& processPath, const QStringList& args);

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

    private slots:
        void readProcessOut();
        void readProcessErr();
        void finishedProcess(int exitCode, QProcess::ExitStatus exitStatus);

    private:
#if defined(Q_OS_WIN)
        QScopedPointer<QProcess> m_process;
#else
        QScopedPointer<KPtyProcess> m_process;
#endif
        QByteArray m_stdOut;
        QByteArray m_stdErr;
        QEventLoop* m_loop;
};

#endif
