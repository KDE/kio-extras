/* Name: corellistboxitem.cpp

   Description: This file is a part of the ccui library.

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


#include <qlistbox.h>
#include <qpainter.h>
#include "corellistboxitem.h"

#ifndef QT_2
	#include <qwindowdefs.h>
#endif

#define kTabSize 100

void CListBoxItem::paint(QPainter *p)
{
	p->drawPixmap(3, 0, pm);
	QFontMetrics fm = p->fontMetrics();
    int yPos; // vertical text position
    
	//if (pm.height() < fm.height())
	//	yPos = fm.ascent() + fm.leading()/2;
	//else
	//	yPos = pm.height()/2 - fm.height()/2 + fm.ascent();
    
	if (pm.height() < fm.height())
		yPos = 0;
	else
		yPos = (pm.height() - fm.height())/2;

	p->setTabStops(kTabSize);

#ifdef QT_2
	QSize sz = fm.size(Qt::AlignLeft | Qt::ExpandTabs, text(), -1, kTabSize);
#else
	QSize sz = fm.size(AlignLeft | ExpandTabs, text(), -1, kTabSize);
#endif
	
	
	int x = pm.width() + 5;
	
	if (x < 25)
		x=25;

#ifdef QT_2
	p->drawText(x, yPos, sz.width(), fm.height(), Qt::AlignLeft | Qt::ExpandTabs | Qt::SingleLine, text());
#else
	p->drawText(x, yPos, sz.width(), fm.height(), AlignLeft | ExpandTabs | SingleLine, text());
#endif
	
	//p->drawText(pm.width() + 5, yPos, text());
}
    
int CListBoxItem::height(const QListBox *lb) const
{
	return QMAX(pm.height(), lb->fontMetrics().lineSpacing() + 1);
}

int CListBoxItem::width(const QListBox *lb) const
{
#ifdef QT_2
	QSize sz = lb->fontMetrics().size(Qt::AlignLeft | Qt::ExpandTabs, text(), -1, kTabSize);
#else	
	QSize sz = lb->fontMetrics().size(AlignLeft | ExpandTabs, text(), -1, kTabSize);
#endif
	
	int x = pm.width() + 5;
	
	if (x < 25)
		x=25;
	
	return x + sz.width(); //pm.width() + sz.width() + 6;
}
