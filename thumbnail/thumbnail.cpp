/*  This file is part of the KDE libraries
    Copyright (C) 2000 Malte Starostik <malte.starostik@t-online.de>
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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

// $Id$

#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>

#include <qpixmap.h>
#include <qpainter.h>
#include <qimage.h>
#include <qbuffer.h>

#include <kurl.h>
#include <kapp.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <kmimetype.h>
#include <kservice.h>
#include <klibloader.h>
#include <kdebug.h>

#include "thumbnail.h"
#include "thumbcreator.h"

// Recognized metadata entries:
// mimeType     - the mime type of the file, used for the overlay icon if any
// iconSize     - the size of the overlay icon to use if any
// extent       - the requested "extent" of the thumbnail. The extent
//                specifies the maximum accepted size of the image, either
//                horizontally or vertically depending on the thumb's
//                orientation.
// transparency - the transparency value used for icon overlays
// plugin       - the name of the plugin to be used for thumbnail creation.
//                Provided by the application to save an addition KTrader
//                query here.
// shmid        - the shared memory segment id to write the image's data to.
//                The segment is assumed to provide enough space for a 32-bit
//                image sized extent x extent pixels.
//                If this is given, the data returned by the slave will be:
//                    int width
//                    int height
//                    int depth
//                Otherwise, the data returned is the image in PNG format.
 
using namespace KIO;

extern "C"
{
    int kdemain(int argc, char **argv);
}

int kdemain(int argc, char **argv)
{
    KApplication app(argc, argv, "kio_thumbnail", false, true);

    if (argc != 4)
    {
        kdError() << "Usage: kio_thumbnail protocol domain-socket1 domain-socket2" << endl;
        exit(-1);
    }

    ThumbnailProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}

ThumbnailProtocol::ThumbnailProtocol(const QCString &pool, const QCString &app)
    : SlaveBase("thumbnail", pool, app)
{
    m_creators.setAutoDelete(true);
    m_iconDict.setAutoDelete(true);
    m_iconSize = 0;
}

ThumbnailProtocol::~ThumbnailProtocol()
{
}

void ThumbnailProtocol::get(const KURL &url)
{
    m_mimeType = metaData("mimeType");
    if (m_mimeType.isEmpty())
    {
        error(KIO::ERR_INTERNAL, "No MIME Type specified.");
        return;
    }
    int iconSize = metaData("iconSize").toInt();
    if (!iconSize)
        iconSize = KGlobal::iconLoader()->currentSize(KIcon::Desktop);
	if (iconSize != m_iconSize)
        m_iconDict.clear();
    m_iconSize = iconSize;

    m_extent = metaData("extent").toInt();
    if (!m_extent)
        m_extent = 64;
    m_transparency = metaData("transparency").toInt();
    if (!m_transparency)
        m_transparency = 0;
    QString plugin = metaData("plugin");
    if (plugin.isEmpty())
    {
        error(KIO::ERR_INTERNAL, "No plugin specified.");
        return;
    }
    ThumbCreator *creator = m_creators[plugin];
    if (!creator)
    {
        // Don't use KLibFactory here, this is not a QObject and
        // neither is ThumbCreator
        KService::Ptr service = KService::serviceByDesktopName(plugin);
        KLibrary *library = KLibLoader::self()->library(service->library().latin1());
        if (library)
        {
            newCreator create = (newCreator)library->symbol("new_creator");
            if (create)
                creator = create();
        }
        if (!creator)
        {
            error(KIO::ERR_INTERNAL, "Cannot load ThumbCreator " + plugin);
            return;
        }
        m_creators.insert(plugin, creator);
    }

    QPixmap pix;
    if (!creator->create(url.path(), m_extent, pix))
    {
        error(KIO::ERR_INTERNAL, "Cannot create thumbnail for " + url.path());
        return;
    }
    ThumbCreator::Flags flags = creator->flags();

    if (flags & ThumbCreator::DrawFrame)
    {
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
    }

    QImage img = pix.convertToImage();
    if ((flags & ThumbCreator::BlendIcon) && KGlobal::iconLoader()->alphaBlending(KIcon::Desktop))
    {
        // blending the mimetype icon in
        QImage icon = getIcon();

        int x = pix.width() - icon.width() - 4;
        x = QMAX( x, 0 );
        int y = pix.height() - icon.height() - 6;
        y = QMAX( y, 0 );
        KImageEffect::blendOnLower( x, y, icon, img );
    }

    QByteArray imgData;
    QDataStream stream( imgData, IO_WriteOnly );
	QString shmid = metaData("shmid");
	if (shmid.isEmpty())
		stream << img;
	else
	{
		uchar *shmaddr = static_cast<uchar *>(shmat(shmid.toInt(), 0, 0));
		if (shmaddr == (uchar *)-1)
		{
			error(KIO::ERR_INTERNAL, "Failed to attach to shared memory segment " + shmid);
			return;
		}
		if (img.width() * img.height() > m_extent * m_extent)
		{
			error(KIO::ERR_INTERNAL, "Image is too big for the shared memory segment");
			shmdt(shmaddr);
			return;
		}
		stream << img.width() << img.height() << img.depth();
		memcpy(shmaddr, img.bits(), img.numBytes());
		shmdt(shmaddr);
	}
    data(imgData);
    finished();
}

const QImage& ThumbnailProtocol::getIcon()
{
    QImage* icon = m_iconDict.find(m_mimeType);
    if ( !icon ) // generate it!
    {
        icon = new QImage( KMimeType::mimeType(m_mimeType)->pixmap( KIcon::Desktop, m_iconSize ).convertToImage() );
        icon->setAlphaBuffer( true );

        int w = icon->width();
        int h = icon->height();
        for ( int y = 0; y < h; y++ )
        {
            QRgb *line = (QRgb *) icon->scanLine( y );
            for ( int x = 0; x < w; x++ )
            line[x] &= m_transparency; // transparency
        }

        m_iconDict.insert( m_mimeType, icon );
    }

    return *icon;
}

