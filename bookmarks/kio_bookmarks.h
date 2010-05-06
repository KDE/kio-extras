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
#ifndef KIO_BOOKMARKS_H
#define KIO_BOOKMARKS_H

#include <kio/slavebase.h>
#include <kbookmarkmanager.h>
#include <kconfig.h>
#include <KConfigGroup>

class KImageCache;

class BookmarksProtocol : public KIO::SlaveBase
{
  public:
    BookmarksProtocol( const QByteArray &pool, const QByteArray &app );
    ~BookmarksProtocol();

    void get( const KUrl& url );

  private:
    int columns;
    int indent;
    int totalsize;
    KImageCache* cache;
    KBookmarkManager* manager;
    KConfig* cfg;
    KConfigGroup config;
    KBookmarkGroup tree;
    void parseTree();
    void flattenTree( const KBookmarkGroup &folder );
    int sizeOfGroup(const KBookmarkGroup &folder, bool real = false);
    int addPlaces();

    // Defined in kde_bookmarks_html.cpp
    void echo( const QString &string );
    QString htmlColor(const QColor &col);
    QString htmlColor(const QBrush &brush);
    void echoIndex();
    void echoHead(const QString &redirect = QString());
    void echoStyle();
    void echoBookmark( const KBookmark &bm);
    void echoSeparator();
    void echoFolder( const KBookmarkGroup &folder );

    // Defined in kde_bookmarks_pixmap.cpp
    void echoImage( const QString &type, const QString &string, const QString &sizestring = QString());
};

#endif
