/* Name: pixmapcombo.cpp

   Description: This file is a part of the libmwn library.

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


#include "common.h"
#include "pixmapcombo.h"
#include "qpainter.h"
#include "corellistboxitem.h"
#include "qapplication.h"

void CPixmapCombo::paintEvent(QPaintEvent *event)
{
	QRect r(2, 2, width()-4, height()-4);

	QPainter p(this);
    
	if (event)
		p.setClipRect(event->rect());
    
	QColorGroup g = colorGroup();
	QColor bg = /*hasFocus() ? (QApplication::winStyleHighlightColor()) : */
		(isEnabled() ? g.base() : g.background());
	
	QBrush fill(bg);
	
	qDrawWinPanel(&p, 0, 0, width(), height(), g, TRUE, NULL);

	p.fillRect(2, r.top(), r.width(), r.height(), fill);

	QRect arrowR = QRect(width() - 2 - 16, 2, 16, height() - 4);

	qDrawWinPanel(&p, arrowR, g, FALSE);
	
	qDrawArrow(&p, QTARROWTYPE(Qt::DownArrow), QTGUISTYLE(Qt::WindowsStyle), FALSE,
		    arrowR.x() + 2, arrowR.y() + 2,
		    arrowR.width() - 4, arrowR.height() - 4, g
//#ifdef QT_20
		    , TRUE
//#endif
		    ); 
	
#ifndef QT_20
	CListBoxItem *lbitem = (CListBoxItem*)(((CWorkaroundListBox*)listBox())->item(listBox()->currentItem()));
#else
	CListBoxItem *lbitem = (CListBoxItem*)(listBox()->item(listBox()->currentItem()));
#endif
	
	if (NULL != lbitem)
	{
		const QPixmap *pPixmap = lbitem->pixmap();
	
		p.drawPixmap(2+(18 - pPixmap->width())/2, 
								 r.top() + (r.height()- pPixmap->height())/2, 
								 *pPixmap);
		
		int nTextOffset = 24;
	
		if (hasFocus())
		{
			QRect r =	p.boundingRect(nTextOffset, 
															 0, 
															 width()-nTextOffset-arrowR.width()-4, 
															 height(), 
															 Qt::AlignVCenter, 
															 lbitem->text());
			
			p.fillRect(r.left()-2, r.top()-1, r.width()+4, r.height()+2, QApplication::winStyleHighlightColor());
			p.drawWinFocusRect(r.left()-2, r.top()-1, r.width()+4, r.height()+2, QApplication::winStyleHighlightColor() /*backgroundColor()*/);
		}
		
		p.setPen(hasFocus() ? g.base() : g.text());
		p.drawText(nTextOffset, 0, width()-24-arrowR.width()-4, height(), AlignVCenter, lbitem->text());
	}

  p.setClipping(FALSE);
}

