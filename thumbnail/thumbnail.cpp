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
#include <iostream>

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

#include "thumbnail.h"
#include "imagecreator.h"
#include "textcreator.h"
#include "htmlcreator.h"

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
        cerr << "Usage: kio_thumbnail protocol domain-socket1 domain-socket2" << endl;
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
    m_iconSize = metaData("iconSize").toInt();
    if (!m_iconSize)
        m_iconSize = KGlobal::iconLoader()->currentSize(KIcon::Desktop);
    m_extent = metaData("extent").toInt();
    if (!m_extent)
        m_extent = 64;
    m_transparency = metaData("transparency").toInt();
    if (!m_transparency)
        m_transparency = 0;

    // Kludge for now, as long as no pluginification is done, this is enough
    bool renderHTML = metaData("renderHTML") == "true";
    // FIXME: After 2.1 this should be handled by KTrader (malte)
    QString key;
    if (renderHTML && m_mimeType == "text/html")
        key = "text/html";
    else if (m_mimeType.startsWith("text/"))
        key = "text/*";
    else if (m_mimeType.startsWith("image/"))
        key = "image/*";
    else
    {
        error(KIO::ERR_INTERNAL, "Unsupported MIME Type: " + m_mimeType);
        return;
    }
    ThumbCreator *creator = m_creators[key];
    if (!creator)
    {
        if (key == "text/html")
            creator = new HTMLCreator;
        else if (key == "text/*")
            creator = new TextCreator;
        else if (key == "image/*")
            creator = new ImageCreator;
        else // Huh?
        {
            error(KIO::ERR_INTERNAL, "Cannot determine ThumbCreator for " + key);
            return;
        }
        m_creators.insert(key, creator);
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
        // paint a black rectangle around the "page"
        QPainter p;
        p.begin( &pix );
        p.setPen( QColor( 88, 88, 88 ));
        p.drawRect( 0, 0, pix.width(), pix.height() );
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

    if (flags & ThumbCreator::SaveThumb)
        setMetaData("save", QString("true"));

    QByteArray imgData;
    QDataStream stream( imgData, IO_WriteOnly );
    stream << img;
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

