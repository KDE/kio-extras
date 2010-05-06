/*
Copyright (C) 2008 Xavier Vello <xavier.vello@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "kio_bookmarks.h"

#include <stdio.h>
#include <stdlib.h>

#include <kstandarddirs.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kimagecache.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <qbuffer.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <kicon.h>

using namespace KIO;

void BookmarksProtocol::echoImage( const QString &type, const QString &string, const QString &sizestring )
{
  int size = sizestring.toInt();
  if (size == 0) {
    if (type == "icon")
      size = 16;
    else
      size = 128;
  }

  // Although KImageCache supports caching pixmaps, we need to send the data to the
  // destination process anyways so don't bother, just hold onto the image data.
  QImage image = cache->findImage(type + string + QString::number(size));
  if (image.isNull()) {
    KIcon icon = KIcon(string);
    QPixmap pix; // KIcon can't give us a QImage anyways.

    if (type == "icon") {
      pix = icon.pixmap(size, size);
    } else {
      pix = QPixmap(size, size);
      pix.fill(Qt::transparent);

      QPixmap overlay = icon.pixmap(size, size);

      // Make overlay semi-transparent (faster than setOpacity)
      QPainter overlayPainter;
      overlayPainter.begin(&overlay);
      overlayPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
      overlayPainter.fillRect(overlay.rect(), QColor(0, 0, 0, 0.3));
      overlayPainter.end();

      QPainter painter(&pix);
      QRectF rect(0, 0, size, size);
      painter.drawPixmap(pix.rect(), overlay, overlay.rect());
    }

    image = pix.toImage();
    cache->insertImage(type + string + QString::number(size), image);
  }

  QBuffer buffer;
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");

  SlaveBase::mimeType("image/png");
  data(buffer.buffer());
}
