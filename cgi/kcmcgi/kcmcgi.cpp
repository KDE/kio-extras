/*
   Copyright (C) 2002 Cornelius Schumacher <schumacher@kde.org>

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kaboutdata.h>
#include <kfiledialog.h>

#include <qlayout.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qhbox.h>

#include "kcmcgi.h"
#include "kcmcgi.moc"

extern "C"
{
  KCModule *create_cgi( QWidget *parent, const char * )
  {
    KGlobal::locale()->insertCatalogue("kcmcgi");
    return new KCMCgi( parent, "kcmcgi" );
  }
}


KCMCgi::KCMCgi(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  setButtons(Default|Apply);

  QVBoxLayout *topLayout = new QVBoxLayout(this);
  topLayout->setMargin( KDialog::marginHint() );

  QGroupBox *topBox = new QGroupBox( 1, Horizontal, i18n("Paths to local CGI Programs"), this );
  topLayout->addWidget( topBox );

  mListBox = new QListBox( topBox );

  QHBox *buttonBox = new QHBox( topBox );
  buttonBox->setSpacing( KDialog::spacingHint() );
    
  mAddButton = new QPushButton( i18n("Add..."), buttonBox );
  connect( mAddButton, SIGNAL( clicked() ), SLOT( addPath() ) );

  mRemoveButton = new QPushButton( i18n("Remove"), buttonBox );
  connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removePath() ) );

  mConfig = new KConfig("kcmcgirc");

  load();
}

KCMCgi::~KCMCgi()
{
  delete mConfig;
}

void KCMCgi::defaults()
{
  mListBox->clear();
}

void KCMCgi::save()
{
  QStringList paths;
  
  uint i;
  for( i = 0; i < mListBox->count(); ++i ) {
    paths.append( mListBox->text( i ) );
  }

  mConfig->setGroup( "General" );
  mConfig->writeEntry( "Paths", paths );

  mConfig->sync();
}

void KCMCgi::load()
{
  mConfig->setGroup( "General" );
  QStringList paths = mConfig->readListEntry( "Paths" );

  mListBox->insertStringList( paths );
}

void KCMCgi::addPath()
{
  QString path = KFileDialog::getExistingDirectory( QString::null, this );
  
  if ( !path.isEmpty() ) {
    mListBox->insertItem( path );
    emit changed( true );
  }
}

void KCMCgi::removePath()
{
  int index = mListBox->currentItem();
  if ( index >= 0 ) {
    mListBox->removeItem( index );
    emit changed( true );
  }
}

QString KCMCgi::quickHelp() const
{
  return i18n("<h1>CGI programs</h1> The CGI KIO slave lets you execute "
              "local CGI programs without the need to run a web server. "
              "In this control module you can configure the paths that "
              "are searched for CGI scripts.");
}

const KAboutData* KCMCgi::aboutData() const
{
  KAboutData *about =
    new KAboutData( I18N_NOOP("kcmcgi"),
                    I18N_NOOP("CGI KIO Slave Control Module"),
                    0, 0, KAboutData::License_GPL,
                    I18N_NOOP("(c) 2002 Cornelius Schumacher") );

  about->addAuthor( "Cornelius Schumacher", 0, "schumacher@kde.org" );

  return about;
}
