/*  This file is part of the KDE libraries
    Copyright (C) 2000 Malte Starostik <malte@kde.org>

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

#ifndef _THUMBNAIL_H_
#define _THUMBNAIL_H_

#include <QHash>
#include <QPainter>
#include <QSet>

#include <kio/slavebase.h>

#include "sandboxedthumbnailerrunner.h"

class ThumbCreator;
class QImage;

class ThumbnailProtocol : public KIO::SlaveBase
{
public:
    ThumbnailProtocol(const QByteArray &pool, const QByteArray &app);
    ~ThumbnailProtocol() override;

    void get(const QUrl &url) override;

protected:
    ThumbCreator* getThumbCreator(const QString& plugin);
    const QImage getIcon();
    bool isOpaque(const QImage &image) const;
    void drawPictureFrame(QPainter *painter, const QPoint &pos, const QImage &image,
                          int frameWidth, QSize imageTargetSize) const;
    QImage thumbForDirectory(const QUrl& directory);
    QString pluginForMimeType(const QString& mimeType);

    float sequenceIndex() const;

private:
    /**
     * Creates a sub thumbnail for the directory thumbnail. If a cached
     * version of the sub thumbnail is available, the cached version will be used.
     * If no cached version is available, the created sub thumbnail will be
     * added to the cache for later use.
     */
    bool createSubThumbnail(QImage& thumbnail, const QString& filePath,
                            int segmentWidth, int segmentHeight);

    /**
     * Scales down the image \p img in a way that it fits into the
     * given maximum width and height.
     */
    void scaleDownImage(QImage& img, int maxWidth, int maxHeight);

    /**
     * Create and draw the SubThumbnail
     **/
    bool drawSubThumbnail(QPainter& p, const QString& filePath, int width, int height,
                          int xPos, int yPos, int frameWidth);

    /**
     * Create a thumbnail using the given creator.
     * If possible, it's done inside a sandbox.
     **/
    bool createThumbnail(const QString &pluginName, ThumbCreator *creator, const QString &path, QSize size, QImage &img, float sequenceIndex);
private:
	SandboxedThumbnailerRunner m_sandboxedThumbnailerRunner;
    QString m_mimeType;
    int m_width;
    int m_height;
    int m_iconSize;
    int m_iconAlpha;
    // Thumbnail creators
    QHash<QString, ThumbCreator*> m_creators;
    // transparent icon cache
    QHash<QString, QImage> m_iconDict;
    QStringList m_enabledPlugins;
    QSet<QString> m_propagationDirectories;
    QString m_thumbBasePath;
    qint64 m_maxFileSize;
};

#endif
