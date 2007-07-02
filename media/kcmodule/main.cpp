/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "main.h"

#include <QTabWidget>
#include <QLayout>
//Added by qt3to4:
#include <QVBoxLayout>

#include <klocale.h>
#include <kaboutdata.h>
#include <kdialog.h>

#include <kgenericfactory.h>

#include "notifiermodule.h"
#include "managermodule.h"


typedef KGenericFactory<MediaModule, QWidget> MediaFactory;
K_EXPORT_COMPONENT_FACTORY( media, MediaFactory( "kcmmedia" ) )


MediaModule::MediaModule( QWidget *parent, const QStringList& )
	: KCModule(MediaFactory::componentData(), parent )
{
	QVBoxLayout *layout = new QVBoxLayout( this );
	layout->setSpacing( KDialog::spacingHint() );
	layout->setMargin( 0 );
	QTabWidget *tab = new QTabWidget( this );

	layout->addWidget( tab );


        m_notifierModule = new NotifierModule( MediaFactory::componentData(), this );
        m_notifierModule->setObjectName( "notifier" );
	tab->addTab( m_notifierModule, i18n( "&Notifications" ) );
	connect( m_notifierModule, SIGNAL( changed( bool ) ),
	         this, SLOT( moduleChanged( bool ) ) );

	m_managerModule = new ManagerModule( MediaFactory::componentData(), this );
        m_managerModule->setObjectName( "manager" );
	tab->addTab( m_managerModule, i18n( "&Advanced" ) );
	connect( m_managerModule, SIGNAL( changed( bool ) ),
	         this, SLOT( moduleChanged( bool ) ) );



	KAboutData * about = new KAboutData("kcmmedia", 0,
	                                    ki18n("Storage Media"),
	                                    "0.6",
	                                    ki18n("Storage Media Control Panel Module"),
	                                    KAboutData::License_GPL_V2,
	                                    ki18n("(c) 2005 Jean-Remy Falleri"));
	about->addAuthor(ki18n("Jean-Remy Falleri"), ki18n("Maintainer"), "jr.falleri@laposte.net");
	about->addAuthor(ki18n("Kevin Ottens"), KLocalizedString(), "ervin ipsquad net");
	about->addCredit(ki18n("Achim Bohnet"), ki18n("Help for the application design"));

	setAboutData( about );
}

void MediaModule::load()
{
	m_notifierModule->load();
	m_managerModule->load();
}

void MediaModule::save()
{
	m_notifierModule->save();
	m_managerModule->save();
}

void MediaModule::defaults()
{
	m_notifierModule->defaults();
	m_managerModule->defaults();
}

void MediaModule::moduleChanged( bool state )
{
	emit changed( state );
}

QString MediaModule::quickHelp() const
{
	return i18n("FIXME : Write me...");
}

#include "main.moc"
