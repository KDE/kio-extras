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

#include "serviceconfigdialog.h"
#include "mimetypelistboxitem.h"

#include <klocale.h>
#include <klineedit.h>
#include <kactionselector.h>
#include <kicondialog.h>
#include <QListWidget>
#include <kservice.h>
#include <kopenwithdialog.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <QPixmap>
#include <QIcon>

ServiceConfigDialog::ServiceConfigDialog(NotifierServiceAction *action,
                                         const QStringList &mimetypesList,
                                         QWidget* parent)
	: KDialog(parent),
	  m_action(action)
{
  setModal( true );
  setCaption( i18n("Edit Service") );
  setButtons( Ok|Cancel );
  setDefaultButton( Ok );
  showButtonSeparator( true );

	m_view = new ServiceView(this);

	m_view->iconButton->setIcon( m_action->iconName() );
	m_view->labelEdit->setText( m_action->label() );
	m_view->commandEdit->setText( m_action->service().exec() );

	m_view->commandButton->setIcon( KIcon("configure") );
	const int pixmapSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
	m_view->commandButton->setFixedSize( pixmapSize+8, pixmapSize+8 );

	m_iconChanged = false;

	QStringList all_mimetypes = mimetypesList;
	QStringList action_mimetypes = action->mimetypes();

	QStringList::iterator it = all_mimetypes.begin();
	QStringList::iterator end = all_mimetypes.end();

	for (  ; it!=end; ++it )
	{
		QListWidget *list;

		if ( action_mimetypes.contains( *it ) )
		{
			list = m_view->mimetypesSelector->selectedListWidget();
		}
		else
		{
			list = m_view->mimetypesSelector->availableListWidget();
		}

		new MimetypeListBoxItem(  *it, list );
	}

	setMainWidget(m_view);
	setCaption( m_action->label() );

	connect( m_view->iconButton, SIGNAL( iconChanged(QString) ),
	         this, SLOT( slotIconChanged() ) );
	connect( m_view->commandButton, SIGNAL( clicked() ),
	         this, SLOT( slotCommand() ) );
	connect( this, SIGNAL(okClicked()),this, SLOT(slotOk()));
}

bool operator==( const KServiceAction& s1, const KServiceAction& s2 )
{
	return ( s1.text()==s2.text() )
	    && ( s1.icon()==s2.icon() )
	    && ( s1.exec()==s2.exec() );
}

bool operator!=( const KServiceAction& s1, const KServiceAction& s2 )
{
	return !( s1==s2 );
}

void ServiceConfigDialog::slotOk()
{
    static int serviceId = 0;
    KServiceAction service(QString::number(++serviceId),
                           m_view->labelEdit->text(),
                           m_view->iconButton->icon(),
                           m_view->commandEdit->text()); // exec

	QStringList mimetypes;

	uint list_count = m_view->mimetypesSelector->selectedListWidget()->count();
	for( uint i=0; i < list_count; ++i )
	{
		QListWidgetItem *item = m_view->mimetypesSelector->selectedListWidget()->item(i);
		MimetypeListBoxItem *mime_item = static_cast<MimetypeListBoxItem*>( item );
		mimetypes.append( mime_item->mimetype() );
	}

	if ( service!=m_action->service() || mimetypes!=m_action->mimetypes() )
	{
		m_action->setService( service );
		m_action->setMimetypes( mimetypes );
		accept();
	}
	else
	{
		reject();
	}
}

void ServiceConfigDialog::slotIconChanged()
{
	m_iconChanged = true;
}

void ServiceConfigDialog::slotCommand()
{
	KOpenWithDialog d(this);
	int value = d.exec();
	if ( value == QDialog::Accepted )
	{
		KService::Ptr service = d.service();
		if ( service )
		{
			m_view->commandEdit->setText( service->exec() );
			if ( m_iconChanged == false )
			{
				m_view->iconButton->setIcon( service->icon() );
			}
		}
	}
}

#include "serviceconfigdialog.moc"
