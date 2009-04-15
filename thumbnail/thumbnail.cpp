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
#include <unistd.h>
#ifdef __FreeBSD__
    #include <machine/param.h>
#endif
#include <sys/types.h>
#ifndef Q_WS_WIN
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtGui/QBitmap>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include <kcodecs.h>
#include <kurl.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <klibloader.h>
#include <kdebug.h>
#include <kservice.h>
#include <kservicetype.h>
#include <kservicetypetrader.h>
#include <kmimetypetrader.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kfilemetainfo.h>
#include <klocale.h>
#include <kde_file.h>

#include <config-runtime.h> // For HAVE_NICE
#include <kio/thumbcreator.h>
#include <kio/thumbsequencecreator.h>
#include <kconfiggroup.h>

#include <iostream>
#include <QtCore/QDirIterator>

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
// shmid        - the shared memory segment id to write the image's data to.
//                The segment is assumed to provide enough space for a 32-bit
//                image sized width x height pixels.
//                If this is given, the data returned by the slave will be:
//                    int width
//                    int height
//                    int depth
//                Otherwise, the data returned is the image in PNG format.

using namespace KIO;

extern "C"
{
    KDE_EXPORT int kdemain(int argc, char **argv);
}


int kdemain(int argc, char **argv)
{
#ifdef HAVE_NICE
    nice( 5 );
#endif

#ifdef USE_KINSTANCE
    KComponentData componentData("kio_thumbnail");
#else
    // creating KApplication in a slave in not a very good idea,
    // as dispatchLoop() doesn't allow it to process its messages,
    // so it for example wouldn't reply to ksmserver - on the other
    // hand, this slave uses QPixmaps for some reason, and they
    // need QApplication
    // and HTML previews need even KApplication :(
    putenv(strdup("SESSION_MANAGER="));
    //KApplication::disableAutoDcopRegistration();
    KAboutData about("kio_thumbnail", 0, ki18n("kio_thumbmail"), "KDE 4.x.x");
    KCmdLineArgs::init(&about);

    KApplication app( true);
#endif


    if (argc != 4) {
        kError(7115) << "Usage: kio_thumbnail protocol domain-socket1 domain-socket2" << endl;
        exit(-1);
    }

    ThumbnailProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}

ThumbnailProtocol::ThumbnailProtocol(const QByteArray &pool, const QByteArray &app)
    : SlaveBase("thumbnail", pool, app)
{
    m_iconSize = 0;
}

ThumbnailProtocol::~ThumbnailProtocol()
{
    qDeleteAll( m_creators );
    m_creators.clear();
}

