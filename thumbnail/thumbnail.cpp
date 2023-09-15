/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "thumbnail.h"
#include "thumbnail-logsettings.h"

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
#include <QColorSpace>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QImage>
#include <QLibrary>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPixmap>
#include <QPluginLoader>
#include <QSaveFile>
#include <QUrl>

#include <KConfigGroup>
#include <KFileItem>
#include <KLocalizedString>
#include <KSharedConfig>

#include <KIO/PreviewJob>
#include <KPluginFactory>

#include <kio_version.h>
#if KIO_VERSION < QT_VERSION_CHECK(5, 240, 0)
#include <KIO/ThumbCreator>
#include <KIO/ThumbDevicePixelRatioDependentCreator>
#include <KIO/ThumbSequenceCreator>
#endif

#include <QDirIterator>

#include <limits>
#include <variant>

// Fix thumbnail: protocol
#define THUMBNAIL_HACK (1)

#ifdef THUMBNAIL_HACK
#include <QFileInfo>
#endif

#include "imagefilter.h"

// Recognized metadata entries:
// mimeType     - the mime type of the file, used for the overlay icon if any
// width        - maximum width for the thumbnail
// height       - maximum height for the thumbnail
// iconSize     - the size of the overlay icon to use if any (deprecated, ignored)
// iconAlpha    - the transparency value used for icon overlays (deprecated, ignored)
// plugin       - the name of the plugin library to be used for thumbnail creation.
//                Provided by the application to save an addition KTrader
//                query here.
// devicePixelRatio - the devicePixelRatio to use for the output,
//                     the dimensions of the output is multiplied by it and output pixmap will have devicePixelRatio
// enabledPlugins - a list of enabled thumbnailer plugins. PreviewJob does not call
//                  this thumbnail worker when a given plugin isn't enabled. However,
//                  for directory thumbnails it doesn't know that the thumbnailer
//                  internally also loads the plugins.
// shmid        - the shared memory segment id to write the image's data to.
//                The segment is assumed to provide enough space for a 32-bit
//                image sized width x height pixels.
//                If this is given, the data returned by the worker will be:
//                    int width
//                    int height
//                    int depth
//                Otherwise, the data returned is the image in PNG format.

using namespace KIO;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.thumbnail" FILE "thumbnail.json")
};

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
#ifdef HAVE_NICE
    nice(5);
#endif

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    // Creating a QApplication in a worker in not a very good idea,
    // as dispatchLoop() doesn't allow it to process its messages,
    // so it for example wouldn't reply to ksmserver - on the other
    // hand, this worker uses QPixmaps for some reason, and they
    // need QGuiApplication
    qunsetenv("SESSION_MANAGER");

    // Some thumbnail plugins use QWidget classes for the rendering,
    // so use QApplication here, not just QGuiApplication
    QApplication app(argc, argv);

    if (argc != 4) {
        qCritical() << "Usage: kio_thumbnail protocol domain-socket1 domain-socket2";
        exit(-1);
    }

    ThumbnailProtocol worker(argv[2], argv[3]);
    worker.dispatchLoop();

    return 0;
}

ThumbnailProtocol::ThumbnailProtocol(const QByteArray &pool, const QByteArray &app)
    : WorkerBase("thumbnail", pool, app)
    , m_width(0)
    , m_height(0)
    , m_devicePixelRatio(1)
    , m_maxFileSize(0)
    , m_randomGenerator()
{
}

ThumbnailProtocol::~ThumbnailProtocol()
{
    qDeleteAll(m_creators);
}

/**
 * Scales down the image \p img in a way that it fits into the given maximum width and height
 */
