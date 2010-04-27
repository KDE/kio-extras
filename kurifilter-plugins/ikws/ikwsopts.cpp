/*
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
 * Copyright (c) 2001, 2002 Dawit Alemayehu <adawit@kde.org>
 * Copyright (c) 2009 Nick Shaforostoff <shaforostoff@kde.ru>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "ikwsopts.h"
#include "ikwsopts_p.h"

#include "kuriikwsfiltereng.h"
#include "searchprovider.h"
#include "searchproviderdlg.h"

#include <kbuildsycocaprogressdialog.h>
#include <kstandarddirs.h>
#include <kservicetypetrader.h>

#include <QtCore/QFile>
#include <QtDBus/QtDBus>
#include <QtGui/QSortFilterProxyModel>



//BEGIN ProvidersModel

ProvidersModel::~ProvidersModel()
{
  qDeleteAll(m_providers);
}

QVariant ProvidersModel::headerData(int section, Qt::Orientation orientation, int role ) const
{
  Q_UNUSED(orientation);
  if (role == Qt::DisplayRole)
  {
    if (section==Name)
      return i18n("Name");
    return i18n("Shortcuts");
  }   
  return QVariant();
}

Qt::ItemFlags ProvidersModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsEnabled;
  if (index.column()==Name)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ProvidersModel::setData (const QModelIndex& index, const QVariant& value, int role)
{
  if (role==Qt::CheckStateRole)
  {
    m_favouriteEngines[m_providers.at(index.row())->desktopEntryName()]=value.toInt()==Qt::Checked;
    emit dataModified();
    return true;
  }
  return false;
}

QVariant ProvidersModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();
  
  if (role == Qt::CheckStateRole && index.column()==Name)
    return m_favouriteEngines[m_providers.at(index.row())->desktopEntryName()] ? Qt::Checked : Qt::Unchecked;

  if (role == Qt::DisplayRole)
  {
    if (index.column()==Name)
      return m_providers.at(index.row())->name();
    if (index.column()==Shortcuts)
      return m_providers.at(index.row())->keys().join(",");
  }

  if (role == Qt::UserRole)
    return index.row();//a nice way to bypass proxymodel

  return QVariant();
}

void ProvidersModel::setProviders(const QList<SearchProvider*>& providers, const QStringList& favouriteEngines)
{
  m_providers=providers;
  foreach(const QString& engine, favouriteEngines)
    m_favouriteEngines[engine]=true;

  reset();
}

int ProvidersModel::rowCount(const QModelIndex & parent) const
{
  if (parent.isValid())
    return 0;
  return m_providers.size();
}

QAbstractListModel* ProvidersModel::createListModel() 
{
  ProvidersListModel* pListModel = new ProvidersListModel(m_providers, this);
  connect(this, SIGNAL(modelAboutToBeReset()),        pListModel, SIGNAL(modelAboutToBeReset())); 
  connect(this, SIGNAL(modelReset()),                 pListModel, SIGNAL(modelReset())); 
  connect(this, SIGNAL(layoutAboutToBeChanged()),     pListModel, SIGNAL(modelReset()));
  connect(this, SIGNAL(layoutChanged()),              pListModel, SIGNAL(modelReset()));
  connect(this, SIGNAL(dataChanged(QModelIndex, QModelIndex)),        pListModel, SLOT(emitDataChanged(QModelIndex, QModelIndex)));
  connect(this, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)), pListModel, SLOT(emitRowsAboutToBeInserted(QModelIndex, int, int ))); 
  connect(this, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)),  pListModel, SLOT(emitRowsAboutToBeRemoved(QModelIndex, int, int ))); 
  connect(this, SIGNAL(rowsInserted(QModelIndex, int, int )),         pListModel, SLOT(emitRowsInserted(QModelIndex, int, int )));
  connect(this, SIGNAL(rowsRemoved(QModelIndex, int, int)),           pListModel, SLOT(emitRowsRemoved(QModelIndex, int, int )));
  
  return pListModel;
}

void ProvidersModel::deleteProvider(SearchProvider* p)
{
  int row=m_providers.indexOf(p);
  beginRemoveRows(QModelIndex(), row, row);
  const QString removed = m_providers.takeAt(row)->desktopEntryName();
  m_favouriteEngines.remove(removed);
  endRemoveRows();
  delete p;
  emit dataModified();
}

void ProvidersModel::addProvider(SearchProvider* p)
{
  beginInsertRows(QModelIndex(), m_providers.size(), m_providers.size());
  m_providers.append(p); 
  endInsertRows();
  emit dataModified();
}

void ProvidersModel::changeProvider(SearchProvider* p)
{
  int row = m_providers.indexOf(p);
  emit dataChanged(index(row,0),index(row,ColumnCount-1));
  emit dataModified();
}

QStringList ProvidersModel::favouriteEngines() const
{
  QStringList f;
  for (QMap<QString, bool>::const_iterator i = m_favouriteEngines.constBegin(); i != m_favouriteEngines.constEnd(); ++i)
  {
    if (i.value())
      f.append(i.key());
  }  
  return f;
}
//END ProvidersModel

//BEGIN ProvidersListModel
ProvidersListModel::ProvidersListModel(QList<SearchProvider*>& providers,  QObject* parent)
    : QAbstractListModel(parent)
    , m_providers(providers)        
{}

QVariant ProvidersListModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role==Qt::DisplayRole)
  {
    if (index.row() == m_providers.size())
      return i18n("None");
    return m_providers.at(index.row())->name();
  }

  if (role==ShortNameRole)
  {
    if (index.row() == m_providers.size())
      return QString();
    return m_providers.at(index.row())->desktopEntryName();
  }
    
  return QVariant();
}

int ProvidersListModel::rowCount (const QModelIndex & parent) const
{
  if (parent.isValid())
    return 0;
  return m_providers.size() + 1;
}
//END ProvidersListModel

static QSortFilterProxyModel* wrapInProxyModel(QAbstractItemModel* model)
{
  QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(model);
  proxyModel->setSourceModel(model);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
  return proxyModel;
}

FilterOptions::FilterOptions(const KComponentData &componentData, QWidget *parent)
 : KCModule(componentData, parent)
 , m_providersModel(new ProvidersModel(this))
{
  m_dlg.setupUi(this);

  m_dlg.lvSearchProviders->setModel(wrapInProxyModel(m_providersModel));
  m_dlg.cmbDefaultEngine->setModel(wrapInProxyModel(m_providersModel->createListModel()));

  // Connect all the signals/slots...
  connect(m_dlg.cbEnableShortcuts, SIGNAL(toggled(bool)), m_dlg.grOpts, SLOT(setEnabled(bool)));
  connect(m_dlg.cbEnableShortcuts, SIGNAL(toggled(bool)), SLOT(changed()));

  connect(m_providersModel, SIGNAL(dataModified()), SLOT(changed()));

  connect(m_dlg.cmbDefaultEngine, SIGNAL(currentIndexChanged(int)),  SLOT(changed()));
  connect(m_dlg.cmbDelimiter,     SIGNAL(currentIndexChanged(int)),  SLOT(changed()));

  connect(m_dlg.pbNew,    SIGNAL(clicked()), SLOT(addSearchProvider()));
  connect(m_dlg.pbDelete, SIGNAL(clicked()), SLOT(deleteSearchProvider()));
  connect(m_dlg.pbChange, SIGNAL(clicked()), SLOT(changeSearchProvider()));
  connect(m_dlg.lvSearchProviders, SIGNAL(activated(QModelIndex)),    SLOT(updateSearchProviderEditingButons()));
  connect(m_dlg.lvSearchProviders, SIGNAL(doubleClicked(QModelIndex)),SLOT(changeSearchProvider()));
}

QString FilterOptions::quickHelp() const
{
  return i18n("<p>In this module you can configure the web shortcuts feature. "
              "Web shortcuts allow you to quickly search or lookup words on "
              "the Internet. For example, to search for information about the "
              "KDE project using the Google engine, you simply type <b>gg:KDE</b> "
              "or <b>google:KDE</b>.</p>"
              "<p>If you select a default search engine, normal words or phrases "
              "will be looked up at the specified search engine by simply typing "
              "them into applications, such as Konqueror, that have built-in support "
              "for such a feature.</p>");
}

void FilterOptions::setDefaultEngine(int index)
{
  QSortFilterProxyModel* proxy=qobject_cast<QSortFilterProxyModel*>(m_dlg.cmbDefaultEngine->model());
  if (index==-1)
    index=proxy->rowCount()-1;//"None" is the last

  QModelIndex sel=proxy->mapFromSource(proxy->sourceModel()->index(index,0));
  m_dlg.cmbDefaultEngine->setCurrentIndex(sel.row());
  m_dlg.cmbDefaultEngine->view()->setCurrentIndex(sel);  //TODO remove this when Qt bug is fixed
}

void FilterOptions::load()
{
  KConfig config(KURISearchFilterEngine::self()->name() + "rc", KConfig::NoGlobals );
  KConfigGroup group = config.group("General");

  QString defaultSearchEngine = group.readEntry("DefaultSearchEngine");

  QStringList favouriteEngines;
  favouriteEngines << "google" << "google_groups" << "google_news" << "webster" << "dmoz" << "wikipedia";
  favouriteEngines = group.readEntry("FavoriteSearchEngines", favouriteEngines );

  const KService::List services = KServiceTypeTrader::self()->query("SearchProvider");

  QList<SearchProvider*> providers;
  int defaultProviderIndex = services.size(); //default is "None", it is last in the list
  foreach(const KService::Ptr &service, services)
  {
    SearchProvider* provider=new SearchProvider(service);
    if (defaultSearchEngine == provider->desktopEntryName())
      defaultProviderIndex = providers.size();
    providers.append(provider);
  }

  m_providersModel->setProviders(providers, favouriteEngines);
  m_dlg.lvSearchProviders->setColumnWidth(0,200);
  m_dlg.lvSearchProviders->resizeColumnToContents(1);
  m_dlg.lvSearchProviders->sortByColumn(0,Qt::AscendingOrder);
  m_dlg.cmbDefaultEngine->model()->sort(0,Qt::AscendingOrder);
  setDefaultEngine(defaultProviderIndex);

  m_dlg.cbEnableShortcuts->setChecked(group.readEntry("EnableWebShortcuts", true));
  
  QString delimiter = group.readEntry ("KeywordDelimiter", ":");
  setDelimiter(delimiter[0].toLatin1() );
}

char FilterOptions::delimiter()
{
  const char delimiters[]={':',' '};
  return delimiters[m_dlg.cmbDelimiter->currentIndex()];
}

void FilterOptions::setDelimiter (char sep)
{
  m_dlg.cmbDelimiter->setCurrentIndex(sep==' ');
}

void FilterOptions::save()
{
  KConfig config(KURISearchFilterEngine::self()->name() + "rc", KConfig::NoGlobals );

  KConfigGroup group = config.group("General");
  group.writeEntry("EnableWebShortcuts", m_dlg.cbEnableShortcuts->isChecked());
  group.writeEntry("KeywordDelimiter", QString(delimiter() ));
  group.writeEntry("DefaultSearchEngine", m_dlg.cmbDefaultEngine->view()->currentIndex().data(ProvidersListModel::ShortNameRole));
  group.writeEntry("FavoriteSearchEngines", m_providersModel->favouriteEngines());
      
  QList<SearchProvider*> providers = m_providersModel->providers();
  QString path = KGlobal::mainComponent().dirs()->saveLocation("services", "searchproviders/");
  int changedProviderCount = 0;
  foreach(SearchProvider* provider, providers)
  {
    if (!provider->isDirty())
      continue;
    changedProviderCount++;

    QString name = provider->desktopEntryName();
    if (name.isEmpty())
    {
      // New provider                        
      // Take the longest search shortcut as filename,
      // if such a file already exists, append a number and increase it
      // until the name is unique
      foreach(const QString& key, provider->keys())
      {
        if (key.length() > name.length())
          name = key.toLower();
      }
      for (int suffix = 0; ; ++suffix)
      {
        QString located, check = name;
        if (suffix)
          check += QString().setNum(suffix);

        if ((located = KStandardDirs::locate("services", "searchproviders/" + check + ".desktop")).isEmpty())
        {
          name = check;
          break;
        }
        else if (located.startsWith(path))
        {
          // If it's a deleted (hidden) entry, overwrite it
          if (KService(located).isDeleted())
            break;
        }
      }
    }

    KConfig _service(path + name + ".desktop", KConfig::SimpleConfig );
    KConfigGroup service(&_service, "Desktop Entry");                                                                                                      
    service.writeEntry("Type",          "Service");
    service.writeEntry("ServiceTypes",  "SearchProvider");
    service.writeEntry("Name",          provider->name());
    service.writeEntry("Query",         provider->query());
    service.writeEntry("Keys",          provider->keys());
    service.writeEntry("Charset",       provider->charset());

    // we might be overwriting a hidden entry
    service.writeEntry("Hidden", false);
  }
 
  foreach(const QString& providerName, m_deletedProviders)
  {
    QStringList matches = KGlobal::mainComponent().dirs()->findAllResources("services", "searchproviders/" + providerName + ".desktop");

    // Shouldn't happen
    if (!matches.size())
      continue;

    changedProviderCount++;
    if (matches.size() == 1 && matches.first().startsWith(path))
    {
      // If only the local copy existed, unlink it
      // TODO: error handling
      QFile::remove(matches.first());
      continue;
    }
    KConfig _service(path + providerName + ".desktop", KConfig::SimpleConfig );
    KConfigGroup service(&_service,     "Desktop Entry");
    service.writeEntry("Type",          "Service");
    service.writeEntry("ServiceTypes",  "SearchProvider");
    service.writeEntry("Hidden",        true);
  }

  config.sync();

  emit changed(false);

  // Update filters in running applications...
  QDBusMessage msg = QDBusMessage::createSignal("/", "org.kde.KUriFilterPlugin", "configure");
  QDBusConnection::sessionBus().send(msg);

  // If the providers changed, tell sycoca to rebuild its database...
  if (changedProviderCount)
    KBuildSycocaProgressDialog::rebuildKSycoca(this);
}

void FilterOptions::defaults()
{
  setDelimiter (':');
  setDefaultEngine(-1);
  m_dlg.cbEnableShortcuts->setChecked(true);
}

void FilterOptions::addSearchProvider()
{
  QList<SearchProvider*> providers = m_providersModel->providers();
  SearchProviderDialog dlg(0, providers, this);

  if (dlg.exec())
    m_providersModel->addProvider(dlg.provider());
  
  m_providersModel->changeProvider(dlg.provider());
}

void FilterOptions::changeSearchProvider()
{
  QList<SearchProvider*> providers = m_providersModel->providers();
  SearchProvider* provider = providers.at(m_dlg.lvSearchProviders->currentIndex().data(Qt::UserRole).toInt());
  SearchProviderDialog dlg(provider, providers, this);

  if (dlg.exec())
    m_providersModel->changeProvider(dlg.provider());
}

void FilterOptions::deleteSearchProvider()
{
  SearchProvider* provider = m_providersModel->providers().at(m_dlg.lvSearchProviders->currentIndex().data(Qt::UserRole).toInt());
  m_deletedProviders.append(provider->desktopEntryName());
  m_providersModel->deleteProvider(provider);
}

void FilterOptions::updateSearchProviderEditingButons()
{
  bool enable=m_dlg.lvSearchProviders->currentIndex().isValid();
  m_dlg.pbChange->setEnabled(enable);
  m_dlg.pbDelete->setEnabled(enable);
}

#include "ikwsopts.moc"
#include "ikwsopts_p.moc"

// kate: replace-tabs 1; indent-width 2;
