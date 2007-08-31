/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2000 Yves Arrouye <yves@realnames.com>
    Copyright (C) 2002, 2003 Dawit Alemayehu <adawit@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <unistd.h>

#include <kdebug.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kglobal.h>

#include "ikwsopts.h"
#include "kuriikwsfiltereng.h"
#include "kurisearchfilter.h"

#include <QtDBus/QtDBus>

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * kdelibs/kio/tests/kurifiltertest
 */

K_PLUGIN_FACTORY(KUriSearchFilterFactory, registerPlugin<KUriSearchFilter>();)
K_EXPORT_PLUGIN(KUriSearchFilterFactory("kcmkurifilt"))

KUriSearchFilter::KUriSearchFilter(QObject *parent,
                                   const QVariantList &)
                 :KUriFilterPlugin( "KUriSearchFilter", parent )
{
  KGlobal::locale()->insertCatalog("kurifilter");
  QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KUriFilterPlugin",
                              "configure", this, SLOT(configure()));
}

KUriSearchFilter::~KUriSearchFilter()
{
}

void KUriSearchFilter::configure()
{
  if ( KURISearchFilterEngine::self()->verbose() )
    kDebug() << "KUriSearchFilter::configure: Config reload request...";

  KURISearchFilterEngine::self()->loadConfig();
}

bool KUriSearchFilter::filterUri( KUriFilterData &data ) const
{
  if ( KURISearchFilterEngine::self()->verbose() )
    kDebug() << "KUriSearchFilter::filterUri: '" << data.typedString() << "'";

  QString result = KURISearchFilterEngine::self()->webShortcutQuery( data.typedString() );

  if ( !result.isEmpty() )
  {
    if ( KURISearchFilterEngine::self()->verbose() )
      kDebug() << "Filtered URL: " << result;

    setFilteredUri( data, KUrl( result ) );
    setUriType( data, KUriFilterData::NET_PROTOCOL );
    return true;
  }

  return false;
}

KCModule *KUriSearchFilter::configModule(QWidget *parent, const char *) const
{
  return new FilterOptions( KUriSearchFilterFactory::componentData(), parent);
}

QString KUriSearchFilter::configName() const
{
  return i18n("Search F&ilters");
}

#include "kurisearchfilter.moc"
