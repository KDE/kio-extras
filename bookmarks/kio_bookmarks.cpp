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

#include <qregexp.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include <kshell.h>
#include <kstandarddirs.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kpixmapcache.h>
#include <kdebug.h>
#include <kfileplacesmodel.h>
#include <solid/device.h>
#include <solid/deviceinterface.h>
#include <ktoolinvocation.h>

using namespace KIO;

BookmarksProtocol::BookmarksProtocol( const QByteArray &pool, const QByteArray &app )
  : SlaveBase( "bookmarks", pool, app )
{
  cache = new KPixmapCache("kio_bookmarks");
  manager = KBookmarkManager::userBookmarksManager();
  cfg = new KConfig( "kiobookmarksrc" );
  config = cfg->group("General");

  indent = 0;
  totalsize = 0;
  columns = 4;
}

BookmarksProtocol::~BookmarksProtocol()
{
  delete manager;
  delete cache;
  delete cfg;
}

void BookmarksProtocol::parseTree()
{
  totalsize = 0;

  cfg->reparseConfiguration();
  columns =  config.readEntry("Columns", 4);
  if (columns < 1)
    columns = 1;

  manager->notifyCompleteChange("kio_bookmarks");
  tree = manager->root();

  if(tree.first().isNull())
    return;

  if(config.readEntry("FlattenTree", false))
    flattenTree(tree);

  KBookmarkGroup root;
  if(config.readEntry("ShowRoot", true))
  {
    root = tree.createNewFolder(i18n("Root"));
    tree.moveBookmark(root, KBookmark::KBookmark());
    root.setIcon("konqueror");
  }

    KBookmark bm = tree.first();
    KBookmark next;
    while(!bm.isNull())
    {
      next = tree.next(bm);
      if (bm.isSeparator())
        tree.deleteBookmark(bm);
      else if (bm.isGroup())
        totalsize += sizeOfGroup(bm.toGroup());
      else
      {
        if(config.readEntry("ShowRoot", true))
          root.addBookmark(bm);

        tree.deleteBookmark(bm);
      }
      bm = next;
    }
    if(config.readEntry("ShowRoot", true))
      totalsize += sizeOfGroup(root);

    if(config.readEntry("ShowPlaces", true))
      totalsize += addPlaces();
}

int BookmarksProtocol::addPlaces()
{
  KFilePlacesModel placesModel;
  KBookmarkGroup folder = tree.createNewFolder(i18n("Places"));
  QList<Solid::Device> batteryList = Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString());

  if (batteryList.isEmpty()) {
    folder.setIcon("computer");
  } else {
    folder.setIcon("computer-laptop");
  }

  for (int row = 0; row < placesModel.rowCount(); ++row) {
    QModelIndex index = placesModel.index(row, 0);

    if (!placesModel.isHidden(index))
      folder.addBookmark(placesModel.bookmarkForIndex(index));
  }
  return sizeOfGroup(folder);
}

void BookmarksProtocol::flattenTree( const KBookmarkGroup &folder )
{
  KBookmark bm = folder.first();
  KBookmark prev = folder;
  KBookmark next;
  while (!bm.isNull())
  {
    if (bm.isGroup()) {
      flattenTree(bm.toGroup());
    }

    next = tree.next(bm);

    if (bm.isGroup() and bm.parentGroup().hasParent()) {
      kDebug() << "moving " << bm.text() << " from " << bm.parentGroup().fullText() << " to " << prev.parentGroup().text() << endl;

      bm.setFullText("| " + bm.parentGroup().fullText() + " > " + bm.fullText());
      tree.moveBookmark(bm, prev);
      prev = bm;
    }
    bm = next;
  }
}

// Should really go to KBookmarkGroup
int BookmarksProtocol::sizeOfGroup( const KBookmarkGroup &folder, bool real )
{
  int size = 1;  // counting the title line
  for (KBookmark bm = folder.first(); !bm.isNull(); bm = folder.next(bm))
  {
    if (bm.isGroup())
      size += sizeOfGroup(bm.toGroup());
    else
      size += 1;
  }

  // CSS sets a min-height for toplevel folders
  if (folder.parentGroup() == tree and size < 8 and real == false)
    size = 8;

  return size;
}

void BookmarksProtocol::get( const KUrl& url )
{
  QString path = url.path();
  QRegExp regexp("^/(background|icon)/([\\S]+)");

  if (path.isEmpty() or path == "/") {
    echoIndex();
  } else if (path == "/config") {
    KToolInvocation::startServiceByDesktopName("bookmarks", "");
    echoHead("bookmarks:/");
  } else if (path == "/editbookmarks") {
    KToolInvocation::kdeinitExec("keditbookmarks");
    echoHead("bookmarks:/");
  } else if (regexp.indexIn(path) >= 0) {
    echoImage(regexp.cap(1), regexp.cap(2), url.queryItem("size"));
  } else {
    echoHead();
    echo("<p class=\"message\">" + i18n("Wrong request : %1",path) + "</p>");
  }
  finished();
}

extern "C" int KDE_EXPORT kdemain(int argc, char **argv)
{
  KAboutData about("kio_bookmarks", 0, ki18n("My bookmarks"), "0.2.2");
  about.addLicense(KAboutData::License_GPL_V2);
  about.addAuthor(ki18n("Xavier Vello"), ki18n("Initial developer"), "xavier.vello@gmail.com", QByteArray());
  KCmdLineArgs::init(&about);
  KApplication app;

  BookmarksProtocol slave(argv[2], argv[3]);
  slave.dispatchLoop();

  return 0;
}
