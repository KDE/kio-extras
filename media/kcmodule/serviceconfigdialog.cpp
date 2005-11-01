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

#include <klocale.h>
#include <klineedit.h>
#include <kactionselector.h>
#include <kicondialog.h>
#include <q3listbox.h>
#include <kservice.h>
#include <kopenwith.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <qpixmap.h>
#include <qicon.h>


#include "mimetypelistboxitem.h"

ServiceConfigDialog::ServiceConfigDialog(NotifierServiceAction *action,
                                         const QStringList &mimetypesList,
                                         QWidget* parent, const char* name)
	: KDialogBase(parent, name, true, i18n("Edit Service"), Ok|Cancel, Ok, true),
	  m_action(action)
{
	m_view = new ServiceView(this);
	
	m_view->iconButton->setIcon( m_action->iconName() );
	m_view->labelEdit->setText( m_action->label() );
	m_view->commandEdit->setText( m_action->service().m_strExec );

	QIcon iconSet = SmallIconSet("configure");
	QPixmap pixMap = iconSet.pixmap( QIcon::Small, QIcon::Normal );
	m_view->commandButton->setIconSet( iconSet );
	m_view->commandButton->setFixedSize( pixMap.width()+8, pixMap.height()+8 );
	
	m_iconChanged = false;

	QStringList all_mimetypes = mimetypesList;
	QStringList action_mimetypes = action->mimetypes();

	QStringList::iterator it = all_mimetypes.begin();
	QStringList::iterator end = all_mimetypes.end();

	for (  ; it!=end; ++it )
	{
		Q3ListBox *list;
		
		if ( action_mimetypes.contains( *it ) )
		{
			list = m_view->mimetypesSelector->selectedListBox();
		}
		else
		{
			list = m_view->mimetypesSelector->availableListBox();
		}
		
		new MimetypeListBoxItem(  *it, list );
	}
	
	setMainWidget(m_view);
	setCaption( m_action->label() );

	connect( m_view->iconButton, SIGNAL( iconChanged(QString) ),
	         this, SLOT( slotIconChanged() ) );
	connect( m_view->commandButton, SIGNAL( clicked() ),
	         this, SLOT( slotCommand() ) );
}

bool operator==( KDEDesktopMimeType::Service s1, KDEDesktopMimeType::Service s2 )
{
	return ( s1.m_strName==s2.m_strName )
	    && ( s1.m_strIcon==s2.m_strIcon )
	    && ( s1.m_strExec==s2.m_strExec );
}

bool operator!=( KDEDesktopMimeType::Service s1, KDEDesktopMimeType::Service s2 )
{
	return !( s1==s2 );
}

void ServiceConfigDialog::slotOk()
{
	KDEDesktopMimeType::Service service;
	service.m_strName = m_view->labelEdit->text();
	service.m_strIcon = m_view->iconButton->icon();
	service.m_strExec = m_view->commandEdit->text();

	QStringList mimetypes;
	
	uint list_count = m_view->mimetypesSelector->selectedListBox()->count();
	for( uint i=0; i < list_count; ++i )
	{
		Q3ListBoxItem *item = m_view->mimetypesSelector->selectedListBox()->item(i);
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
	KOpenWithDlg d(this);
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