void scaleDownImage(QImage &img, int maxWidth, int maxHeight)
{
    if (img.width() > maxWidth || img.height() > maxHeight) {
        img = img.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}

/**
 * @brief convertToStandardRgb
 * Convert preview to sRGB for proper viewing on most monitors.
 */
void convertToStandardRgb(QImage &img)
{
    auto cs = img.colorSpace();
    if (!cs.isValid()) {
        return;
    }
    if (cs.transferFunction() != QColorSpace::TransferFunction::SRgb || cs.primaries() != QColorSpace::Primaries::SRgb) {
        img.convertToColorSpace(QColorSpace(QColorSpace::SRgb));
    }
}

KIO::WorkerResult ThumbnailProtocol::get(const QUrl &url)
{
    m_mimeType = metaData("mimeType");
    m_enabledPlugins = metaData("enabledPlugins").split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (m_enabledPlugins.isEmpty()) {
        const KConfigGroup globalConfig(KSharedConfig::openConfig(), "PreviewSettings");
        m_enabledPlugins = globalConfig.readEntry("Plugins", KIO::PreviewJob::defaultPlugins());
    }

    Q_ASSERT(url.scheme() == "thumbnail");
    QFileInfo info(url.path());
    Q_ASSERT(info.isAbsolute());

    if (!info.exists()) {
        // The file does not exist
        return KIO::WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, url.path());
    } else if (!info.isReadable()) {
        // The file is not readable!
        return KIO::WorkerResult::fail(KIO::ERR_CANNOT_READ, url.path());
    }

    // qDebug() << "Wanting MIME Type:" << m_mimeType;
#ifdef THUMBNAIL_HACK
    // ### HACK
    bool direct = false;
    if (m_mimeType.isEmpty()) {
        // qDebug() << "PATH: " << url.path() << "isDir:" << info.isDir();
        if (info.isDir()) {
            m_mimeType = "inode/directory";
        } else {
            const QMimeDatabase db;

            m_mimeType = db.mimeTypeForFile(info).name();
        }

        // qDebug() << "Guessing MIME Type:" << m_mimeType;
        direct = true; // thumbnail: URL was probably typed in Konqueror
    }
#endif

    if (m_mimeType.isEmpty()) {
        return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("No MIME Type specified."));
    }

    m_width = metaData("width").toInt();
    m_height = metaData("height").toInt();

    if (m_width < 0 || m_height < 0) {
        return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("No or invalid size specified."));
    }
#ifdef THUMBNAIL_HACK
    else if (!m_width || !m_height) {
        // qDebug() << "Guessing height, width, icon size!";
        m_width = 128;
        m_height = 128;
    }
#endif
    bool ok;
    m_devicePixelRatio = metaData("devicePixelRatio").toInt(&ok);
    if (!ok || m_devicePixelRatio == 0) {
        m_devicePixelRatio = 1;
    } else {
        m_width *= m_devicePixelRatio;
        m_height *= m_devicePixelRatio;
    }

    QImage img;
    QString plugin = metaData("plugin");

    if ((plugin.isEmpty() || plugin.contains("directorythumbnail")) && m_mimeType == "inode/directory") {
        img = thumbForDirectory(info.canonicalFilePath());
        if (img.isNull()) {
            return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for directory"));
        }
    } else {
#ifdef THUMBNAIL_HACK
        if (plugin.isEmpty()) {
            plugin = pluginForMimeType(m_mimeType).fileName();
        }

        // qDebug() << "Guess plugin: " << plugin;
#endif
        if (plugin.isEmpty()) {
            return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("No plugin specified."));
        }

        ThumbCreatorWithMetadata *creator = getThumbCreator(plugin);
        if (!creator) {
            return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("Cannot load ThumbCreator %1", plugin));
        }

        if (creator->handleSequences) {
#if KIO_VERSION < QT_VERSION_CHECK(5, 240, 0)
            if (std::holds_alternative<LegacyThumbCreatorPtr>(creator->creator)) {
                auto *sequenceCreator = dynamic_cast<ThumbSequenceCreator *>(std::get<LegacyThumbCreatorPtr>(creator->creator).get());

                if (sequenceCreator) {
                    sequenceCreator->setSequenceIndex(sequenceIndex());
                    setMetaData("handlesSequences", QStringLiteral("1"));
                }
            } else {
                setMetaData("handlesSequences", QStringLiteral("1"));
            }
#else
            setMetaData("handlesSequences", QStringLiteral("1"));
#endif
        }

        if (!createThumbnail(creator, info.canonicalFilePath(), m_width, m_height, img)) {
            return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for %1", info.canonicalFilePath()));
        }

        // We MUST do this after calling create(), because the create() call itself might change it.
        if (creator->handleSequences) {
            setMetaData("sequenceIndexWraparoundPoint", QString::number(m_sequenceIndexWrapAroundPoint));
        }
    }

    scaleDownImage(img, m_width, m_height);

    if (img.isNull()) {
        return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("Failed to create a thumbnail."));
    }

    convertToStandardRgb(img);

