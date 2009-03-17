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
#include <kfilemetainfo.h>
#include <klocale.h>

#include <config-runtime.h> // For HAVE_NICE
#include <kio/thumbcreator.h>
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


    if (argc != 4)
    {
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
    if (m_mimeType.isEmpty())
    {
        QFileInfo info(url.path());
        kDebug(7115) << "PATH: " << url.path() << "isDir:" << info.isDir();
        if (!info.exists())
        {
            // The file does not exist
            error(KIO::ERR_DOES_NOT_EXIST,url.path());
            return;
        }
        else if (!info.isReadable())
        {
            // The file is not readable!
            error(KIO::ERR_COULD_NOT_READ,url.path());
            return;
        }

        if (info.isDir())
            m_mimeType = "inode/directory";
        else
          KMimeType::findByUrl(KUrl(info.filePath()))->name();

        kDebug(7115) << "Guessing MIME Type:" << m_mimeType;
        direct=true; // thumbnail: was probably called from Konqueror
    }
#endif

    if (m_mimeType.isEmpty())
    {
        error(KIO::ERR_INTERNAL, i18n("No MIME Type specified."));
        return;
    }

    m_width = metaData("width").toInt();
    m_height = metaData("height").toInt();
    int iconSize = metaData("iconSize").toInt();

    if (m_width < 0 || m_height < 0)
    {
        error(KIO::ERR_INTERNAL, i18n("No or invalid size specified."));
        return;
    }
#ifdef THUMBNAIL_HACK
    else if (!m_width || !m_height)
    {
        kDebug(7115) << "Guessing height, width, icon size!";
        m_width=128;
        m_height=128;
        iconSize=128;
    }
#endif

    if (!iconSize)
        iconSize = KIconLoader::global()->currentSize(KIconLoader::Desktop);
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
            service->property("SupportsThumbnail").toBool())
        {
            // was:  KFileMetaInfo info(url.path(), m_mimeType, KFileMetaInfo::Thumbnail);
            // but m_mimeType and WhatFlags are now unused in KFileMetaInfo, and not present in the
            // call that takes a KUrl
            KFileMetaInfo info(url);
            if (info.isValid())
            {
                KFileMetaInfoItem item = info.item("thumbnail");
                if (item.isValid() && item.value().type() == QVariant::Image)
                {
                    img = item.value().value<QImage>();
                    kDebug(7115) << "using KFMI for the thumbnail\n";
                    kfmiThumb = true;
                }
            }
        }
    }
    ThumbCreator::Flags flags = ThumbCreator::None;

    if (!kfmiThumb)
    {
        QString plugin = metaData("plugin");
        if((plugin.isEmpty() || plugin == "directorythumbnail") && m_mimeType == "inode/directory") {
          img = thumbForDirectory(url);
          flags = ThumbCreator::BlendIcon;
          m_iconAlpha = 180;
          if(img.isNull()) {
            error(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for directory"));
            return;
          }
        }else{
#ifdef THUMBNAIL_HACK
          if (plugin.isEmpty())
              plugin = pluginForMimeType(m_mimeType);

          kDebug(7115) << "Guess plugin: " << plugin;
#endif
          if (plugin.isEmpty())
          {
              error(KIO::ERR_INTERNAL, i18n("No plugin specified."));
              return;
          }

          ThumbCreator* creator = getThumbCreator(plugin);
          if(!creator) {
              error(KIO::ERR_INTERNAL, i18n("Cannot load ThumbCreator %1", plugin));
              return;
          }

          if (!creator->create(url.path(), m_width, m_height, img))
          {
              error(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for %1", url.path()));
              return;
          }
          flags = creator->flags();
        }
    }

    if (img.width() > m_width || img.height() > m_height)
    {
        double imgRatio = (double)img.height() / (double)img.width();
        if (imgRatio > (double)m_height / (double)m_width)
            img = img.scaled( int(qMax((double)m_height / imgRatio, 1.0)), m_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        else
            img = img.scaled(m_width, int(qMax((double)m_width * imgRatio, 1.0)), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

// ### FIXME
#ifndef USE_KINSTANCE
    if (flags & ThumbCreator::DrawFrame)
    {
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
        if ( !mask.isNull() ) // need to update it so we can see the frame
        {
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

    if ((flags & ThumbCreator::BlendIcon) && KIconLoader::global()->alphaBlending(KIconLoader::Desktop))
    {
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

    if (img.isNull())
    {
        error(KIO::ERR_INTERNAL, i18n("Failed to create a thumbnail."));
        return;
    }

    const QString shmid = metaData("shmid");
    if (shmid.isEmpty())
    {
#ifdef THUMBNAIL_HACK
        if (direct)
        {
            // If thumbnail was called directly from Konqueror, then the image needs to be raw
            //kDebug(7115) << "RAW IMAGE TO STREAM";
            QBuffer buf;
            if (!buf.open(QIODevice::WriteOnly))
            {
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
    }
    else
    {
#ifndef Q_WS_WIN
        QByteArray imgData;
        QDataStream stream( &imgData, QIODevice::WriteOnly );
        //kDebug(7115) << "IMAGE TO SHMID";
        void *shmaddr = shmat(shmid.toInt(), 0, 0);
        if (shmaddr == (void *)-1)
        {
            error(KIO::ERR_INTERNAL, i18n("Failed to attach to shared memory segment %1", shmid));
            return;
        }
        if (img.width() * img.height() > m_width * m_height)
        {
            error(KIO::ERR_INTERNAL, i18n("Image is too big for the shared memory segment"));
            shmdt((char*)shmaddr);
            return;
        }
        if( img.depth() != 32 ) // KIO::PreviewJob and this code below completely ignores colortable :-/,
            img = img.convertToFormat(QImage::Format_ARGB32); //  so make sure there is none
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
    KService::List offers = KMimeTypeTrader::self()->query( mimeType, QLatin1String( "ThumbCreator" ) );

    if(!offers.isEmpty())
    {
        KService::Ptr serv;
        serv = offers.first();
        return serv->library();
    }

    return QString();
}

QImage ThumbnailProtocol::thumbForDirectory(const KUrl& directory) {
  QImage img;
  ///@todo Make this work with remote urls
  QDirIterator dir(directory.path(), QDir::Files | QDir::Readable);

  int tiles = 2;
  int borderWidth = 2;

  //Leave some borders at the sides
  int segmentWidth = (m_width- (tiles+2) * borderWidth) /  tiles;
  int segmentHeight = (m_width-(tiles+2) * borderWidth) /  tiles;

  if(!dir.hasNext() || segmentWidth < 5 || segmentHeight <  5) {
    //Too small, or no content
//     kDebug(7115) <<  "cannot do" << directory.prettyUrl() << dir.hasNext() << segmentHeight << segmentWidth;
    return img;
  }

  //Make the image transparent
  img = QImage( QSize(m_width, m_height), QImage::Format_ARGB32 );

  // TODO: don't use the obsolete (and slow) method QImage::setAlphaChannel()
  QImage alphaChannel( QSize(m_width, m_height), QImage::Format_RGB32 );
  alphaChannel.fill(QColor(0, 0, 0).rgb());
  img.setAlphaChannel(alphaChannel);

  QPainter p(&img);

  int xPos = borderWidth;
  int yPos = borderWidth;

  int iterations = 0;
  bool hadThumbnail = false;

  while(dir.hasNext() && xPos < (m_width-borderWidth)  && yPos  < (m_height-borderWidth)) {
      ++iterations;
      if(iterations > 50) {
//         kDebug(7115) << "maximum iteration reached";
        return QImage();
      }

      dir.next();

      QString subPlugin = pluginForMimeType(KMimeType::findByUrl(KUrl(dir.filePath()))->name());
      if(subPlugin.isEmpty()) {
//         kDebug(7115) << "found no sub-plugin for" << dir.filePath();
        continue;
      }
      ThumbCreator* subCreator = getThumbCreator(subPlugin);
      if(!subCreator) {
//         kDebug(7115) << "found no creator for" << dir.filePath();
        continue;
      }

      QImage subImg;

      if(!subCreator->create(dir.filePath(), segmentWidth, segmentHeight, subImg)) {
//         kDebug(7115) <<  "failed to create thumbnail for" << dir.filePath();
        continue;
      }

      hadThumbnail = true;

      if(subImg.width() > segmentWidth || subImg.height() > segmentHeight)
        subImg = subImg.scaled(segmentWidth, segmentHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

      QRect target(xPos, yPos, segmentWidth, segmentHeight);

      if(target.width() > subImg.width()) {
        int dif = target.width() - subImg.width();
        target.setWidth(subImg.width());
        target.moveLeft(target.left() + (dif/2));
      }

      if(target.height() > subImg.height()) {
        int dif = target.height() - subImg.height();
        target.setHeight(subImg.height());
        target.moveTop(target.top() + (dif/2));
      }

      p.drawImage(target, subImg);

      xPos += segmentWidth + borderWidth;
      if(xPos >= (m_width-borderWidth) ) {
        xPos = borderWidth;
        yPos += segmentHeight + borderWidth;
      }
  }

  if(!hadThumbnail)
    return QImage();

  return img;
}

ThumbCreator* ThumbnailProtocol::getThumbCreator(const QString& plugin)
{
    ThumbCreator *creator = m_creators[plugin];
    if (!creator)
    {
        // Don't use KLibFactory here, this is not a QObject and
        // neither is ThumbCreator
        KLibrary *library = KLibLoader::self()->library(plugin);
        if (library)
        {
            newCreator create = (newCreator)library->resolveFunction("new_creator");
            if (create)
                creator = create();
        }
        if (!creator)
          return 0;

        m_creators.insert(plugin, creator);
    }

    return creator;
}


const QImage ThumbnailProtocol::getIcon()
{
    if ( !m_iconDict.contains(m_mimeType) ) { // generate it
        QImage icon( KIconLoader::global()->loadMimeTypeIcon( KMimeType::mimeType(m_mimeType)->iconName(), KIconLoader::Desktop, m_iconSize ).toImage() );
	icon = icon.convertToFormat( QImage::Format_ARGB32 );
        m_iconDict.insert( m_mimeType, icon );

        return icon;
    }

    return m_iconDict.value( m_mimeType );
}

