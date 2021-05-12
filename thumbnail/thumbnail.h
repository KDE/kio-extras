/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _THUMBNAIL_H_
#define _THUMBNAIL_H_

#include <QHash>
#include <QPainter>
#include <QSet>
#include <QRandomGenerator>

#include <kio/slavebase.h>

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
    bool isOpaque(const QImage &image) const;
    void drawPictureFrame(QPainter *painter, const QPoint &pos, const QImage &image,
                          int frameWidth, QSize imageTargetSize, int rotationAngle) const;
    QImage thumbForDirectory(const QString& directory);
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
     * Draw the SubThumbnail
     **/
    void drawSubThumbnail(QPainter& p, QImage subThumbnail, int width, int height,
                          int xPos, int yPos, int frameWidth);
private:
    QString m_mimeType;
    int m_width;
    int m_height;
    // Thumbnail creators
    QHash<QString, ThumbCreator*> m_creators;
    QStringList m_enabledPlugins;
    QSet<QString> m_propagationDirectories;
    QString m_thumbBasePath;
    qint64 m_maxFileSize;
    QRandomGenerator m_randomGenerator;
};

#endif