#ifdef THUMBNAIL_HACK
    if (direct) {
        // If thumbnail was called directly from Konqueror, then the image needs to be raw
        // qDebug() << "RAW IMAGE TO STREAM";
        QBuffer buf;
        if (!buf.open(QIODevice::WriteOnly)) {
            return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("Could not write image."));
        }
        img.save(&buf, "PNG");
        buf.close();
        mimeType("image/png");
        data(buf.buffer());
        return KIO::WorkerResult::pass();
    }
#endif

    QByteArray imgData;
    QDataStream stream(&imgData, QIODevice::WriteOnly);

    // Keep in sync with kio/src/previewjob.cpp
    stream << img.width() << img.height() << img.format() << ((int)img.devicePixelRatio());

#ifndef Q_OS_WIN
    const QString shmid = metaData("shmid");
    if (shmid.isEmpty())
#endif
    {
        // qDebug() << "IMAGE TO STREAM";
        stream << img;
    }
#ifndef Q_OS_WIN
    else {
        // qDebug() << "IMAGE TO SHMID";
        void *shmaddr = shmat(shmid.toInt(), nullptr, 0);
        if (shmaddr == (void *)-1) {
            return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("Failed to attach to shared memory segment %1", shmid));
        }
        if (img.format() != QImage::Format_ARGB32) { // KIO::PreviewJob and this code below completely ignores colortable :-/,
            img = img.convertToFormat(QImage::Format_ARGB32); //  so make sure there is none
        }
        struct shmid_ds shmStat;
        if (shmctl(shmid.toInt(), IPC_STAT, &shmStat) == -1 || shmStat.shm_segsz < (uint)img.sizeInBytes()) {
            return KIO::WorkerResult::fail(KIO::ERR_INTERNAL, i18n("Image is too big for the shared memory segment"));
            shmdt((char *)shmaddr);
        }
        memcpy(shmaddr, img.constBits(), img.sizeInBytes());
        shmdt((char *)shmaddr);
    }
#endif
    mimeType("application/octet-stream");
    data(imgData);

    return KIO::WorkerResult::pass();
}

KPluginMetaData ThumbnailProtocol::pluginForMimeType(const QString &mimeType)
{
    const QVector<KPluginMetaData> plugins = KIO::PreviewJob::availableThumbnailerPlugins();
    for (const KPluginMetaData &plugin : plugins) {
        if (plugin.supportsMimeType(mimeType)) {
            return plugin;
        }
    }
    for (const auto &plugin : plugins) {
        const QStringList mimeTypes = plugin.mimeTypes() + plugin.value(QStringLiteral("ServiceTypes"), QStringList());
        for (const QString &mime : mimeTypes) {
            if (mime.endsWith('*')) {
                const auto mimeGroup = QStringView(mime).left(mime.length() - 1);
                if (mimeType.startsWith(mimeGroup)) {
                    return plugin;
                }
            }
        }
    }

    return {};
}

float ThumbnailProtocol::sequenceIndex() const
{
    return metaData("sequence-index").toFloat();
}

bool ThumbnailProtocol::isOpaque(const QImage &image) const
{
    // Test the corner pixels
    return qAlpha(image.pixel(QPoint(0, 0))) == 255 && qAlpha(image.pixel(QPoint(image.width() - 1, 0))) == 255
        && qAlpha(image.pixel(QPoint(0, image.height() - 1))) == 255 && qAlpha(image.pixel(QPoint(image.width() - 1, image.height() - 1))) == 255;
}

