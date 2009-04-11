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

#include <QtCore/QHash>

#include <kio/slavebase.h>

class ThumbCreator;
class QImage;

class ThumbnailProtocol : public KIO::SlaveBase
{
public:
    ThumbnailProtocol(const QByteArray &pool, const QByteArray &app);
    virtual ~ThumbnailProtocol();

    virtual void get(const KUrl &url);

protected:
    ThumbCreator* getThumbCreator(const QString& plugin);
    const QImage getIcon();
    void drawPictureFrame(QPainter *painter, const QPoint &pos, const QImage &image,
                          int frameWidth, QSize imageTargetSize) const;
    QImage thumbForDirectory(const KUrl& directory);
    QString pluginForMimeType(const QString& mimeType);

    float sequenceIndex() const;

private:
    QString m_mimeType;
    int m_width;
    int m_height;
    bool m_keepAspectRatio;
    int m_iconSize;
    int m_iconAlpha;
    // Thumbnail creators
    QHash<QString, ThumbCreator*> m_creators;
    // transparent icon cache
    QHash<QString, QImage> m_iconDict;
    QStringList m_enabledPlugins;
    QString m_thumbBasePath;
};

#endif