void ThumbnailProtocol::get(const KUrl &url)
{
    m_mimeType = metaData("mimeType");
    kDebug(7115) << "Wanting MIME Type:" << m_mimeType;
#ifdef THUMBNAIL_HACK
    // ### HACK
    bool direct=false;
    if (m_mimeType.isEmpty()) {
        QFileInfo info(url.path());
        kDebug(7115) << "PATH: " << url.path() << "isDir:" << info.isDir();
        if (!info.exists()) {
            // The file does not exist
            error(KIO::ERR_DOES_NOT_EXIST,url.path());
            return;
        } else if (!info.isReadable()) {
            // The file is not readable!
            error(KIO::ERR_COULD_NOT_READ,url.path());
            return;
        }

        if (info.isDir()) {
            m_mimeType = "inode/directory";
        } else {
            KMimeType::findByUrl(KUrl(info.filePath()))->name();
        }

        kDebug(7115) << "Guessing MIME Type:" << m_mimeType;
        direct=true; // thumbnail: was probably called from Konqueror
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
        kDebug(7115) << "Guessing height, width, icon size!";
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

    KConfigGroup group( KGlobal::config(), "PreviewSettings" );

    // ### KFMI
    bool kfmiThumb = false;
    if (group.readEntry( "UseFileThumbnails", true)) {
        KService::Ptr service =
            KMimeTypeTrader::self()->preferredService( m_mimeType, "KFilePlugin");

        if (service && service->isValid() &&
            service->property("SupportsThumbnail").toBool()) {
            // was:  KFileMetaInfo info(url.path(), m_mimeType, KFileMetaInfo::Thumbnail);
            // but m_mimeType and WhatFlags are now unused in KFileMetaInfo, and not present in the
            // call that takes a KUrl
            KFileMetaInfo info(url);
            if (info.isValid()) {
                KFileMetaInfoItem item = info.item("thumbnail");
                if (item.isValid() && item.value().type() == QVariant::Image) {
                    img = item.value().value<QImage>();
                    kDebug(7115) << "using KFMI for the thumbnail\n";
                    kfmiThumb = true;
                }
            }
        }
    }
    ThumbCreator::Flags flags = ThumbCreator::None;

    if (!kfmiThumb) {
        QString plugin = metaData("plugin");
        if ((plugin.isEmpty() || plugin == "directorythumbnail") && m_mimeType == "inode/directory") {
            img = thumbForDirectory(url);
            if(img.isNull()) {
              error(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for directory"));
              return;
            }
        } else {
#ifdef THUMBNAIL_HACK
            if (plugin.isEmpty()) {
                plugin = pluginForMimeType(m_mimeType);
            }

            kDebug(7115) << "Guess plugin: " << plugin;
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

            if (!creator->create(url.path(), m_width, m_height, img)) {
                error(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for %1", url.path()));
                return;
            }
            flags = creator->flags();
        }
    }

    if (img.width() > m_width || img.height() > m_height) {
        double imgRatio = (double)img.height() / (double)img.width();
        if (imgRatio > (double)m_height / (double)m_width) {
            img = img.scaled( int(qMax((double)m_height / imgRatio, 1.0)), m_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        } else {
            img = img.scaled(m_width, int(qMax((double)m_width * imgRatio, 1.0)), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
    }

// ### FIXME
#ifndef USE_KINSTANCE
    if (flags & ThumbCreator::DrawFrame) {
        QPixmap pix = QPixmap::fromImage( img );
        int x2 = pix.width() - 1;
        int y2 = pix.height() - 1;
        // paint a black rectangle around the "page"
        QPainter p;
        p.begin( &pix );
        p.setPen( QColor( 48, 48, 48 ));
        p.drawLine( x2, 0, x2, y2 );
        p.drawLine( 0, y2, x2, y2 );
        p.setPen( QColor( 215, 215, 215 ));
        p.drawLine( 0, 0, x2, 0 );
        p.drawLine( 0, 0, 0, y2 );
        p.end();

        const QBitmap& mask = pix.mask();
        if (!mask.isNull()) { // need to update it so we can see the frame
            QBitmap bitmap( mask );
            QPainter painter;
            painter.begin( &bitmap );
            painter.drawLine( x2, 0, x2, y2 );
            painter.drawLine( 0, y2, x2, y2 );
            painter.drawLine( 0, 0, x2, 0 );
            painter.drawLine( 0, 0, 0, y2 );
            painter.end();

            pix.setMask( bitmap );
        }

        img = pix.toImage();
    }
#endif

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
            //kDebug(7115) << "RAW IMAGE TO STREAM";
            QBuffer buf;
            if (!buf.open(QIODevice::WriteOnly)) {
                error(KIO::ERR_INTERNAL, i18n("Could not write image."));
                return;
            }
            img.save(&buf,"PNG");
            buf.close();
            data(buf.buffer());
        }
        else
#endif
        {
            QByteArray imgData;
            QDataStream stream( &imgData, QIODevice::WriteOnly );
            //kDebug(7115) << "IMAGE TO STREAM";
            stream << img;
            data(imgData);
        }
    } else {
#ifndef Q_WS_WIN
        QByteArray imgData;
        QDataStream stream( &imgData, QIODevice::WriteOnly );
        //kDebug(7115) << "IMAGE TO SHMID";
        void *shmaddr = shmat(shmid.toInt(), 0, 0);
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
        memcpy(shmaddr, img.bits(), img.numBytes());
        shmdt((char*)shmaddr);
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

    return QString();
}

float ThumbnailProtocol::sequenceIndex() const {
    return metaData("sequence-index").toFloat();
}

void ThumbnailProtocol::drawPictureFrame(QPainter *painter, const QPoint &centerPos,
                                         const QImage &image, int frameWidth, QSize imageTargetSize) const
{
    // Scale the image down so it matches the aspect ratio
    float scaling = 1.0;

    if ((image.size().width() > imageTargetSize.width()) && (imageTargetSize.width() != 0)) {
        scaling = float(imageTargetSize.width()) / float(image.size().width());
    }

    QImage frame(imageTargetSize + QSize(frameWidth * 2, frameWidth * 2),
                 QImage::Format_ARGB32);
    frame.fill(0);

    float scaledFrameWidth = frameWidth / scaling;

    QTransform m;
    m.rotate(qrand() % 16 - 8); // Random rotation ±8°
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

    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(frameRect, scaledFrameWidth / 2, scaledFrameWidth / 2);
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

QImage ThumbnailProtocol::thumbForDirectory(const KUrl& directory)
{
    QImage img;

    const int tiles = 2; //Count of items shown on each dimension
    const int spacing = 1;

    // TODO: the margins are optimized for the Oxygen iconset
    // Provide a fallback solution for other iconsets (e. g. draw folder
    // only as small overlay, use no margins)
    const QPixmap folder = KIconLoader::global()->loadMimeTypeIcon("inode-directory",
                                                                   KIconLoader::Desktop,
                                                                   qMin(m_width, m_height));
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

    QString localFile = directory.path();

    QDirIterator dir(localFile, QDir::Files | QDir::Readable);
    if (!dir.hasNext()) {
        return img;
    }

    // Multiply with a high number, so we get some semi-random sequence
    int skipValidItems = ((int)sequenceIndex()) * tiles * tiles;
    if (skipValidItems) {
        skipValidItems = skipValidItems % QDir(localFile).count();
    }

    // Seed the random number generator so that it always returns the same result
    // for the same directory and sequence-item
    qsrand(qHash(localFile) + skipValidItems);

    img = QImage(QSize(folderWidth, folderHeight), QImage::Format_ARGB32);
    img.fill(0);

    QPainter p(&img);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawPixmap(0, 0, folder);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    int xPos = leftMargin;
    int yPos = topMargin;

    int frameWidth = qRound(folderWidth / 85.);

    int iterations = 0;
    bool hadThumbnail = false;
    int skipped = 0;

    const int maxYPos = folderHeight - bottomMargin - segmentHeight;

    while (dir.hasNext() && (yPos <= maxYPos)) {
        ++iterations;
        if (iterations > 50 + 10 * skipValidItems) {
            // kDebug(7115) << "maximum iteration reached";
            return QImage();
        }

        dir.next();

        if (skipped < skipValidItems) {
          ++skipped;;
          continue;
        }

        QImage subThumbnail;
        if (!createSubThumbnail(subThumbnail, dir.filePath(), segmentWidth, segmentHeight)) {
            continue;
        }

        hadThumbnail = true;

        // Apply fake smooth scaling, as seen on several blogs
        if(subThumbnail.width() > segmentWidth * 4 || subThumbnail.height() > segmentHeight * 4)
            subThumbnail = subThumbnail.scaled(segmentWidth*4, segmentHeight*4, Qt::KeepAspectRatio, Qt::FastTransformation);

        QSize targetSize(subThumbnail.size());

        targetSize.scale(segmentWidth, segmentHeight, Qt::KeepAspectRatio);

        // center the image inside the segment boundaries
        const QPoint centerPos(xPos + (segmentWidth / 2), yPos + (segmentHeight / 2));
        drawPictureFrame(&p, centerPos, subThumbnail, frameWidth, targetSize);

        xPos += segmentWidth + spacing;
        if (xPos > folderWidth - rightMargin - segmentWidth) {
            xPos = leftMargin;
            yPos += segmentHeight + spacing;
        }
    }

    if (!hadThumbnail) {
        return QImage();
    }

    return img;
}

ThumbCreator* ThumbnailProtocol::getThumbCreator(const QString& plugin)
{
    ThumbCreator *creator = m_creators[plugin];
    if (!creator) {
        // Don't use KLibFactory here, this is not a QObject and
        // neither is ThumbCreator
        KLibrary *library = KLibLoader::self()->library(plugin);
        if (library) {
            newCreator create = (newCreator)library->resolveFunction("new_creator");
            if (create) {
                creator = create();
            }
        }
        if (!creator) {
            return 0;
        }

        m_creators.insert(plugin, creator);
    }

    return creator;
}


const QImage ThumbnailProtocol::getIcon()
{
    ///@todo Can we really do this? It doesn't seem to respect the size
    if (!m_iconDict.contains(m_mimeType)) { // generate it
        QImage icon( KIconLoader::global()->loadMimeTypeIcon( KMimeType::mimeType(m_mimeType)->iconName(), KIconLoader::Desktop, m_iconSize ).toImage() );
        icon = icon.convertToFormat(QImage::Format_ARGB32);
        m_iconDict.insert(m_mimeType, icon);

        return icon;
    }

    return m_iconDict.value(m_mimeType);
}

bool ThumbnailProtocol::createSubThumbnail(QImage& thumbnail, const QString& filePath,
                                           int segmentWidth, int segmentHeight)
{
    if (m_enabledPlugins.isEmpty()) {
        const KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
        m_enabledPlugins = globalConfig.readEntry("Plugins", QStringList()
                                                             << "imagethumbnail"
                                                             << "jpegthumbnail");
    }

    const KUrl fileName = filePath;
    const QString subPlugin = pluginForMimeType(KMimeType::findByUrl(fileName)->name());
    if (subPlugin.isEmpty() || !m_enabledPlugins.contains(subPlugin)) {
        // kDebug(7115) << "found no sub-plugin for" << dir.filePath();
        return false;
    }

    ThumbCreator* subCreator = getThumbCreator(subPlugin);
    if (!subCreator) {
        // kDebug(7115) << "found no creator for" << dir.filePath();
        return false;
    }

    if ((segmentWidth <= 256) && (segmentHeight <= 256)) {
        // check whether a cached version of the file is available for
        // 128 x 128 or 256 x 256 pixels
        int cacheSize = 0;
        KMD5 md5(QFile::encodeName(fileName.url()));
        const QString thumbName = QFile::encodeName(md5.hexDigest()) + ".png";
        if (m_thumbBasePath.isEmpty()) {
            m_thumbBasePath = QDir::homePath() + "/.thumbnails/";
            KStandardDirs::makeDir(m_thumbBasePath + "normal/", 0700);
            KStandardDirs::makeDir(m_thumbBasePath + "large/", 0700);
        }

        QString thumbPath = m_thumbBasePath;
        if ((segmentWidth <= 128) && (segmentHeight <= 128)) {
            cacheSize = 128;
            thumbPath += "normal/";
        } else {
            cacheSize = 256;
            thumbPath += "large/";
        }

        if (!thumbnail.load(thumbPath + thumbName)) {
            // no cached version is available, a new thumbnail must be created
            if (subCreator->create(filePath, cacheSize, cacheSize, thumbnail)) {
                // The thumbnail has been created successfully. Store the thumbnail
                // to the cache for future access.
                KTemporaryFile temp;
                temp.setPrefix(thumbPath + "kde-tmp-");
                temp.setSuffix(".png");
                temp.setAutoRemove(false);
                if (temp.open()) {
                    thumbnail.save(temp.fileName(), "PNG");
                    KDE::rename(temp.fileName(), thumbPath + thumbName);
                }
            } else {
                // kDebug(7115) <<  "failed to create thumbnail for" << dir.filePath();
                return false;
            }
        }
    } else if (!subCreator->create(filePath, segmentWidth, segmentHeight, thumbnail)) {
        return false;
    }

    return true;
}


