/* Name: wintree.cpp

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


/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#include "wintree.h"
#include "qregion.h"
#include <qapplication.h>
#include "qobject.h"

static CWindowsTreeItemBuddy WinTreeBuddy;

////////////////////////////////////////////////////////////////////////////

void CWindowsTreeItem::paintCell( QPainter * p, const QColorGroup & cg,
			       int column, int wd, int align)
{
	// Change width() if you change this.

	if (!p)
		return;

  if (listView()->iconView())
	{
		CListViewItem::paintCell(p, cg, column, wd, align);
		return;
	}

	CListView *lv = listView();
	int r = lv ? lv->itemMargin() : 1;
	
	const QPixmap * icon = pixmap(column);

	p->fillRect(0, 0, wd, height(), cg.base());
	
	QString t = text(column);

	int marg = lv ? lv->itemMargin() : 1;

	if (isSelected() && (column==0 || listView()->allColumnsShowFocus()))
	{
		int realwidth;
		if (t.isEmpty())
			realwidth = wd;
		else
		{
			if (listView()->allColumnsShowFocus())
				realwidth = wd;
			else
			{
				realwidth = width(listView()->fontMetrics(), listView(), 0);
				
				if (realwidth > wd)
					realwidth = wd;
			}
		}
		
		p->fillRect(r - marg, 0, realwidth - r + marg, height(), QApplication::winStyleHighlightColor());
	  p->setPen(white); // ### cg.highlightedText());
  }
	else
		p->setPen(cg.text());

	if (icon)
	{
		p->drawPixmap( r, (height()-icon->height())/2, *icon);
		r += icon->width() + listView()->itemMargin();
	}

	if (!t.isEmpty())
	{
		// should do the ellipsis thing in drawText()
		int maxwd = wd - marg - r;
		
		if (p->boundingRect(r,0,wd-marg-r,height(), Qt::AlignVCenter, t).width() > maxwd && t.length() > 1)
		{
			QString t2 = t.left(t.length()-1);

			while (p->boundingRect(r,0,wd-marg-r,height(),Qt::AlignVCenter, t2+"...").width() > maxwd && t2.length() > 1)
				t2 = t2.left(t2.length()-1);
			
			t = t2 + "...";
		}
	
		p->drawText(r, 0, wd-marg-r, height(), align | Qt::AlignVCenter, t);
	}
}

////////////////////////////////////////////////////////////////////////////

void CWindowsTreeItem::paintFocus(QPainter *p, const QColorGroup &cg,	const QRect & r)
{
  if (listView()->iconView())
	{
		CListViewItem::paintFocus(p, cg, r);
		return;
	}

	p->drawWinFocusRect(r.left(), 
											r.top(), 
											width(listView()->fontMetrics(), listView(), 0), 
											r.height());
}

////////////////////////////////////////////////////////////////////////////

void CWindowsTreeItem::StartLabelEdit()
{
	CListView *pLV = listView();
	CLabelEditor *pLE = new CLabelEditor(pLV->viewport(), this);
			
	QObject::connect(pLE, SIGNAL(RemoveRequest(CLabelEditor *)), &WinTreeBuddy, SLOT(OnRemoveLabelEditor(CLabelEditor *)));

	LPCSTR s = text(0);
	QRect r1(pLV->itemRect(this));
			
	int nOffset = 0;
  
  for (CListViewItem *i=parent(); NULL != i; i=i->parent())
    nOffset += pLV->treeStepSize();

	if (pLV->iconView())
	{
		int w = 6;
		
		w += width(pLV->fontMetrics(), pLV, 0);
		
		if (w > r1.width() - 2)
		{
			w = r1.width() - 2;
		}
		pLE->setGeometry(r1.x() + (r1.width() - w)/2 - pLV->contentsX(),
										 r1.y() + 32 + ICON_VIEW_MARGIN*2 - 1 - pLV->contentsY(),
										 w, pLV->fontMetrics().height() + 4);
	}
	else
  {
		int off = nOffset + pixmap(0)->width() + pLV->itemMargin()*2 + r1.left();
    int w;
     
    w = width(pLV->fontMetrics(), pLV, 0);
    
    //if (w < 80)
    //  w = 80;
    
    pLE->setGeometry(off - pLV->contentsX(), r1.top(), w, r1.height());
	}
			
	pLE->setText(s);
	pLE->setSelection(0, strlen(s));
	pLE->show();
	pLE->setFocus();
}

////////////////////////////////////////////////////////////////////////////

void CWindowsTreeItemBuddy::OnRemoveLabelEditor(CLabelEditor *pLE)
{
	disconnect(pLE, SIGNAL(RemoveRequest(CLabelEditor *)), this, SLOT(OnRemoveLabelEditor(CLabelEditor *)));
	
  QWidget *pParent = (QWidget*)(pLE->parent());
  
	delete pLE;
  
  if (NULL != pParent && NULL == qApp->focusWidget())
    pParent->setFocus();
}

////////////////////////////////////////////////////////////////////////////

BOOL CWindowsTreeItem::IsOKToEditLabel(int xoff)
{
	if (!CanEditLabel())
		return FALSE;

	CListView *pLV = listView();
	
  int nOffset = 0;
  
  for (CListViewItem *i=parent(); NULL != i; i=i->parent())
    nOffset += pLV->treeStepSize();

	int off = nOffset + 
						pixmap(0)->width() + 
						pLV->itemMargin()*2 + 
						pLV->itemRect(this).left() -
						pLV->contentsX();
			
	return (xoff >= off && xoff < off + width(pLV->fontMetrics(), pLV, 0));
}			

////////////////////////////////////////////////////////////////////////////