void ThumbnailProtocol::drawPictureFrame(QPainter *painter,
                                         const QPoint &centerPos,
                                         const QImage &image,
                                         int borderStrokeWidth,
                                         QSize imageTargetSize,
                                         int rotationAngle) const
{
    // Scale the image down so it matches the aspect ratio
    float scaling = 1.0;

    const bool landscapeDimension = image.width() > image.height();
    const bool hasTargetSizeWidth = imageTargetSize.width() != 0;
    const bool hasTargetSizeHeight = imageTargetSize.height() != 0;
    const int widthWithFrames = image.width() + (2 * borderStrokeWidth);
    const int heightWithFrames = image.height() + (2 * borderStrokeWidth);
    if (landscapeDimension && (widthWithFrames > imageTargetSize.width()) && hasTargetSizeWidth) {
        scaling = float(imageTargetSize.width()) / float(widthWithFrames);
    } else if ((heightWithFrames > imageTargetSize.height()) && hasTargetSizeHeight) {
        scaling = float(imageTargetSize.height()) / float(heightWithFrames);
    }

    const float scaledFrameWidth = borderStrokeWidth / scaling;

    QTransform m;
    m.rotate(rotationAngle);
    m.scale(scaling, scaling);

    const QRectF frameRect(
        QPointF(0, 0),
        QPointF(image.width() / image.devicePixelRatio() + scaledFrameWidth * 2, image.height() / image.devicePixelRatio() + scaledFrameWidth * 2));

    QRect r = m.mapRect(QRectF(frameRect)).toAlignedRect();

    QImage transformed(r.size(), QImage::Format_ARGB32);
    transformed.fill(0);
    QPainter p(&transformed);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);
    p.setCompositionMode(QPainter::CompositionMode_Source);

    p.translate(-r.topLeft());
    p.setWorldTransform(m, true);

    if (isOpaque(image)) {
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawRoundedRect(frameRect, scaledFrameWidth / 2, scaledFrameWidth / 2);
    }
    p.drawImage(scaledFrameWidth, scaledFrameWidth, image);
    p.end();

    int radius = qMax(borderStrokeWidth, 1);

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

