/* Name: lefttree.cpp

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

////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "treeitem.h"
#include "lefttree.h"
#include <header.h>
#include "copymove.h"
#include "qapplication.h"
#include "qpainter.h"
#include "mainfrm.h"
#include "smbfile.h"
#include "ftpfile.h"
#include "localfile.h"
#include "expcommon.h"
#include "dropselector.h"
#include "qmetaobject.h"
#ifdef QT_20
#include <q1xcompatibility.h>
#endif

////////////////////////////////////////////////////////////////////////////

QWidget *GetMenuBar()
{
	return (QWidget*)FindChildByName(qApp->mainWidget(), "MenuBar");
}

////////////////////////////////////////////////////////////////////////////

CLeftTreeView::CLeftTreeView(QWidget *parent)
							: CListView(parent, "", false),
								QDropSite(viewport())
#ifndef QT_20
  ,m_DropZone(viewport(), DndURL)
#endif
{
	m_bCanBeginDrag = FALSE;
	m_bInDrag = FALSE;
	m_bMayBeEditLabel = FALSE;
	m_nDropTargetInstanceHandleCount = 0;

	setAcceptDrops(false);
	connect(this, SIGNAL(doubleClicked(CListViewItem*)), this, SLOT(OnDoubleClicked(CListViewItem*)));
#ifndef QT_20
	connect(&m_DropZone, SIGNAL(dropEnter(KDNDDropZone*)), this, SLOT(OnKDNDDropEnter(KDNDDropZone*)));
	connect(&m_DropZone, SIGNAL(dropAction(KDNDDropZone*)), this, SLOT(OnKDNDDropAction(KDNDDropZone*)));
	connect(&m_DropZone, SIGNAL(dropLeave(KDNDDropZone*)), this, SLOT(OnKDNDDropLeave(KDNDDropZone*)));
#endif
  //m_pCurrentSelected = NULL;

	const QColorGroup &CG = colorGroup();


	QColor NewBase((CG.base().red() + CG.midlight().red()) / 2,
								 (CG.base().green() + CG.midlight().green()) / 2,
								 (CG.base().blue() + CG.midlight().blue()) / 2);

	QColorGroup cg(CG.foreground(), CG.background(), CG.light(), CG.dark(), CG.midlight(), CG.text(), NewBase);
	QPalette pal(cg,cg,cg);

	//viewport()->setPalette(pal);
	setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::focusInEvent(QFocusEvent * /*e*/)
{
  m_bMayBeEditLabel = 2;
	CListViewItem *pItem = currentItem();

  if (NULL != pItem)
    setSelected(pItem, TRUE);
}

////////////////////////////////////////////////////////////////////////////

