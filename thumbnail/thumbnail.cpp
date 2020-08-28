/*  This file is part of the KDE libraries
    Copyright (C) 2000 Malte Starostik <malte@kde.org>
                  2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "thumbnail.h"

#include <stdlib.h>
#ifdef __FreeBSD__
    #include <machine/param.h>
#endif
#include <sys/types.h>
#ifndef Q_OS_WIN
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h> // nice()
#endif

#include <QApplication>
#include <QBuffer>
#include <QFile>
#include <QSaveFile>
#include <QBitmap>
#include <QCryptographicHash>
#include <QImage>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QUrl>
#include <QMimeType>
#include <QMimeDatabase>
#include <QLibrary>
#include <QTemporaryFile>
#include <QDebug>

#include <KFileItem>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>
#include <KPluginLoader>

#include <kaboutdata.h>
#include <kiconloader.h>

#include <kio/thumbcreator.h>
#include <kio/thumbsequencecreator.h>
#include <kio/previewjob.h>

#include <iostream>
#include <limits>
#include <QDirIterator>

// Use correctly KComponentData instead of KApplication (but then no QPixmap)
#undef USE_KINSTANCE
// Fix thumbnail: protocol
#define THUMBNAIL_HACK (1)

#ifdef THUMBNAIL_HACK
# include <QFileInfo>
#endif

#include "imagefilter.h"

// Recognized metadata entries:
// mimeType     - the mime type of the file, used for the overlay icon if any
// width        - maximum width for the thumbnail
// height       - maximum height for the thumbnail
// iconSize     - the size of the overlay icon to use if any
// iconAlpha    - the transparency value used for icon overlays
// plugin       - the name of the plugin library to be used for thumbnail creation.
//                Provided by the application to save an addition KTrader
//                query here.
// enabledPlugins - a list of enabled thumbnailer plugins. PreviewJob does not call
//                  this thumbnail slave when a given plugin isn't enabled. However,
//                  for directory thumbnails it doesn't know that the thumbnailer
//                  internally also loads the plugins.
// shmid        - the shared memory segment id to write the image's data to.
//                The segment is assumed to provide enough space for a 32-bit
//                image sized width x height pixels.
//                If this is given, the data returned by the slave will be:
//                    int width
//                    int height
//                    int depth
//                Otherwise, the data returned is the image in PNG format.

using namespace KIO;
//using namespace KCodecs;

extern "C" Q_DECL_EXPORT int kdemain( int argc, char **argv )
{
#ifdef HAVE_NICE
    nice( 5 );
#endif

#ifdef USE_KINSTANCE
    KComponentData componentData("kio_thumbnail");
#else

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // creating KApplication in a slave in not a very good idea,
    // as dispatchLoop() doesn't allow it to process its messages,
    // so it for example wouldn't reply to ksmserver - on the other
    // hand, this slave uses QPixmaps for some reason, and they
    // need QApplication
    // and HTML previews need even KApplication :(
    putenv(strdup("SESSION_MANAGER="));

    // some thumbnail plugins reuse QWidget-tainted code for the rendering,
    // so use QApplication here, not just QGuiApplication
    QApplication app(argc, argv);
#endif


    if (argc != 4) {
        qCritical() << "Usage: kio_thumbnail protocol domain-socket1 domain-socket2";
        exit(-1);
    }

    ThumbnailProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}


ThumbnailProtocol::ThumbnailProtocol(const QByteArray &pool, const QByteArray &app)
    : SlaveBase("thumbnail", pool, app),
      m_iconSize(0),
      m_maxFileSize(0)
{

}

ThumbnailProtocol::~ThumbnailProtocol()
{
    qDeleteAll( m_creators );
    m_creators.clear();
}

void ThumbnailProtocol::get(const QUrl &url)
{
    m_mimeType = metaData("mimeType");
    m_enabledPlugins = metaData("enabledPlugins").split(QLatin1Char(','), QString::SkipEmptyParts);
    if (m_enabledPlugins.isEmpty()) {
        const KConfigGroup globalConfig(KSharedConfig::openConfig(), "PreviewSettings");
        m_enabledPlugins = globalConfig.readEntry("Plugins", KIO::PreviewJob::defaultPlugins());
    }

    Q_ASSERT(url.scheme() == "thumbnail");
    QFileInfo info(url.path());
    Q_ASSERT(info.isAbsolute());

    if (!info.exists()) {
	// The file does not exist
	error(KIO::ERR_DOES_NOT_EXIST, url.path());
	return;
    } else if (!info.isReadable()) {
	// The file is not readable!
	error(KIO::ERR_CANNOT_READ, url.path());
	return;
    }

    //qDebug() << "Wanting MIME Type:" << m_mimeType;
#ifdef THUMBNAIL_HACK
    // ### HACK
    bool direct=false;
    if (m_mimeType.isEmpty()) {
        //qDebug() << "PATH: " << url.path() << "isDir:" << info.isDir();
        if (info.isDir()) {
            m_mimeType = "inode/directory";
        } else {
            const QMimeDatabase db;

            m_mimeType = db.mimeTypeForFile(info).name();
        }

        //qDebug() << "Guessing MIME Type:" << m_mimeType;
        direct=true; // thumbnail: URL was probably typed in Konqueror
    }
#endif

    if (m_mimeType.isEmpty()) {
        error(KIO::ERR_INTERNAL, i18n("No MIME Type specified."));
        return;
    }

    m_width = metaData("width").toInt();
    m_height = metaData("height").toInt();
    int iconSize = metaData("iconSize").toInt();

    if (m_width < 0 || m_height < 0) {
        error(KIO::ERR_INTERNAL, i18n("No or invalid size specified."));
        return;
    }
#ifdef THUMBNAIL_HACK
    else if (!m_width || !m_height) {
        //qDebug() << "Guessing height, width, icon size!";
        m_width = 128;
        m_height = 128;
        iconSize = 128;
    }
#endif

    if (!iconSize) {
        iconSize = KIconLoader::global()->currentSize(KIconLoader::Desktop);
    }
    if (iconSize != m_iconSize) {
        m_iconDict.clear();
    }
    m_iconSize = iconSize;

    m_iconAlpha = metaData("iconAlpha").toInt();

    QImage img;

    KConfigGroup group( KSharedConfig::openConfig(), "PreviewSettings" );
    bool kfmiThumb = false; // TODO Figure out if we can use KFileMetadata as a last resource

    ThumbCreator::Flags flags = ThumbCreator::None;

    if (!kfmiThumb) {
        QString plugin = metaData("plugin");
        if ((plugin.isEmpty() || plugin == "directorythumbnail") && m_mimeType == "inode/directory") {
            img = thumbForDirectory(info.canonicalFilePath());
            if(img.isNull()) {
              error(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for directory"));
              return;
            }
        } else {
#ifdef THUMBNAIL_HACK
            if (plugin.isEmpty()) {
                plugin = pluginForMimeType(m_mimeType);
            }

            //qDebug() << "Guess plugin: " << plugin;
#endif
            if (plugin.isEmpty()) {
                error(KIO::ERR_INTERNAL, i18n("No plugin specified."));
                return;
            }

            ThumbCreator* creator = getThumbCreator(plugin);
            if(!creator) {
                error(KIO::ERR_INTERNAL, i18n("Cannot load ThumbCreator %1", plugin));
                return;
            }

            ThumbSequenceCreator* sequenceCreator = dynamic_cast<ThumbSequenceCreator*>(creator);
            if(sequenceCreator)
                sequenceCreator->setSequenceIndex(sequenceIndex());

            if (!creator->create(info.canonicalFilePath(), m_width, m_height, img)) {
                error(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for %1", info.canonicalFilePath()));
                return;
            }
            flags = creator->flags();
        }
    }

    scaleDownImage(img, m_width, m_height);

    if ((flags & ThumbCreator::BlendIcon) && KIconLoader::global()->alphaBlending(KIconLoader::Desktop)) {
        // blending the mimetype icon in
        QImage icon = getIcon();

        int x = img.width() - icon.width() - 4;
        x = qMax( x, 0 );
        int y = img.height() - icon.height() - 6;
        y = qMax( y, 0 );
        QPainter p(&img);
        p.setOpacity(m_iconAlpha/255.0);
        p.drawImage(x, y, icon);
    }

    if (img.isNull()) {
        error(KIO::ERR_INTERNAL, i18n("Failed to create a thumbnail."));
        return;
    }

    const QString shmid = metaData("shmid");
    if (shmid.isEmpty()) {
#ifdef THUMBNAIL_HACK
        if (direct) {
            // If thumbnail was called directly from Konqueror, then the image needs to be raw
            //qDebug() << "RAW IMAGE TO STREAM";
            QBuffer buf;
            if (!buf.open(QIODevice::WriteOnly)) {
                error(KIO::ERR_INTERNAL, i18n("Could not write image."));
                return;
            }
            img.save(&buf,"PNG");
            buf.close();
            mimeType("image/png");
            data(buf.buffer());
        }
        else
#endif
        {
            QByteArray imgData;
            QDataStream stream( &imgData, QIODevice::WriteOnly );
            //qDebug() << "IMAGE TO STREAM";
            stream << img;
            mimeType("application/octet-stream");
            data(imgData);
        }
    } else {
#ifndef Q_OS_WIN
        QByteArray imgData;
        QDataStream stream( &imgData, QIODevice::WriteOnly );
        //qDebug() << "IMAGE TO SHMID";
        void *shmaddr = shmat(shmid.toInt(), nullptr, 0);
        if (shmaddr == (void *)-1) {
            error(KIO::ERR_INTERNAL, i18n("Failed to attach to shared memory segment %1", shmid));
            return;
        }
        if (img.width() * img.height() > m_width * m_height) {
            error(KIO::ERR_INTERNAL, i18n("Image is too big for the shared memory segment"));
            shmdt((char*)shmaddr);
            return;
        }
        if( img.format() != QImage::Format_ARGB32 ) { // KIO::PreviewJob and this code below completely ignores colortable :-/,
            img = img.convertToFormat(QImage::Format_ARGB32); //  so make sure there is none
        }
        // Keep in sync with kdelibs/kio/kio/previewjob.cpp
        stream << img.width() << img.height() << quint8(img.format());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        memcpy(shmaddr, img.bits(), img.sizeInBytes());
#else
        memcpy(shmaddr, img.bits(), img.byteCount());
#endif
        shmdt((char*)shmaddr);
        mimeType("application/octet-stream");
        data(imgData);
#endif
    }
    finished();
}

QString ThumbnailProtocol::pluginForMimeType(const QString& mimeType) {
    KService::List offers = KMimeTypeTrader::self()->query( mimeType, QLatin1String("ThumbCreator"));
    if (!offers.isEmpty()) {
        KService::Ptr serv;
        serv = offers.first();
        return serv->library();
    }

    //Match group mimetypes
    ///@todo Move this into some central location together with the related matching code in previewjob.cpp. This doesn't handle inheritance and such
    const KService::List plugins = KServiceTypeTrader::self()->query("ThumbCreator");
    for (const KService::Ptr& plugin : plugins) {
        const QStringList mimeTypes = plugin->serviceTypes();
        for (const QString& mime : mimeTypes) {
            if(mime.endsWith('*')) {
                const auto mimeGroup = mime.leftRef(mime.length()-1);
                if(mimeType.startsWith(mimeGroup))
                    return plugin->library();
            }
        }
    }

    return QString();
}

float ThumbnailProtocol::sequenceIndex() const {
    return metaData("sequence-index").toFloat();
}

bool ThumbnailProtocol::isOpaque(const QImage &image) const
{
    // Test the corner pixels
    return qAlpha(image.pixel(QPoint(0, 0))) == 255 &&
           qAlpha(image.pixel(QPoint(image.width()-1, 0))) == 255 &&
           qAlpha(image.pixel(QPoint(0, image.height()-1))) == 255 &&
           qAlpha(image.pixel(QPoint(image.width()-1, image.height()-1))) == 255;
}

void ThumbnailProtocol::drawPictureFrame(QPainter *painter, const QPoint &centerPos,
                                         const QImage &image, int frameWidth, QSize imageTargetSize) const
{
    // Scale the image down so it matches the aspect ratio
    float scaling = 1.0;

    if ((image.size().width() > imageTargetSize.width()) && (imageTargetSize.width() != 0)) {
        scaling = float(imageTargetSize.width()) / float(image.size().width());
    } else if ((image.size().height() > imageTargetSize.height()) && (imageTargetSize.height() != 0)) {
        scaling = float(imageTargetSize.height()) / float(image.size().height());
    }

    QImage frame(imageTargetSize + QSize(frameWidth * 2, frameWidth * 2),
                 QImage::Format_ARGB32);
    frame.fill(0);

    float scaledFrameWidth = frameWidth / scaling;

    QTransform m;
    m.rotate(qrand() % 17 - 8); // Random rotation ±8°
    m.scale(scaling, scaling);

    QRectF frameRect(QPointF(0, 0), QPointF(image.width() + scaledFrameWidth*2, image.height() + scaledFrameWidth*2));

    QRect r = m.mapRect(QRectF(frameRect)).toAlignedRect();

    QImage transformed(r.size(), QImage::Format_ARGB32);
    transformed.fill(0);
    QPainter p(&transformed);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setCompositionMode(QPainter::CompositionMode_Source);

    p.translate(-r.topLeft());
    p.setWorldTransform(m, true);

    if (isOpaque(image)) {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawRoundedRect(frameRect, scaledFrameWidth / 2, scaledFrameWidth / 2);
    }
    p.drawImage(scaledFrameWidth, scaledFrameWidth, image);
    p.end();

    int radius = qMax(frameWidth, 1);

    QImage shadow(r.size() + QSize(radius * 2, radius * 2), QImage::Format_ARGB32);
    shadow.fill(0);

    p.begin(&shadow);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(radius, radius, transformed);
    p.end();

    ImageFilter::shadowBlur(shadow, radius, QColor(0, 0, 0, 128));

    r.moveCenter(centerPos);

    painter->drawImage(r.topLeft() - QPoint(radius / 2, radius / 2), shadow);
    painter->drawImage(r.topLeft(), transformed);
}

QImage ThumbnailProtocol::thumbForDirectory(const QString& directory)
{
    QImage img;
    if (m_propagationDirectories.isEmpty()) {
        // Directories that the directory preview will be propagated into if there is no direct sub-directories
        const KConfigGroup globalConfig(KSharedConfig::openConfig(), "PreviewSettings");
        m_propagationDirectories = globalConfig.readEntry("PropagationDirectories", QStringList() << "VIDEO_TS").toSet();
        m_maxFileSize = globalConfig.readEntry("MaximumSize", std::numeric_limits<qint64>::max());
    }

    const int tiles = 2; //Count of items shown on each dimension
    const int spacing = 1;
    const int visibleCount = tiles * tiles;

    // TODO: the margins are optimized for the Oxygen iconset
    // Provide a fallback solution for other iconsets (e. g. draw folder
    // only as small overlay, use no margins)

    KFileItem item(QUrl::fromLocalFile(directory));
    const int extent = qMin(m_width, m_height);
    QPixmap folder = QIcon::fromTheme(item.iconName()).pixmap(extent);

    // Scale up base icon to ensure overlays are rendered with
    // the best quality possible even for low-res custom folder icons
    if (qMax(folder.width(), folder.height()) < extent) {
        folder = folder.scaled(extent, extent, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    const int folderWidth  = folder.width();
    const int folderHeight = folder.height();

    const int topMargin = folderHeight * 30 / 100;
    const int bottomMargin = folderHeight / 6;
    const int leftMargin = folderWidth / 13;
    const int rightMargin = leftMargin;

    const int segmentWidth  = (folderWidth  - leftMargin - rightMargin  + spacing) / tiles - spacing;
    const int segmentHeight = (folderHeight - topMargin  - bottomMargin + spacing) / tiles - spacing;
    if ((segmentWidth < 5) || (segmentHeight <  5)) {
        // the segment size is too small for a useful preview
        return img;
    }

    // Advance to the next tile page each second
    int skipValidItems = ((int)sequenceIndex()) * visibleCount;

    img = QImage(QSize(folderWidth, folderHeight), QImage::Format_ARGB32);
    img.fill(0);

    QPainter p;
    p.begin(&img);

    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawPixmap(0, 0, folder);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    int xPos = leftMargin;
    int yPos = topMargin;

    int frameWidth = qRound(folderWidth / 85.);

    int iterations = 0;
    QString hadFirstThumbnail;
    QImage firstThumbnail;

    int validThumbnails = 0;

    while (true) {
        QDirIterator dir(directory, QDir::Files | QDir::Readable);
        int skipped = 0;

        // Seed the random number generator so that it always returns the same result
        // for the same directory and sequence-item
        qsrand(qHash(directory) + skipValidItems);

        while (dir.hasNext()) {
            ++iterations;
            if (iterations > 500) {
                skipValidItems = skipped = 0;
                break;
            }

            dir.next();

            auto fileSize = dir.fileInfo().size();
            if ((fileSize == 0) || (fileSize > m_maxFileSize)) {
                // don't create thumbnails for files that exceed
                // the maximum set file size or are empty
                continue;
            }

            QImage subThumbnail;
            if (!createSubThumbnail(subThumbnail, dir.filePath(), segmentWidth, segmentHeight)) {
                continue;
            }

            if (skipped < skipValidItems) {
                ++skipped;
                continue;
            }

            if (!drawSubThumbnail(p, subThumbnail, segmentWidth, segmentHeight, xPos, yPos, frameWidth)) {
                continue;
            }

            if (hadFirstThumbnail.isEmpty()) {
                hadFirstThumbnail = dir.filePath();
                firstThumbnail = subThumbnail;
            }

            ++validThumbnails;
            if (validThumbnails >= visibleCount) {
                break;
            }

            xPos += segmentWidth + spacing;
            if (xPos > folderWidth - rightMargin - segmentWidth) {
                xPos = leftMargin;
                yPos += segmentHeight + spacing;
            }
        }

        if (validThumbnails > 0) {
            break;
        }

        if (skipped == 0) {
            break; // No valid items were found
        }

        // Calculate number of (partial) pages for all valid items in the directory
        auto skippedPages = (skipped + visibleCount - 1) / visibleCount;

        // The sequence is continously repeated after all valid items, calculate remainder
        skipValidItems = (((int)sequenceIndex()) % skippedPages) * visibleCount;
    }

    p.end();

    if (validThumbnails == 0) {
        // Eventually propagate the contained items from a sub-directory
        QDirIterator dir(directory, QDir::Dirs);
        int max = 50;
        while (dir.hasNext() && max > 0) {
            --max;
            dir.next();
            if (m_propagationDirectories.contains(dir.fileName())) {
                return thumbForDirectory(dir.filePath());
            }
        }

        // If no thumbnail could be found, return an empty image which indicates
        // that no preview for the directory is available.
        img = QImage();
    }

    // If only for one file a thumbnail could be generated then paint an image with only one tile
    if (validThumbnails == 1) {
        QImage oneTileImg(folder.size(), QImage::Format_ARGB32);
        oneTileImg.fill(0);

        QPainter oneTilePainter(&oneTileImg);
        oneTilePainter.setCompositionMode(QPainter::CompositionMode_Source);
        oneTilePainter.drawPixmap(0, 0, folder);
        oneTilePainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        const int oneTileWidth = folderWidth - leftMargin - rightMargin;
        const int oneTileHeight = folderHeight - topMargin - bottomMargin;

        if (firstThumbnail.width() < oneTileWidth && firstThumbnail.height() < oneTileHeight) {
            createSubThumbnail(firstThumbnail, hadFirstThumbnail, oneTileWidth, oneTileHeight);
        }
        drawSubThumbnail(oneTilePainter, firstThumbnail, oneTileWidth, oneTileHeight, leftMargin, topMargin, frameWidth);
        return oneTileImg;
    }

    return img;
}

ThumbCreator* ThumbnailProtocol::getThumbCreator(const QString& plugin)
{
    ThumbCreator *creator = m_creators[plugin];
    if (!creator) {
        // Don't use KPluginFactory here, this is not a QObject and
        // neither is ThumbCreator
        QLibrary library(KPluginLoader::findPlugin((plugin)));
        if (library.load()) {
            newCreator create = (newCreator)library.resolve("new_creator");
            if (create) {
                creator = create();
            }
        }
        if (!creator) {
            return nullptr;
        }

        m_creators.insert(plugin, creator);
    }

    return creator;
}


const QImage ThumbnailProtocol::getIcon()
{
    const QMimeDatabase db;

    ///@todo Can we really do this? It doesn't seem to respect the size
    if (!m_iconDict.contains(m_mimeType)) { // generate it
        QImage icon(KIconLoader::global()->loadMimeTypeIcon(db.mimeTypeForName(m_mimeType).iconName(), KIconLoader::Desktop, m_iconSize).toImage());
        icon = icon.convertToFormat(QImage::Format_ARGB32);
        m_iconDict.insert(m_mimeType, icon);

        return icon;
    }

    return m_iconDict.value(m_mimeType);
}

bool ThumbnailProtocol::createSubThumbnail(QImage& thumbnail, const QString& filePath,
                                           int segmentWidth, int segmentHeight)
{
    auto getSubCreator = [&filePath, this]() -> ThumbCreator* {
        const QMimeDatabase db;
        const QString subPlugin = pluginForMimeType(db.mimeTypeForFile(filePath).name());
        if (subPlugin.isEmpty() || !m_enabledPlugins.contains(subPlugin)) {
            return nullptr;
        }
        return getThumbCreator(subPlugin);
    };

    if ((segmentWidth <= 256) && (segmentHeight <= 256)) {
        // check whether a cached version of the file is available for
        // 128 x 128 or 256 x 256 pixels
        int cacheSize = 0;
        QCryptographicHash md5(QCryptographicHash::Md5);
        const QByteArray fileUrl = QUrl::fromLocalFile(filePath).toEncoded();
        md5.addData(fileUrl);
        const QString thumbName = QString::fromLatin1(md5.result().toHex()).append(".png");

        if (m_thumbBasePath.isEmpty()) {
            m_thumbBasePath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/thumbnails/");
            QDir basePath(m_thumbBasePath);
            basePath.mkpath("normal/");
            QFile::setPermissions(basePath.absoluteFilePath("normal"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
            basePath.mkpath("large/");
            QFile::setPermissions(basePath.absoluteFilePath("large"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        }

        QDir thumbPath(m_thumbBasePath);
        if ((segmentWidth <= 128) && (segmentHeight <= 128)) {
            cacheSize = 128;
            thumbPath.cd("normal");
        } else {
            cacheSize = 256;
            thumbPath.cd("large");
        }

        if (thumbnail.load(thumbPath.absoluteFilePath(thumbName))) {
            return true;
        } else if (cacheSize == 128) {
            QDir fallbackPath(m_thumbBasePath);
            fallbackPath.cd("large");
            if (thumbnail.load(fallbackPath.absoluteFilePath(thumbName))) {
                return true;
            }
        }

        if (thumbnail.isNull()) {
            // no cached version is available, a new thumbnail must be created

            QSaveFile thumbnailfile(thumbPath.absoluteFilePath(thumbName));
            bool savedCorrectly = false;
            ThumbCreator* subCreator = getSubCreator();
            if (subCreator && subCreator->create(filePath, cacheSize, cacheSize, thumbnail)) {
                scaleDownImage(thumbnail, cacheSize, cacheSize);

                // The thumbnail has been created successfully. Store the thumbnail
                // to the cache for future access.
                if (thumbnailfile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    savedCorrectly = thumbnail.save(&thumbnailfile, "PNG");
                }
            } else {
                return false;
            }
            if(savedCorrectly)
            {
                thumbnailfile.commit();
            }
        }
    } else {
        ThumbCreator* subCreator = getSubCreator();
        return subCreator && subCreator->create(filePath, segmentWidth, segmentHeight, thumbnail);
    }
    return true;
}

void ThumbnailProtocol::scaleDownImage(QImage& img, int maxWidth, int maxHeight)
{
    if (img.width() > maxWidth || img.height() > maxHeight) {
        img = img.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}

bool ThumbnailProtocol::drawSubThumbnail(QPainter& p, QImage subThumbnail, int width, int height, int xPos, int yPos, int frameWidth)
{
    // Apply fake smooth scaling, as seen on several blogs
    if (subThumbnail.width() > width * 4 || subThumbnail.height() > height * 4) {
        subThumbnail = subThumbnail.scaled(width*4, height*4, Qt::KeepAspectRatio, Qt::FastTransformation);
    }

    QSize targetSize(subThumbnail.size());
    targetSize.scale(width, height, Qt::KeepAspectRatio);

    // center the image inside the segment boundaries
    const QPoint centerPos(xPos + (width/ 2), yPos + (height / 2));
    drawPictureFrame(&p, centerPos, subThumbnail, frameWidth, targetSize);

    return true;
}
