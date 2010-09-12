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

#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

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

void KAutoWebSearch::populateProvidersList(KUriFilterPlugin::ProviderInfoList& providerInfo, const KUriFilterData& data, bool allproviders) const
{
  QList<SearchProvider*> providers;
  KURISearchFilterEngine *filter = KURISearchFilterEngine::self();
  const QString searchTerm = filter->keywordDelimiter() + data.typedString();

  if (allproviders)
    providers = SearchProvider::findAll();
  else
  {
    // Start with the search engines marked as preferred...
    QStringList favEngines = filter->favoriteEngineList();
    if (favEngines.isEmpty())
      favEngines = data.alternateSearchProviders();

    // Add the search engine set as the default provider...
    const QString defaultEngine = filter->defaultSearchEngine();
    if (!defaultEngine.isEmpty()) {
        favEngines.removeAll(defaultEngine);
        favEngines.prepend(defaultEngine);
    }

    // Get rid of duplicates...
    favEngines.removeDuplicates();

    QStringListIterator it (favEngines);
    while (it.hasNext())
    {
      SearchProvider *favProvider = SearchProvider::findByDesktopName(it.next());
      if (favProvider)
          providers << favProvider;
    }
  }

  for (int i= 0; i < providers.count(); ++i)
  {
    QString query;
    QStringListIterator it (providers[i]->keys());
    while (it.hasNext())
    {
      if (query.count())
        query += QL1S(",");
      query += it.next();
      query += searchTerm;
    }
    providerInfo.insert(providers[i]->name(), qMakePair(query, iconNameFor(providers[i]->query(), KUriFilterData::NetProtocol)));
  }

  qDeleteAll(providers);
}

bool KAutoWebSearch::filterUri( KUriFilterData &data ) const
{
  kDebug(7023) << data.typedString();

  KURISearchFilterEngine *filter = KURISearchFilterEngine::self();
  KUriFilterData::SearchFilterOptions option = data.searchFilteringOptions();

  // Handle the flag to retrieve only preferred providers, no filtering...
  if (option & KUriFilterData::RetrievePreferredSearchProvidersOnly)
  {
      KUriFilterPlugin::ProviderInfoList providerInfo;
      populateProvidersList(providerInfo, data);
      if (providerInfo.isEmpty()) {
        if (!(option & KUriFilterData::RetrieveSearchProvidersOnly))
        {
          setUriType(data, KUriFilterData::Error);
          setErrorMsg(data, i18n("No preferred search providers were found."));
          return false;
        }
      }
      else
      {
        setSearchProvider(data, QString(), data.typedString(), QL1C(filter->keywordDelimiter()));
        setPreferredSearchProviders(data, providerInfo);
        return true;
      }

  }

  if (option & KUriFilterData::RetrieveSearchProvidersOnly)
  {
      KUriFilterPlugin::ProviderInfoList providerInfo;
      populateProvidersList(providerInfo, data, true);
      if (providerInfo.isEmpty()) {
        setUriType(data, KUriFilterData::Error);
        setErrorMsg(data, i18n("No search providers were found."));
        return false;
      }

      setSearchProvider(data, QString(), data.typedString(), QL1C(filter->keywordDelimiter()));
      setPreferredSearchProviders(data, providerInfo);
      return true;
  }

  if ( data.uriType() == KUriFilterData::Unknown && data.uri().pass().isEmpty() )
  {
    SearchProvider *provider = filter->autoWebSearchQuery( data.typedString(), data.alternateDefaultSearchProvider() );
    if( provider )
    {
      const QString result = filter->formatResult(provider->query(), provider->charset(),
                                                  QString(), data.typedString(), true);
      kDebug(7203) << "filtered to" << result;
      setFilteredUri( data, KUrl( result ) );
      setUriType( data, KUriFilterData::NetProtocol );
      setSearchProvider(data, provider->name(), data.typedString(), QL1C(filter->keywordDelimiter()));

      KUriFilterPlugin::ProviderInfoList providerInfo;
      populateProvidersList(providerInfo, data);
      setPreferredSearchProviders( data, providerInfo);
      delete provider;
      return true;
    }
  }
  return false;
}

#include "kuriikwsfilter.moc"