bool CLeftTreeView::event(QEvent *e)
{
  if (focusPolicy() != QWidget::StrongFocus)
		setFocusPolicy(QWidget::StrongFocus);

  return CListView::event(e);
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::focusOutEvent(QFocusEvent * /*e*/)
{
	setFocusPolicy(QWidget::StrongFocus);
  QTimer::singleShot(0, this, SLOT(OnFocusLost()));
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::OnDoubleClicked(CListViewItem * /*pItem*/)
{
	//pItem->setOpen(!pItem->isOpen()); This is commented because in Qt 1.42 this is not required anymore!
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::contentsDragMoveEvent(QDragMoveEvent *e)
{
	m_ButtonState = GetMouseState(this);

  CListViewItem *pItem = itemAt(e->pos());
	

	if (pItem != NULL)
	{
		CListViewItem *pI = currentItem();

		//disconnect(this, SIGNAL(currentChanged(CListViewItem*)), qApp->mainWidget(), SLOT(OnTreeSelectionChanged(CListViewItem*)));

		setSelected(pItem, TRUE);
		
		setMultiSelection(true);
		setCurrentItem(pI);
		setMultiSelection(false);
		
    if (pItem != m_pTreeDropTargetItem)
		{
			if (NULL != m_pTreeDropTargetItem &&
					m_pTreeDropTargetItem->isSelected())
			{
				setSelected(m_pTreeDropTargetItem, FALSE);
				setMultiSelection(true);
				setCurrentItem(pI);
				setMultiSelection(false);
			}
			
			m_pTreeDropTargetItem = pItem;
			m_nDropTargetInstanceHandleCount++;

      QTimer::singleShot(1000, this, SLOT(AutoExpand()));
		}

		//connect(this, SIGNAL(currentChanged(CListViewItem*)), qApp->mainWidget(), SLOT(OnTreeSelectionChanged(CListViewItem*)));

		if (IS_NETWORKTREEITEM(pItem) &&
        ((CNetworkTreeItem*)pItem)->ItemAcceptsDrops() &&
        ItemAcceptsThisDrop((CNetworkTreeItem*)pItem, m_DraggedURLList, FALSE))
			e->accept();
		else
			e->ignore();
	}

	if (NULL == pItem)
		pItem = currentItem();

	if (NULL == pItem)
		pItem = firstChild();

	if (NULL != pItem)
	{
		QRect r(itemRect(pItem));

		if (e->pos().y() < r.height()*2)
		{
			if (m_ScrollMode != 1)
			{
				QTimer::singleShot(300, this, SLOT(ScrollDown()));
				m_ScrollMode = 1;
			}
		}
		else
			if (e->pos().y() > viewport()->height()-r.height()*2)
			{
				if (m_ScrollMode != 2)
				{
					QTimer::singleShot(300, this, SLOT(ScrollUp()));
					m_ScrollMode = 2;
				}
			}
			else
				m_ScrollMode = 0;
	}
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::contentsDragEnterEvent(QDragEnterEvent *e)
{
	m_bInDrag = TRUE;
	m_ButtonState = GetMouseState(this);

  if (DecodeURLList(e, m_DraggedURLList))
		e->accept();

//	if (QUrlDrag::canDecode(e))
		//e->accept();
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::contentsDragLeaveEvent(QDragLeaveEvent *)
{
	CListViewItem *pI = currentItem();
	
	m_bInDrag = FALSE;
	m_DraggedURLList.clear();
	m_ScrollMode = 0;

	if (NULL != m_pTreeDropTargetItem &&
			m_pTreeDropTargetItem->isSelected())
	{
		setSelected(m_pTreeDropTargetItem, FALSE);
	}
	
	if (NULL != pI && !pI->isSelected())
	{
		setSelected(pI, TRUE);

		if (this != qApp->focusWidget())
			setSelected(pI, FALSE);
	}
	else
		setSelected(pI, FALSE);
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::contentsDropEvent(QDropEvent *e)
{
	m_ScrollMode = 0;
	CListViewItem *pItem = itemAt(e->pos());

	if (pItem != NULL && IS_NETWORKTREEITEM(pItem) && ((CNetworkTreeItem*)pItem)->ItemAcceptsDrops())
	{
		CNetworkTreeItem *pI = (CNetworkTreeItem *)pItem;
		QStrList list;

		if (QUrlDrag::decode(e, list))
		{
			QString URL;
			QString Path(pI->FullName(FALSE));

      CDropSelector Selector;

      if (IsTrashFolder((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
      ))
      {
        Selector.Go(this, QCursor::pos(), TRUE);

        if (Selector.type() == keDropMove)
          StartDelete(list, TRUE);
      }
      else
      {
        BOOL bMove = FALSE;

      	if (m_ButtonState & RightButton)
        {
          BOOL bCanMove = ItemAcceptsThisDrop(pI, list, TRUE);
					Selector.Go(this, QCursor::pos(), FALSE, !bCanMove);

          if (Selector.type() == keDropMove)
            bMove = TRUE;
          else
            if (Selector.type() != keDropCopy)
              return; // Aborted
        }

  			URL = MakeItemURL((CNetworkTreeItem *)pItem);
        StartCopyMove(list, (LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
        , bMove, FALSE);
      }
		}
	}

	CListViewItem *pI = currentItem();

	if (NULL != pI && !pI->isSelected())
	{
		setSelected(pI, TRUE);

		if (this != qApp->focusWidget())
			setSelected(pI, FALSE);
	}
}

////////////////////////////////////////////////////////////////////////////

CListViewItem *CLeftTreeView::GetItemAt(const QPoint& p)
{
	QPoint pos = viewport()->mapFromGlobal(p);

	CListViewItem *pItem = itemAt(pos);

	if (NULL == pItem || iconView())
	{
		return pItem;
	}

	int x = pos.x();

	if (x < header()->cellPos(header()->mapToActual(0)) ||
			x > header()->cellPos(header()->mapToActual(0)) + columnWidth(0))
		return NULL;

	QPainter painter;
	painter.begin(this);
	int w = painter.boundingRect(0, 0, columnWidth(0), 500, AlignLeft, pItem->text(0)).width();
	painter.end();

  int nOffset = 0;

  for (CListViewItem *i = pItem->parent(); NULL != i; i=i->parent())
    nOffset += treeStepSize();

	if (nOffset-treeStepSize() > x || x > nOffset + header()->cellPos(header()->mapToActual(0)) + w + itemMargin()*2 + pItem->pixmap(0)->width())
		return NULL;

	return pItem;
}


#ifndef QT_20
void CLeftTreeView::mousePressEvent(QMouseEvent *e)
#else
void CLeftTreeView::viewportMousePressEvent(QMouseEvent *e)
#endif
{
	viewport()->setFocus();

	CListViewItem *pOldCurrentItem = currentItem();

  //if (NULL == GetItemAt(viewport()->mapToGlobal(e->pos())))
  //{
    //setSelected(currentItem(), FALSE); don't want this in tree, keep this in right panel only!
//    return;
//  }

	if (e->button() == LeftButton)
	{
		if (m_bMayBeEditLabel == 2)
		{
			m_bMayBeEditLabel = FALSE;
		}
		else
		{
			CListViewItem *pNewCurrentItem = itemAt(e->pos());
			pOldCurrentItem = currentItem();

			if (pNewCurrentItem == pOldCurrentItem && pNewCurrentItem->isSelected())
			{
				if (((CWindowsTreeItem*)pNewCurrentItem)->IsOKToEditLabel(e->x()))
				{
					m_bMayBeEditLabel = TRUE;
				}
			}
		}
	}

	CListViewItem *pOldItem = currentItem();

#ifndef QT_20
	CListView::mousePressEvent(e);
#else
	CListView::viewportMousePressEvent(e);
#endif

	if (NULL != pOldItem &&
			e->button() == RightButton)	// necessary because viewportMousePressEvent changes current item to the one clicked
		setCurrentItem(pOldItem);
	
	m_bCanBeginDrag = TRUE;
  m_StartDragPoint = e->pos();
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CLeftTreeView::mouseReleaseEvent(QMouseEvent *e)
#else
void CLeftTreeView::viewportMouseReleaseEvent(QMouseEvent *e)
#endif
{
  if (e->button() != LeftButton)
  {
#ifndef QT_20
    CListView::mouseReleaseEvent(e);
#else
    CListView::viewportMouseReleaseEvent(e);
#endif
  }
	
	m_bCanBeginDrag = FALSE;

	if (m_bMayBeEditLabel == TRUE)
		StartLabelEdit();

	CListViewItem *pNewCurrentItem = itemAt(e->pos());

	if (NULL != pNewCurrentItem &&
			isSelected(pNewCurrentItem) /*pCurrentItem == pNewCurrentItem*/)
	{
    if (currentItem() != pNewCurrentItem)
		{
			setCurrentItem(pNewCurrentItem);
		}

		emit NavigateRequest(pNewCurrentItem);
	}

	m_bMayBeEditLabel = FALSE;
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CLeftTreeView::mouseDoubleClickEvent(QMouseEvent *e)
#else
void CLeftTreeView::viewportMouseDoubleClickEvent(QMouseEvent *e)
#endif
{
	m_bMayBeEditLabel = FALSE;

  if (e->button() == LeftButton && NULL != GetItemAt(viewport()->mapToGlobal(e->pos())))
#ifndef QT_20
	  CListView::mouseDoubleClickEvent(e);
#else	  
	  CListView::viewportMouseDoubleClickEvent(e);
#endif
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CLeftTreeView::mouseMoveEvent(QMouseEvent *e)
#else
void CLeftTreeView::viewportMouseMoveEvent(QMouseEvent *e)
#endif
{
  CListViewItem *pItem;
	CListViewItem *x = currentItem();

#ifndef QT_20
	CListView::mouseMoveEvent(e);
#else
	CListView::viewportMouseMoveEvent(e);
#endif

	pItem = currentItem();

	if (pItem != x)
		setCurrentItem(x);

	m_bMayBeEditLabel = FALSE;

	if (m_bCanBeginDrag)
	{
		QPoint dist = e->pos() - m_StartDragPoint;

    if (dist.x() * dist.x() + dist.y() * dist.y() > 16 &&
        ItemIsDraggable(pItem))
		{
			QRect r = itemRect(pItem);

			if (r.contains(e->pos()))
			{
				m_bCanBeginDrag = FALSE;

				QString URL;

				MakeURL(((CNetworkTreeItem *)pItem)->FullName(FALSE), NULL, URL);

				if (URL.right(1) != "/" && IS_NETWORKTREEITEM(pItem))
				{
					CNetworkTreeItem *pI = (CNetworkTreeItem *)pItem;

					if ((pI->Kind() == keFileItem && ((CFileItem*)pI)->IsFolder()) ||
							(pI->Kind() == keLocalFileItem && ((CLocalFileItem*)pI)->IsFolder()) ||
							(pI->Kind() == keFTPFileItem && ((CFTPFileItem*)pI)->IsFolder()) ||
							(pI->Kind() == keShareItem))
					{
						URL += "/";
					}
				}

				QStrList list;
				list.append(URL);

				QUrlDrag *d = new QUrlDrag(list, viewport());

				CListViewItem *x = pItem;
				int offx = 0;

				while (x->parent() != NULL)
				{
					offx += treeStepSize();
					x = x->parent();
				}

				d->setPixmap(CreateDragPixmap(pItem), QPoint(e->pos().x() - r.left() - offx, e->pos().y() - r.top()));
			  d->dragCopy();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::keyPressEvent(QKeyEvent *e)
{
	if (this != qApp->focusWidget() &&
			viewport() != qApp->focusWidget())
		return;

  CListViewItem *pOldCurrent = currentItem();

	switch (e->key())
	{
		case Qt::Key_Home:
			ensureItemVisible(firstChild());

			if (ControlButton != (e->state() & ControlButton))
				setSelected(firstChild(), TRUE);
		break;

		case Qt::Key_A:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+A invokes "Select All"
			{
				emit SelectAllRequest();
				return;
			}
		break;

		case Qt::Key_End:
		{
			CListViewItem *pItem = firstChild();
			CListViewItem *below;

			while ((below = pItem->itemBelow()) != NULL)
				pItem = below;

			ensureItemVisible(pItem);

			if (ControlButton != (e->state() & ControlButton))
				setSelected(pItem, TRUE);
		}
		break;

		case Qt::Key_X:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+X invokes "Cut"
			{
				emit CutRequest();
				return;
			}
		break;

		case Qt::Key_C:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+C invokes "Copy"
			{
				emit CopyRequest();
				return;
			}
		break;

    case Qt::Key_T:
      if (ControlButton == (e->state() & ControlButton)) // Ctrl+C invokes "Copy"
      {
        emit ConsoleRequest();
        return;
      }
    break;

    case Qt::Key_V:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+V invokes "Paste"
			{
				emit PasteRequest();
				return;
			}
		break;

		case Qt::Key_Z:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+Z invokes "Undo"
			{
				emit UndoRequest();
				return;
			}
		break;

		case Qt::Key_Delete:
			if (AltButton != (e->state() & AltButton) && ControlButton != (e->state() & ControlButton))
			{
				if (ShiftButton == (e->state() & ShiftButton))
				{
					emit NukeRequest();
				}
				else
				{
					emit DeleteRequest();
				}

				return;
			}
		break;

		case Qt::Key_Backspace:
			emit GoParentRequest();
		break;

		case Qt::Key_F2:
		{
			if (ControlButton != (e->state() & ControlButton) &&
					ShiftButton != (e->state() & ShiftButton) &&
					AltButton != (e->state() & AltButton) &&
					NULL != currentItem() &&
					((CWindowsTreeItem*)currentItem())->CanEditLabel())
			{
				StartLabelEdit();
			}
		}
		break;

		case Qt::Key_F6:
			if (AltButton != (e->state() & AltButton))
				emit TabRequest(ShiftButton == (e->state() & ShiftButton));
		break;
	}

	CListView::keyPressEvent(e);

	if (pOldCurrent != currentItem())
	{
		emit NavigateRequest(currentItem());
	}
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::StartLabelEdit()
{
	CListViewItem *pItem = currentItem();

	if (NULL != pItem && isSelected(pItem))
	{
		if (IS_NETWORKTREEITEM(pItem))
		{
			((CWindowsTreeItem *)pItem)->StartLabelEdit();
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::AutoExpand()
{
	m_nDropTargetInstanceHandleCount--;

	if (InDrag() &&
			!m_nDropTargetInstanceHandleCount &&
			NULL != m_pTreeDropTargetItem &&
			m_pTreeDropTargetItem->isSelected() &&
			!m_pTreeDropTargetItem->isOpen())
	{
		m_pTreeDropTargetItem->setOpen(TRUE);
	}
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CLeftTreeView::OnKDNDDropEnter(KDNDDropZone *pZone)
{
	m_ButtonState = GetMouseState(this);
	m_bInDrag = TRUE;
	QPoint pos(viewport()->mapFromGlobal(QPoint(pZone->getMouseX(), pZone->getMouseY())));

	CListViewItem *pItem = itemAt(pos);

	QRect r = itemRect(pItem);

 	if (pos.y() < r.height()*2)
	{
		if (m_ScrollMode != 1)
		{
			QTimer::singleShot(300, this, SLOT(ScrollDown()));
			m_ScrollMode = 1;
		}
	}
	else
		if (pos.y() > viewport()->height()-r.height()*2)
		{
			if (m_ScrollMode != 2)
			{
				QTimer::singleShot(300, this, SLOT(ScrollUp()));
				m_ScrollMode = 2;
			}
		}
		else
			m_ScrollMode = 0;

	if (pItem != NULL)
	{
		CListViewItem *pI = currentItem();

		//disconnect(this, SIGNAL(currentChanged(CListViewItem*)), qApp->mainWidget(), SLOT(OnTreeSelectionChanged(CListViewItem*)));

		setSelected(pItem, TRUE);
		setCurrentItem(pI);

    if (pItem != m_pTreeDropTargetItem)
		{
			m_pTreeDropTargetItem = pItem;
			m_nDropTargetInstanceHandleCount++;
			QTimer::singleShot(1000, this, SLOT(AutoExpand()));
		}

		//connect(this, SIGNAL(currentChanged(CListViewItem*)), qApp->mainWidget(), SLOT(OnTreeSelectionChanged(CListViewItem*)));

		///if (IS_NETWORKTREEITEM(pItem) && ((CNetworkTreeItem*)pItem)->ItemAcceptsDrops())
			//e->accept();
		//else
//			e->ignore();
	}
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::OnKDNDDropAction(KDNDDropZone *pZone)
{
	m_bInDrag = FALSE;
	QStrList &list = pZone->getURLList();
	CListViewItem *pItem = m_pTreeDropTargetItem;

	if (pItem != NULL && IS_NETWORKTREEITEM(pItem) && ((CNetworkTreeItem*)pItem)->ItemAcceptsDrops())
	{
		CNetworkTreeItem *pI = (CNetworkTreeItem *)pItem;

		QString Path(pI->FullName(FALSE));
		CDropSelector Selector;

		if (IsTrashFolder((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif

    ))
		{
			Selector.Go(this, QCursor::pos(), TRUE);

			if (Selector.type() == keDropMove)
				StartDelete(list, TRUE);
		}
		else
		{
			BOOL bMove = FALSE;

			if (m_ButtonState & RightButton)
			{
				BOOL bCanMove = ItemAcceptsThisDrop(pI, list, TRUE);

				Selector.Go(this, QCursor::pos(), FALSE, !bCanMove);

				if (Selector.type() == keDropMove)
					bMove = TRUE;
				else
					if (Selector.type() != keDropCopy)
						return; // Aborted
			}

			QString URL = MakeItemURL((CNetworkTreeItem *)pItem);
			StartCopyMove(list, (LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
      , bMove, FALSE);
		}
	}

	CListViewItem *pI = currentItem();

	if (NULL != pI && !pI->isSelected())
	{
		setSelected(pI, TRUE);

		if (this != qApp->focusWidget())
			setSelected(pI, FALSE);
	}
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::OnKDNDDropLeave(KDNDDropZone*)
{
	m_bInDrag = FALSE;
	dragLeaveEvent(NULL);
}
#endif

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::ScrollDown()
{
	if (1 == m_ScrollMode)
	{
		scrollBy(0,-18);
		QDragMoveEvent x(viewport()->mapFromGlobal(QCursor::pos()));

		dragMoveEvent(&x);

		QTimer::singleShot(300, this, SLOT(ScrollDown()));
	}
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::ScrollUp()
{
	if (2 == m_ScrollMode)
	{
		scrollBy(0,18);

		QDragMoveEvent x(viewport()->mapFromGlobal(QCursor::pos()));

		dragMoveEvent(&x);
		QTimer::singleShot(300, this, SLOT(ScrollUp()));
	}
}

////////////////////////////////////////////////////////////////////////////

bool CLeftTreeView::eventFilter(QObject *pObject, QEvent *pEvent)
{
  if (m_pCurrentFocusTrack == pObject && pEvent->type() == Event_FocusOut)
  {
    m_pCurrentFocusTrack->removeEventFilter(this);
    QTimer::singleShot(0, this, SLOT(OnFocusLost()));
  }

  return CListView::eventFilter(pObject, pEvent);
}

////////////////////////////////////////////////////////////////////////////

void CLeftTreeView::OnFocusLost()
{
  QWidget *pFocus = qApp->focusWidget();

  if (NULL != pFocus &&
      (pFocus->inherits("QPopupMenu") || pFocus->inherits("QMenuBar")))
  {
    m_pCurrentFocusTrack = pFocus;
    m_pCurrentFocusTrack->installEventFilter(this);
  }
  else
  {
    CListViewItem *pItem = currentItem();

    if (NULL != pItem)
      setSelected(pItem, FALSE);
  }
}

////////////////////////////////////////////////////////////////////////////

