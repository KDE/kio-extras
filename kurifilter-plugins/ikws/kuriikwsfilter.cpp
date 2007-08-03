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
#include <kurl.h>
#include <kglobal.h>

#include "ikwsopts.h"
#include "kuriikwsfiltereng.h"
#include "kuriikwsfilter.h"

#include <QtDBus/QtDBus>

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * kdelibs/kio/tests/kurifiltertest
 */

typedef KGenericFactory<KAutoWebSearch> KAutoWebSearchFactory;
K_EXPORT_COMPONENT_FACTORY (libkuriikwsfilter, KAutoWebSearchFactory("kcmkurifilt"))

KAutoWebSearch::KAutoWebSearch(QObject *parent, const QStringList&)
               :KUriFilterPlugin( "KURIIKWSFilterIface", parent )
{
  KGlobal::locale()->insertCatalog("kurifilter");
  QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KUriFilterPlugin",
                              "configure", this, SLOT(configure()));
}

KAutoWebSearch::~KAutoWebSearch()
{
}

void KAutoWebSearch::configure()
{
  if ( KURISearchFilterEngine::self()->verbose() )
    kDebug() << "KAutoWebSearch::configure: Config reload requested...";

  KURISearchFilterEngine::self()->loadConfig();
}

bool KAutoWebSearch::filterUri( KUriFilterData &data ) const
{
  if( data.uriType() != KUriFilterData::UNKNOWN )
      return false;

  if (KURISearchFilterEngine::self()->verbose())
    kDebug() << "KAutoWebSearch::filterURI: '" <<  data.uri().url() << "'";

  KUrl u = data.uri();
  if ( u.pass().isEmpty() )
  {
    QString result = KURISearchFilterEngine::self()->autoWebSearchQuery( data.typedString() );
    if( !result.isEmpty() )
    {
      if ( KURISearchFilterEngine::self()->verbose() )
        kDebug () << "Filtered URL: " << result;

      setFilteredUri( data, KUrl( result ) );
      setUriType( data, KUriFilterData::NET_PROTOCOL );
      return true;
    }
  }
  return false;
}

#include "kuriikwsfilter.moc"
