/* Name: lefttree.h

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

#ifndef __INC_LEFTTREE_H__
#define __INC_LEFTTREE_H__

#include "common.h"
#include <listview.h>
#include "qdropsite.h"
#include <qdragobject.h>
#include "labeledit.h"
#ifndef QT_20
#include "drag.h"
#endif

class CLeftTreeView : public CListView, public QDropSite
{
	Q_OBJECT

public slots:

	void OnDoubleClicked(CListViewItem *pItem);
	void AutoExpand();
#ifndef QT_20
//	void OnKDNDDropEnter(KDNDDropZone *pZone);
//	void OnKDNDDropAction(KDNDDropZone *pZone);
//	void OnKDNDDropLeave(KDNDDropZone*);
#endif
	void ScrollDown();
	void ScrollUp();
  void OnFocusLost();

public:
	CLeftTreeView(QWidget *parent);

	BOOL InDrag()
	{
		return m_bInDrag;
	}

  CListViewItem *GetItemAt(const QPoint& p);

signals:
	void TabRequest(BOOL bIsBacktab);
	void GoParentRequest();
	void UndoRequest();
	void CutRequest();
	void CopyRequest();
	void PasteRequest();
	void DeleteRequest();
	void NukeRequest();
	void SelectAllRequest();
	void NavigateRequest(CListViewItem *);
  void ConsoleRequest();

protected:
	void focusInEvent(QFocusEvent *e);
	void focusOutEvent(QFocusEvent *e);

private:
	void contentsDragEnterEvent(QDragEnterEvent *);
	void contentsDragMoveEvent(QDragMoveEvent *);
	void contentsDragLeaveEvent(QDragLeaveEvent *);
	void contentsDropEvent(QDropEvent *);
#ifndef QT_20
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);
#else
	void viewportMouseMoveEvent(QMouseEvent *e);
	void viewportMousePressEvent(QMouseEvent *e);
	void viewportMouseReleaseEvent(QMouseEvent *e);
	void viewportMouseDoubleClickEvent(QMouseEvent *e);
#endif
	bool event(QEvent *e);
	void keyPressEvent(QKeyEvent *e);

private:
	bool eventFilter(QObject *pObject, QEvent *pEvent);
  BOOL m_bCanBeginDrag;
  QPoint m_StartDragPoint;
	BOOL m_bInDrag;
	CListViewItem *m_pTreeDropTargetItem; // to auto-expand drag-n-drop targets in left tree
	int m_nDropTargetInstanceHandleCount;

	/* Label editor */
	BOOL m_bMayBeEditLabel;
	void StartLabelEdit();
#ifndef QT_20
  KDNDDropZone m_DropZone;
#endif
  int m_ScrollMode;
	QStrList m_DraggedURLList;
  int m_ButtonState;
  QObject *m_pCurrentFocusTrack;
};


#endif /* __INC_LEFTTREE_H__ */
