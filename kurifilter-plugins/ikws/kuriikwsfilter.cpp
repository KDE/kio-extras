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

#include "kuriikwsfilter.h"
#include "kuriikwsfiltereng.h"
#include "searchprovider.h"
#include "ikwsopts.h"

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kglobal.h>

#include <QtDBus/QtDBus>

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * ../tests/kurifiltertest
 */

K_PLUGIN_FACTORY(KAutoWebSearchFactory, registerPlugin<KAutoWebSearch>();)
K_EXPORT_PLUGIN(KAutoWebSearchFactory("kcmkurifilt"))

KAutoWebSearch::KAutoWebSearch(QObject *parent, const QVariantList&)
               :KUriFilterPlugin( "kuriikwsfilter", parent )
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
  kDebug(7023) << "Config reload requested...";
  KURISearchFilterEngine::self()->loadConfig();
}

bool KAutoWebSearch::filterUri( KUriFilterData &data ) const
{
  kDebug(7023) << data.typedString();

  if ( data.uriType() == KUriFilterData::Unknown && data.uri().pass().isEmpty() )
  {
    KURISearchFilterEngine *filter = KURISearchFilterEngine::self();
    QStringList favEngines = filter->favoriteEngineList();
    if (favEngines.isEmpty())
      favEngines = data.alternateSearchProviders();

    SearchProvider *provider = filter->autoWebSearchQuery( data.typedString(), data.alternateDefaultSearchProvider() );

    if( provider )
    {
      const QString result = filter->formatResult(provider->query(), provider->charset(),
                                                  QString(), data.typedString(), true);
      kDebug(7203) << "filtered to" << result;
      setFilteredUri( data, KUrl( result ) );
      setUriType( data, KUriFilterData::NetProtocol );
      setSearchProvider(data, provider->name(), data.typedString(), QLatin1Char(filter->keywordDelimiter()));

      KUriFilterPlugin::ProviderInfoList providerInfo;
      Q_FOREACH(const QString &engine, favEngines)
      {
        SearchProvider *favProvider = SearchProvider::findByDesktopName(engine);
        if (favProvider)
        {
          const QString query = favProvider->keys().at(0) + filter->keywordDelimiter() + data.typedString();
          providerInfo.insert(favProvider->name(), qMakePair(query, iconNameFor(favProvider->query(), KUriFilterData::NetProtocol)));
          delete favProvider;
        }
      }
      setPreferredSearchProviders( data, providerInfo);
      delete provider;
      return true;
    }
  }
  return false;
}

#include "kuriikwsfilter.moc"
