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

#include <stdlib.h>
#include <unistd.h>
#ifdef __FreeBSD__
    #include <machine/param.h>
#endif
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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
#include <kimageeffect.h>
#include <kmimetype.h>
#include <klibloader.h>
#include <kdebug.h>
#include <kservice.h>
#include <kservicetype.h>
#include <kservicetypetrader.h>
#include <kmimetypetrader.h>
#include <kfilemetainfo.h>
#include <klocale.h>

#include <config.h> // For HAVE_NICE
#include "thumbnail.h"
#include <kio/thumbcreator.h>

// Use correctly KInstance instead of KApplication (but then no QPixmap)
#undef USE_KINSTANCE
// Fix thumbnail: protocol
#define THUMBNAIL_HACK (1)

#ifdef THUMBNAIL_HACK
# include <qfileinfo.h>
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
    KInstance instance("kio_thumbnail");
#else
    // creating KApplication in a slave in not a very good idea,
    // as dispatchLoop() doesn't allow it to process its messages,
    // so it for example wouldn't reply to ksmserver - on the other
    // hand, this slave uses QPixmaps for some reason, and they
    // need QApplication
    // and HTML previews need even KApplication :(
    putenv(strdup("SESSION_MANAGER="));
    //KApplication::disableAutoDcopRegistration();
    KAboutData about("kio_thumbnail", "kio_thumbmail", "KDE 4.x.x");
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
    kDebug(7115) << "Wanting MIME Type:" << m_mimeType << endl;
#ifdef THUMBNAIL_HACK
    // ### HACK
    bool direct=false;
    if (m_mimeType.isEmpty())
    {
        kDebug(7115) << "PATH: " << url.path() << endl;
        QFileInfo info(url.path());
        if (info.isDir())
        {
            // We cannot process a directory
            error(KIO::ERR_IS_DIRECTORY,url.path());
            return;
        }
        else if (!info.exists())
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
        m_mimeType = KMimeType::findByUrl(url)->name();
        kDebug(7115) << "Guessing MIME Type:" << m_mimeType << endl;
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
        kDebug(7115) << "Guessing height, width, icon sizre!" << endl;
        m_width=128;
        m_height=128;
        iconSize=128;
    }
#endif

    if (!iconSize)
        iconSize = KIconLoader::global()->currentSize(K3Icon::Desktop);
    if (iconSize != m_iconSize) {
        m_iconDict.clear();
    }
    m_iconSize = iconSize;

    m_iconAlpha = metaData("iconAlpha").toInt();
    if (m_iconAlpha)
        m_iconAlpha = (m_iconAlpha << 24) | 0xffffff;

    QImage img;

    KConfigGroup group( KGlobal::config(), "PreviewSettings" );


    // ### KFMI
    bool kfmiThumb = false;
    if (group.readEntry( "UseFileThumbnails", QVariant(true )).toBool()) {
        KService::Ptr service =
            KMimeTypeTrader::self()->preferredService( m_mimeType, "KFilePlugin");

        if ( service && service->isValid() && /*url.isLocalFile() && */
            service->property("SupportsThumbnail").toBool())
        {
            KFileMetaInfo info(url.path(), m_mimeType, KFileMetaInfo::Thumbnail);
            if (info.isValid())
            {
                KFileMetaInfoItem item = info.item(KFileMimeTypeInfo::Thumbnail);
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
        kDebug(7115) << "using thumb creator for the thumbnail\n";
        QString plugin = metaData("plugin");
#ifdef THUMBNAIL_HACK
        if (plugin.isEmpty())
        {
            KService::List plugins = KServiceTypeTrader::self()->query("ThumbCreator");
            QMap<QString, KService::Ptr> mimeMap;

            for (KService::List::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
            {
                QStringList mimeTypes = (*it)->property("MimeTypes").toStringList();
                for (QStringList::ConstIterator mt = mimeTypes.begin(); mt != mimeTypes.end(); ++mt)
                {
                    if  ((*mt)==m_mimeType)
                    {
                        plugin=(*it)->library();
                        break;
                    }
                }
                if (!plugin.isEmpty())
                    break;
            }
        }
        kDebug(7115) << "Guess plugin: " << plugin << endl;
#endif
        if (plugin.isEmpty())
        {
            error(KIO::ERR_INTERNAL, i18n("No plugin specified."));
            return;
        }

        ThumbCreator *creator = m_creators[plugin];
        if (!creator)
        {
            // Don't use KLibFactory here, this is not a QObject and
            // neither is ThumbCreator
            KLibrary *library = KLibLoader::self()->library(QFile::encodeName(plugin));
            if (library)
            {
                newCreator create = (newCreator)library->symbol("new_creator");
                if (create)
                    creator = create();
            }
            if (!creator)
            {
                error(KIO::ERR_INTERNAL, i18n("Cannot load ThumbCreator %1", plugin));
                return;
            }
            m_creators.insert(plugin, creator);
        }

        if (!creator->create(url.path(), m_width, m_height, img))
        {
            error(KIO::ERR_INTERNAL, i18n("Cannot create thumbnail for %1", url.path()));
            return;
        }
        flags = creator->flags();
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

    if ((flags & ThumbCreator::BlendIcon) && KIconLoader::global()->alphaBlending(K3Icon::Desktop))
    {
        // blending the mimetype icon in
        QImage icon = getIcon();

        int x = img.width() - icon.width() - 4;
        x = qMax( x, 0 );
        int y = img.height() - icon.height() - 6;
        y = qMax( y, 0 );
        KImageEffect::blendOnLower( x, y, icon, img );
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
            //kDebug(7115) << "RAW IMAGE TO STREAM" << endl;
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
            //kDebug(7115) << "IMAGE TO STREAM" << endl;
            stream << img;
            data(imgData);
        }
    }
    else
    {
        QByteArray imgData;
        QDataStream stream( &imgData, QIODevice::WriteOnly );
        //kDebug(7115) << "IMAGE TO SHMID" << endl;
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
        // Keep in sync with kdelibs/kio/kio/previewjob.cpp
        stream << img.width() << img.height() << quint8(img.format());
        memcpy(shmaddr, img.bits(), img.numBytes());
        shmdt((char*)shmaddr);
        data(imgData);
    }
    finished();
}

const QImage ThumbnailProtocol::getIcon()
{
    if ( !m_iconDict.contains(m_mimeType) ) { // generate it
        QImage icon( KMimeType::mimeType(m_mimeType)->pixmap( K3Icon::Desktop, m_iconSize ).toImage() );
        icon.setAlphaBuffer( true );

        int w = icon.width();
        int h = icon.height();
        for ( int y = 0; y < h; y++ )
        {
            QRgb *line = (QRgb *) icon.scanLine( y );
            for ( int x = 0; x < w; x++ )
                line[x] &= m_iconAlpha; // transparency
        }

        m_iconDict.insert( m_mimeType, icon );

        return icon;
    }

    return m_iconDict.value( m_mimeType );
}

