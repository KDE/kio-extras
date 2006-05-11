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

#ifndef _ACTIONLISTBOXITEM_H_
#define _ACTIONLISTBOXITEM_H_

#include <q3listbox.h>
#include <QString>

#include "notifieraction.h"

class ActionListBoxItem : public Q3ListBoxPixmap
{
public:
	ActionListBoxItem(NotifierAction *action, QString mimetype, Q3ListBox *parent);
	~ActionListBoxItem();

	NotifierAction *action() const;

private:
	NotifierAction *m_action;
};

#endif
