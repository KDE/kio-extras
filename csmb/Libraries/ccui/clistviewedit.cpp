/* Name: clistviewedit.cpp

   Description: This file is a part of the ccui library.

   Author:	Philippe Bouchard

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


#include "clistviewedit.h"

#include <qheader.h>
#include <qlineedit.h>
#include <qapplication.h>

/**
    Construct an editable list view, using a specific predicate.

    @param  a_pcWidget                Parent widget
    @param  a_pzName                  Widget's name
    @param  a_pOwner                  Owner object that will be passed in
                                      parameter to the following functions
    @param  a_pEntryType              Function which returns a pointer to a
                                      widget that will be used to edit an entry
    @param  a_pValidator              Function that will determine if an entry
                                      has a valid entry or not
    @param  a_pPredicate              Predicate that will be used to determine
                                      if a cell is read-only or not
    @param  a_bExplicit               Will determine if edition must be called 
      	      	      	      	      explicitly
*/

CListViewEdit::CListViewEdit(QWidget * a_pcWidget, char const * a_pzName, QObject * a_pcOwner, QWidget * (* a_pEntryType)(QObject *, QListViewItem *, int), bool (* a_pValidator)(QObject *, QListViewItem *, int), bool (* a_pPredicate)(QObject *, QListViewItem *, int), bool a_bExplicit): QListView(a_pcWidget, a_pzName)
{
  this->pcOwner = a_pcOwner;
  this->bExplicit = a_bExplicit;
  this->pEntryType = a_pEntryType;
  this->pValidator = a_pValidator;
  this->pPredicate = a_pPredicate;

  this->pcListViewItem = 0;
  this->pcEventFilter = new CListViewEdit::EventFilter(this, "pcEventFilter");

  this->installEventFilter(this->pcEventFilter);
  this->connect(this, SIGNAL(clicked(QListViewItem *, const QPoint &, int)), SLOT(cellSelected(QListViewItem *, const QPoint &, int)));

  this->setAllColumnsShowFocus(true);
  this->setSelectionMode(QListView::Single);
}


/**
    Select a cell.

    Called when a cell is requested for selection by the
    keyboard or mouse.

    @param  a_pcListViewItem          Requested QListViewItem
    @param  a_iColumn                 Requested column
*/

void CListViewEdit::cellSelected(QListViewItem * a_pcListViewItem, const QPoint &, int a_iColumn)
{
  if (! this->bExplicit && a_pcListViewItem != 0 && this->pPredicate(this->pcOwner, a_pcListViewItem, a_iColumn))
  {
    class QPoint  cCellPos;
    class QSize   cCellSize;
    class QRect   cCellGeometry;

    if (this->pcListViewItem != 0)
    {
      if (! this->cellDeselected())
      {
        return;
      }
    }

    this->pcEntry = this->pEntryType(this->pcOwner, a_pcListViewItem, a_iColumn);

    this->iColumn = a_iColumn;
    this->pcListViewItem = a_pcListViewItem;

    this->setSelected(this->pcListViewItem, true);

    cCellPos = this->contentsRect().topLeft();
    cCellSize = QSize(this->columnWidth(this->iColumn), 0);
    cCellGeometry = this->itemRect(this->pcListViewItem);
    cCellSize.setHeight(cCellGeometry.height());

    for (int i = 0; i < this->iColumn; ++ i)
    {
      cCellPos += QPoint(this->columnWidth(i), 0);
    }
    cCellPos += cCellGeometry.topLeft();
    cCellPos += QPoint(0, this->header()->height());

    if (this->iColumn == 0)
    {
      int nOffset = this->pcListViewItem->depth() * this->treeStepSize();
      cCellPos += QPoint(nOffset, 0);
      cCellSize -= QSize(nOffset, 0);
    }

    (this->pcEntry->*dynamic_cast<CListViewEdit::EntryType *>(this->pcEntry)->pSetText)(this->pcListViewItem->text(this->iColumn));
    this->pcEntry->setGeometry(QRect(cCellPos, cCellSize));
    this->pcEntry->setFocus();
    this->pcEntry->show();
    this->pcEntry->grabMouse();
    this->pcEntry->grabKeyboard();
    this->pcEntry->installEventFilter(this->pcEventFilter);
  }
}


/**
    Deselect a cell.

    Called when a cell is deselected, or another one is selected.

    @param  a_bSave                   True if the text must be stored
    @return   	      	      	      True if there is no selected cell left
*/

bool CListViewEdit::cellDeselected(bool a_bSave)
{
  if (this->pcListViewItem != 0)
  {
    if (a_bSave)
    {
      if (! this->pValidator || this->pValidator(this->pcOwner, this->pcListViewItem, this->iColumn))
      {
        this->pcListViewItem->setText(this->iColumn, (this->pcEntry->*dynamic_cast<CListViewEdit::EntryType *>(this->pcEntry)->pText)());
      }
      else
      {
      	return false;
      }
    }

    this->pcEntry->removeEventFilter(this->pcEventFilter);
    this->pcEntry->releaseKeyboard();
    this->pcEntry->releaseMouse();
    this->pcEntry->hide();
    this->pcListViewItem = 0;
    this->setFocus();
  }
  
  return true;
}


