/* Name: tips.h

   Description: This file is a part of the Corel File Manager application.

   Author:	Oleg Noskov (olegn@corel.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.


*/

/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#ifndef __INC_TIPS_H__
#define __INC_TIPS_H__

#include <qtooltip.h>
#include <listview.h>

class CTreeToolTip : public QToolTip
{
public:
		CTreeToolTip(CListView *parent) : QToolTip(parent->viewport())
		{
		    m_pParent = parent;
		}

protected:
		void maybeTip(const QPoint&);
		CListView *m_pParent;
};

#endif /* __INC_TIPS_H__ */