QImage ThumbnailProtocol::thumbForDirectory(const QString &directory)
{
    QImage img;
    if (m_propagationDirectories.isEmpty()) {
        // Directories that the directory preview will be propagated into if there is no direct sub-directories
        const KConfigGroup globalConfig(KSharedConfig::openConfig(), "PreviewSettings");
        const QStringList propagationDirectoriesList = globalConfig.readEntry("PropagationDirectories", QStringList() << "VIDEO_TS");
        m_propagationDirectories = QSet<QString>(propagationDirectoriesList.begin(), propagationDirectoriesList.end());
        m_maxFileSize = globalConfig.readEntry("MaximumSize", std::numeric_limits<qint64>::max());
    }

    const int tiles = 2; // Count of items shown on each dimension
    const int spacing = 1 * m_devicePixelRatio;
    const int visibleCount = tiles * tiles;

    // TODO: the margins are optimized for the Oxygen iconset
    // Provide a fallback solution for other iconsets (e. g. draw folder
    // only as small overlay, use no margins)

    KFileItem item(QUrl::fromLocalFile(directory));
    const int extent = qMin(m_width, m_height);
    QPixmap folder = QIcon::fromTheme(item.iconName()).pixmap(extent);
    folder.setDevicePixelRatio(m_devicePixelRatio);

    // Scale up base icon to ensure overlays are rendered with
    // the best quality possible even for low-res custom folder icons
    if (qMax(folder.width(), folder.height()) < extent) {
        folder = folder.scaled(extent, extent, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    const int folderWidth = folder.width();
    const int folderHeight = folder.height();

    const int topMargin = folderHeight * 30 / 100;
    const int bottomMargin = folderHeight / 6;
    const int leftMargin = folderWidth / 13;
    const int rightMargin = leftMargin;
    // the picture border stroke width 1/170 rounded up
    // (i.e for each 170px the folder width increases those border increase by 1 px)
    const int borderStrokeWidth = qRound(folderWidth / 170.);

    const int segmentWidth = (folderWidth - leftMargin - rightMargin + spacing) / tiles - spacing;
    const int segmentHeight = (folderHeight - topMargin - bottomMargin + spacing) / tiles - spacing;
    if ((segmentWidth < 5 * m_devicePixelRatio) || (segmentHeight < 5 * m_devicePixelRatio)) {
        // the segment size is too small for a useful preview
        return img;
    }

    // Advance to the next tile page each second
    int skipValidItems = ((int)sequenceIndex()) * visibleCount;

    img = QImage(QSize(folderWidth, folderHeight), QImage::Format_ARGB32);
    img.setDevicePixelRatio(m_devicePixelRatio);
    img.fill(0);

    QPainter p;
    p.begin(&img);

    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawPixmap(0, 0, folder);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    int xPos = leftMargin;
    int yPos = topMargin;

    int iterations = 0;
    QString hadFirstThumbnail;
    QImage firstThumbnail;

    int validThumbnails = 0;
    int totalValidThumbs = -1;

    while (true) {
        QDirIterator dir(directory, QDir::Files | QDir::Readable);
        int skipped = 0;

        // Seed the random number generator so that it always returns the same result
        // for the same directory and sequence-item
        m_randomGenerator.seed(qHash(directory) + skipValidItems);
        while (dir.hasNext()) {
            ++iterations;
            if (iterations > 500) {
                skipValidItems = skipped = 0;
                break;
            }

            dir.next();

            if (dir.fileInfo().isSymbolicLink()) {
                // Skip symbolic links, as these may point to e.g. network file
                // systems or other slow storage. The calling code already
                // checks for the directory itself, and if it is fine any
                // contained plain file is fine as well.
                continue;
            }

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

            drawSubThumbnail(p, subThumbnail, segmentWidth, segmentHeight, xPos, yPos, borderStrokeWidth);

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

        if (!dir.hasNext() && totalValidThumbs < 0) {
            // We iterated over the entire directory for the first time, so now we know how many thumbs
            // were actually created.
            totalValidThumbs = skipped + validThumbnails;
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

    if (totalValidThumbs >= 0) {
        // We only know this once we've iterated over the entire directory, so this will only be
        // set for large enough sequence indices.
        const int wraparoundPoint = (totalValidThumbs - 1) / visibleCount + 1;
        setMetaData("sequenceIndexWraparoundPoint", QString().setNum(wraparoundPoint));
    }
    setMetaData("handlesSequences", QStringLiteral("1"));

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
        oneTileImg.setDevicePixelRatio(m_devicePixelRatio);
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
        drawSubThumbnail(oneTilePainter, firstThumbnail, oneTileWidth, oneTileHeight, leftMargin, topMargin, borderStrokeWidth);
        return oneTileImg;
    }

    return img;
}

ThumbCreatorWithMetadata *ThumbnailProtocol::getThumbCreator(const QString &plugin)
{
    auto it = m_creators.constFind(plugin);
    if (it != m_creators.constEnd()) {
        return *it;
    }

    const KPluginMetaData md(plugin);
    const KPluginFactory::Result result = KPluginFactory::instantiatePlugin<KIO::ThumbnailCreator>(md);

    if (result) {
        auto creator = new ThumbCreatorWithMetadata{
            std::unique_ptr<ThumbnailCreator>(result.plugin),
            md.value("CacheThumbnail", true),
            true, // KIO::ThumbnailCreator are always dpr-aware
            md.value("HandleSequences", false),
        };

        m_creators.insert(plugin, creator);
        return creator;
    }

#if KIO_VERSION < QT_VERSION_CHECK(5, 240, 0)
    // Don't use KPluginFactory here, this is not a QObject and
    // neither is ThumbCreator
    ThumbCreator *creator = nullptr;
    QLibrary library(QPluginLoader(plugin).fileName());
    if (library.load()) {
        auto createFn = (newCreator)library.resolve("new_creator");
        if (createFn) {
            creator = createFn();
        }
    }

    ThumbCreatorWithMetadata *thumbCreator = nullptr;
    if (creator) {
        KPluginMetaData data;
        if (plugin.contains(QLatin1String("kf5/thumbcreator"))) {
            data = KPluginMetaData(plugin);
        } else {
            const QVector<KPluginMetaData> plugins = KIO::PreviewJob::availableThumbnailerPlugins();
            auto findPluginIt = std::find_if(plugins.begin(), plugins.end(), [&plugin](const KPluginMetaData &data) {
                return data.fileName() == plugin;
            });
            if (findPluginIt != plugins.end()) {
                data = *findPluginIt;
            }
        }
        if (!data.isValid()) {
            qCWarning(KIO_THUMBNAIL_LOG) << "Plugin not found:" << plugin;
        } else {
            thumbCreator = new ThumbCreatorWithMetadata{
                std::unique_ptr<ThumbCreator>(creator),
                data.value("CacheThumbnail", true),
                data.value("DevicePixelRatioDependent", false),
                data.value("HandleSequences", false),
            };
        }
    } else {
        qCWarning(KIO_THUMBNAIL_LOG) << "Failed to load" << plugin << library.errorString();
    }

    m_creators.insert(plugin, thumbCreator);

    return thumbCreator;
#endif

    return nullptr;
}

void ThumbnailProtocol::ensureDirsCreated()
{
    if (m_thumbBasePath.isEmpty()) {
        m_thumbBasePath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/thumbnails/");
        QDir basePath(m_thumbBasePath);
        basePath.mkpath("normal/");
        QFile::setPermissions(basePath.absoluteFilePath("normal"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        basePath.mkpath("large/");
        QFile::setPermissions(basePath.absoluteFilePath("large"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        if (m_devicePixelRatio > 1) {
            basePath.mkpath("x-large/");
            QFile::setPermissions(basePath.absoluteFilePath("x-large"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
            basePath.mkpath("xx-large/");
            QFile::setPermissions(basePath.absoluteFilePath("xx-large"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        }
    }
}

bool ThumbnailProtocol::createSubThumbnail(QImage &thumbnail, const QString &filePath, int segmentWidth, int segmentHeight)
{
    auto getSubCreator = [&filePath, this]() -> ThumbCreatorWithMetadata * {
        const QMimeDatabase db;
        const KPluginMetaData subPlugin = pluginForMimeType(db.mimeTypeForFile(filePath).name());
        if (!subPlugin.isValid() || !m_enabledPlugins.contains(subPlugin.pluginId())) {
            return nullptr;
        }
        return getThumbCreator(subPlugin.fileName());
    };

    const auto maxDimension = qMin(1024, 512 * m_devicePixelRatio);
    if ((segmentWidth <= maxDimension) && (segmentHeight <= maxDimension)) {
        // check whether a cached version of the file is available for
        // 128 x 128, 256 x 256 pixels or 512 x 512 pixels taking into account devicePixelRatio
        int cacheSize = 0;
        QCryptographicHash md5(QCryptographicHash::Md5);
        const QByteArray fileUrl = QUrl::fromLocalFile(filePath).toEncoded();
        md5.addData(fileUrl);
        const QString thumbName = QString::fromLatin1(md5.result().toHex()).append(".png");

        ensureDirsCreated();

        struct CachePool {
            QString path;
            int minSize;
        };

        static const auto pools = {
            CachePool{QStringLiteral("normal/"), 128},
            CachePool{QStringLiteral("large/"), 256},
            CachePool{QStringLiteral("x-large/"), 512},
            CachePool{QStringLiteral("xx-large/"), 1024},
        };

        const int wants = std::max(segmentWidth, segmentHeight);
        for (const auto &pool : pools) {
            if (pool.minSize < wants) {
                continue;
            } else if (cacheSize == 0) {
                // the lowest cache size the thumbnail could be at
                cacheSize = pool.minSize;
            }
            // try in folders with higher image quality as well
            if (thumbnail.load(m_thumbBasePath + pool.path + thumbName, "png")) {
                thumbnail.setDevicePixelRatio(m_devicePixelRatio);
                break;
            }
        }

        // no cached version is available, a new thumbnail must be created
        if (thumbnail.isNull()) {
            ThumbCreatorWithMetadata *subCreator = getSubCreator();
            if (subCreator && createThumbnail(subCreator, filePath, cacheSize, cacheSize, thumbnail)) {
                scaleDownImage(thumbnail, cacheSize, cacheSize);

                // The thumbnail has been created successfully. Check if we can store
                // the thumbnail to the cache for future access.
                if (subCreator->cacheThumbnail && metaData("cache").toInt() && !thumbnail.isNull()) {
                    QString thumbPath;
                    const int wants = std::max(thumbnail.width(), thumbnail.height());
                    for (const auto &pool : pools) {
                        if (pool.minSize < wants) {
                            continue;
                        } else if (thumbPath.isEmpty()) {
                            // that's the appropriate path for this thumbnail
                            thumbPath = m_thumbBasePath + pool.path;
                        }
                    }

                    // The thumbnail has been created successfully. Store the thumbnail
                    // to the cache for future access.
                    QSaveFile thumbnailfile(QDir(thumbPath).absoluteFilePath(thumbName));
                    if (thumbnailfile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        QFileInfo fi(filePath);
                        thumbnail.setText(QStringLiteral("Thumb::URI"), QString::fromUtf8(fileUrl));
                        thumbnail.setText(QStringLiteral("Thumb::MTime"), QString::number(fi.lastModified().toSecsSinceEpoch()));
                        thumbnail.setText(QStringLiteral("Thumb::Size"), QString::number(fi.size()));

                        if (thumbnail.save(&thumbnailfile, "png")) {
                            thumbnailfile.commit();
                        }
                    }
                }
            }
        }

        if (thumbnail.isNull()) {
            return false;
        }

    } else {
        // image requested is too big to be stored in the cache
        // create an image on demand
        ThumbCreatorWithMetadata *subCreator = getSubCreator();
        if (!subCreator || !createThumbnail(subCreator, filePath, segmentWidth, segmentHeight, thumbnail)) {
            return false;
        }
    }

    // Make sure the image fits in the segments
    // Some thumbnail creators do not respect the width / height parameters
    scaleDownImage(thumbnail, segmentWidth, segmentHeight);
    return true;
}

bool ThumbnailProtocol::createThumbnail(ThumbCreatorWithMetadata *thumbCreator, const QString &filePath, int width, int height, QImage &thumbnail)
{
    bool success = false;

#if KIO_VERSION < QT_VERSION_CHECK(5, 240, 0)
    if (std::holds_alternative<LegacyThumbCreatorPtr>(thumbCreator->creator)) {
        auto creator = std::get<LegacyThumbCreatorPtr>(thumbCreator->creator).get();

        int scaledWidth = width;
        int scaledHeight = height;

        if (thumbCreator->devicePixelRatioDependent) {
            auto *dprDependentCreator = static_cast<KIO::ThumbDevicePixelRatioDependentCreator *>(creator);

            if (dprDependentCreator) {
                dprDependentCreator->setDevicePixelRatio(m_devicePixelRatio);
            }

            scaledWidth /= m_devicePixelRatio;
            scaledHeight /= m_devicePixelRatio;
        }

        success = creator->create(filePath, scaledWidth, scaledHeight, thumbnail);

        if (thumbCreator->handleSequences) {
            auto *sequenceCreator = dynamic_cast<ThumbSequenceCreator *>(creator);
            m_sequenceIndexWrapAroundPoint = sequenceCreator->sequenceIndexWraparoundPoint();
        }

    } else {
#endif
        auto result = std::get<ThumbnailCreatorPtr>(thumbCreator->creator)
                          ->create(KIO::ThumbnailRequest(QUrl::fromLocalFile(filePath), QSize(width, height), m_mimeType, m_devicePixelRatio, sequenceIndex()));

        success = result.isValid();
        thumbnail = result.image();
        m_sequenceIndexWrapAroundPoint = result.sequenceIndexWraparoundPoint();

#if KIO_VERSION < QT_VERSION_CHECK(5, 240, 0)
    }
#endif

    if (!success) {
        return false;
    }

    // make sure the image is not bigger than the expected size
    scaleDownImage(thumbnail, width, height);

    thumbnail.setDevicePixelRatio(m_devicePixelRatio);
    convertToStandardRgb(thumbnail);

    return true;
}

void ThumbnailProtocol::drawSubThumbnail(QPainter &p, QImage subThumbnail, int width, int height, int xPos, int yPos, int borderStrokeWidth)
{
    scaleDownImage(subThumbnail, width, height);

    // center the image inside the segment boundaries
    const QPoint centerPos((xPos + width / 2) / m_devicePixelRatio, (yPos + height / 2) / m_devicePixelRatio);
    const int rotationAngle = m_randomGenerator.bounded(-8, 9); // Random rotation ±8°
    drawPictureFrame(&p, centerPos, subThumbnail, borderStrokeWidth, QSize(width, height), rotationAngle);
}

#include "thumbnail.moc"