/**
    Default predicate.

    Defines QListViewItems that have child to read-only.

    @param  a_pcListViewItem
    @return                           True if editable
*/

bool CListViewEdit::defaultPredicate(QObject *, QListViewItem * a_pcListViewItem, int)
{
  return a_pcListViewItem->childCount() == 0;
}


/**
    Event filter.

    Receive direct events as well as forwarded event from
    CListViewEdit::pcLineEdit member.

    @param  a_pcEvent                 Event caught
    @return                           True if processed
*/

bool CListViewEdit::EventFilter::eventFilter(QObject * a_pcObject, QEvent * a_pcEvent)
{
  int iColumn;
  class QListViewItem * pcListViewItem;

  // Entry widget not activated:
  if (dynamic_cast<CListViewEdit *>(a_pcObject))
  {
    class CListViewEdit * const pcParent = dynamic_cast<CListViewEdit *>(this->parent());
    
    pcListViewItem = pcParent->currentItem();
    
    if (class QKeyEvent * pcKeyEvent = dynamic_cast<QKeyEvent *>(a_pcEvent))
    {
      if (pcKeyEvent->type() == QEvent::KeyPress)
      {
        switch(pcKeyEvent->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
          for (iColumn = -1; true; )
          {
            if (iColumn < pcParent->columns() - 1)
            {
              ++ iColumn;
            }
            else
            {
              iColumn = 0;
              if (pcListViewItem = pcListViewItem->itemBelow(), ! pcListViewItem)
              {
                return true;
              }
            }

            if (pcParent->pPredicate(pcParent->pcOwner, pcListViewItem, iColumn))
            {
              pcParent->cellSelected(pcListViewItem, QPoint(0, 0), iColumn);
              return true;
            }
          }
          break;
        }
      }
    }
  }
  // Entry widget activated:
  else if (dynamic_cast<CListViewEdit::EntryType *>(a_pcObject))
  {
    class CListViewEdit * const pcParent = dynamic_cast<CListViewEdit *>(this->parent());
    class QWidget       * const pcCousin = pcParent->pcEntry;

    iColumn = pcParent->iColumn;
    pcListViewItem = pcParent->pcListViewItem;

    if (class QFocusEvent * pcFocusEvent = dynamic_cast<QFocusEvent *>(a_pcEvent))
    {
      if (pcFocusEvent->lostFocus())
      {
        pcParent->cellDeselected();
        return true;
      }
    }
    else if (class QMouseEvent * pcMouseEvent = dynamic_cast<QMouseEvent *>(a_pcEvent))
    {
      if (pcMouseEvent->type() == QEvent::MouseButtonPress)
      {
        if (! QRect(QPoint(0, 0), dynamic_cast<QWidget *>(pcCousin)->size()).contains(pcMouseEvent->pos()))
        {
          pcParent->cellDeselected();
          return true;
        }
      }
    }
    else if (class QKeyEvent * pcKeyEvent = dynamic_cast<QKeyEvent *>(a_pcEvent))
    {
      if (pcKeyEvent->type() == QEvent::KeyPress)
      {
        switch(pcKeyEvent->key())
        {
        case Qt::Key_Escape:
          pcParent->cellDeselected(false);
          return true;

        case Qt::Key_BackTab:
          while (true)
          {
            if (iColumn > 0)
            {
              -- iColumn;
            }
            else
            {
              iColumn = pcParent->columns() - 1;
              if (pcListViewItem = pcListViewItem->itemAbove(), ! pcListViewItem)
              {
                return true;
              }
            }

            if (pcParent->pPredicate(pcParent->pcOwner, pcListViewItem, iColumn))
            {
              pcParent->cellSelected(pcListViewItem, QPoint(0, 0), iColumn);
              return true;
            }
          }
          break;

        case Qt::Key_Up:
          while (true)
          {
            if (pcListViewItem = pcListViewItem->itemAbove(), ! pcListViewItem)
            {
              return true;
            }

            if (pcParent->pPredicate(pcParent->pcOwner, pcListViewItem, iColumn))
            {
              pcParent->cellSelected(pcListViewItem, QPoint(0, 0), iColumn);
              return true;
            }
          }
          break;

        case Qt::Key_Tab:
        case Qt::Key_Enter:
        case Qt::Key_Return:
          while (true)
          {
            if (iColumn < pcParent->columns() - 1)
            {
              ++ iColumn;
            }
            else
            {
              iColumn = 0;
              if (pcListViewItem = pcListViewItem->itemBelow(), ! pcListViewItem)
              {
                return true;
              }
            }

            if (pcParent->pPredicate(pcParent->pcOwner, pcListViewItem, iColumn))
            {
              pcParent->cellSelected(pcListViewItem, QPoint(0, 0), iColumn);
              return true;
            }
          }
          break;

        case Qt::Key_Down:
          while (true)
          {
            if (pcListViewItem = pcListViewItem->itemBelow(), ! pcListViewItem)
            {
              return true;
            }

            if (pcParent->pPredicate(pcParent->pcOwner, pcListViewItem, iColumn))
            {
              pcParent->cellSelected(pcListViewItem, QPoint(0, 0), iColumn);
              return true;
            }
          }
          break;
        }
      }
    }
  }

  return false;
}
