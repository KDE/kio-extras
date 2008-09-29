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

  QPixmap pix;
  if (!cache->find(type + string + QString::number(size), pix)) {
    KIcon icon = KIcon(string);
    if (type == "icon") {
      pix = icon.pixmap(size, size);
    } else {
      pix = QPixmap(size, size);
      pix.fill(Qt::transparent);
      QPainter painter(&pix);
      painter.setOpacity(0.3);

      QRectF rect(0, 0, size, size);
      painter.drawPixmap(rect, icon.pixmap(size, size), rect);
    }
    cache->insert(type + string + QString::number(size), pix);
  }
  echoPixmap(pix);
}

void BookmarksProtocol::echoPixmap(const QPixmap &pixmap)
{
  SlaveBase::mimeType("image/png");

  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  pixmap.save(&buffer, "PNG");
  data(bytes);
}
