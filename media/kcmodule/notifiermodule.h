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

#ifndef _NOTIFIERMODULE_H_
#define _NOTIFIERMODULE_H_

#include <kcmodule.h>

#include "notifiersettings.h"
#include "notifiermoduleview.h"

class NotifierModule : public KCModule
{
	Q_OBJECT

public:
	NotifierModule( KInstance *inst, QWidget* parent);
	~NotifierModule();

	void load();
	void save();
	void defaults();

private slots:
	void slotAdd();
	void slotDelete();
	void slotEdit();
	void slotToggleAuto();

	void slotActionSelected( Q3ListBoxItem * item );
	void slotMimeTypeChanged( int index );

private:
	void updateListBox();

	QString m_mimetype;
	NotifierSettings m_settings;
	NotifierModuleView *m_view;
};

#endif
