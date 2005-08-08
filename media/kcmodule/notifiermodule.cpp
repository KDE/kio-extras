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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "notifiermodule.h"

#include <klocale.h>

#include <qlayout.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <Q3ValueList>
#include <QBoxLayout>
#include <kcombobox.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>

#include "notifiersettings.h"
#include "serviceconfigdialog.h"
#include "actionlistboxitem.h"
#include "mimetypelistboxitem.h"

NotifierModule::NotifierModule(QWidget *parent, const char *name)
	: KCModule(parent, name)
{
	QBoxLayout *layout = new QVBoxLayout( this, 0, KDialog::spacingHint() );
	
	m_view = new NotifierModuleView( this );
	layout->addWidget( m_view );
	
	m_view->addButton->setGuiItem( KStdGuiItem::add() );
	m_view->editButton->setGuiItem( KStdGuiItem::properties() );
	m_view->deleteButton->setGuiItem( KStdGuiItem::del() );
	
	m_view->mimetypesCombo->insertItem( i18n("All Mime Types") );

	QStringList mimetypes = m_settings.supportedMimetypes();

	QStringList::iterator it = mimetypes.begin();
	QStringList::iterator end = mimetypes.end();

#warning "Needs porting. listBox() is no longer supplied in Qt4 QComboBox." 
#if 0
	for ( ; it!=end; ++it )
	{
		new MimetypeListBoxItem( *it, m_view->mimetypesCombo->listBox() );
	}
#endif
	updateListBox();

	connect( m_view->mimetypesCombo, SIGNAL( activated(int) ),
	         this, SLOT( slotMimeTypeChanged(int) ) );
	connect( m_view->actionsList, SIGNAL( selectionChanged(Q3ListBoxItem*) ),
	         this, SLOT( slotActionSelected(Q3ListBoxItem*) ) );
	connect( m_view->addButton, SIGNAL( clicked() ),
	         this, SLOT( slotAdd() ) );
	connect( m_view->editButton, SIGNAL( clicked() ),
	         this, SLOT( slotEdit() ) );
	connect( m_view->deleteButton, SIGNAL( clicked() ),
	         this, SLOT( slotDelete() ) );
	connect( m_view->toggleAutoButton, SIGNAL( clicked() ),
	         this, SLOT( slotToggleAuto() ) );
}

NotifierModule::~NotifierModule()
{
}

void NotifierModule::load()
{
	m_settings.reload();
	slotMimeTypeChanged( m_view->mimetypesCombo->currentItem() );
}

void NotifierModule::save()
{
	m_settings.save();
}

void NotifierModule::defaults()
{
	m_settings.clearAutoActions();
	slotMimeTypeChanged( m_view->mimetypesCombo->currentItem() );
}

void NotifierModule::updateListBox()
{
	m_view->actionsList->clear();
	slotActionSelected( 0L );

	Q3ValueList<NotifierAction*> services;
	if ( m_mimetype.isEmpty() )
	{
		services = m_settings.actions();
	}
	else
	{
		services = m_settings.actionsForMimetype( m_mimetype );
	}

	Q3ValueList<NotifierAction*>::iterator it;
	
	for ( it = services.begin(); it != services.end(); ++it )
	{
		new ActionListBoxItem( *it, m_mimetype, m_view->actionsList );
	}
}

void NotifierModule::slotActionSelected(Q3ListBoxItem *item)
{
	NotifierAction *action = 0L;

	if ( item!=0L )
	{
		ActionListBoxItem *action_item
			= static_cast<ActionListBoxItem*>(item);
		action = action_item->action();
	}

	bool isWritable = action!=0L && action->isWritable();
	m_view->deleteButton->setEnabled( isWritable );
	m_view->editButton->setEnabled( isWritable );
	m_view->addButton->setEnabled( TRUE );
	m_view->toggleAutoButton->setEnabled( action!=0L && !m_mimetype.isEmpty() );
}

void NotifierModule::slotMimeTypeChanged(int index)
{
	if ( index == 0 )
	{
		m_mimetype = QString();
	}
	else
	{
#warning "Needs porting. listBox() is no longer supplied in Qt4 QComboBox." 
#if 0
		Q3ListBoxItem *item = m_view->mimetypesCombo->listBox()->item( index );
		MimetypeListBoxItem *mime_item
			= static_cast<MimetypeListBoxItem*>( item );
		m_mimetype = mime_item->mimetype();
#endif
	}

	updateListBox();
}

void NotifierModule::slotAdd()
{
	NotifierServiceAction *action = new NotifierServiceAction();
	ServiceConfigDialog dialog(action, m_settings.supportedMimetypes(), this);
	
	int value = dialog.exec();
	
	if ( value == QDialog::Accepted )
	{
		m_settings.addAction( action );
		updateListBox();
		emit changed( true );
	}
	else
	{
		delete action;
	}
}

void NotifierModule::slotEdit()
{
	ActionListBoxItem *action_item
		= static_cast<ActionListBoxItem*>(m_view->actionsList->selectedItem());
	
	NotifierServiceAction * action;
	if ( (action = dynamic_cast<NotifierServiceAction*>( action_item->action() )) )
	{
		ServiceConfigDialog dialog(action, m_settings.supportedMimetypes(), this);
		
		int value = dialog.exec();
		
		if ( value == QDialog::Accepted )
		{
			updateListBox();
			emit changed( true );
		}
	}
}

void NotifierModule::slotDelete()
{
	ActionListBoxItem *action_item
		= static_cast<ActionListBoxItem*>(m_view->actionsList->selectedItem());
	
	NotifierServiceAction *action;
	if ( (action = dynamic_cast<NotifierServiceAction*>( action_item->action() ) ))
	{
		m_settings.deleteAction( action );
		updateListBox();
		emit changed( true );
	}
}

void NotifierModule::slotToggleAuto()
{
	ActionListBoxItem *action_item
		= static_cast<ActionListBoxItem*>( m_view->actionsList->selectedItem() );
	NotifierAction *action = action_item->action();

	int index = m_view->actionsList->index( action_item );
	
	if ( action->autoMimetypes().contains( m_mimetype ) )
	{
		m_settings.resetAutoAction( m_mimetype );
	}
	else
	{
		m_settings.setAutoAction( m_mimetype, action );
	}

	updateListBox();
	emit changed( true );

	m_view->actionsList->setSelected( index, true );
}

#include "notifiermodule.moc"
