/* Name: DriveDiagram.cpp

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


#include "common.h"
#include <qpainter.h>
#include <qcolor.h>
#include <qpaintdevice.h>
#include "DriveDiagram.h"

#define PIEHEIGHT 12

void CDriveDiagram::paintEvent(QPaintEvent *event)
{
	int alen = (360*16*m_Percent)/100;

	int i;
	QPainter p(this);
    
	if (event != NULL)
		p.setClipRect(event->rect());
	
	QColor ColorUsed(QRgb(16711680));
	QColor ColorFree(QRgb(16711935));
	QColor ColorUsedDark(ColorUsed.red() / 2, ColorUsed.green()/2, ColorUsed.blue() / 2);
	QColor ColorFreeDark(ColorFree.red() / 2, ColorFree.green()/2, ColorFree.blue() / 2);
	
	p.setPen(m_Percent == 100 ? ColorFreeDark : ColorUsedDark);
	
	for (i=0; i < PIEHEIGHT; i++)
		p.drawEllipse(0, i, width(), height()-PIEHEIGHT);

	if (m_Percent > 0 && m_Percent < 100)
	{
		p.setPen(ColorFreeDark);
 	
		for (i=0; i < PIEHEIGHT; i++)
			p.drawArc(0,i,width(), height()-PIEHEIGHT, 180*16, alen);
	}
	
	p.setBrush(m_Percent == 100 ? ColorFree : ColorUsed);
	p.setPen(black);

	p.drawEllipse(0, 0, width(), height()-PIEHEIGHT);
	
	if (m_Percent > 0 && m_Percent < 100)
	{
		p.setBrush(ColorFree);
		p.drawPie(0, 0, width(), height()-PIEHEIGHT, 180*16, alen);
	}
}

