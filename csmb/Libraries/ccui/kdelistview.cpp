/* This file is part of the KDE libraries
   Copyright (C) 2000 Reginald Stadlbauer <reggie@kde.org>

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

#include <qtimer.h>

#include <kglobalsettings.h>
#include <kcursor.h>
#include <kapp.h>
#include <kipc.h>
#include <kdebug.h>

#include "ccui_common.h"
#include "kdelistview.h"
#include "header.h"

#include <X11/Xlib.h>

CKDEListView::CKDEListView( QWidget *parent, const char *name,
        bool bUseHeaderExtender )
    : CListView( parent, name, bUseHeaderExtender )
{
    oldCursor = viewport()->cursor();
    connect( this, SIGNAL( onViewport() ),
	     this, SLOT( slotOnViewport() ) );
    connect( this, SIGNAL( onItem( CListViewItem * ) ),
	     this, SLOT( slotOnItem( CListViewItem * ) ) );

    slotSettingsChanged(KApplication::SETTINGS_MOUSE);
    connect( kapp, SIGNAL( settingsChanged(int) ), SLOT( slotSettingsChanged(int) ) );
    kapp->addKipcEventMask( KIPC::SettingsChanged );

    m_pCurrentItem = 0L;

    m_pAutoSelect = new QTimer( this );
    connect( m_pAutoSelect, SIGNAL( timeout() ),
    	     this, SLOT( slotAutoSelect() ) );
}

bool CKDEListView::isExecuteArea( const QPoint& point )
{
   if ( itemAt( point ) )
   {
     if( allColumnsShowFocus() || iconView() )
       return true;
     else {

      int x = point.x();
      int pos = header()->mapToIndex( 0 );
      int offset = 0;
      int width = columnWidth( pos );

      for ( int index = 0; index < pos; index++ )
         offset += columnWidth( index );

      return ( x > offset && x < ( offset + width ) );
     }
   }
   return false;
}

void CKDEListView::slotOnItem( CListViewItem *item )
{
  if ( item && (m_autoSelectDelay > -1) && m_bUseSingle ) {
    m_pAutoSelect->start( m_autoSelectDelay, true );
    m_pCurrentItem = item;
  }
}

void CKDEListView::slotOnViewport()
{
  if ( m_bChangeCursorOverItem )
    viewport()->setCursor( oldCursor );

  m_pAutoSelect->stop();
  m_pCurrentItem = 0L;
}

void CKDEListView::slotSettingsChanged(int category)
{
    if (category != KApplication::SETTINGS_MOUSE)
        return;
    m_bUseSingle = KGlobalSettings::singleClick();

    disconnect( this, SIGNAL( mouseButtonClicked( int, CListViewItem *,
						  const QPoint &, int ) ),
		this, SLOT( slotMouseButtonClicked( int, CListViewItem *,
						    const QPoint &, int ) ) );
//       disconnect( this, SIGNAL( doubleClicked( CListViewItem *,
// 					       const QPoint &, int ) ),
// 		  this, SLOT( slotExecute( CListViewItem *,
// 					   const QPoint &, int ) ) );

    if( m_bUseSingle )
    {
      connect( this, SIGNAL( mouseButtonClicked( int, CListViewItem *,
						 const QPoint &, int ) ),
	       this, SLOT( slotMouseButtonClicked( int, CListViewItem *,
						   const QPoint &, int ) ) );
    }
    else
    {
//       connect( this, SIGNAL( doubleClicked( CListViewItem *,
// 					    const QPoint &, int ) ),
// 	       this, SLOT( slotExecute( CListViewItem *,
// 					const QPoint &, int ) ) );
    }

    m_bChangeCursorOverItem = KGlobalSettings::changeCursorOverIcon();
    m_autoSelectDelay = KGlobalSettings::autoSelectDelay();

    if( !m_bUseSingle || !m_bChangeCursorOverItem )
	viewport()->setCursor( oldCursor );
}

void CKDEListView::slotAutoSelect()
{
  //Give this widget the keyboard focus.
  if( !hasFocus() )
    setFocus();

  Window root;
  Window child;
  int root_x, root_y, win_x, win_y;
  uint keybstate;
  XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
		 &root_x, &root_y, &win_x, &win_y, &keybstate );

  CListViewItem* previousItem = currentItem();
  setCurrentItem( m_pCurrentItem );

  if( m_pCurrentItem ) {
    //Shift pressed?
    if( (keybstate & ShiftMask) ) {
      bool block = signalsBlocked();
      blockSignals( true );

      //No Ctrl? Then clear before!
      if( !(keybstate & ControlMask) )
	clearSelection();

      bool select = !m_pCurrentItem->isSelected();
      bool update = viewport()->isUpdatesEnabled();
      viewport()->setUpdatesEnabled( false );

      if (iconView())
      {
          // adapted from CListView::doAutoScroll()
          CListViewItem *b = previousItem;
          CListViewItem *c = m_pCurrentItem;
          bool down = ( c->itemPos() > b->itemPos() );
          bool right = ( down || itemRect(c).x() > itemRect(b).x() );
          while( b && c && b != c )
          {
              if ( b->isSelectable() )
                  b->setSelected( select );

              // if on same row as target, move toward it
              if ( itemRect(b).y() == itemRect(c).y() )
                  b = right ? b->itemRight() : b->itemLeft();

              // otherwise select/deselect this whole row
              else
              {
                  CListViewItem * pNext = right ? b->itemRight() : b->itemLeft();
                  if ( pNext == 0 )
                  {
                      // if at end of row, go to next row...
                      b = down ? b->itemBelow() : b->itemAbove();
                      pNext = b;
                      // ...and rewind to opposite end
                      do
                      {
                          b = pNext;
                          pNext = right ? b->itemLeft() : b->itemRight();
                      } while ( pNext != 0 );
                  }
                  else
                      b = pNext;
              }
          }
      }
      else
      {
          bool down = previousItem->itemPos() < m_pCurrentItem->itemPos();
          CListViewItemIterator lit( down ? previousItem : m_pCurrentItem );
          for ( ; lit.current(); ++lit ) {
	    if ( down && lit.current() == m_pCurrentItem ) {
	      m_pCurrentItem->setSelected( select );
	      break;
	    }
	    if ( !down && lit.current() == previousItem ) {
	      previousItem->setSelected( select );
	      break;
	    }
	    lit.current()->setSelected( select );
          }
     }

      blockSignals( block );
      viewport()->setUpdatesEnabled( update );
      triggerUpdate();

      emit selectionChanged();

      if( selectionMode() == CListView::Single )
	emit selectionChanged( m_pCurrentItem );
    }
    else if( (keybstate & ControlMask) )
      setSelected( m_pCurrentItem, !m_pCurrentItem->isSelected() );
    else {
      bool block = signalsBlocked();
      blockSignals( true );

      if( !m_pCurrentItem->isSelected() )
	clearSelection();

      blockSignals( block );

      setSelected( m_pCurrentItem, true );
    }
  }
  else
    kdDebug() << "That´s not supposed to happen!!!!" << endl;
}

void CKDEListView::emitExecute( CListViewItem *item, const QPoint &pos, int c
) {
  if( isExecuteArea( viewport()->mapFromGlobal(pos) ) ) {

    Window root;
    Window child;
    int root_x, root_y, win_x, win_y;
    uint keybstate;
    XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
		   &root_x, &root_y, &win_x, &win_y, &keybstate );

    m_pAutoSelect->stop();

    //Don´t emit executed if in SC mode and Shift or Ctrl are pressed
    if( !( m_bUseSingle && ((keybstate & ShiftMask) || (keybstate & ControlMask)) ) ) {
      emit executed( item );
      emit executed( item, pos, c );
    }
  }
}

void CKDEListView::focusOutEvent( QFocusEvent *fe )
{
  m_pAutoSelect->stop();

  CListView::focusOutEvent( fe );
}

void CKDEListView::leaveEvent( QEvent *e )
{
  m_pAutoSelect->stop();

  CListView::leaveEvent( e );
}

void CKDEListView::contentsMousePressEvent( QMouseEvent *e )
{
  if( (selectionMode() == Extended) && (e->state() & ShiftButton) && !(e->state() & ControlButton) ) {
    bool block = signalsBlocked();
    blockSignals( true );

    clearSelection();

    blockSignals( block );
  }

  CListView::contentsMousePressEvent( e );
}

void CKDEListView::contentsMouseMoveEvent( QMouseEvent *e )
{
  QPoint vp = contentsToViewport(e->pos());
  CListViewItem *item = itemAt( vp );

  //do we process cursor changes at all?
  if ( item && m_bChangeCursorOverItem && m_bUseSingle ) {
    //Cursor moved on a new item or in/out the execute area
    if( (item != m_pCurrentItem) ||
	(isExecuteArea(vp) != m_cursorInExecuteArea) ) {

      m_cursorInExecuteArea = isExecuteArea(vp);

      if( m_cursorInExecuteArea ) //cursor moved in execute area
	viewport()->setCursor( KCursor().handCursor() );
      else //cursor moved out of execute area
	viewport()->setCursor( oldCursor );
    }
  }

  CListView::contentsMouseMoveEvent( e );
}

void CKDEListView::contentsMouseDoubleClickEvent ( QMouseEvent *e )
{
  CListView::contentsMouseDoubleClickEvent( e );

  QPoint vp = contentsToViewport(e->pos());
  CListViewItem *item = itemAt( vp );
  int col = item ? header()->mapToSection( header()->sectionAt( vp.x() ) ) :
	-1;
  if( item ) {
    emit doubleClicked( item, e->globalPos(), col );

    if( (e->button() == LeftButton) && !m_bUseSingle )
      emitExecute( item, e->globalPos(), col );
  }
}

void CKDEListView::slotMouseButtonClicked( int btn, CListViewItem *item, const
QPoint &pos, int c ) {
  if( (btn == LeftButton) && item )
    emitExecute( item, pos, c );
}


