/****************************************************************************
** $Id$
**
** Implementation of CListView widget class
**
** Created : 970809
**
** Copyright (C) 1992-2000 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing, or see
** http://www.trolltech.com/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "header.h"
#include "listview.h"

#include <qtimer.h>
#include <qpainter.h>
#include <qstack.h>
#include <qlist.h>
#include <qstrlist.h>
#include <qapplication.h>
#include <qbitmap.h>
#include <qdatetime.h>
#include <qptrdict.h>
#include <qvector.h>
#include <qiconset.h>
#include <qframe.h>
#include <qimage.h>
#include <qpixmapcache.h>
#include <qpushbutton.h>

#include <stdlib.h> // qsort
#include <ctype.h> // tolower
#include <iostream>

const int Unsorted = 16383;

static QBitmap * verticalLine = 0;
static QBitmap * horizontalLine = 0;

static void cleanupBitmapLines()
{
    delete verticalLine;
    delete horizontalLine;
    verticalLine = 0;
    horizontalLine = 0;
}

void tint(QPixmap &pix, QColor tintColour, float fIntensity)
{
	if (fIntensity < 0)
		fIntensity = 0;
	if (fIntensity > 1)
		fIntensity = 1;
	
	QImage image;
	image = pix;
	unsigned int r_bgnd = tintColour.red();
	unsigned int g_bgnd = tintColour.green();
	unsigned int b_bgnd = tintColour.blue();
	unsigned int r, g, b, a;
	int ind;

	register int x, y;

	unsigned int *data = (unsigned int *)image.bits();

	for (y = 0; y < image.height(); y++)
	{
		for (x = 0; x < image.width(); x++)
		{
			ind = x + image.width() * y;
			a = (data[ind] >> 24) & 0xff;
			if (a)
			{
				r = (int)((qRed(data[ind]) * (1 - fIntensity)) + (fIntensity * r_bgnd));
				g = (int)((qGreen(data[ind]) * (1 - fIntensity)) + (fIntensity * g_bgnd));
				b = (int)((qBlue(data[ind]) * (1 - fIntensity)) + (fIntensity * b_bgnd));
				data[ind] = qRgb(r, g, b) | ((a & 0xff) << 24);
			}
		}
	}
	pix = image;
}


class QResizeEvent;

class CStationaryHeader : public QPushButton
{
public:
	CStationaryHeader(QString szLabel = QString::null,
										QWidget *pParent = 0,
										const char *szName = 0);
	void addCloseButton(const QPixmap &);
	void removeCloseButton();
	QPushButton *button() {return m_pCloseButton;}
	~CStationaryHeader();

protected:
    void drawButton(QPainter *);
	void drawButtonLabel(QPainter *);
	void resizeEvent(QResizeEvent *);

private:
	QPushButton *m_pCloseButton;
	QFrame *m_pButtonFrame;
};

CStationaryHeader::CStationaryHeader( QString szLabel,
																		 QWidget *pParent,
																		 const char *szName)
									:	QPushButton(szLabel, pParent, szName)
{
	m_pCloseButton = 0;
	m_pButtonFrame = 0;
#ifndef QT_20 
	setEnabled(false);
#else
	setEnabled(true); // need this in order to enable "close" button
#endif
	show();
}

void CStationaryHeader::addCloseButton(const QPixmap &pixmap)
{
	if (!m_pCloseButton)
	{
		m_pButtonFrame = new QFrame(this);
		m_pButtonFrame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		m_pCloseButton = new QPushButton(m_pButtonFrame);
	}
	m_pCloseButton->setPixmap(pixmap);
}

void CStationaryHeader::removeCloseButton()
{
	if (m_pCloseButton)
	{
		delete m_pCloseButton;
		m_pCloseButton = 0;
		delete m_pButtonFrame;
		m_pButtonFrame = 0;
	}
}

CStationaryHeader::~CStationaryHeader()
{
	if (m_pCloseButton)
	{
		delete m_pCloseButton;
		m_pCloseButton = 0;
		delete m_pButtonFrame;
		m_pButtonFrame = 0;
	}
}

void CStationaryHeader::resizeEvent(QResizeEvent *)
{
	static int nBorder = 2;
	if (m_pCloseButton)
	{
		int nSize = height() - 2*nBorder;
		m_pButtonFrame->setGeometry(width() - nSize - nBorder - 2, nBorder,
																nSize, nSize);
		m_pCloseButton->setGeometry(1, 1, m_pButtonFrame->width() - 2,
																m_pButtonFrame->height() - 2);
	}
}

void CStationaryHeader::drawButton(QPainter *pPainter)
{
    style().drawBevelButton( pPainter, 0, 0, width(), height(), colorGroup() );
    drawButtonLabel( pPainter );
}

void CStationaryHeader::drawButtonLabel(QPainter *pPainter)
{
	QPalette colourPalette;
	QRect r = rect();
	int x, y, w, h;

	r.rect( &x, &y, &w, &h );
	x += 2;  y += 2;  w -= 4;  h -= 4;
	qDrawItem( pPainter, style(), x, y, w, h,
						 AlignLeft,
						 colourPalette.active(), true,
						 pixmap(), text() );
}

struct CListViewPrivate
{
    // classes that are here to avoid polluting the global name space

    // the magical hidden mother of all items
    class Root: public CListViewItem {
    public:
	Root( CListView * parent );

	void setHeight( int );
	void invalidateHeight();
	void setup();
	CListView * theListView() const;

	CListView * lv;
    };

    // for the stack used in drawContentsOffset()
    class Pending {
    public:
	Pending( int level, int ypos, CListViewItem * item, int xpos = 0 )
	    : l(level), y(ypos), i(item), x(xpos) {};

	int l; // level of this item; root is -1 or 0
	int y; // level of this item in the tree
	CListViewItem * i; // the item itself
	int x; // added to support icon view
    };

    // to remember what's on screen
    class DrawableItem {
    public:
	DrawableItem( Pending * pi ) { y=pi->y; l=pi->l; i=pi->i; x=pi->x; };
	int x;  // added to support icon view
	int y;
	int l;
	CListViewItem * i;
    };

    // for sorting
    class SortableItem {
    public:
	QString key;
	CListViewItem * i;
    };

    class ItemColumnInfo {
    public:
	ItemColumnInfo(): pm( 0 ), next( 0 ), truncated( FALSE ), width( 0 ) {}
	~ItemColumnInfo() { delete pm; delete next; }
	QString text, tmpText;
	QPixmap * pm;
	ItemColumnInfo * next;
	bool truncated;
	int width;
    };

    class ViewColumnInfo {
    public:
	ViewColumnInfo(): align(Qt::AlignLeft), sortable(TRUE), next( 0 ) {}
	~ViewColumnInfo() { delete next; }
	int align;
	bool sortable;
	ViewColumnInfo * next;
    };

    // private variables used in CListView
    ViewColumnInfo * vci;
    CHeader * h;
    Root * r;
    uint rootIsExpandable : 1;
    int margin;

    CListViewItem * focusItem, *highlighted;

    QTimer * timer;
    QTimer * dirtyItemTimer;
    QTimer * visibleTimer;
    int levelWidth;

    // the list of drawables, and the range drawables covers entirely
    // (it may also include a few items above topPixel)
    QList<DrawableItem> * drawables;
    int topPixel;
    int bottomPixel;

    QPtrDict<void> * dirtyItems;

    CListView::SelectionMode selectionMode;

    // TRUE if the widget should take notice of mouseReleaseEvent
    bool buttonDown;
    // TRUE if the widget should ignore a double-click
    bool ignoreDoubleClick;

    // Per-column structure for information not in the CHeader
    struct Column {
	CListView::WidthMode wmode;
    };
    QVector<Column> column;

    // sort column and order   #### may need to move to CHeader [subclass]
    int sortcolumn;
    bool ascending;
    bool sortIndicator;

    // suggested height for the items
    int fontMetricsHeight;
    int minLeftBearing, minRightBearing;
    int ellipsisWidth;
    bool allColumnsShowFocus;

    // currently typed prefix for the keyboard interface, and the time
    // of the last key-press
    QString currentPrefix;
    QTime currentPrefixTime;

    // whether to select or deselect during this mouse press.
    bool select;

    // holds a list of iterators
    QList<CListViewItemIterator> *iterators;
    CListViewItem *pressedItem, *selectAnchor;

    QTimer *scrollTimer;

    bool clearing;
    bool makeCurrentVisibleOnUpdate;
    bool pressedSelected;

};

// these should probably be in CListViewPrivate, for future thread safety
static bool activatedByClick;
static QPoint activatedP;


// NOT REVISED
/*!
  \class CListViewItem qlistview.h
  \brief The CListViewItem class implements a list view item.

  A list viev item is a multi-column object capable of displaying
  itself.  Its design has the following main goals: <ul> <li> Work
  quickly and well for \e large sets of data. <li> Be easy to use in
  the simple case. </ul>

  The simplest way to use CListViewItem is to construct one with a few
  constant strings.  This creates an item which is a child of \e
  parent, with two fixed-content strings, and discards the pointer to
  it:

  \code
     (void) new CListViewItem( parent, "first column", "second column" );
  \endcode

  This object will be deleted when \e parent is deleted, as for \link
  QObject QObjects. \endlink

  The parent is either another CListViewItem or a CListView.  If the
  parent is a CListView, this item is a top-level item within that
  CListView.  If the parent is another CListViewItem, this item
  becomes a child of the parent item.

  If you keep the pointer, you can set or change the texts using
  setText(), add pixmaps using setPixmap(), change its mode using
  setSelectable(), setSelected(), setOpen() and setExpandable(),
  change its height using setHeight(), and do much tree traversal.
  The set* functions in CListView also affect CListViewItem, of
  course.

  You can traverse the tree as if it were a doubly linked list using
  itemAbove() and itemBelow(); they return pointers to the items
  directly above and below this item on the screen (even if none of
  the three are actually visible at the moment).

  You can also traverse it as a tree, using parent(), firstChild() and
  nextSibling().  This code does something to each of an item's
  children:

  \code
    CListViewItem * myChild = myItem->firstChild();
    while( myChild ) {
	doSomething( myChild );
	myChild = myChild->nextSibling();
    }
  \endcode

  Also there is now an interator class to traverse a tree of list view items.
  To iterate over all items of a list view, do:

  \code
    CListViewItemIterator it( listview );
    for ( ; it.current(); ++it )
      do_something_with_the_item( it.current() );
  \endcode

  Note that the order of the children will change when the sorting
  order changes, and is undefined if the items are not visible.  You
  can however call enforceSortOrder() at any time, and CListView will
  always call it before it needs to show an item.

  Many programs will need to reimplement CListViewItem.  The most
  commonly reimplemented functions are: <ul> <li> text() returns the
  text in a column.  Many subclasses will compute that on the
  fly. <li> key() is used for sorting.  The default key() simply calls
  text(), but judicious use of key can be used to sort by e.g. date
  (as QFileDialog does).  <li> setup() is called before showing the
  item, and whenever e.g. the font changes. <li> activate() is called
  whenever the user clicks on the item or presses space when the item
  is the currently highlighted item.</ul>

  Some subclasses call setExpandable( TRUE ) even when they have no
  children, and populate themselves when setup() or setOpen( TRUE ) is
  called.  The dirview/dirview.cpp example program uses precisely this
  technique to start up quickly: The files and subdirectories in a
  directory aren't entered into the tree until they need to.
*/

/*!
  Constructs a new top-level list view item in the CListView \a parent.
*/

CListViewItem::CListViewItem( CListView * parent )
{
    init();
    parent->insertItem( this );
}


/*!  Constructs a new list view item which is a child of \a parent and first
  in the parent's list of children. */

CListViewItem::CListViewItem( CListViewItem * parent )
{
    init();
    parent->insertItem( this );
}




/*!  Constructs an empty list view item which is a child of \a parent
  and is after \a after in the parent's list of children */

CListViewItem::CListViewItem( CListView * parent, CListViewItem * after )
{
    init();
    parent->insertItem( this );
    moveToJustAfter( after );
}


/*!  Constructs an empty list view item which is a child of \a parent
  and is after \a after in the parent's list of children */

CListViewItem::CListViewItem( CListViewItem * parent, CListViewItem * after )
{
    init();
    parent->insertItem( this );
    moveToJustAfter( after );
}



/*!  Constructs a new list view item in the CListView \a parent,
  \a parent, with at most 8 constant strings as contents.

  \code
     (void)new CListViewItem( lv, "/", "Root directory" );
  \endcode

  \sa setText()
*/

CListViewItem::CListViewItem( CListView * parent,
			      QString label1,
			      QString label2,
			      QString label3,
			      QString label4,
			      QString label5,
			      QString label6,
			      QString label7,
			      QString label8 )
{
    init();
    parent->insertItem( this );

    setText( 0, label1 );
    setText( 1, label2 );
    setText( 2, label3 );
    setText( 3, label4 );
    setText( 4, label5 );
    setText( 5, label6 );
    setText( 6, label7 );
    setText( 7, label8 );
}


/*!  Constructs a new list view item that's a child of the CListViewItem
  \a parent, with at most 8 constant strings as contents.  Possible
  example in a threaded news or e-mail reader:

  \code
     (void)new CListViewItem( parentMessage, author, subject );
  \endcode

  \sa setText()
*/

CListViewItem::CListViewItem( CListViewItem * parent,
			      QString label1,
			      QString label2,
			      QString label3,
			      QString label4,
			      QString label5,
			      QString label6,
			      QString label7,
			      QString label8 )
{
    init();
    parent->insertItem( this );

    setText( 0, label1 );
    setText( 1, label2 );
    setText( 2, label3 );
    setText( 3, label4 );
    setText( 4, label5 );
    setText( 5, label6 );
    setText( 6, label7 );
    setText( 7, label8 );
}

/*!  Constructs a new list view item in the CListView \a parent,
  after item \a after, with at most 8 constant strings as contents.

  Note that the order is changed according to CListViewItem::key()
  unless the list view's sorting is disabled using
  CListView::setSorting( -1 ).

  \sa setText()
*/

CListViewItem::CListViewItem( CListView * parent, CListViewItem * after,
			      QString label1,
			      QString label2,
			      QString label3,
			      QString label4,
			      QString label5,
			      QString label6,
			      QString label7,
			      QString label8 )
{
    init();
    parent->insertItem( this );
    moveToJustAfter( after );

    setText( 0, label1 );
    setText( 1, label2 );
    setText( 2, label3 );
    setText( 3, label4 );
    setText( 4, label5 );
    setText( 5, label6 );
    setText( 6, label7 );
    setText( 7, label8 );
}


/*!  Constructs a new list view item that's a child of the CListViewItem
  \a parent, after item \a after, with at most 8 constant strings as
  contents.

  Note that the order is changed according to CListViewItem::key()
  unless the list view's sorting is disabled using
  CListView::setSorting( -1 ).

  \sa setText()
*/

CListViewItem::CListViewItem( CListViewItem * parent, CListViewItem * after,
			      QString label1,
			      QString label2,
			      QString label3,
			      QString label4,
			      QString label5,
			      QString label6,
			      QString label7,
			      QString label8 )
{
    init();
    parent->insertItem( this );
    moveToJustAfter( after );

    setText( 0, label1 );
    setText( 1, label2 );
    setText( 2, label3 );
    setText( 3, label4 );
    setText( 4, label5 );
    setText( 5, label6 );
    setText( 6, label7 );
    setText( 7, label8 );
}

/*!
  (Re)sorts all child items of this item using the last sorting
  configuration (sort column and direction).

  \sa enforceSortOrder()
*/

void CListViewItem::sort()
{
    if ( !listView() )
	 return;
    lsc = Unsorted;
    enforceSortOrder();
    listView()->triggerUpdate();
}


/*!  Performs the initializations that's common to the constructors. */

void CListViewItem::init()
{
    ownHeight = 0;
    maybeTotalHeight = -1;
    open = FALSE;

    nChildren = 0;
    parentItem = 0;
    siblingItem = prevSiblingItem = childItem = 0;

    columns = 0;

    selected = 0;

    lsc = Unsorted;
    lso = TRUE; // unsorted in ascending order :)
    configured = FALSE;
    expandable = FALSE;
    selectable = TRUE;
    is_root = FALSE;

    m_pIconViewPixmap = 0;
}


/*!  Destroys the item, deleting all its children, freeing up all
  allocated resources.
*/

CListViewItem::~CListViewItem()
{
    CListView *lv = listView();

    if ( lv ) {
	if ( lv->d->iterators ) {
	    CListViewItemIterator *i = lv->d->iterators->first();
	    while ( i ) {
		if ( i->current() == this )
		    i->currentRemoved();
		i = lv->d->iterators->next();
	    }
	}
    }

    if ( parentItem )
	parentItem->takeItem( this );
    CListViewItem * i = childItem;
    childItem = 0;
    while ( i ) {
	i->parentItem = 0;
	CListViewItem * n = i->siblingItem;
	delete i;
	i = n;
    }
    delete (CListViewPrivate::ItemColumnInfo *)columns;

    if (m_pIconViewPixmap)
    {
        delete m_pIconViewPixmap;
        m_pIconViewPixmap = 0;
    }
}


/*!  Inserts \a newChild into its list of children.  You should not
  need to call this function; it is called automatically by the
  constructor of \a newChild.

  This function works even if this item is not contained in a list view.
*/

void CListViewItem::insertItem( CListViewItem * newChild )
{
    if ( !newChild || newChild->parentItem == this )
	return;
    if ( newChild->parentItem )
	newChild->parentItem->takeItem( newChild );
    if ( open )
	invalidateHeight();
    newChild->siblingItem = childItem;
    if(childItem)
    {
        childItem->prevSiblingItem = newChild;
    }
    childItem = newChild;
    nChildren++;
    newChild->parentItem = this;
    lsc = Unsorted;
    newChild->ownHeight = 0;
    newChild->configured = FALSE;
    CListView *lv = listView();
    lv->d->makeCurrentVisibleOnUpdate = FALSE;
    if ( lv && lv->hasFocus() && !lv->d->focusItem ) {
	lv->d->focusItem = lv->firstChild();
	lv->repaintItem( lv->d->focusItem );
    }
}


/*!\obsolete

  This function has been renamed takeItem().
*/
void CListViewItem::removeItem( CListViewItem * item )
{
    takeItem( item );
}


/*!
  Removes \a item from this object's list of children and causes an update
  of the screen display.  The item is not deleted.  You should normally not
  need to call this function, as CListViewItem::~CListViewItem() calls it.
  The normal way to delete an item is \c delete.

  \warning This function leaves \a item and its children in a state
  where most member functions are unsafe.  Only the few functions that
  are explicitly documented to work in this state may be used then.

  \sa CListViewItem::insertItem()
*/

void CListViewItem::takeItem( CListViewItem * item )
{
    if ( !item )
	return;

    CListView *lv = listView();
    if ( lv && !lv->d->clearing ) {

	if ( lv->d->iterators ) {
	    CListViewItemIterator *i = lv->d->iterators->first();
	    while ( i ) {
		if ( i->current() == item )
		    i->currentRemoved();
		i = lv->d->iterators->next();
	    }
	}

	invalidateHeight();

	if ( lv->d && lv->d->drawables ) {
	    delete lv->d->drawables;
	    lv->d->drawables = 0;
	}

	if ( lv->d->dirtyItems ) {
	    if ( item->childItem ) {
		delete lv->d->dirtyItems;
		lv->d->dirtyItems = 0;
		lv->d->dirtyItemTimer->stop();
		lv->triggerUpdate();
	    } else {
		lv->d->dirtyItems->take( (void *)item );
	    }
	}

	item->setSelected( FALSE );

#if 0
	// ##### do we really want that???
	if ( lv->selectedItem() ) {
	    CListViewItem * c = lv->selectedItem();
	    while( c && c != item )
		c = c->parentItem;
	    if ( c == item ) {
		emit lv->selectionChanged( 0 );
	    }
	}
#endif

	if ( lv->d->focusItem ) {
	    bool was_selected = lv->d->focusItem->isSelected();
	    const CListViewItem * c = lv->d->focusItem;
	    while( c && c != item )
		c = c->parentItem;
	    if ( c == item ) {
		if ( item->nextSibling() )
		    lv->d->focusItem = item->nextSibling();
 		else if ( item->itemAbove() )
 		    lv->d->focusItem = item->itemAbove();
		else
		    lv->d->focusItem = 0;
		emit lv->currentChanged( lv->d->focusItem );
		if ( was_selected )
		    emit lv->selectionChanged();
	    }
	}

	if ( lv->d->selectAnchor == item )
	    lv->d->selectAnchor = lv->d->focusItem;
    }

    nChildren--;

    CListViewItem ** nextChild = &childItem;
    while( nextChild && *nextChild && item != *nextChild )
	nextChild = &((*nextChild)->siblingItem);

    if ( nextChild && item == *nextChild )
    {
	*nextChild = (*nextChild)->siblingItem;
	if ( *nextChild && (*nextChild)->prevSiblingItem )
	{
	    (*nextChild)->prevSiblingItem =
		(*nextChild)->prevSiblingItem->prevSiblingItem;
	}
    }

    item->parentItem = 0;
    item->siblingItem = 0;
    item->prevSiblingItem = 0;
    item->ownHeight = 0;
    item->maybeTotalHeight = -1;
    item->configured = FALSE;
}


/*!
  \fn QString CListViewItem::key( int column, bool ascending ) const

  Returns a key that can be used for sorting by column \a column.
  The default implementation returns text().  Derived classes may
  also incorporate the order indicated by \a ascending into this
  key, although this is not recommended.

  You can use this function to sort by non-alphabetic data.  This code
  excerpt sort by file modification date, for example

  \code
    if ( column == 3 ) {
	QDateTime epoch( QDate( 1980, 1, 1 ) );
	tmpString.sprintf( "%08d", epoch.secsTo( myFile.lastModified() ) );
    } else {
	// ....
    }
    return tmpString;
  \endcode

  \sa sortChildItems()
*/

QString CListViewItem::key( int column, bool ) const
{
    return text( column );
}


#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static int cmp( const void *n1, const void *n2 )
{
    if ( !n1 || !n2 )
	return 0;

  // I added a .lower() call to each QString so a lowercase string
  // will get sorted, this will cause it to sort properly.
  // (sort by character 'a' == 'A', not by ASCII value)
    return ((CListViewPrivate::SortableItem *)n1)->key.lower().
	    compare( ((CListViewPrivate::SortableItem *)n2)->key.lower() );
}

#if defined(Q_C_CALLBACKS)
}
#endif


/*!  Sorts the children of this item by the return values of
  key(\a column, \a ascending), in ascending order if \a ascending
  is TRUE and in descending order of \a descending is FALSE.

  Asks some of the children to sort their children.  (CListView and
  CListViewItem ensure that all on-screen objects are properly sorted,
  but may avoid or defer sorting other objects in order to be more
  responsive.)

  \sa key()
*/

void CListViewItem::sortChildItems( int column, bool ascending )
{
    // we try HARD not to sort.  if we're already sorted, don't.
    if ( column == (int)lsc && ascending == (bool)lso )
	return;

    if ( column < 0 )
	return;

    // more dubiously - only sort if the child items "exist"
    if ( !isOpen() || !childCount() )
	return;

    lsc = column;
    lso = ascending;

    // and don't sort if we already have the right sorting order
    if ( childItem == 0 || childItem->siblingItem == 0 )
	return;

    // make an array we can sort in a thread-safe way using qsort()
    CListViewPrivate::SortableItem * siblings
	= new CListViewPrivate::SortableItem[nChildren];
    CListViewItem * s = childItem;
    int i = 0;
    while ( s && i<nChildren ) {
	siblings[i].key = s->key( column, ascending );
	siblings[i].i = s;
	s = s->siblingItem;
	i++;
    }

    // and do it.
    qsort( siblings, nChildren,
	   sizeof( CListViewPrivate::SortableItem ), cmp );

    // build the linked list of siblings, in the appropriate
    // direction, and finally set this->childItem to the new top
    // child.
    if ( ascending ) {
	for( i=0; i < nChildren-1; i++ )
	    siblings[i].i->siblingItem = siblings[i+1].i;
	siblings[nChildren-1].i->siblingItem = 0;
	childItem = siblings[0].i;
    } else {
	for( i=nChildren-1; i >0; i-- )
	    siblings[i].i->siblingItem = siblings[i-1].i;
	siblings[0].i->siblingItem = 0;
	childItem = siblings[nChildren-1].i;
    }

    // we don't want no steenking memory leaks.
    delete[] siblings;
}


/*!  Sets this item's own height to \a height pixels.  This implicitly
  changes totalHeight() too.

  Note that e.g. a font change causes this height to be overwritten
  unless you reimplement setup().

  For best results in Windows style, we suggest using an even number
  of pixels.

  \sa height() totalHeight() isOpen();
*/

void CListViewItem::setHeight( int height )
{
    if ( ownHeight != height ) {
	ownHeight = height;
	invalidateHeight();
    }
}


/*!  Invalidates the cached total height of this item including
  all open children.

  This function works even if this item is not contained in a list view.

  \sa setHeight() height() totalHeight()
*/

void CListViewItem::invalidateHeight()
{
    if ( maybeTotalHeight < 0 )
	return;
    maybeTotalHeight = -1;
    if ( parentItem && parentItem->isOpen() )
	parentItem->invalidateHeight();
}


/*!  Sets this item to be open (its children are visible) if \a o is
  TRUE, and to be closed (its children are not visible) if \a o is
  FALSE.

  Also does some bookkeeping.

  \sa height() totalHeight()
*/

void CListViewItem::setOpen( bool o )
{
    if ( o == (bool)open )
	return;
    open = o;

    CListView *lv = listView();
    if ( lv && this != lv->d->r ) {
	if ( o )
	    emit lv->expanded( this );
	else
	    emit lv->collapsed( this );
    }


    if ( !nChildren )
	return;
    invalidateHeight();

    if ( !configured ) {
	CListViewItem * l = this;
	QStack<CListViewItem> s;
	while( l ) {
	    if ( l->open && l->childItem ) {
		s.push( l->childItem );
	    } else if ( l->childItem ) {
		// first invisible child is unconfigured
		CListViewItem * c = l->childItem;
		while( c ) {
		    c->configured = FALSE;
		    c = c->siblingItem;
		}
	    }
	    l->configured = TRUE;
	    l->setup();
	    l = (l == this) ? 0 : l->siblingItem;
	    if ( !l && !s.isEmpty() )
		l = s.pop();
	}
    }

    if ( !open )
	return;

    enforceSortOrder();
}


/*!  This virtual function is called before the first time CListView
  needs to know the height or any other graphical attribute of this
  object, and whenever the font, GUI style or colors of the list view
  change.

  The default calls widthChanged() and sets the item's height to the
  height of a single line of text in the list view's font.  (If you
  use icons, multi-line text etc. you will probably need to call
  setHeight() yourself or reimplement this.)
*/

void CListViewItem::setup()
{
    widthChanged();
    CListView * v = listView();
    int h;
    if (v->iconView())
    {
        h = v->d->fontMetricsHeight;
    }
    else
    {
        int ph = 0;
        for ( uint i = 0; i < v->d->column.size(); ++i ) {
	    if ( pixmap( i ) )
	        ph = QMAX( ph, pixmap( i )->height() );
        }
        h = QMAX( v->d->fontMetricsHeight, ph ) + 2*v->itemMargin();
    }
    if ( h % 2 > 0 )
	h++;
    if (v->iconView())
    {
        h += 32 + ICON_VIEW_MARGIN*3;
    }
    setHeight( h );
}




/*!
  This virtual function is called whenever the user clicks on this
  item or presses Space on it.

  \sa activatedPos()
*/

void CListViewItem::activate()
{
}



/*!
  When called from a reimplementation of activate(), this function
  gives information on how the item was activated. Otherwise, the
  behaviour is undefined.

  If activate() was caused by a mouse press, the function sets \a
  pos to where the user clicked and returns TRUE, otherwise it returns
  FALSE and does not change \a pos.

  Pos is relative to the top-left corner of this item.

  We recommend not using this function; it will most likely be
  obsoleted at the first opportunity.

  \sa activate()
*/

bool CListViewItem::activatedPos( QPoint &pos )
{
    if ( activatedByClick )
	pos = activatedP;
    return activatedByClick;
}





/*! \fn bool CListViewItem::isSelectable() const

  Returns TRUE if the item is selectable (as it is by default) and
  FALSE if it isn't.

  \sa setSelectable()
*/


/*!  Sets this items to be selectable if \a enable is TRUE (the
  default) or not to be selectable if \a enable is FALSE.

  The user is not able to select a non-selectable item using either
  the keyboard or mouse.  The application programmer still can, of
  course.  \sa isSelectable() */

void CListViewItem::setSelectable( bool enable )
{
    selectable = enable;
}


/*! \fn bool CListViewItem::isExpandable() const

  Returns TRUE if this item is expandable even when it has no
  children.
*/

/*!  Sets this item to be expandable even if it has no children if \a
  enable is TRUE, and to be expandable only if it has children if \a
  enable is FALSE (the default).

  The dirview example uses this in the canonical fashion: It checks
  whether the directory is empty in setup() and calls
  setExpandable(TRUE) if not, and in setOpen() it reads the contents
  of the directory and inserts items accordingly.  This strategy means
  that dirview can display the entire file system without reading very
  much at start-up.

  Note that root items are not expandable by the user unless
  CListView::setRootIsDecorated() is set to TRUE.

  \sa setSelectable()
*/

void CListViewItem::setExpandable( bool enable )
{
    expandable = enable;
}


/*!  Makes sure that this object's children are sorted appropriately.

  This only works if every item in the chain from the root item to
  this item is sorted appropriately.

  \sa sortChildItems()
*/


void CListViewItem::enforceSortOrder() const
{
    if( parentItem &&
	(parentItem->lsc != lsc || parentItem->lso != lso) &&
	(int)parentItem->lsc != Unsorted )
	((CListViewItem *)this)->sortChildItems( (int)parentItem->lsc,
						 (bool)parentItem->lso );
    else if ( !parentItem &&
	      ( (int)lsc != listView()->d->sortcolumn ||
		(bool)lso != listView()->d->ascending ) &&
	      listView()->d->sortcolumn != Unsorted )
	((CListViewItem *)this)->sortChildItems( listView()->d->sortcolumn,
						 listView()->d->ascending );
}


/*! \fn bool CListViewItem::isSelected() const

  Returns TRUE if this item is selected, or FALSE if it is not.

  \sa setSelected() CListView::setSelected() CListView::selectionChanged()
*/


/*!  Sets this item to be selected \a s is TRUE, and to not be
  selected if \a o is FALSE.

  This function does not maintain any invariants or repaint anything -
  CListView::setSelected() does that.

  \sa height() totalHeight() */

void CListViewItem::setSelected( bool s )
{
    selected = s && isSelectable() ? 1 : 0;
}


/*!  Returns the total height of this object, including any visible
  children.  This height is recomputed lazily and cached for as long
  as possible.

  setHeight() can be used to set the item's own height, setOpen()
  to show or hide its children, and invalidateHeight() to invalidate
  the cached height.

  \sa height()
*/

int CListViewItem::totalHeight() const
{
    if ( maybeTotalHeight >= 0 )
	return maybeTotalHeight;
    CListViewItem * that = (CListViewItem *)this;
    if ( !that->configured ) {
	that->configured = TRUE;
	that->setup(); // ### virtual non-const function called in const
    }
    that->maybeTotalHeight = that->ownHeight;

    if ( !that->isOpen() || !that->childCount() )
	return that->ownHeight;

    CListViewItem * child = that->childItem;
    while ( child != 0 ) {
	that->maybeTotalHeight += child->totalHeight();
	child = child->siblingItem;
    }

    if (listView()->iconView() && this == listView()->d->r)
    {
        listView()->buildDrawableList();
    }

    return that->maybeTotalHeight;
}


/*!  Returns the text in column \a column, or a
  \link QString::operator!() null string \endlink if there
  is no text in that column.

  This function works even if this item is not contained in a list
  view, but reimplementations of it are not required to work properly
  in that case.

  \sa key() paintCell()
*/

QString CListViewItem::text( int column ) const
{
    CListViewPrivate::ItemColumnInfo * l
	= (CListViewPrivate::ItemColumnInfo*) columns;

    while( column && l ) {
	l = l->next;
	column--;
    }

    return l ? l->text : QString::null;
}


/*!  Sets the text in column \a column to \a text, if \a column is a
  valid column number and \a text is non-null.

  If \a text() has been reimplemented, this function may be a no-op.

  \sa text() key() invalidate()
*/

void CListViewItem::setText( int column, const QString &text )
{
    if ( column < 0 )
	return;

    CListViewPrivate::ItemColumnInfo * l
	= (CListViewPrivate::ItemColumnInfo*) columns;
    if ( !l ) {
	l = new CListViewPrivate::ItemColumnInfo;
	columns = (void*)l;
    }
    for( int c=0; c<column; c++ ) {
	if ( !l->next )
	    l->next = new CListViewPrivate::ItemColumnInfo;
	l = l->next;
    }
    if ( l->text == text )
	return;

    l->text = text;
    if ( column == (int)lsc )
	lsc = Unsorted;
    CListView * lv = listView();
    int oldW = lv ? lv->columnWidth( column ) : 0;
    widthChanged( column );
    if ( !lv )
	return;
    if ( oldW != lv->columnWidth( column ) )
	listView()->triggerUpdate();
    else
	repaint();
}


/*!  Sets the pixmap in column \a column to \a pm, if \a pm is
  non-null and \a column is non-negative.

  \sa pixmap() setText()
*/

void CListViewItem::setPixmap( int column, const QPixmap & pm )
{
    int oldW = 0;
    int oldH = 0;
    if ( pixmap( column ) ) {
	oldW = pixmap( column )->width();
	oldH = pixmap( column )->height();
    }

    if ( column < 0 )
	return;

    CListViewPrivate::ItemColumnInfo * l
	= (CListViewPrivate::ItemColumnInfo*) columns;
    if ( !l ) {
	l = new CListViewPrivate::ItemColumnInfo;
	columns = (void*)l;
    }

    for( int c=0; c<column; c++ ) {
	if ( !l->next )
	    l->next = new CListViewPrivate::ItemColumnInfo;
	l = l->next;
    }

    if ( ( pm.isNull() && ( !l->pm || l->pm->isNull() ) ) ||
	 ( l->pm && pm.serialNumber() == l->pm->serialNumber() ) )
	return;

    if ( pm.isNull() ) {
	delete l->pm;
	l->pm = 0;
    } else {
	if ( l->pm )
	    *(l->pm) = pm;
	else
	    l->pm = new QPixmap( pm );
    }

    int newW = 0;
    int newH = 0;
    if ( pixmap( column ) ) {
	newW = pixmap( column )->width();
	newH = pixmap( column )->height();
    }

    if ( oldW != newW || oldH != newH ) {
	setup();
	widthChanged( column );
	invalidateHeight();
    }
    repaint();
}


/*!  Returns a pointer to the pixmap for \a column, or a null pointer
  if there is no pixmap for \a column.

  This function works even if this item is not contained in a list
  view, but reimplementations of it are not required to work properly
  in that case.

  \sa setText() setPixmap()
*/

const QPixmap * CListViewItem::pixmap( int column ) const
{
    CListViewPrivate::ItemColumnInfo * l
    = (CListViewPrivate::ItemColumnInfo*) columns;

    while( column && l ) {
	l = l->next;
	column--;
    }

    return (l && l->pm) ? l->pm : 0;
}


void CListViewItem::setIconViewPixmap(const QPixmap &pix)
{
    if (m_pIconViewPixmap)
    {
        *m_pIconViewPixmap = pix;
    }
    else
    {
        m_pIconViewPixmap = new QPixmap(pix);
    }
}

const QPixmap *CListViewItem::iconViewPixmap() const
{
    return m_pIconViewPixmap;
}


/*!  This virtual function paints the contents of one column of one item.

  \a p is a QPainter open on the relevant paint device.  \a pa is
  translated so 0, 0 is the top left pixel in the cell and \a width-1,
  height()-1 is the bottom right pixel \e in the cell.  The other
  properties of \a p (pen, brush etc) are undefined.  \a cg is the
  color group to use.  \a column is the logical column number within
  the item that is to be painted; 0 is the column which may contain a
  tree.

  This function may use CListView::itemMargin() for readability
  spacing on the left and right sides of information such as text,
  and should honor isSelected() and CListView::allColumnsShowFocus().

  If you reimplement this function, you should also reimplement
  width().

  The rectangle to be painted is in an undefined state when this
  function is called, so you \e must draw on all the pixels.  The
  painter \a p has the right font on entry.

  \sa paintBranches(), CListView::drawContentsOffset()
*/

void CListViewItem::paintCell( QPainter * p, const QColorGroup & cg,
			       int column, int width, int align )
{
    if (listView()->iconView())
    {
        paintCellIconView(p, cg, column, width, align);
    }
    else
    {
        paintCellNormalView(p, cg, column, width, align);
    }
}


void CListViewItem::paintCellIconView(QPainter *p, const QColorGroup &cg,
					int column, int width, int align)
{
	if (!p)
	{
		return;
	}
	
	CListView *lv = listView();
	const QPixmap * icon = iconViewPixmap();

	p->fillRect(0, 0, width, height(), cg.base());

	int marg = 1;
	int w = 0;
	int h = 0;
	QString t = text(column);
	int maxW = width - 4 - marg*2;
	if (!t.isEmpty())
	{
		w = lv->fontMetrics().width(t);
		if (w > maxW)
		{
			while (w > maxW && t.length() > 1)
			{
				t = t.left(t.length() - 1);
				w = lv->fontMetrics().width(t + "...");
			}
			t = t + "...";
		}
	}
	/*if (w > width - marg*2)
	{
		w = width - marg*2;
	}*/
	if (w > maxW)
	{
		w = maxW;
	}
	m_nIconViewTextWidth = w;
	h = lv->fontMetrics().height();

	lv->m_iconViewTextRect.setRect((width - w)/2 - 2,
		32 + ICON_VIEW_MARGIN*2,
		w + 4,
		h + 2);
	if (isSelected())
	{
		QPixmap back;
		if (icon)
		{
			back = *icon;
			tint(back, QApplication::winStyleHighlightColor(), 0.5);
			p->drawPixmap(width/2 - 16, ICON_VIEW_MARGIN, back);
		}
		
		p->fillRect(lv->m_iconViewTextRect, QApplication::winStyleHighlightColor());
		p->setPen(white); // ###
	}
	else
	{
		p->setPen(cg.text());
	}

	if (icon && !isSelected())
	{
		p->drawPixmap(width/2 - 16, ICON_VIEW_MARGIN, *icon);
	}

	if (!t.isEmpty())
	{
		p->drawText(lv->m_iconViewTextRect.x() + 2,
								lv->m_iconViewTextRect.y() + 1,
								w, h, align | AlignVCenter, t);
	}
}


void CListViewItem::paintCellNormalView( QPainter * p, const QColorGroup & cg,
			       int column, int width, int align )
{
    // Change width() if you change this.

    if ( !p )
	return;

    CListView *lv = listView();

    // had, but we _need_ the column info for the ellipsis thingy!!!
    if ( !columns ) {
	for ( uint i = 0; i < lv->d->column.size(); ++i ) {
	    setText( i, text( i ) );
	}
    }

    QString t = text( column );

    if ( columns ) {
	CListViewPrivate::ItemColumnInfo *ci = 0;
	// try until we have a column info....
	while ( !ci ) {
	    ci = (CListViewPrivate::ItemColumnInfo*)columns;
	    for ( int i = 0; ci && (i < column); ++i )
		ci = ci->next;

	    if ( !ci ) {
		setText( column, t );
		ci = 0;
	    }
	}

	// if the column width changed and this item was not painted since this change
	if ( ci && ci->width != width || ci->text != t ) {
	    QFontMetrics fm( lv->fontMetrics() );
	    ci->width = width;
	    ci->truncated = FALSE;
	    // if we have to do the ellipsis thingy calc the truncated text
	    int pw = pixmap( column ) ? pixmap( column )->width() + lv->itemMargin() : lv->itemMargin();
	    if ( fm.width( t ) + pw > width ) {
		ci->truncated = TRUE;
		ci->tmpText = "...";
		int i = 0;
		while ( fm.width( ci->tmpText + t[ i ] ) + pw < width )
		    ci->tmpText += t[ i++ ];
		ci->tmpText.remove( 0, 3 );
		if ( ci->tmpText.isEmpty() )
		    ci->tmpText = t.left( 1 );
		ci->tmpText += "...";
	    }
	}

	// if we have to draw the ellipsis thingy, use the truncated text
	if ( ci && ci->truncated )
	    t = ci->tmpText;
    }

    int r = lv ? lv->itemMargin() : 1;
    const QPixmap * icon = pixmap( column );

    p->fillRect( 0, 0, width, height(), cg.brush( QColorGroup::Base ) );

    int marg = lv ? lv->itemMargin() : 1;
    if ( align != AlignLeft )
	marg -= lv->d->minRightBearing;
    if ( isSelected() &&
	 (column==0 || listView()->allColumnsShowFocus()) ) {
	    p->fillRect( r - marg, 0, width - r + marg, height(),
		     cg.brush( QColorGroup::Highlight ) );
	    p->setPen( cg.highlightedText() );
    } else {
	p->setPen( cg.text() );
    }

    if ( icon ) {
	p->drawPixmap( r, (height()-icon->height())/2, *icon );
	r += icon->width() + listView()->itemMargin();
    }

    if ( !t.isEmpty() ) {
	p->drawText( r, 0, width-marg-r, height(),
		     align | AlignVCenter, t );
    }
}

/*!
  Returns the number of pixels of width required to draw column \a c
  of listview \a lv, using the metrics \a fm without cropping.
  The list view containing this item may use
  this information, depending on the CListView::WidthMode settings
  for the column.

  The default implementation returns the width of the bounding
  rectangle of the text of column \a c.

  \sa listView() widthChanged() CListView::setColumnWidthMode()
  CListView::itemMargin()
*/
int CListViewItem::width( const QFontMetrics& fm,
			  const CListView* lv, int c ) const
{
    if (lv->iconView())
    {
        return m_nIconViewTextWidth;
    }

    int w = fm.width( text( c ) ) + lv->itemMargin() * 2
	    - lv->d->minLeftBearing - lv->d->minRightBearing;
    const QPixmap * pm = pixmap( c );
    if ( pm )
	w += pm->width() + lv->itemMargin(); // ### correct margin stuff?
    return w;
}


/*!  Paints a focus indication on the rectangle \a r using painter \a p
  and colors \a cg.

  \a p is already clipped.

  \sa paintCell() paintBranches() CListView::setAllColumnsShowFocus()
*/

void CListViewItem::paintFocus( QPainter *p, const QColorGroup &cg,
				const QRect & r )
{
    listView()->style().drawFocusRect( p, r, cg, isSelected()? & cg.highlight() : & cg.base(), isSelected() );
}


/*!  Paints a set of branches from this item to (some of) its children.

  \a p is set up with clipping and translation so that you can draw
  only in the rectangle you need to; \a cg is the color group to use;
  the update rectangle is at 0, 0 and has size \a w, \a h.  The top of
  the rectangle you own is at \a y (which is never greater than 0 but
  can be outside the window system's allowed coordinate range).

  The update rectangle is in an undefined state when this function is
  called; this function must draw on \e all of the pixels.

  \sa paintCell(), CListView::drawContentsOffset()
*/

void CListViewItem::paintBranches( QPainter * p, const QColorGroup & cg,
				   int w, int y, int h, GUIStyle s )
{
    p->fillRect( 0, 0, w, h, cg.brush( QColorGroup::Base ) );
    CListViewItem * child = firstChild();
    int linetop = 0, linebot = 0;

    int dotoffset = (itemPos() + height() - y) %2;

    // each branch needs at most two lines, ie. four end points
    QPointArray dotlines( childCount() * 4 );
    int c = 0;

    // skip the stuff above the exposed rectangle
    while ( child && y + child->height() <= 0 ) {
	y += child->totalHeight();
	child = child->nextSibling();
    }

    int bx = w / 2;

    // paint stuff in the magical area
    while ( child && y < h ) {
	linebot = y + child->height()/2;
	if ( (child->expandable || child->childCount()) &&
	     (child->height() > 0) ) {
	    // needs a box
	    p->setPen( cg.text() );
	    p->drawRect( bx-4, linebot-4, 9, 9 );
// 	    p->setPen( cg.text() ); // ### windows uses black
	    if ( s == WindowsStyle ) {
		// plus or minus
		p->drawLine( bx - 2, linebot, bx + 2, linebot );
		if ( !child->isOpen() )
		    p->drawLine( bx, linebot - 2, bx, linebot + 2 );
	    } else {
		QPointArray a;
		if ( child->isOpen() )
		    a.setPoints( 3, bx-2, linebot-2,
				 bx, linebot+2,
				 bx+2, linebot-2 ); //RightArrow
		else
		    a.setPoints( 3, bx-2, linebot-2,
				 bx+2, linebot,
				 bx-2, linebot+2 ); //DownArrow
		p->setBrush( cg.text() );
		p->drawPolygon( a );
		p->setBrush( NoBrush );
	    }
	    // dotlinery
	    dotlines[c++] = QPoint( bx, linetop );
	    dotlines[c++] = QPoint( bx, linebot - 5 );
	    dotlines[c++] = QPoint( bx + 5, linebot );
	    dotlines[c++] = QPoint( w, linebot );
	    linetop = linebot + 5;
	} else {
	    // just dotlinery
	    dotlines[c++] = QPoint( bx+1, linebot );
	    dotlines[c++] = QPoint( w, linebot );
	}

	y += child->totalHeight();
	child = child->nextSibling();
    }

    if ( child ) // there's a child, so move linebot to edge of rectangle
	linebot = h;

    if ( linetop < linebot ) {
	dotlines[c++] = QPoint( bx, linetop );
	dotlines[c++] = QPoint( bx, linebot );
    }

    p->setPen( cg.dark() );
    if ( s == WindowsStyle ) {
	if ( !verticalLine ) {
	    // make 128*1 and 1*128 bitmaps that can be used for
	    // drawing the right sort of lines.
	    verticalLine = new QBitmap( 1, 129, TRUE );
	    horizontalLine = new QBitmap( 128, 1, TRUE );
	    QPointArray a( 64 );
	    QPainter p;
	    p.begin( verticalLine );
	    int i;
	    for( i=0; i<64; i++ )
		a.setPoint( i, 0, i*2+1 );
	    p.setPen( color1 );
	    p.drawPoints( a );
	    p.end();
	    QApplication::flushX();
	    verticalLine->setMask( *verticalLine );
	    p.begin( horizontalLine );
	    for( i=0; i<64; i++ )
		a.setPoint( i, i*2+1, 0 );
	    p.setPen( color1 );
	    p.drawPoints( a );
	    p.end();
	    QApplication::flushX();
	    horizontalLine->setMask( *horizontalLine );
	    qAddPostRoutine( cleanupBitmapLines );
	}
	int line; // index into dotlines
	for( line = 0; line < c; line += 2 ) {
	    // assumptions here: lines are horizontal or vertical.
	    // lines always start with the numerically lowest
	    // coordinate.

	    // point ... relevant coordinate of current point
	    // end ..... same coordinate of the end of the current line
	    // other ... the other coordinate of the current point/line
	    if ( dotlines[line].y() == dotlines[line+1].y() ) {
		int end = dotlines[line+1].x();
		int point = dotlines[line].x();
		int other = dotlines[line].y();
		while( point < end ) {
		    int i = 128;
		    if ( i+point > end )
			i = end-point;
		    p->drawPixmap( point, other, *horizontalLine,
				   0, 0, i, 1 );
		    point += i;
		}
	    } else {
		int end = dotlines[line+1].y();
		int point = dotlines[line].y();
		int other = dotlines[line].x();
		int pixmapoffset = ((point & 1) != dotoffset ) ? 1 : 0;
		while( point < end ) {
		    int i = 128;
		    if ( i+point > end )
			i = end-point;
		    p->drawPixmap( other, point, *verticalLine,
				   0, pixmapoffset, 1, i );
		    point += i;
		}
	    }
	}
    } else {
	int line; // index into dotlines
	p->setPen( cg.text() );
	for( line = 0; line < c; line += 2 ) {
	    p->drawLine( dotlines[line].x(), dotlines[line].y(),
			 dotlines[line+1].x(), dotlines[line+1].y() );
	}
    }
}


CListViewPrivate::Root::Root( CListView * parent )
    : CListViewItem( parent )
{
    lv = parent;
    setHeight( 0 );
    setOpen( TRUE );
}


void CListViewPrivate::Root::setHeight( int )
{
    CListViewItem::setHeight( 0 );
}


void CListViewPrivate::Root::invalidateHeight()
{
    CListViewItem::invalidateHeight();
    lv->triggerUpdate();
}


CListView * CListViewPrivate::Root::theListView() const
{
    return lv;
}


void CListViewPrivate::Root::setup()
{
    // explicitly nothing
}


/*! \fn void  CListView::onItem( CListViewItem *i )
  This signal is emitted, when the user moves the mouse cursor onto an item.
  It´s only emitted once per item.
*/

/*! \fn void  CListView::onViewport()
  This signal is emitted, when the user moves the mouse cursor, which was
  on an item away from the item onto the viewport.
*/

/*! \enum CListView::SelectionMode

  This enumerated type is used by CListView to indicate how it reacts
  to selection by the user.  It has four values: <ul>

  <li> \c Single - When the user selects an item, any already-selected
  item becomes unselected, and the user cannot unselect the selected
  item. This means that the user can never clear the selection, even
  though the selection may be cleared by the application programmer
  using CListView::clearSelection().

  <li> \c Multi - When the user selects an item in the most ordinary
  way, the selection status of that item is toggled and the other
  items are left alone.

  <li> \c Extended - When the user selects an item in the most
  ordinary way, the selection is cleared and the new item selected.
  However, if the user presses the CTRL key when clicking on an item,
  the clicked item gets toggled and all other items are left untouched. And
  if the user presses the SHIFT key while clicking on an item, all items
  between the current item and the clicked item get selected or unselected
  depending on the state of the clicked item.
  Also multiple items can be selected by dragging the mouse while the
  left mouse button stayes pressed.

  <li> \c NoSelection - Items cannot be selected.

  </ul>

  In other words, \c Single is a real single-selection listview, \c
  Multi a real multi-selection listview, and \c Extended listview
  where users can select multiple items but usually want to select
  either just one or a range of contiguous items, and \c NoSelection
  is for a listview where the user can look but not touch.
*/

/*!
  \class CListView qlistview.h
  \brief The CListView class implements a list/tree view.
  \ingroup advanced

  It can display and control a hierarchy of multi-column items, and
  provides the ability to add new items at any time, let the user
  select one or many items, sort the list in increasing or decreasing
  order by any column, and so on.

  The simplest mode of usage is to create a CListView, add some column
  headers using setColumn(), create one or more CListViewItem objects
  with the CListView as parent, set up the list view's geometry(), and
  show() it.

  The main setup functions are <ul>

  <li>addColumn() - adds a column, with text and perhaps width.

  <li>setColumnWidthMode() - sets the column to be resized
  automatically or not.

  <li>setAllColumnsShowFocus() - decides whether items should show
  keyboard focus using all columns, or just column 0.  The default is
  to show focus using just column 0.

  <li>setRootIsDecorated() - decides whether root items can be opened
  and closed by the user, and have open/close decoration to their left.
  The default is FALSE.

  <li>setTreeStepSize() - decides the how many pixels an item's
  children are indented relative to their parent.  The default is 20.
  This is mostly a matter of taste.

  <li>setSorting() - decides whether the items should be sorted,
  whether it should be in ascending or descending order, and by what
  column it should be sorted.</ul>

  To handle events such as mouse-presses on the listview, derived classes
  can reimplement the QScrollView functions
\link QScrollView::contentsMousePressEvent() contentsMousePressEvent\endlink,
\link QScrollView::contentsMouseReleaseEvent() contentsMouseReleaseEvent\endlink,
\link QScrollView::contentsMouseDoubleClickEvent() contentsMouseDoubleClickEvent\endlink,
\link QScrollView::contentsMouseMoveEvent() contentsMouseMoveEvent\endlink,
\link QScrollView::contentsDragEnterEvent() contentsDragEnterEvent\endlink,
\link QScrollView::contentsDragMoveEvent() contentsDragMoveEvent\endlink,
\link QScrollView::contentsDragLeaveEvent() contentsDragLeaveEvent\endlink,
\link QScrollView::contentsDropEvent() contentsDropEvent\endlink, and
\link QScrollView::contentsWheelEvent() contentsWheelEvent\endlink.

  There are also several functions for mapping between items and
  coordinates.  itemAt() returns the item at a position on-screen,
  itemRect() returns the rectangle an item occupies on the screen and
  itemPos() returns the position of any item (not on-screen, in the
  list view).  firstChild() returns the item at the top of the view
  (not necessarily on-screen) so you can iterate over the items using
  either CListViewItem::itemBelow() or a combination of
  CListViewItem::firstChild() and CListViewItem::nextSibling().

  Naturally, CListView provides a clear() function, as well as an
  explicit insertItem() for when CListViewItem's default insertion
  won't do.

  There is a variety of selection modes, described in the
  CListView::SelectionMode documentation. The default is
  single-selection, and you can change it using setSelectionMode().
  For compatibility with previous Qt versions there is still the
  setMultiSelection() methode. Calling setMultiSelection( TRUE )
  is equivalent to setSelectionMode( Multi ), and setMultiSelection( FALSE )
  is equivalent to setSelectionMode( Single ). It's suggested not to
  use setMultiSelection() anymore, but to use setSelectionMode()
  instead.

  Since CListView offers multiple selection it has to display keyboard
  focus and selection state separately.  Therefore there are functions
  both to set the selection state of an item, setSelected(), and to
  select which item displays keyboard focus, setCurrentItem().

  CListView emits two groups of signals: One group signals changes in
  selection/focus state and one signals selection.  The first group
  consists of selectionChanged(), applicable to all list views, and
  selectionChanged( CListViewItem * ), applicable only to
  single-selection list view, and currentChanged( CListViewItem * ).
  The second group consists of doubleClicked( CListViewItem * ),
  returnPressed( CListViewItem * ) and rightButtonClicked(
  CListViewItem *, const QPoint&, int ), etc.

  In Motif style, CListView deviates fairly strongly from the look and
  feel of the Motif hierarchical tree view.  This is done mostly to
  provide a usable keyboard interface and to make the list view look
  better with a white background.

  <img src=qlistview-m.png> <img src=qlistview-w.png>

  \internal

  need to say stuff about the mouse and keyboard interface.
*/

/*!  Constructs a new empty list view, with \a parent as a parent and \a
  name as object name. */

CListView::CListView( QWidget * parent, const char *name,
        bool bUseHeaderExtender )
    : QScrollView( parent, name, WNorthWestGravity | WRepaintNoErase ),
    m_pStationaryHeader( 0 ),
    m_bUseStationaryHeader( false ),
    m_bIconView( false )
{
    d = new CListViewPrivate;
    d->vci = 0;
    d->timer = new QTimer( this );
    d->levelWidth = 20;
    d->r = 0;
    d->rootIsExpandable = 0;
    d->h = new CHeader( this, "list view header", bUseHeaderExtender );
    d->h->installEventFilter( this );
    d->focusItem = 0;
    d->drawables = 0;
    d->dirtyItems = 0;
    d->dirtyItemTimer = new QTimer( this );
    d->visibleTimer = new QTimer( this );
    d->margin = 1;
    d->selectionMode = CListView::Single;
    d->sortcolumn = 0;
    d->ascending = TRUE;
    d->allColumnsShowFocus = FALSE;
    d->fontMetricsHeight = fontMetrics().height();
    d->h->setTracking(TRUE);
    d->buttonDown = FALSE;
    d->ignoreDoubleClick = FALSE;
    d->column.setAutoDelete( TRUE );
    d->iterators = 0;
    d->scrollTimer = 0;
    d->sortIndicator = FALSE;
    d->clearing = FALSE;
    d->minLeftBearing = fontMetrics().minLeftBearing();
    d->minRightBearing = fontMetrics().minRightBearing();
    d->ellipsisWidth = fontMetrics().width( "..." ) * 2;
    d->highlighted = 0;
    d->pressedItem = 0;
    d->makeCurrentVisibleOnUpdate = TRUE;
    d->selectAnchor = 0;
    d->select = TRUE;

    setMouseTracking( TRUE );
    viewport()->setMouseTracking( TRUE );

    connect( d->timer, SIGNAL(timeout()),
	     this, SLOT(updateContents()) );
    connect( d->dirtyItemTimer, SIGNAL(timeout()),
	     this, SLOT(updateDirtyItems()) );
    connect( d->visibleTimer, SIGNAL(timeout()),
	     this, SLOT(makeVisible()) );

    connect( d->h, SIGNAL(sizeChange( int, int, int )),
	     this, SLOT(handleSizeChange( int, int, int )) );
    connect( d->h, SIGNAL(moved( int, int )),
	     this, SLOT(triggerUpdate()) );
    connect( d->h, SIGNAL(sectionClicked( int )),
	     this, SLOT(changeSortColumn( int )) );
    connect( horizontalScrollBar(), SIGNAL(sliderMoved(int)),
	     d->h, SLOT(setOffset(int)) );
    connect( horizontalScrollBar(), SIGNAL(valueChanged(int)),
	     d->h, SLOT(setOffset(int)) );

    // will access d->r
    CListViewPrivate::Root * r = new CListViewPrivate::Root( this );
    r->is_root = TRUE;
    d->r = r;
    d->r->setSelectable( FALSE );

    viewport()->setFocusProxy( this );
    viewport()->setFocusPolicy( WheelFocus );
}

int CListView::numIconWidth() const
{
#ifdef QT_2
  return viewport()->width() / ICON_VIEW_WIDTH;
#else
  // temporary hack until QScrollView::viewport() becomes const-correct in Qt2
  CListView *ptr = const_cast<CListView *>(this);
  return ptr->viewport()->width() / ICON_VIEW_WIDTH;
#endif
}

bool CListView::iconWidthChange(bool bUpdateStatus)
{
    int nNewWidth = numIconWidth();
    if (nNewWidth != nOldWidth)
    {
        if (bUpdateStatus)
        {
            nOldWidth = nNewWidth;
        }
        return true;
    }
    else
    {
        return false;
    }
}

/*!
  If \a show is TRUE, draw an arrow in the header of the listview
  to indicate the sort order of the listview contents. The arrow
  will be drawn in the correct column and will point to the correct
  direction. Set \a show to FALSE to disable this feature.

  \sa CHeader::setSortIndicator()
*/

void CListView::setShowSortIndicator( bool show )
{
    d->sortIndicator = show;
    if ( d->sortcolumn != Unsorted )
	d->h->setSortIndicator( d->sortcolumn, d->ascending );
}

/*!
  Returns TRUE, if the sort order and column are indicated
  in the header, else FALSE.

  \sa CListView::setSortIndicator()
*/

bool CListView::showSortIndicator() const
{
    return d->sortIndicator;
}

/*!
  Destructs the listview, deleting all items in it, and frees up all
  allocated resources.
*/

CListView::~CListView()
{
    if ( d->iterators ) {
	CListViewItemIterator *i = d->iterators->first();
	while ( i ) {
	    i->listView = 0;
	    i = d->iterators->next();
	}
	delete d->iterators;
    }

    d->focusItem = 0;
    delete d->r;
    d->r = 0;
    delete d->dirtyItems;
    d->dirtyItems = 0;
    delete d->drawables;
    d->drawables = 0;
    delete d->vci;
    d->vci = 0;
    delete d->h;
    d->h = 0;
    delete d;
    d = 0;

    if (m_bUseStationaryHeader && m_pStationaryHeader)
    {
        delete m_pStationaryHeader;
        m_pStationaryHeader = 0;
    }
}


void CListView::drawContentsOffset( QPainter * p, int ox, int oy,
				    int cx, int cy, int cw, int ch )
{
   if (iconView())
   {
       drawContentsOffsetIconView(p, ox, oy, cx, cy, cw, ch);
   }
   else
   {
       drawContentsOffsetNormalView(p, ox, oy, cx, cy, cw, ch);
   }
}

void CListView::drawContentsOffsetIconView(QPainter * p, int ox, int oy,
				    int cx, int cy, int cw, int ch)
{
	int ih = 100;
	if (firstChild() && firstChild()->height())
	{
		ih = firstChild()->height();
	}
	int nW = cw / ICON_VIEW_WIDTH + 1;
	if (cw % ICON_VIEW_WIDTH)
	{
		nW++;
	}
	int nH = ch / ih + 1;
	if (ch % ih)
	{
		nH++;
	}
	bool **grid = new bool*[nW];
	for (int nLoop = 0; nLoop < nW; nLoop++)
	{
		grid[nLoop] = new bool[nH];
	}
	for (int nX = 0; nX < nW; nX++)
	{
		for (int nY = 0; nY < nH; nY++)
		{
			grid[nX][nY] = false;
		}
	}
	int nOX = cx / ICON_VIEW_WIDTH;
	int nOY = cy / ih;
	
	if (!d->drawables || d->drawables->isEmpty() ||
			//d->topPixel > cy || d->bottomPixel < cy + ch - 1 ||
			iconWidthChange() ||
			d->r->maybeTotalHeight < 0)
	{
		buildDrawableList();
	}
	
	if (d->dirtyItems)
	{
		QRect br(cx - ox, cy - oy, cw, ch);
		QPtrDictIterator<void> it(*(d->dirtyItems));
		CListViewItem * i;
		while ((i=(CListViewItem *)(it.currentKey())) != 0)
		{
			++it;
			QRect ir = itemRect(i).intersect(viewport()->rect());
			if (ir.isEmpty() || br.contains(ir))
			{
				// we're painting this one, or it needs no painting: forget it
				d->dirtyItems->remove((void *)i);
			}
		}
		if (d->dirtyItems->count())
		{
			// there are still items left that need repainting
			d->dirtyItemTimer->start(0, TRUE);
		}
		else
		{
			// we're painting all items that need to be painted
			delete d->dirtyItems;
			d->dirtyItems = 0;
			d->dirtyItemTimer->stop();
		}
	}

	p->setFont(font());

	QListIterator<CListViewPrivate::DrawableItem> it(*(d->drawables));

	QRect r;
	CListViewPrivate::DrawableItem * current;

	while ((current = it.current()) != 0)
	{
		++it;
		ih = current->i->height();
		
		// need to paint current?
		if (ih > 0 && current->y < cy+ch && current->y+ih >= cy &&
				current->x < cx + cw && current->x + ICON_VIEW_WIDTH >= cx)
		{
			int x = current->x / ICON_VIEW_WIDTH - nOX;
			int y = current->y / ih - nOY;
			if (x >= 0 && x < nW && y >= 0 && y < nH )
			{
				grid[x][y] = true;
			}
			
			r.setRect(current->x - ox, current->y - oy, ICON_VIEW_WIDTH, ih);
		
			p->save();
			if (p->clipRegion().isNull())
				p->setClipRegion(QRegion(r));
			else
			  	p->setClipRegion(p->clipRegion().intersect(QRegion(r)));
			p->translate(r.left(), r.top());
			current->i->paintCell(p, colorGroup(), 0, r.width(),
		  											columnAlignment(0));
			p->restore();

			// draw object border
			if (current->i == d->focusItem && hasFocus() &&
					!d->allColumnsShowFocus)
			{
				p->save();
				//r.setRect(current->i->m_iconViewTextRect.x(),
				//					current->i->m_iconViewTextRect.y(),
				r.setRect(r.x() + m_iconViewTextRect.x(),
									r.y() + m_iconViewTextRect.y(),
									m_iconViewTextRect.width(),
									m_iconViewTextRect.height());
	    			if (p->clipRegion().isNull())
	    				p->setClipRegion(QRegion(r));
	    			else
	    			  	p->setClipRegion(p->clipRegion().intersect(QRegion(r)));
				current->i->paintFocus(p, colorGroup(), r);
				p->restore();
			}
		}
	}// while

	// clear empty areas	
	for (int nX = 0; nX < nW; nX++)
	{
		for (int nY = 0; nY < nH; nY++)
		{
			if (!grid[nX][nY])
			{
				int x = (nOX + nX) * ICON_VIEW_WIDTH - ox;
				int y = (nOY + nY) * ih - oy;
				paintEmptyArea(p, QRect(x, y, ICON_VIEW_WIDTH, ih));
			}
		}
	}
	
	for (int nLoop = 0; nLoop < nW; nLoop++)
	{
		delete grid[nLoop];
	}
	delete [] grid;
}



/*!  Calls CListViewItem::paintCell() and/or
  CListViewItem::paintBranches() for all list view items that
  require repainting.  See the documentation for those functions for
  details.
*/

void CListView::drawContentsOffsetNormalView(QPainter * p, int ox, int oy,
				    int cx, int cy, int cw, int ch)
{
    if ( !d->drawables ||
	 d->drawables->isEmpty() ||
	 d->topPixel > cy ||
	 d->bottomPixel < cy + ch - 1 ||
	 d->r->maybeTotalHeight < 0 )
	buildDrawableList();

    if ( d->dirtyItems ) {
	QRect br( cx - ox, cy - oy, cw, ch );
	QPtrDictIterator<void> it( *(d->dirtyItems) );
	CListViewItem * i;
	while( (i=(CListViewItem *)(it.currentKey())) != 0 ) {
	    ++it;
	    QRect ir = itemRect( i ).intersect( viewport()->rect() );
	    if ( ir.isEmpty() || br.contains( ir ) )
		// we're painting this one, or it needs no painting: forget it
		d->dirtyItems->remove( (void *)i );
	}
	if ( d->dirtyItems->count() ) {
	    // there are still items left that need repainting
	    d->dirtyItemTimer->start( 0, TRUE );
	} else {
	    // we're painting all items that need to be painted
	    delete d->dirtyItems;
	    d->dirtyItems = 0;
	    d->dirtyItemTimer->stop();
	}
    }

    p->setFont( font() );

    QListIterator<CListViewPrivate::DrawableItem> it( *(d->drawables) );

    QRect r;
    int fx = -1, x, fc = 0, lc = 0;
    int tx = -1;
    CListViewPrivate::DrawableItem * current;

    while ( (current = it.current()) != 0 ) {
	++it;

	int ih = current->i->height();
	int ith = current->i->totalHeight();
	int c;
	int cs;

	// need to paint current?
	if ( ih > 0 && current->y < cy+ch && current->y+ih >= cy ) {
	    if ( fx < 0 ) {
		// find first interesting column, once
		x = 0;
		c = 0;
		cs = d->h->cellSize( 0 );
		while ( x + cs <= cx && c < d->h->count() ) {
		    x += cs;
		    c++;
		    if ( c < d->h->count() )
			cs = d->h->cellSize( c );
		}
		fx = x;
		fc = c;
		while( x < cx + cw && c < d->h->count() ) {
		    x += cs;
		    c++;
		    if ( c < d->h->count() )
			cs = d->h->cellSize( c );
		}
		lc = c;
	    }

	    x = fx;
	    c = fc;
	    // draw to last interesting column
	    while( c < lc && d->drawables ) {
		int i = d->h->mapToLogical( c );
		cs = d->h->cellSize( c );
		r.setRect( x - ox, current->y - oy, cs, ih );
		if ( i==0 && current->i->parentItem )
		    r.setLeft( r.left() + current->l * treeStepSize() );

		p->save();
		p->translate( r.left(), r.top() );
		int ac = d->h->mapToLogical( c );
		current->i->paintCell( p, colorGroup(), ac, r.width(),
				       columnAlignment( ac ) );
		p->restore();
		x += cs;
		c++;
	    }
	    if ( current->i == d->focusItem && hasFocus() &&
		 !d->allColumnsShowFocus ) {
		p->save();
		int c = d->h->mapToActual( 0 );
		QRect r( d->h->cellPos( c ) - ox, current->y - oy, d->h->cellSize( c ), ih );
		if ( current->i->parentItem )
		    r.setLeft( r.left() + current->l * treeStepSize() );
		current->i->paintFocus( p, colorGroup(), r );
		p->restore();
	    }
	}

	// does current need focus indication?
	if ( current->i == d->focusItem && hasFocus() &&
	     d->allColumnsShowFocus ) {
	    p->save();
	    int x = -contentsX();
	    int w = header()->cellPos( header()->count() - 1 ) +
		    header()->cellSize( header()->count() - 1 );

	    r.setRect( x, current->y - oy, w, ih );
	    if ( d->h->mapToActual( 0 ) == 0 || ( current->l == 0 && !rootIsDecorated() ) ) {
		r.setLeft( r.left() + current->l * treeStepSize() );
		current->i->paintFocus( p, colorGroup(), r );
	    } else {
		int xdepth = treeStepSize() * ( current->i->depth() + ( rootIsDecorated() ? 1 : 0) )
			     + itemMargin();
		xdepth += d->h->cellPos( d->h->mapToActual( 0 ) );
		QRect r1( r );
		r1.setRight( d->h->cellPos( d->h->mapToActual( 0 ) ) - 1 );
		QRect r2( r );
		r2.setLeft( xdepth - 1 );
 		current->i->paintFocus( p, colorGroup(), r1 );
 		current->i->paintFocus( p, colorGroup(), r2 );
	    }
	    p->restore();
	}

	if ( tx < 0 )
	    tx = d->h->cellPos( d->h->mapToActual( 0 ) );

	// do any children of current need to be painted?
	if ( ih != ith &&
	     (current->i != d->r || d->rootIsExpandable) &&
	     current->y + ith > cy &&
	     current->y + ih < cy + ch &&
	     tx + current->l * treeStepSize() < cx + cw &&
	     tx + (current->l+1) * treeStepSize() > cx ) {
	    // compute the clip rectangle the safe way

	    int rtop = current->y + ih;
	    int rbottom = current->y + ith;
	    int rleft = tx + current->l*treeStepSize();
	    int rright = rleft + treeStepSize();

	    int crtop = QMAX( rtop, cy );
	    int crbottom = QMIN( rbottom, cy+ch );
	    int crleft = QMAX( rleft, cx );
	    int crright = QMIN( rright, cx+cw );

	    r.setRect( crleft-ox, crtop-oy,
		       crright-crleft, crbottom-crtop );

	    if ( r.isValid() ) {
		p->save();
		p->translate( rleft-ox, crtop-oy );
		current->i->paintBranches( p, colorGroup(), treeStepSize(),
					   rtop - crtop, r.height(), style() );
		p->restore();
	    }
	}
    }

    if ( d->r->totalHeight() < cy + ch )
	paintEmptyArea( p, QRect( cx - ox, d->r->totalHeight() - oy,
				  cw, cy + ch - d->r->totalHeight() ) );

    int c = d->h->count()-1;
    if ( c >= 0 &&
	 d->h->cellPos( c ) + d->h->cellSize( c ) < cx + cw ) {
	c = d->h->cellPos( c ) + d->h->cellSize( c );
	paintEmptyArea( p, QRect( c - ox, cy - oy, cx + cw - c, ch ) );
    }
}



/*!  Paints \a rect so that it looks like empty background using
  painter p.  \a rect is is widget coordinates, ready to be fed to \a
  p.

  The default function fills \a rect with colorGroup().brush( QColorGroup::Base ).
*/

void CListView::paintEmptyArea( QPainter * p, const QRect & rect )
{
    p->fillRect( rect, colorGroup().brush( QColorGroup::Base ) );
}


/*! Rebuilds the list of drawable CListViewItems.  This function is
  const so that const functions can call it without requiring
  d->drawables to be mutable */

void CListView::buildDrawableList() const
{
	if (iconView())
	{
		buildDrawableListIconView();
	}
	else
	{
		buildDrawableListNormalView();
	}
}


/*! Rebuilds the list of drawable CListViewItems.  This function is
  const so that const functions can call it without requiring
  d->drawables to be mutable */

void CListView::buildDrawableListIconView() const
{
	d->r->enforceSortOrder();

	QStack<CListViewPrivate::Pending> stack;
	/*stack.push(new CListViewPrivate::Pending(((int)d->rootIsExpandable)-1,
						 0, d->r));*/

	// could mess with cy and ch in order to speed up vertical
	// scrolling
	//int cy = contentsY();
	int ch = ((CListView *)this)->viewport()->height();
	// ### hack to help sizeHint().  if not visible, assume that we'll
	// ### use 200 pixels rather than whatever QSrollView.  this lets
	// ### sizeHint() base its width on a more realistic number of
	// ### items.
	if (!isVisible() && ch < 200)
	{
		ch = 200;	// strange
	}
	//d->topPixel = cy + ch; // one below bottom
	//d->bottomPixel = cy - 1; // one above top
	
	// used to work around lack of support for mutable
	QList<CListViewPrivate::DrawableItem> * dl;
	
	dl = new QList<CListViewPrivate::DrawableItem>;
	dl->setAutoDelete(TRUE);
	if (d->drawables)
	{
		delete d->drawables;
	}
	d->drawables = dl;

	
	// NEW STUFF	
	int nMaxSizeX = ((CListView*)this)->numIconWidth();
	if (nMaxSizeX < 1)
	{
		nMaxSizeX = 1;
	}
	CListViewItem *current = firstChild();
	int x = 0;
	int y = 0;
	int ih = 0;
	while (current)
	{
		ih = current->height();
		d->r->maybeTotalHeight = (y + 1) * ih;
	
		stack.push(new CListViewPrivate::Pending(0, y * ih, current,
			x * ICON_VIEW_WIDTH));
 		x++;
		if (x >= nMaxSizeX)
		{
			x = 0;
			y++;
		}
		current = current->nextSibling();
	}
	
	CListViewPrivate::Pending * cur;
	
	while (!stack.isEmpty())
	{
		cur = stack.pop();

		dl->append(new CListViewPrivate::DrawableItem(cur));

		/*ih = cur->i->height();
		if (cur->y + ih >= cy && cur->y <= cy + ch)
		{

			dl->append(new CListViewPrivate::DrawableItem(cur));
			if (cur->y < d->topPixel)
			{
				d->topPixel = cur->y;
			}
			if (cur->y + ih - 1 > d->bottomPixel)
			{
				d->bottomPixel = cur->y + ih - 1;
			}
		}*/

		delete cur;
	}
	
	/*while (!stack.isEmpty())
	{
		cur = stack.pop();

		int ih = cur->i->height();
		int ith = cur->i->totalHeight();
		
		// is this item, or its branch symbol, inside the viewport?
		if (cur->y + ith >= cy && cur->y < cy + ch)
		{
			dl->append(new CListViewPrivate::DrawableItem(cur));
			// perhaps adjust topPixel up to this item?  may be adjusted
			// down again if any children are not to be painted
			if (cur->y < d->topPixel)
			{
				d->topPixel = cur->y;
			}
			// bottompixel is easy: the bottom item drawn contains it
			d->bottomPixel = cur->y + ih - 1;
		}
		// push younger sibling of cur on the stack?
		if (cur->y + ith < cy+ch && cur->i->nextSiblingItem)
		{
			stack.push(new CListViewPrivate::Pending(cur->l,
																							 cur->y + ith,
																							 cur->i->nextSiblingItem));
		}
		
		// do any children of cur need to be painted?
		if (cur->i->isOpen() && cur->i->childCount() &&
				cur->y + ith > cy && cur->y + ih < cy + ch)
		{
			cur->i->enforceSortOrder();
			
			CListViewItem * c = cur->i->childItem;
			int y = cur->y + ih;
			
			// if any of the children are not to be painted, skip them
			// and invalidate topPixel
			while (c && y + c->totalHeight() <= cy)
			{
				y += c->totalHeight();
				c = c->nextSiblingItem;
				d->topPixel = cy + ch;
			}
			
			// push one child on the stack, if there is at least one
			// needing to be painted
			if (c && y < cy+ch)
			{
				stack.push(new CListViewPrivate::Pending(cur->l + 1, y, c));
			}
		}
		
		delete cur;
	}*/
}




/*! Rebuilds the lis of drawable CListViewItems.  This function is
  const so that const functions can call it without requiring
  d->drawables to be mutable */

void CListView::buildDrawableListNormalView() const
{
    d->r->enforceSortOrder();

    QStack<CListViewPrivate::Pending> stack;
    stack.push( new CListViewPrivate::Pending( ((int)d->rootIsExpandable)-1,
					       0, d->r ) );

    // could mess with cy and ch in order to speed up vertical
    // scrolling
    int cy = contentsY();
    int ch = ((CListView *)this)->visibleHeight();
    // ### hack to help sizeHint().  if not visible, assume that we'll
    // ### use 200 pixels rather than whatever QScrollView thinks.
    // ### this lets sizeHint() base its width on a more realistic
    // ### number of items.
    if ( !isVisible() && ch < 200 )
	ch = 200;
    d->topPixel = cy + ch; // one below bottom
    d->bottomPixel = cy - 1; // one above top

    CListViewPrivate::Pending * cur;

    // used to work around lack of support for mutable
    QList<CListViewPrivate::DrawableItem> * dl;

    dl = new QList<CListViewPrivate::DrawableItem>;
    dl->setAutoDelete( TRUE );
    if ( d->drawables )
	delete ((CListView *)this)->d->drawables;
    ((CListView *)this)->d->drawables = dl;

    while ( !stack.isEmpty() ) {
	cur = stack.pop();

	int ih = cur->i->height();
	int ith = cur->i->totalHeight();

	// is this item, or its branch symbol, inside the viewport?
	if ( cur->y + ith >= cy && cur->y < cy + ch ) {
	    dl->append( new CListViewPrivate::DrawableItem(cur));
	    // perhaps adjust topPixel up to this item?  may be adjusted
	    // down again if any children are not to be painted
	    if ( cur->y < d->topPixel )
		d->topPixel = cur->y;
	    // bottompixel is easy: the bottom item drawn contains it
	    d->bottomPixel = cur->y + ih - 1;
	}

	// push younger sibling of cur on the stack?
	if ( cur->y + ith < cy+ch && cur->i->siblingItem )
	    stack.push( new CListViewPrivate::Pending(cur->l,
						      cur->y + ith,
						      cur->i->siblingItem));

	// do any children of cur need to be painted?
	if ( cur->i->isOpen() && cur->i->childCount() &&
	     cur->y + ith > cy &&
	     cur->y + ih < cy + ch ) {
	    cur->i->enforceSortOrder();

	    CListViewItem * c = cur->i->childItem;
	    int y = cur->y + ih;

	    // if any of the children are not to be painted, skip them
	    // and invalidate topPixel
	    while ( c && y + c->totalHeight() <= cy ) {
		y += c->totalHeight();
		c = c->siblingItem;
		d->topPixel = cy + ch;
	    }

	    // push one child on the stack, if there is at least one
	    // needing to be painted
	    if ( c && y < cy+ch )
		stack.push( new CListViewPrivate::Pending( cur->l + 1,
							   y, c ) );
	}

	delete cur;
    }
}




/*!  Returns the number of pixels a child is offset from its parent.
  This number has meaning only for tree views.  The default is 20.

  \sa setTreeStepSize()
*/

int CListView::treeStepSize() const
{
    return d->levelWidth;
}


/*!  Sets the the number of pixels a child is offset from its parent,
  in a tree view to \a l.  The default is 20.

  \sa treeStepSize()
*/

void CListView::setTreeStepSize( int l )
{
    if ( l != d->levelWidth ) {
	d->levelWidth = l;
	// update
    }
}


/*!  Inserts \a i into the list view as a top-level item.  You do not
  need to call this unless you've called takeItem( \a i ) or
  CListViewItem::takeItem( i ) and need to reinsert \a i elsewhere.

  \sa CListViewItem::takeItem() (important) takeItem()
*/

void CListView::insertItem( CListViewItem * i )
{
    if ( d->r ) // not for d->r itself
	d->r->insertItem( i );
}


/*!  Remove and delete all the items in this list view, and trigger an
  update. \sa triggerUpdate() */

void CListView::clear()
{
    blockSignals( TRUE );
    d->clearing = TRUE;
    clearSelection();
    if ( d->iterators ) {
	CListViewItemIterator *i = d->iterators->first();
	while ( i ) {
	    i->curr = 0;
	    i = d->iterators->next();
	}
    }

    if ( d->drawables )
	d->drawables->clear();
    delete d->dirtyItems;
    d->dirtyItems = 0;
    d->dirtyItemTimer->stop();

    d->focusItem = 0;

    // if it's down its downness makes no sense, so undown it
    d->buttonDown = FALSE;

    CListViewItem *c = (CListViewItem *)d->r->firstChild();
    CListViewItem *n;
    while( c ) {
	n = (CListViewItem *)c->nextSibling();
	delete c;
	c = n;
    }
    resizeContents( d->h->sizeHint().width(), contentsHeight() );
    delete d->r;
    d->r = 0;
    CListViewPrivate::Root * r = new CListViewPrivate::Root( this );
    r->is_root = TRUE;
    d->r = r;
    d->r->setSelectable( FALSE );
    blockSignals( FALSE );
    triggerUpdate();
    d->clearing = FALSE;
}

/*!
  \reimp
*/

void CListView::setContentsPos( int x, int y )
{
    updateGeometries();
    QScrollView::setContentsPos( x, y );
}

/*!
  Adds a new column at the right end of the widget, with the header \a
  label, and returns the index of the column.

  If \a width is negative, the new column will have WidthMode Maximum,
  otherwise it will be Manual at \a width pixels wide.

  \sa setColumnText() setColumnWidth() setColumnWidthMode()
*/
int CListView::addColumn( const QString &label, int width )
{
    int c = d->h->addLabel( label, width );
    d->column.resize( c+1 );
    d->column.insert( c, new CListViewPrivate::Column );
    d->column[c]->wmode = width >=0 ? Manual : Maximum;
    return c;
}

/*!
  Adds a new column at the right end of the widget, with the header \a
  label and \a iconset, and returns the index of the column.

  If \a width is negative, the new column will have WidthMode Maximum,
  otherwise it will be Manual at \a width pixels wide.

  \sa setColumnText() setColumnWidth() setColumnWidthMode()
*/
int CListView::addColumn( const QIconSet& iconset, const QString &label, int width )
{
    int c = d->h->addLabel( iconset, label, width );
    d->column.resize( c+1 );
    d->column.insert( c, new CListViewPrivate::Column );
    d->column[c]->wmode = width >=0 ? Manual : Maximum;
    return c;
}

/*!
  Returns the number of columns of this list view.

  \sa addColumn(), removeColumn()
*/

int CListView::columns() const
{
    return d->column.count();
}

/*!
  Removes the column at position \a index.
*/

void CListView::removeColumn( int index )
{
    if ( index < 0 || index > (int)d->column.count() - 1 )
	return;

    if ( d->vci ) {
	CListViewPrivate::ViewColumnInfo *vi = d->vci, *prev = 0, *next = 0;
	for ( int i = 0; i < index; ++i ) {
	    if ( vi ) {
		prev = vi;
		vi = vi->next;
	    }
	}
	if ( vi ) {
	    next = vi->next;
	    if ( prev )
		prev->next = next;
	    vi->next = 0;
	    delete vi;
	    if ( index == 0 )
		d->vci = next;
	}
    }

    CListViewItemIterator it( this );
    for ( ; it.current(); ++it ) {
	CListViewPrivate::ItemColumnInfo *ci = (CListViewPrivate::ItemColumnInfo*)it.current()->columns;
	if ( ci ) {
	    CListViewPrivate::ItemColumnInfo *prev = 0, *next = 0;
	    for ( int i = 0; i < index; ++i ) {
		if ( ci ) {
		    prev = ci;
		    ci = ci->next;
		}
	    }
	    if ( ci ) {
		next = ci->next;
		if ( prev )
		    prev->next = next;
		ci->next = 0;
		delete ci;
		if ( index == 0 )
		    it.current()->columns = next;
	    }
	}
    }

    for ( int i = index; i < (int)d->column.count() - 1; ++i ) {
	d->column.take( i );
	d->column.insert( i, d->column[ i + 1 ] );
    }
    d->column.take( d->column.size() - 1 );
    d->column.resize( d->column.size() - 1 );

    d->h->removeLabel( index );

    triggerUpdate();
    if ( d->column.count() == 0 )
	clear();
}

/*!
  Sets the heading text of column \a column to \a label.  The leftmost
  colum is number 0.
*/
void CListView::setColumnText( int column, const QString &label )
{
    if ( column < d->h->count() )
	d->h->setLabel( column, label );
}

/*!
  Sets the heading text of column \a column to \a iconset and \a
  label.  The leftmost colum is number 0.
*/
void CListView::setColumnText( int column, const QIconSet& iconset, const QString &label )
{
    if ( column < d->h->count() )
	d->h->setLabel( column, iconset, label );
}

/*!
  Sets the width of column \a column to \a w pixels.  Note that if the
  column has a WidthMode other than Manual, this width setting may be
  subsequently overridden.  The leftmost colum is number 0.
*/
void CListView::setColumnWidth( int column, int w )
{
    if ( column < d->h->count() && d->h->cellSize( column ) != w ) {
	d->h->resizeSection( column, w );
    }
}


/*!
  Returns the text of column \a c.
*/

QString CListView::columnText( int c ) const
{
    return d->h->label(c);
}

/*!
  Returns the width of column \a c.
*/

int CListView::columnWidth( int c ) const
{
    int actual = d->h->mapToActual( c );
    return d->h->cellSize( actual );
}


/*! \enum CListView::WidthMode

  This enum type describes how the width of a column in the view
  changes.  The currently defined modes are: <ul>

  <li> \c Manual - the column width does not change automatically

  <li> \c Maximum - the column is automatically sized according to the
  widths of all items in the column.  (Note: The column never shrinks
  in this case.) This means the column is always resized to the
  width of the item with the largest width in the column.

  </ul>

  \sa setColumnWidth() setColumnWidthMode() columnWidth()
*/


/*!
  Sets column \c to behave according to \a mode.  The default depends
  on whether the width argument to addColumn was positive or negative.

  \sa CListViewItem::width()
*/

void CListView::setColumnWidthMode( int c, WidthMode mode )
{
    d->column[c]->wmode = mode;
}


/*!
  Returns the currently set WidthMode for column \a c.
  \sa setColumnWidthMode()
*/

CListView::WidthMode CListView::columnWidthMode( int c ) const
{
    return d->column[c]->wmode;
}


/*!
  Configures the logical column \a column to have alignment \a align.
  The alignment is ultimately passed to CListViewItem::paintCell()
  for each item in the view.

  \sa Qt::AlignmentFlags
*/

void CListView::setColumnAlignment( int column, int align )
{
    if ( column < 0 )
	return;
    if ( !d->vci )
	d->vci = new CListViewPrivate::ViewColumnInfo;
    CListViewPrivate::ViewColumnInfo * l = d->vci;
    while( column ) {
	if ( !l->next )
	    l->next = new CListViewPrivate::ViewColumnInfo;
	l = l->next;
	column--;
    }
    if ( l->align == align )
	return;
    l->align = align;
    triggerUpdate();
}


/*!
  Returns the alignment of logical column \a column.  The default
  is \c AlignLeft.

  \sa Qt::AlignmentFlags
*/

int CListView::columnAlignment( int column ) const
{
    if ( column < 0 || !d->vci )
	return AlignLeft;
    CListViewPrivate::ViewColumnInfo * l = d->vci;
    while( column ) {
	if ( !l->next )
	    l->next = new CListViewPrivate::ViewColumnInfo;
	l = l->next;
	column--;
    }
    return l ? l->align : AlignLeft;
}



/*! \reimp
 */
void CListView::show()
{
    // Reimplemented to setx the correct background mode and viewed
    // area size.
    if ( !isVisible() ) {
	QWidget * v = viewport();
	if ( v )
	    v->setBackgroundMode( PaletteBase );

	reconfigureItems();
	updateGeometries();
    }
    QScrollView::show();
}


/*!  Updates the sizes of the viewport, header, scrollbars and so on.
  Don't call this directly; call triggerUpdate() instead.
*/

void CListView::updateContents()
{
    if ( !isVisible() )
	return;

    if (iconView())
    {
        d->r->maybeTotalHeight = -1;    // don't remove!
    }

    //viewport()->setUpdatesEnabled( TRUE ); // ### what should setting it two times to TRUE bring? :-)
    updateGeometries();
    //viewport()->setUpdatesEnabled( TRUE );
    viewport()->repaint( FALSE );
    if ( d->makeCurrentVisibleOnUpdate )
	ensureItemVisible( d->focusItem );
    d->makeCurrentVisibleOnUpdate = TRUE;
}


void CListView::updateGeometries()
{
    int th = d->r->totalHeight();
    int tw = d->h->cellPos( d->h->count()-1 ) +
	    d->h->cellSize( d->h->count()-1 );
    if ( d->h->offset() &&
	 tw < d->h->offset() + d->h->width() )
	horizontalScrollBar()->setValue( tw - CListView::d->h->width() );
    resizeContents( tw, th );
    QSize hs( d->h->sizeHint() );
    if ( d->h->testWState( WState_ForceHide ) || iconView() ) {
	setMargins( 0, 0, 0, 0 );
    } else {
	setMargins( 0, hs.height(), 0, 0 );
	d->h->setGeometry( viewport()->x(), viewport()->y()-hs.height(),
			   visibleWidth(), hs.height() );
	d->h->repaint();
    }

    // fix size and position of the stationary header
    if (m_bUseStationaryHeader)
    {
        m_pStationaryHeader->setGeometry(viewport()->x(),
            viewport()->y() - hs.height(), viewport()->width(), hs.height());
    }

}


/*!
  Updates the display when a section has changed size.
*/

void CListView::handleSizeChange( int section, int os, int ns )
{
    bool upe = viewport()->isUpdatesEnabled();
    viewport()->setUpdatesEnabled( FALSE );
    int sx = horizontalScrollBar()->value();
    updateGeometries();
    bool fullRepaint = sx != horizontalScrollBar()->value();
    viewport()->setUpdatesEnabled( upe );

    if ( fullRepaint ) {
	viewport()->repaint( FALSE );
	return;
    }

    int actual = d->h->mapToActual( section );
    int dx = ns - os;
    int left = d->h->cellPos( actual ) - contentsX() + d->h->cellSize( actual );
    if ( dx > 0)
	left -= dx;
    if ( left < visibleWidth() )
	viewport()->scroll( dx, 0, QRect( left, 0, visibleWidth() - left, visibleHeight() ) );
    viewport()->repaint( left - 4 - d->ellipsisWidth, 0, 4 + d->ellipsisWidth,
			 visibleHeight(), FALSE ); // border between the items and ellipses width

    if ( columnAlignment( section ) != AlignLeft )
	viewport()->repaint( d->h->cellPos( actual ) - contentsX(), 0,
			     d->h->cellSize( actual ), visibleHeight() );

}


/*!  Very smart internal slot that'll repaint JUST the items that need
  to be repainted.  Don't use this directly; call repaintItem() and
  this slot gets called by a null timer.
*/

void CListView::updateDirtyItems()
{
    if ( d->timer->isActive() || !d->dirtyItems)
	return;
    QRect ir;
    QPtrDictIterator<void> it( *(d->dirtyItems) );
    CListViewItem * i;
    while( (i=(CListViewItem *)(it.currentKey())) != 0 ) {
	++it;
	ir = ir.unite( itemRect(i) );
    }
    if ( !ir.isEmpty() )  {		      // rectangle to be repainted
	if ( ir.x() < 0 )
	    ir.moveBy( -ir.x(), 0 );
	viewport()->repaint( ir, FALSE );
    }
}


void CListView::makeVisible()
{
    if ( d->focusItem )
	ensureItemVisible( d->focusItem );
}


/*!  Ensures that the header is correctly sized and positioned.
*/

void CListView::resizeEvent( QResizeEvent *e )
{
    QScrollView::resizeEvent( e );
    d->h->resize( visibleWidth(), d->h->height() );
    updateGeometries();
    if (iconView() && iconWidthChange(false))
    {
        viewport()->update();
    }
}


/*! \reimp */

void CListView::enabledChange( bool e )
{
    d->h->setEnabled( e );
    triggerUpdate();
}


/*!  Triggers a size, geometry and content update during the next
  iteration of the event loop.  Cleverly makes sure that there'll be
  just one update, to avoid flicker. */

void CListView::triggerUpdate()
{
    if ( !isVisible() || !isUpdatesEnabled() )
	return; // it will update when shown, or something.

    if ( d && d->drawables ) {
	delete d->drawables;
	d->drawables = 0;
    }
    d->timer->start( 0, TRUE );
}


/*!  Redirects events for the viewport to mousePressEvent(),
  keyPressEvent() and friends. */

bool CListView::eventFilter( QObject * o, QEvent * e )
{
    if ( !o || !e )
	return FALSE;

    if ( o == d->h &&
	 e->type() >= QEvent::MouseButtonPress &&
	 e->type() <= QEvent::MouseMove ) {
	QMouseEvent * me = (QMouseEvent *)e;
	QMouseEvent me2( me->type(),
			 QPoint( me->pos().x(),
				 me->pos().y() - d->h->height() ),
			 me->button(), me->state() );
	switch( me2.type() ) {
	case QEvent::MouseButtonPress:
	    if ( me2.button() == RightButton ) {
		viewportMousePressEvent( &me2 );
		return TRUE;
	    }
	    break;
	case QEvent::MouseButtonDblClick:
	    if ( me2.button() == RightButton )
		return TRUE;
	    break;
	case QEvent::MouseMove:
	    if ( me2.state() & RightButton ) {
		viewportMouseMoveEvent( &me2 );
		return TRUE;
	    }
	    break;
	case QEvent::MouseButtonRelease:
	    if ( me2.button() == RightButton ) {
		viewportMouseReleaseEvent( &me2 );
		return TRUE;
	    }
	    break;
	default:
	    break;
	}
    } else if ( o == viewport() ) {
	QFocusEvent * fe = (QFocusEvent *)e;

	switch( e->type() ) {
	case QEvent::FocusIn:
	    focusInEvent( fe );
	    return TRUE;
	case QEvent::FocusOut:
	    focusOutEvent( fe );
	    return TRUE;
	default:
	    // nothing
	    break;
	}
    }
    return QScrollView::eventFilter( o, e );
}


/*! Returns a pointer to the listview containing this item.
*/

CListView * CListViewItem::listView() const
{
    const CListViewItem* c = this;

    while ( c && !c->is_root )
	c = c->parentItem;
    if ( !c )
	return 0;
    return ((CListViewPrivate::Root*)c)->theListView();
}


/*!
  Returns the depth of this item.
*/
int CListViewItem::depth() const
{
    return parentItem ? parentItem->depth()+1 : -1; // -1 == the hidden root
}


/*!
  Returns a pointer to the item immediately above this item on the
  screen.  This is usually the item's closest older sibling, but may
  also be its parent or its next older sibling's youngest child, or
  something else if anyoftheabove->height() returns 0.  Returns a null
  pointer if there is no item immediately above this item.

  This function assumes that all parents of this item are open
  (ie. that this item is visible, or can be made visible by
  scrolling).

  \sa itemBelow() CListView::itemRect()
*/

CListViewItem * CListViewItem::itemAbove()
{
    if ( !parentItem )
	return 0;

    CListViewItem * c = parentItem;
    if (listView()->iconView())
    {
        c = 0;
        QListIterator<CListViewPrivate::DrawableItem>
            it(*(listView()->d->drawables));
        CListViewPrivate::DrawableItem * current = 0;
        CListViewPrivate::DrawableItem * found = 0;

        while ((current = it.current()) != 0)
        {
            ++it;
            if (current->i == this)
            {
                found = current;
            }
        }
        if (found)
        {
            it.toFirst();
            int ih = height();
            while ((current = it.current()) != 0)
            {
                ++it;
                if (current->y + ih == found->y && current->x == found->x)
                {
                    c = current->i;
                }
            }
        }
    }
    else
    {
        if ( c->childItem != this ) {
	    c = c->childItem;
	    while( c && c->siblingItem != this )
	        c = c->siblingItem;
	    if ( !c )
	        return 0;
	    while( c->isOpen() && c->childItem ) {
	        c = c->childItem;
	        while( c->siblingItem )
		    c = c->siblingItem;		// assign c's sibling to c
	    }
        }
        if ( c && !c->height() )
	    return c->itemAbove();
    }

    return c;
}


CListViewItem * CListViewItem::itemLeft()
{
	CListViewItem * c = 0;
	if (listView()->iconView())
	{
		QListIterator<CListViewPrivate::DrawableItem> 
			it(*(listView()->d->drawables));
		CListViewPrivate::DrawableItem * current = 0;
		CListViewPrivate::DrawableItem * found = 0;

		while ((current = it.current()) != 0)
		{
			++it;
			if (current->i == this)
			{
				found = current;
			}
		}
		if (found)
		{
			it.toFirst();
			while ((current = it.current()) != 0)
			{
				++it;
				if (current->y == found->y && current->x + ICON_VIEW_WIDTH == found->x)
				{
					c = current->i;
				}
			}
		}
	}
	
	return c;
}


CListViewItem * CListViewItem::itemRight()
{
	CListViewItem * c = 0;
	if (listView()->iconView())
	{
		QListIterator<CListViewPrivate::DrawableItem> 
			it(*(listView()->d->drawables));
		CListViewPrivate::DrawableItem * current = 0;
		CListViewPrivate::DrawableItem * found = 0;

		while ((current = it.current()) != 0)
		{
			++it;
			if (current->i == this)
			{
				found = current;
			}
		}
		if (found)
		{
			it.toFirst();
			while ((current = it.current()) != 0)
			{
				++it;
				if (current->y == found->y && current->x == found->x + ICON_VIEW_WIDTH)
				{
					c = current->i;
				}
			}
		}
	}
	
	return c;
}


/*!
  Returns a pointer to the item immediately below this item on the
  screen.  This is usually the item's eldest child, but may also be
  its next younger sibling, its parent's next younger sibling,
  grandparent's etc., or something else if anyoftheabove->height()
  returns 0.  Returns a null pointer if there is no item immediately
  above this item.

  This function assumes that all parents of this item are open
  (ie. that this item is visible, or can be made visible by
  scrolling).

  \sa itemAbove() CListView::itemRect()
*/

CListViewItem * CListViewItem::itemBelow()
{
    CListViewItem * c = 0;
	if (listView()->iconView())
	{
		CListViewItem * toFind = this;
		if (listView()->d->r == toFind)
		{
			toFind = toFind->firstChild();
		}
		QListIterator<CListViewPrivate::DrawableItem> 
			it(*(listView()->d->drawables));
		CListViewPrivate::DrawableItem * current = 0;
		CListViewPrivate::DrawableItem * found = 0;

		while ((current = it.current()) != 0)
		{
			++it;
			if (current->i == toFind)
			{
				found = current;
			}
		}
		if (found)
		{
			it.toFirst();
			int ih = height();
			while ((current = it.current()) != 0)
			{
				++it;
				if (current->y == found->y + ih && current->x == found->x)
				{
					c = current->i;
				}
			}
		}
	}
	else
	{
        if ( isOpen() && childItem ) {
            c = childItem;
        } else if ( siblingItem ) {
            c = siblingItem;
        } else if ( parentItem ) {
            c = this;
            do {
                c = c->parentItem;
            } while( c->parentItem && !c->siblingItem );
            if ( c )
                c = c->siblingItem;
        }
        if ( c && !c->height() )
            return c->itemBelow();
    }

    return c;
}


/*! \fn bool CListViewItem::isOpen () const

  Returns TRUE if this list view item has children \e and they are
  potentially visible, or FALSE if the item has no children or they
  are hidden.

  \sa setOpen()
*/

/*!
  Returns a pointer to the first (top) child of this item, or a null
  pointer if this item has no children.

  Note that the children are not guaranteed to be sorted properly.
  CListView and CListViewItem try to postpone or avoid sorting to the
  greatest degree possible, in order to keep the user interface
  snappy.

  \sa nextSibling()
*/

CListViewItem* CListViewItem::firstChild () const
{
    enforceSortOrder();
    return childItem;
}


/*!
  Returns a pointer to the parent of this item, or a null pointer if this
  item has no parent.

  \sa firstChild(), nextSibling()
*/

CListViewItem* CListViewItem::parent () const
{
    if ( !parentItem || parentItem->is_root ) return 0;
    return parentItem;
}


/*! \fn CListViewItem* CListViewItem::nextSibling () const

  Returns a pointer to the sibling item below this item, or a
  null pointer if there is no sibling item after this item.

  Note that the siblings are not guaranteed to be sorted properly.
  CListView and CListViewItem try to postpone or avoid sorting to the
  greatest degree possible, in order to keep the user interface
  snappy.

  \sa firstChild()
*/

/*! \fn int CListViewItem::childCount () const

  Returns the current number of children of this item.
*/


/*!
  Returns the height of this item in pixels.  This does not include
  the height of any children; totalHeight() returns that.
*/
int CListViewItem::height() const
{
    CListViewItem * that = (CListViewItem *)this;
    if ( !that->configured ) {
	that->configured = TRUE;
	that->setup(); // ### virtual non-const function called in const
    }

    return ownHeight;
}

/*!
  Call this function when the value of width() may have changed
  for column \a c.  Normally, you should call this if text(c) changes.
  Passing -1 for \a c indicates all columns may have changed.
  For efficiency, you should do this if more than one
  call to widthChanged() is required.

  \sa width()
*/
void CListViewItem::widthChanged( int c ) const
{
    listView()->widthChanged( this, c );
}

/*! \fn void CListView::selectionChanged()

  This signal is emitted whenever the set of selected items has
  changed (normally before the screen update).  It is available both
  in single-selection and multi-selection mode, but is most meaningful
  in multi-selection mode.

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.

  \sa setSelected() CListViewItem::setSelected()
*/


/*! \fn void CListView::pressed( CListViewItem *item )

  This signal is emitted whenever the user presses the mouse button
  on a listview.
  \a item is the pointer to the listview item onto which the user pressed the
  mouse button or NULL, if the user didn't press the mouse on an item.

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.
*/

/*! \fn void CListView::pressed( CListViewItem *item, const QPoint &pnt, int c )

  This signal is emitted whenever the user presses the mouse button
  on a listview.
  \a item is the pointer to the listview item onto which the user pressed the
  mouse button or NULL, if the user didn't press the mouse on an item.
  \a pnt is the position of the mouse cursor, and \a c the
  column into which the mouse cursor was when the user pressed the mouse
  button.

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.
*/

/*! \fn void CListView::clicked( CListViewItem *item )

  This signal is emitted whenever the user clicks (mouse pressed + mouse released)
  into the listview.
  \a item is the pointer to the clicked listview item or NULL, if the user didn't click on an item.

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.
*/

/*!
  \fn void CListView::mouseButtonClicked(int button, CListViewItem * item, const QPoint & pos, int c)

  This signal is emitted whenever the user clicks (mouse pressed + mouse released)
  into the listview. \a button is the mouse button which the user pressed,
  \a item is the pointer to the clicked listview item or NULL, if the user didn't click on an item, and
  \a c the listview column into which the user pressed (this argument is only valid, if \a item
  is not NULL!)

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.
*/

/*!
  \fn void CListView::mouseButtonPressed(int button, CListViewItem * item, const QPoint & pos, int c)

  This signal is emitted whenever the user pressed the mouse button
  onto the listview. \a button is the mouse button which the user pressed,
  \a item is the pointer to the pressed listview item or NULL, if the user didn't press on an item, and
  \a c the listview column into which the user pressed (this argument is only valid, if \a item
  is not NULL!)

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.
*/

/*! \fn void CListView::clicked( CListViewItem *item, const QPoint &pnt, int c )

  This signal is emitted whenever the user clicks (mouse pressed + mouse released)
  into the listview.
  \a item is the pointer to the clicked listview item or NULL, if the user didn't click on an item.
  \a pnt is the position where the user
  has clicked, and \a c the column into which the user clicked.

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.
*/

/*! \fn void CListView::selectionChanged( CListViewItem * )

  This signal is emitted whenever the selected item has changed in
  single-selection mode (normally after the screen update).  The
  argument is the newly selected item.

  There is another signal which is more useful in multi-selection
  mode.

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.

  \sa setSelected() CListViewItem::setSelected() currentChanged()
*/


/*! \fn void CListView::currentChanged( CListViewItem * )

  This signal is emitted whenever the current item has changed
  (normally after the screen update).  The current item is the item
  responsible for indicating keyboard focus.

  The argument is the newly current item, or 0 if the change was to
  make no item current.  This can happen e.g. if all items in the list
  view are deleted.

  Note that you may not delete any CListViewItem objects in slots
  connected to this signal.

  \sa setCurrentItem() currentItem()
*/


/*! \fn void CListView::expanded( CListViewItem *item )

  This signals is emitted when the \a item has been expanded. This means
  the children of the item are shown because the user double-clicked
  the item or clicked on the root decoration, or setOpen() with TRUE
  as argument has been called.

  \sa collapsed()
*/

/*! \fn void CListView::collapsed( CListViewItem *item )

  This signals is emitted when the \a item has been collapsed. This means
  the children of the item are hidden because the user double-clicked
  the item or clicked on the root decoration, or setOpen() with FALSE
  as argument has been called.

  \sa expanded()
*/

/*!
  Processes mouse move events on behalf of the viewed widget.
*/
void CListView::contentsMousePressEvent( QMouseEvent * e )
{
    if ( !e )
	return;

    QPoint vp = contentsToViewport( e->pos() );

    d->ignoreDoubleClick = FALSE;
    d->buttonDown = TRUE;

    CListViewItem * i = itemAt( vp );
    CListViewItem *oldCurrent = currentItem();
    if ( !oldCurrent && !i && firstChild() ) {
	d->focusItem = firstChild();
	repaintItem( d->focusItem );
	oldCurrent = currentItem();
    }

    if ( !i ) {
	if ( isMultiSelection() )
	    clearSelection();
	goto emit_signals;
    } else {
	d->selectAnchor = i;
    }

    if (!iconView())
    {
        if ( (i->isExpandable() || i->childCount()) &&
             d->h->mapToLogical( d->h->cellAt( vp.x() ) ) == 0 ) {
            int x1 = vp.x() +
            	 d->h->offset() -
            	 d->h->cellPos( d->h->mapToActual( 0 ) );
            QListIterator<CListViewPrivate::DrawableItem> it( *(d->drawables) );
            while( it.current() && it.current()->i != i )
                ++it;

            if ( it.current() ) {
                x1 -= treeStepSize() * (it.current()->l - 1);
                if ( x1 >= 0 && x1 < treeStepSize() ) {
            	bool close = i->isOpen();
            	setOpen( i, !i->isOpen() );
            	d->makeCurrentVisibleOnUpdate = FALSE;
            	qApp->processEvents();
            	if ( !d->focusItem ) {
            	    d->focusItem = i;
            	    repaintItem( d->focusItem );
            	    emit currentChanged( d->focusItem );
            	}
            	if ( close ) {
            	    bool newCurrent = FALSE;
            	    CListViewItem *ci = d->focusItem;
            	    while ( ci ) {
            		if ( ci->parent() && ci->parent() == i ) {
            		    newCurrent = TRUE;
            		    break;
            		}
            		ci = ci->parent();
            	    }
            	    if ( newCurrent ) {
            		setCurrentItem( i );
            	    }
            	}

            	d->buttonDown = FALSE;
            	d->ignoreDoubleClick = TRUE;
            	d->buttonDown = FALSE;
            	return;
                }
            }
        }
    }

    d->select = d->selectionMode == Multi ? !i->isSelected() : TRUE;
    {// calculate activatedP
	activatedByClick = TRUE;
	QPoint topLeft = itemRect( i ).topLeft(); //### inefficient?
	activatedP = vp - topLeft;
	int xdepth = treeStepSize() * (i->depth() + (rootIsDecorated() ? 1 : 0))
		     + itemMargin();
	xdepth += d->h->cellPos( d->h->mapToActual( 0 ) );
	activatedP.rx() -= xdepth;
    }
    i->activate();
    activatedByClick = FALSE;
    
    if ( i != d->focusItem )
	setCurrentItem( i );
    else
	repaintItem( i );

    d->pressedSelected = i && i->isSelected();

    if ( i->isSelectable() && selectionMode() != NoSelection ) {
	if ( selectionMode() == Single )
	    setSelected( i, TRUE );
	else if ( selectionMode() == Multi  )
	    setSelected( i, d->select );
	else if ( selectionMode() == Extended ) {
	    bool changed = FALSE;
	    if ( !( ( e->state() & ControlButton ) ||
		    ( e->state() & ShiftButton ) ) ) {
		if ( !i->isSelected() ) {
		    bool blocked = signalsBlocked();
		    blockSignals( TRUE );
		    clearSelection();
		    blockSignals( blocked );
		    i->setSelected( TRUE );
		    changed = TRUE;
		}
	    } else {
		if ( e->state() & ShiftButton )
		    d->pressedSelected = FALSE;
		if ( e->state() & ControlButton && i ) {
		    i->setSelected( !i->isSelected() );
		    changed = TRUE;
		    d->pressedSelected = FALSE;
		} else if ( !oldCurrent || !i || oldCurrent == i ) {
		    if ( (bool)i->selected != d->select ) {
			changed = TRUE;
			i->setSelected( d->select );
		    }
		} else {
		    bool down = oldCurrent->itemPos() < i->itemPos();
		    CListViewItemIterator lit( down ? oldCurrent : i );
		    for ( ;; ++lit ) {
			if ( !lit.current() ) {
			    triggerUpdate();
			    goto emit_signals;
			}
			if ( down && lit.current() == i ) {
			    if ( (bool)i->selected != d->select ) {
				i->setSelected( d->select );
				changed = TRUE;
			    }
			    triggerUpdate();
			    break;
			}
			if ( !down && lit.current() == oldCurrent ) {
			    oldCurrent->setSelected( d->select );
			    triggerUpdate();
			    break;
			}
			if ( (bool)lit.current()->selected != d->select ) {
			    lit.current()->setSelected( d->select );
			    changed = TRUE;
			}
		    }
		}
	    }
	    if ( changed )
		emit selectionChanged();
	}
    }

 emit_signals:

    if ( i && vp.x() + contentsX() < itemMargin() + ( i->depth() + ( rootIsDecorated() ? 1 : 0 ) ) * treeStepSize() )
	i = 0;
    d->pressedItem = i;

    int c = 0;
    if ( !iconView() )
	c = d->h->mapToLogical( d->h->cellAt( vp.x() ) );

    emit pressed( i );
    emit pressed( i, viewport()->mapToGlobal( vp ), c );
    emit mouseButtonPressed( e->button(), i, viewport()->mapToGlobal( vp ),
	i ? c : 0 );

    if ( e->button() == RightButton && i == d->pressedItem ) {
	if ( !i ) {
	    clearSelection();
	    emit rightButtonPressed( 0, viewport()->mapToGlobal( vp ), -1 );
	    return;
	}

	emit rightButtonPressed( i, viewport()->mapToGlobal( vp ), c );
    }
}


/*!
  Processes mouse move events on behalf of the viewed widget.
*/
void CListView::contentsMouseReleaseEvent( QMouseEvent * e )
{
    bool emitClicked = !d->pressedItem || d->buttonDown;
    d->buttonDown = FALSE;
    // delete and disconnect autoscroll timer, if we have one
    if ( d->scrollTimer ) {
	disconnect( d->scrollTimer, SIGNAL(timeout()),
		    this, SLOT(doAutoScroll()) );
	d->scrollTimer->stop();
	delete d->scrollTimer;
	d->scrollTimer = 0;
    }

    if ( !e )
	return;

    if ( d->selectionMode == Extended &&
	 d->focusItem == d->pressedItem &&
	 d->pressedSelected && d->focusItem ) {
	bool block = signalsBlocked();
	blockSignals( TRUE );
	clearSelection();
	blockSignals( block );
	d->focusItem->setSelected( TRUE );
	emit selectionChanged();
    }

    QPoint vp = contentsToViewport(e->pos());
    CListViewItem *i = itemAt( vp );
    if ( i && vp.x() + contentsX() < itemMargin() + ( i->depth() + ( rootIsDecorated() ? 1 : 0 ) ) * treeStepSize() )
	i = 0;
    emitClicked = emitClicked && d->pressedItem == i;
    d->pressedItem = 0;

    if ( emitClicked ) {
	int c = 0;
	if ( !iconView() )
	    c = d->h->mapToLogical( d->h->cellAt( vp.x() ) );

	emit clicked( i );
	emit clicked( i, viewport()->mapToGlobal( vp ), c );
	emit mouseButtonClicked( e->button(), i, viewport()->mapToGlobal( vp ),
	    i ? c : -1 );

	if ( e->button() == RightButton ) {
	    if ( !i ) {
		clearSelection();
		emit rightButtonClicked( 0, viewport()->mapToGlobal( vp ), -1 );
		return;
	    }

	    emit rightButtonClicked( i, viewport()->mapToGlobal( vp ), c );
	}
    }
}


/*!
  Processes mouse double-click events on behalf of the viewed widget.
*/
void CListView::contentsMouseDoubleClickEvent( QMouseEvent * e )
{
    if ( !e )
	return;

    // ensure that the following mouse moves and eventual release is
    // ignored.
    d->buttonDown = FALSE;

    if ( d->ignoreDoubleClick ) {
	d->ignoreDoubleClick = FALSE;
	return;
    }

    QPoint vp = contentsToViewport(e->pos());

    CListViewItem * i = itemAt( vp );

    if ( !i )
	return;

    if (!iconView())
    {
	if ( !i->isOpen() ) {
	    if ( i->isExpandable() || i->childCount() )
		setOpen( i, TRUE );
	} else {
	    setOpen( i, FALSE );
	}
    }

    emit doubleClicked( i );
}


/*!
  Processes mouse move events on behalf of the viewed widget.
*/
void CListView::contentsMouseMoveEvent( QMouseEvent * e )
{
    if ( !e )
	return;

    bool needAutoScroll = FALSE;

    QPoint vp = contentsToViewport(e->pos());

    CListViewItem * i = itemAt( vp );
    if ( i != d->highlighted ) {
	if ( i ) {
	    emit onItem( i );
	} else {
	    emit onViewport();
	}
	d->highlighted = i;
    }

    if ( !d->buttonDown || e->state() == NoButton )
	return;

    // check, if we need to scroll
    if ( vp.y() > visibleHeight() || vp.y() < 0 )
	needAutoScroll = TRUE;

    // if we need to scroll and no autoscroll timer is started,
    // connect the timer
    if ( needAutoScroll && !d->scrollTimer ) {
	d->scrollTimer = new QTimer( this );
	connect( d->scrollTimer, SIGNAL(timeout()),
		 this, SLOT(doAutoScroll()) );
	d->scrollTimer->start( 100, FALSE );
	// call it once manually
	doAutoScroll();
    }

    // if we don't need to autoscroll
    if ( !needAutoScroll ) {
	// if there is a autoscroll timer, delete it
	if ( d->scrollTimer ) {
	    disconnect( d->scrollTimer, SIGNAL(timeout()),
			this, SLOT(doAutoScroll()) );
	    d->scrollTimer->stop();
	    delete d->scrollTimer;
	    d->scrollTimer = 0;
	}
	// call this to select an item
	doAutoScroll();
    }
}


/*!  This slot handles auto-scrolling when the mouse button is pressed
and the mouse is outside the widget.
*/

void CListView::doAutoScroll()
{
    if ( !d->focusItem )
	return;

    QPoint pos = QCursor::pos();
    pos = viewport()->mapFromGlobal( pos );

    bool down = pos.y() > itemRect( d->focusItem ).y();
    bool right = pos.x() > itemRect( d->focusItem ).x();

    int g = pos.y() + contentsY();
    int g2 = pos.x();

    if ( down && pos.y() > height()  )
	g = height() + contentsY();
    else if ( pos.y() < 0 )
	g = contentsY();

    if ( right && pos.x() > width()  )
	g2 = width();
    else if ( pos.x() < 0 )
	g2 = 0;

    CListViewItem *c = d->focusItem, *old = 0;
    if ( down ) {
	int y = itemRect( d->focusItem ).y() + contentsY();
	while( c && y + c->height() <= g ) {
	    y += c->height();
	    old = c;
	    c = c->itemBelow();
	}
	if ( !c && old )
	    c = old;
    } else {
	int y = itemRect( d->focusItem ).y() + contentsY();
	while( c && y >= g ) {
	    old = c;
	    c = c->itemAbove();
	    if ( c )
		y -= c->height();
	}
	if ( !c && old )
	    c = old;
    }

    if (iconView())
    {
        if ( right ) {
            int x = itemRect( d->focusItem ).x();
            while( c && x + ICON_VIEW_WIDTH <= g2 ) {
                x += ICON_VIEW_WIDTH;
                old = c;
                c = c->itemRight();
            }
            if ( !c && old )
                c = old;
        } else {
            int x = itemRect( d->focusItem ).x();
            while( c && x >= g2 ) {
                old = c;
                c = c->itemLeft();
                if ( c )
                    x -= ICON_VIEW_WIDTH;
            }
            if ( !c && old )
                c = old;
        }
    }

    if ( !c || c == d->focusItem )
	return;

    if ( d->focusItem ) {
	if ( d->selectionMode == Multi ) {
	    // also (de)select the ones in between
	    CListViewItem * b = d->focusItem;
	    bool down = ( itemPos( c ) > itemPos( b ) );
            if (iconView())
            {
                bool right = ( down || itemRect(c).x() > itemRect(b).x() );
                while( b && b != c )
                {
		    if ( b->isSelectable() )
		        setSelected( b, d->select );

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
	        while( b && b != c ) {
		    if ( b->isSelectable() )
		        setSelected( b, d->select );
		    b = down ? b->itemBelow() : b->itemAbove();
	        }
            }
	    if ( c->isSelectable() )
		setSelected( c, d->select );
	} else if ( d->selectionMode == Extended ) {
	    if ( d->focusItem == d->pressedItem && d->pressedSelected ) {
		d->pressedItem = 0;
		bool block = signalsBlocked();
		blockSignals( TRUE );
		clearSelection();
		blockSignals( block );
		c->setSelected( TRUE );
		emit selectionChanged();
	    } else {
		// also (de)select the ones in between
		CListViewItem * b = d->focusItem;
		bool down = ( itemPos( c ) > itemPos( b ) );
		while( b && b != c ) {
		    if ( b->isSelectable() )
			setSelected( b, d->select );
		    b = down ? b->itemBelow() : b->itemAbove();
		}
		if ( c->isSelectable() )
		    setSelected( c, d->select );
	    }
        }
    }

    setCurrentItem( c );
    d->visibleTimer->start( 1, TRUE );
}

/*!\reimp
*/

void CListView::focusInEvent( QFocusEvent *e )
{
    d->buttonDown = FALSE;
    if ( d->focusItem )
	repaintItem( d->focusItem );
    else if ( firstChild() && e->reason() != QFocusEvent::Mouse ) {
	d->focusItem = firstChild();
	emit currentChanged( d->focusItem );
	repaintItem( d->focusItem );
    }
}


/*!\reimp
*/

void CListView::focusOutEvent( QFocusEvent * )
{
    if ( d->focusItem )
	repaintItem( d->focusItem );
}


/*!\reimp
*/

void CListView::keyPressEvent( QKeyEvent * e )
{
    if ( !e || !firstChild() || !hasFocus() )
	return; // subclass bug

    CListViewItem* oldCurrent = currentItem();
    if ( !oldCurrent ) {
	setCurrentItem( firstChild() );
	if ( d->selectionMode == Single )
	    setSelected( firstChild(), TRUE );
	return;
    }

    CListViewItem * i = currentItem();
    CListViewItem *old = i;

    if ( isMultiSelection() && i->isSelectable() && e->ascii() == ' ' ) {
	setSelected( i, !i->isSelected() );
	d->currentPrefix.truncate( 0 );
	return;
    }

    QRect r( itemRect( i ) );
    CListViewItem * i2;

    bool singleStep = FALSE;
    bool selectCurrent = TRUE;

    switch( e->key() ) {
    case Key_Backspace:
    case Key_Delete:
	d->currentPrefix.truncate( 0 );
	break;
    case Key_Enter:
    case Key_Return:
	d->currentPrefix.truncate( 0 );
	if (!iconView())
	{
	    if ( i && !i->isSelectable() &&
	       ( i->childCount() || i->isExpandable() || i->isOpen() ) ) {
		i->setOpen( !i->isOpen() );
		return;
	    }
	}
	e->ignore();
	emit returnPressed( currentItem() );
	// do NOT accept.  QDialog.
	return;
    case Key_Down:
	selectCurrent = FALSE;
	i = i->itemBelow();
	d->currentPrefix.truncate( 0 );
	singleStep = TRUE;
	break;
    case Key_Up:
	selectCurrent = FALSE;
	i = i->itemAbove();
	d->currentPrefix.truncate( 0 );
	singleStep = TRUE;
	break;
    case Key_Home:
	selectCurrent = FALSE;
	i = firstChild();
	d->currentPrefix.truncate( 0 );
	break;
    case Key_End:
	selectCurrent = FALSE;
	i = firstChild();
	while ( i->nextSibling() )
	    i = i->nextSibling();
	while ( i->itemBelow() )
	    i = i->itemBelow();
	d->currentPrefix.truncate( 0 );
	break;
    case Key_Next:
	selectCurrent = FALSE;
	if (iconView())
 	    i2 = itemAt( QPoint( itemRect(i).x(), visibleHeight()-1 ) );
	else
	    i2 = itemAt( QPoint( 0, visibleHeight()-1 ) );
	if ( !i2 )
	    i2 = i;

	if ( i2 == i || !r.isValid() ||
	     visibleHeight() <= itemRect( i ).bottom() ) {
	    if ( i2 )
		i = i2;
	    int left = visibleHeight();
	    if ( left < i2->height() )
	    {
		i2 = i->itemBelow();
		if ( i2 )
		    i = i2;
	    }
	    while( (i2 = i->itemBelow()) != 0 && left > i2->height() ) {
		left -= i2->height();
		i = i2;
	    }
	} else {
	    i = i2;
	}
	d->currentPrefix.truncate( 0 );
	break;
    case Key_Prior:
	selectCurrent = FALSE;
	if (iconView())
	    i2 = itemAt( QPoint( itemRect(i).x(), 0 ) );
	else
	    i2 = itemAt( QPoint( 0, 0 ) );
	if ( i == i2 || !r.isValid() || r.top() <= 0 ) {
	    if ( i2 )
		i = i2;
	    int left = visibleHeight();
	    if ( i2 != 0 && left < i2->height() )
	    {
		i2 = i->itemAbove();
		if ( i2 )
		    i = i2;
	    }
	    while( (i2 = i->itemAbove()) != 0 && left > i2->height() ) {
		left -= i2->height();
		i = i2;
	    }
	} else {
	    i = i2;
	}
	d->currentPrefix.truncate( 0 );
	break;
    case Key_Plus:
	d->currentPrefix.truncate( 0 );
	if ( !iconView() && !i->isOpen() && (i->isExpandable() || i->childCount()) )
	    setOpen( i, TRUE );
	else
	    return;
	break;
    case Key_Right:
	d->currentPrefix.truncate( 0 );
	if (iconView())
	    i = i->itemRight();
	else
	{
	    if ( i->isOpen() && i->childItem ) {
	        i = i->childItem;
	    } else if ( !i->isOpen() && (i->isExpandable() || i->childCount()) ) {
	        setOpen( i, TRUE );
	    } else if ( contentsX() + visibleWidth() < contentsWidth() ) {
	        horizontalScrollBar()->addLine();
	        return;
	    } else {
	        return;
	    }
        }
	break;
    case Key_Minus:
	d->currentPrefix.truncate( 0 );
	if ( !iconView() && i->isOpen() )
	    setOpen( i, FALSE );
	else
	    return;
	break;
    case Key_Left:
	d->currentPrefix.truncate( 0 );
	if (iconView())
	    i = i->itemLeft();
	else
	{
	    if ( i->isOpen() ) {
	        setOpen( i, FALSE );
	    } else if ( i->parentItem && i->parentItem != d->r ) {
	        i = i->parentItem;
	    } else if ( contentsX() ) {
	        horizontalScrollBar()->subtractLine();
	        return;
	    } else {
	        return;
	    }
	}
	break;
    case Key_Space:
	activatedByClick = FALSE;
	i->activate();
	d->currentPrefix.truncate( 0 );
	break;
    case Key_Escape:
	e->ignore(); // For QDialog
	return;
    default:
	if ( e->text().length() > 0 && e->text()[ 0 ].isPrint() ) {
	    selectCurrent = FALSE;
	    QString input( d->currentPrefix );
	    CListViewItem * keyItem = i;
	    QTime now( QTime::currentTime() );
	    while( keyItem ) {
		// try twice, first with the previous string and this char
		input = input + e->text().lower();
		QString keyItemKey;
		QString prefix;
		bool tryFirst = TRUE;
		while( keyItem ) {
		    // Look for text in column 0, then left-to-right
		    keyItemKey = keyItem->text(0);
		    if (!iconView())
		    {
		        for (int col=0; col < d->h->count() && !keyItemKey; col++ )
			    keyItemKey = keyItem->text( d->h->mapToLogical(col) );
		    }
		    if ( !keyItemKey.isEmpty() ) {
			prefix = keyItemKey;
			prefix.truncate( input.length() );
			prefix = prefix.lower();
			if ( prefix == input ) {
			    d->currentPrefix = input;
			    d->currentPrefixTime = now;
			    i = keyItem;
				// nonoptimal double-break...
			    keyItem = 0;
			    input.truncate( 0 );
			    tryFirst = FALSE;
			}
		    }
		    if ( keyItem )
                    {
                        if (iconView())
                        {
                            if (keyItem == d->r)
                                keyItem = firstChild();
                            else
                                keyItem = keyItem->nextSibling();
                        }
                        else
                        {
			    keyItem = keyItem->itemBelow();
                        }
                    }
		    if ( !keyItem && tryFirst ) {
			keyItem = firstChild();
			tryFirst = FALSE;
		    }
		}
		// then, if appropriate, with just this character
		if ( input.length() > 1 &&
		     d->currentPrefixTime.msecsTo( now ) > 1500 ) {
		    input.truncate( 0 );
		    keyItem = d->r;
		}
	    }
	} else {
	    d->currentPrefix.truncate( 0 );
	    if ( e->state() & ControlButton ) {
		d->currentPrefix = QString::null;
		switch ( e->key() ) {
		case Key_A:
		    selectAll( TRUE );
		    break;
		}
	    }
	    e->ignore();
	    return;
	}
    }

    if ( !i )
	return;

    if ( !( e->state() & ShiftButton ) || !d->selectAnchor )
	d->selectAnchor = i;

    setCurrentItem( i );
    if ( i->isSelectable() ) {
	handleItemChange( old, e->state() & ShiftButton, e->state() & ControlButton );
    }

    if ( d->focusItem && !d->focusItem->isSelected() && d->selectionMode == Single && selectCurrent )
	setSelected( d->focusItem, TRUE );

    if ( singleStep )
	d->visibleTimer->start( 1, TRUE );
    else
	ensureItemVisible( i );
}


/*!  Returns a pointer to the CListViewItem at \a viewPos.  Note
  that \a viewPos is in the coordinate system of viewport(), not in
  the listview's own, much larger, coordinate system.

  itemAt() returns 0 if there is no such item.

  Note, that you also get the pointer to the item if \a viewPos points onto the
  root decoration (see setRootIsDecorated()) of the item. To check if
  \a viewPos is on the root decoration of the item or not, you can do something
  like

  \code
  CListViewItem *i = itemAt( p );
  if ( i ) {
      if ( p.x() > header()->cellPos( header()->mapToActual( 0 ) ) +
	     treeStepSize() * ( i->depth() + ( rootIsDecorated() ? 1 : 0) ) + itemMargin() ||
	     p.x() < header()->cellPos( header()->mapToActual( 0 ) ) ) {
          ; // p is not not in root decoration
      else
          ; // p is in the root decoration
  }
  \endcode

  This might be interesting if you use this method to find out where the user
  clicked and if you e.g. want to start a drag (which you do not want to do if the
  user clicked onto the root decoration of an item)

  \sa itemPos() itemRect()
*/

CListViewItem * CListView::itemAt( const QPoint & viewPos ) const
{
    if (iconView())
    {
        return itemAtIconView(viewPos);
    }
    else
    {
        return itemAtNormalView(viewPos);
    }
}

CListViewItem * CListView::itemAtIconView(const QPoint & viewPos) const
{
	if (!d->drawables || d->drawables->isEmpty())
	{
		buildDrawableList();
	}
	
	CListViewPrivate::DrawableItem * c = d->drawables->first();
	int x = viewPos.x() + contentsX();
	int y = viewPos.y() + contentsY();
	
	while(c && c->i &&
				(c->y > y ||
				c->x > x))
	{
		c = d->drawables->next();
	}
	
	if (c &&
			c->y <= y && c->y + c->i->height() > y &&
			c->x <= x && c->x + ICON_VIEW_WIDTH > x)
	{
		return c->i;
	}
	else
	{
		return 0;
	}
}


/*!  Returns a pointer to the CListViewItem at \a viewPos.  Note
  that \a viewPos is in the coordinate system of viewport(), not in
  the listview's own, much larger, coordinate system.

  itemAt() returns 0 if there is no such item.

  \sa itemPos() itemRect()
*/

CListViewItem * CListView::itemAtNormalView(const QPoint & viewPos) const
{
    if ( viewPos.x() > contentsWidth() - contentsX() )
	return 0;

    if ( !d->drawables || d->drawables->isEmpty() )
	buildDrawableList();

    CListViewPrivate::DrawableItem * c = d->drawables->first();
    int g = viewPos.y() + contentsY();

    while( c && c->i && c->y + c->i->height() <= g )
	c = d->drawables->next();

    CListViewItem *i = (c && c->y <= g) ? c->i : 0;
    return i;
}


/*!  Returns the y coordinate of \a item in the list view's
  coordinate system.  This functions is normally much slower than
  itemAt(), but it works for all items, while itemAt() normally works
  only for items on the screen.

  This is a thin wrapper around CListViewItem::itemPos().

  \sa itemAt() itemRect()
*/

int CListView::itemPos( const CListViewItem * item )
{
    return item ? item->itemPos() : 0;
}


/*!
  Sets the list view to multi-selection mode if \a enable is TRUE,
  and to single-selection mode if \a enable is FALSE.

  If you enable multi-selection mode, it's possible to specify
  if this mode should be \a extended or not. Extended means, that the
  user can only select multiple items when pressing the Shift
  or Control button at the same time.

  \sa isMultiSelection()
*/

void CListView::setMultiSelection( bool enable )
{
    if ( !enable )
	d->selectionMode = CListView::Single;
    else if ( !isMultiSelection() )
	d->selectionMode = CListView::Multi;
}



/*!
  Returns TRUE if this list view is in multi-selection mode and
  FALSE if it is in single-selection mode.

  \sa setMultiSelection()
*/

bool CListView::isMultiSelection() const
{
    return d->selectionMode != CListView::Single;
}

/*!
  Sets the list view's selection mode, which may be one of
  \c Single (the default), \c Extended, \c Multi or \c NoSelection.

  \sa selectionMode()
 */

void CListView::setSelectionMode( SelectionMode mode )
{
    setMultiSelection( isMultiSelection() );
    d->selectionMode = mode;
}

/*!
  Returns the selection mode of the list view.  The initial mode is \c Single.

  \sa setSelectionMode(), isMultiSelection(), setMultiSelection()
 */
CListView::SelectionMode CListView::selectionMode() const
{
    return d->selectionMode;
}


/*!  Sets \a item to be selected if \a selected is TRUE, and to be not
  selected if \a selected is FALSE.

  If the list view is in single-selection mode and \a selected is
  TRUE, the currently selected item is unselected and \a item made
  current.  Unlike CListViewItem::setSelected(), this function updates
  the list view as necessary and emits the selectionChanged() signals.

  \sa isSelected() setMultiSelection() isMultiSelection() setCurrentItem()
*/

void CListView::setSelected( CListViewItem * item, bool selected )
{
    if ( !item || item->isSelected() == selected ||
	 !item->isSelectable() || selectionMode() == NoSelection )
	return;

    bool emitHighlighted = FALSE;
    if ( selectionMode() == Single && d->focusItem != item ) {
	CListViewItem *o = d->focusItem;
	if ( d->focusItem && d->focusItem->selected )
	    d->focusItem->setSelected( FALSE );
	d->focusItem = item;
	if ( o )
	    repaintItem( o );
	emitHighlighted = TRUE;
    }

    item->setSelected( selected );

    repaintItem( item );

    if ( d->selectionMode == Single && selected )
	emit selectionChanged( item );
    emit selectionChanged();

    if ( emitHighlighted )
	emit currentChanged( d->focusItem );
}


/*! Sets all items to be not selected, updates the list view as
necessary and emits the selectionChanged() signals.  Note that for
multi-selection list views, this function needs to iterate over \e all
items.

\sa setSelected(), setMultiSelection()
*/

void CListView::clearSelection()
{
    selectAll( FALSE );
}

/*!
  If \a select is TRUE, all items get selected, else all get unselected.
  This works only in the selection modes Multi and Extended. In
  Single and NoSelection mode the selection of the current item is
  just set to \a select.
*/

void CListView::selectAll( bool select )
{
    if ( isMultiSelection() ) {
	bool b = signalsBlocked();
	blockSignals( TRUE );
	bool anything = FALSE;
	CListViewItem * i = firstChild();
	QStack<CListViewItem> s;
	while ( i ) {
	    if ( i->childItem )
		s.push( i->childItem );
	    if ( (bool)i->selected != select ) {
		i->setSelected( select );
		anything = TRUE;
		repaintItem( i );
	    }
	    i = i->siblingItem;
	    if ( !i )
		i = s.pop();
	}
	blockSignals( b );
	if ( anything )
	    emit selectionChanged();
    } else if ( d->focusItem ) {
	CListViewItem * i = d->focusItem;
	setSelected( i, select );
    }
}

/*!
  Inverts the selection. Works only in Multi and Extended selection mode.
*/

void CListView::invertSelection()
{
    if ( d->selectionMode == Single ||
	 d->selectionMode == NoSelection )
	return;

    bool b = signalsBlocked();
    blockSignals( TRUE );
    CListViewItemIterator it( this );
    for ( ; it.current(); ++it )
	it.current()->setSelected( !it.current()->isSelected() );
    blockSignals( b );
    emit selectionChanged();
}


/*!  Returns \link CListViewItem::isSelected() i->isSelected(). \endlink

  Provided only because CListView provides setSelected() and trolls
  are neat creatures and like neat, orthogonal interfaces.
*/

bool CListView::isSelected( const CListViewItem * i ) const
{
    return i ? i->isSelected() : FALSE;
}


/*!  Returns a pointer to the selected item, if the list view is in
single-selection mode and an item is selected.

If no items are selected or the list view is in multi-selection mode
this function returns 0.

\sa setSelected() setMultiSelection()
*/

CListViewItem * CListView::selectedItem() const
{
    if ( d->selectionMode != Single )
	return 0;
    if ( d->focusItem && d->focusItem->isSelected() )
	return d->focusItem;
    return 0;
}


/*!  Sets \a i to be the current highlighted item and repaints
  appropriately.  This highlighted item is used for keyboard
  navigation and focus indication; it doesn't mean anything else.

  \sa currentItem()
*/

void CListView::setCurrentItem( CListViewItem * i )
{
    if ( !i && firstChild() &&
	 ( firstChild()->firstChild() || firstChild()->nextSibling() ) )
	return;

    CListViewItem * prev = d->focusItem;
    d->focusItem = i;

    if ( i != prev ) {
	if ( i && d->selectionMode == Single ) {
	    bool changed = FALSE;
	    if ( prev && prev->selected ) {
		changed = TRUE;
		prev->setSelected( FALSE );
	    }
	    if ( i && !i->selected && d->selectionMode != NoSelection && i->isSelectable() ) {
		i->setSelected( TRUE );
		changed = TRUE;
		emit selectionChanged( i );
	    }
	    if ( changed )
		emit selectionChanged();
	}

	if ( i )
	    repaintItem( i );
	if ( prev )
	    repaintItem( prev );
	emit currentChanged( i );
    }
}


/*!  Returns a pointer to the currently highlighted item, or 0 if
  there isn't any.

  \sa setCurrentItem()
*/

CListViewItem * CListView::currentItem() const
{
    return d ? d->focusItem : 0;
}


/*!  Returns the rectangle on the screen \a i occupies in
  viewport()'s coordinates, or an invalid rectangle if \a i is a null
  pointer or is not currently visible.

  The rectangle returned does not include any children of the
  rectangle (ie. it uses CListViewItem::height() rather than
  CListViewItem::totalHeight()).  If you want the rectangle including
  children, you can use something like this code:

  \code
    QRect r( listView->itemRect( item ) );
    r.setHeight( (QCOORD)(QMIN( item->totalHeight(),
				listView->viewport->height() - r.y() ) ) )
  \endcode

  Note the way it avoids too-high rectangles.  totalHeight() can be
  much larger than the window system's coordinate system allows.

  itemRect() is comparatively slow.  It's best to call it only for
  items that are probably on-screen.
*/

QRect CListView::itemRect( const CListViewItem * i ) const
{
    if ( !d->drawables || d->drawables->isEmpty() )
	buildDrawableList();

    CListViewPrivate::DrawableItem * c = d->drawables->first();

    while( c && c->i && c->i != i )
	c = d->drawables->next();

    if ( c && c->i == i ) {
	int y = c->y - contentsY();
	if ( y + c->i->height() >= 0 &&
	     y < ((CListView *)this)->visibleHeight() )
	{
            if (iconView())
            {
                QRect r(c->x, y, ICON_VIEW_WIDTH, i->height());
                return r;
            }
            else
            {
	        QRect r( -contentsX(), y, d->h->width(), i->height() );
	        return r;
	    }
	}
    }

    return QRect( 0, 0, -1, -1 );
}


/*! \fn void CListView::doubleClicked( CListViewItem *item )

  This signal is emitted whenever an item is double-clicked.  It's
  emitted on the second button press, not the second button release.
  \a item is the listview item onto which the user did the double click.
*/


/*! \fn void CListView::returnPressed( CListViewItem * )

  This signal is emitted when enter or return is pressed.  The
  argument is currentItem().
*/


/*!  Set the list view to be sorted by \a column and to be sorted
  in ascending order if \a ascending is TRUE or descending order if it
  is FALSE.

  If \a column is -1, sorting is disabled and the user cannot sort
  columns by clicking on the column headers.
*/

void CListView::setSorting( int column, bool ascending )
{
    if ( column == -1 ) column = Unsorted;

    if ( d->sortcolumn == column && d->ascending == ascending )
	return;

    d->ascending = ascending;
    d->sortcolumn = column;
    if ( d->sortcolumn != Unsorted && d->sortIndicator )
	d->h->setSortIndicator( d->sortcolumn, d->ascending );
    triggerUpdate();
}


/*!  Changes the column the list view is sorted by (by using header). */

void CListView::changeSortColumn( int column )
{
    if ( d->sortcolumn != Unsorted ) {
	int lcol = d->h->mapToLogical( column );
	setSorting( lcol, d->sortcolumn == lcol ? !d->ascending : TRUE);
    }
}

/*!
  (Re)sorts the listview using the last sorting configuration (sort column
  and ascending/descending)
*/

void CListView::sort()
{
    if ( d->r )
	d->r->sort();
}

/*! Sets the advisory item margin which list items may use to \a m.

  The item margin defaults to one pixel and is the margin between the
  item's edges and the area where it draws its contents.
  CListViewItem::paintFocus() draws in the margin.

  \sa CListViewItem::paintCell()
*/

void CListView::setItemMargin( int m )
{
    if ( d->margin == m )
	return;
    d->margin = m;
    if ( isVisible() ) {
	if ( d->drawables )
	    d->drawables->clear();
	triggerUpdate();
    }
}

/*! Returns the advisory item margin which list items may use.

  \sa CListViewItem::paintCell() setItemMargin()
*/

int CListView::itemMargin() const
{
    return d->margin;
}


/*! \fn void CListView::rightButtonClicked( CListViewItem *, const QPoint&, int )

  This signal is emitted when the right button is clicked (ie. when
  it's released).  The arguments are the relevant CListViewItem (may
  be 0), the point in global coordinates and the relevant column (or -1 if the
  click was outside the list).
*/


/*! \fn void CListView::rightButtonPressed (CListViewItem *, const QPoint &, int)

  This signal is emitted when the right button is pressed.  Then
  arguments are the relevant CListViewItem (may be 0), the point in
  global coordinates and the relevant column (or -1 if the
  click was outside the list).
*/

/*!\reimp
*/
void CListView::styleChange( QStyle& old )
{
    reconfigureItems();
    QScrollView::styleChange( old );
}


/*!  \reimp
*/
void CListView::setFont( const QFont & f )
{
    d->h->setFont( f );
    QScrollView::setFont( f );
    reconfigureItems();
}


/*!\reimp
*/
void CListView::setPalette( const QPalette & p )
{
    d->h->setPalette( p );
    QScrollView::setPalette( p );
    reconfigureItems();
}


/*!  Ensures that setup() are called for all currently visible items,
  and that it will be called for currently invisible items as soon as
  their parents are opened.

  (A visible item, here, is an item whose parents are all open.  The
  item may happen to be offscreen.)

  \sa CListViewItem::setup()
*/

void CListView::reconfigureItems()
{
    d->fontMetricsHeight = fontMetrics().height();
    d->minLeftBearing = fontMetrics().minLeftBearing();
    d->minRightBearing = fontMetrics().minRightBearing();
    d->ellipsisWidth = fontMetrics().width( "..." ) * 2;
    d->r->setOpen( FALSE );
    d->r->setOpen( TRUE );
}

/*!
  Ensures the width mode of column \a c is updated according
  to the width of \a item.
*/

void CListView::widthChanged( const CListViewItem* item, int c )
{
    if ( c >= d->h->count() || iconView() )
	return;


    QFontMetrics fm = fontMetrics();
    int col = c < 0 ? 0 : c;
    while ( col == c || ( c < 0 && col < d->h->count() ) ) {
	if ( d->column[col]->wmode == Maximum ) {
	    int w = item->width( fm, this, col );
	    if ( col == 0 ) {
		int indent = treeStepSize() * item->depth();
		if ( rootIsDecorated() )
		    indent += treeStepSize();
		w += indent;
	    }
	    if ( w > columnWidth( col ) )
		setColumnWidth( col, w );
	}
	col++;
    }
}

/*!  Sets this list view to assume that the items show focus and
  selection state using all of their columns if \a enable is TRUE, or
  that they show it just using column 0 if \a enable is FALSE.

  The default is FALSE.

  Setting this to TRUE if it isn't necessary can cause noticeable
  flicker.

  \sa allColumnsShowFocus()
*/

void CListView::setAllColumnsShowFocus( bool enable )
{
    d->allColumnsShowFocus = enable;
}


/*!  Returns TRUE if the items in this list view indicate focus and
  selection state using all of their columns, else FALSE.

  \sa setAllColumnsShowFocus()
*/

bool CListView::allColumnsShowFocus() const
{
    return d->allColumnsShowFocus;
}


/*!  Returns the first item in this CListView.  You can use its \link
  CListViewItem::firstChild() firstChild() \endlink and \link
  CListViewItem::nextSibling() nextSibling() \endlink functions to
  traverse the entire tree of items.

  Returns 0 if there is no first item.

  \sa itemAt() itemBelow() itemAbove()
*/

CListViewItem * CListView::firstChild() const
{
    d->r->enforceSortOrder();
    return d->r->childItem;
}


/*!  Repaints this item on the screen, if it is currently visible. */

void CListViewItem::repaint() const
{
    listView()->repaintItem( this );
}


/*!  Repaints \a item on the screen, if \a item is currently visible.
  Takes care to avoid multiple repaints. */

void CListView::repaintItem( const CListViewItem * item ) const
{
    if ( !item )
	return;
    d->dirtyItemTimer->start( 0, TRUE );
    if ( !d->dirtyItems )
	d->dirtyItems = new QPtrDict<void>();
    d->dirtyItems->replace( (void *)item, (void *)item );
}



/*!
  \class CCheckListItem qlistview.h
  \brief The CCheckListItem class implements checkable list view items.

  There are three types of check list items: CheckBox, RadioButton and
  Controller.

  Checkboxes may be inserted at top level in the list view. A radio
  button must be child of a controller.
*/

/*! \enum CCheckListItem::Type

  This enum type defines the modes in which a CCheckListItem can be: <ul>
  <li> \c RadioButton -
  <li> \c CheckBox -
  <li> \c Controller -
  </ul>
*/

/* XPM */
static const char * const def_item_xpm[] = {
"16 16 4 1",
" 	c None",
".	c #000000000000",
"X	c #FFFFFFFF0000",
"o	c #C71BC30BC71B",
"                ",
"                ",
" ..........     ",
" .XXXXXXXX.     ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" .XXXXXXXX.oo   ",
" ..........oo   ",
"   oooooooooo   ",
"   oooooooooo   ",
"                ",
"                "};




static QPixmap *defaultIcon = 0;
static const int BoxSize = 16;


/*!
  Constructs a checkable item with parent \a parent, text \a text and type
  \a tt. Note that a RadioButton must be child of a Controller, otherwise
  it will not toggle.
 */
CCheckListItem::CCheckListItem( CCheckListItem *parent, const QString &text,
				Type tt )
    : CListViewItem( parent, text, QString::null )
{
    myType = tt;
    init();
    if ( myType == RadioButton ) {
	if ( parent->type() != Controller )
	    qWarning( "CCheckListItem::CCheckListItem(), radio button must be "
		     "child of a controller" );
	else
	    exclusive = parent;
    }
}

/*!
  Constructs a checkable item with parent \a parent, text \a text and type
  \a tt. Note that this item must not be a a RadioButton. Radio buttons must
  be children on a Controller.
 */
CCheckListItem::CCheckListItem( CListViewItem *parent, const QString &text,
				Type tt )
    : CListViewItem( parent, text, QString::null )
{
    myType = tt;
    if ( myType == RadioButton ) {
      qWarning( "CCheckListItem::CCheckListItem(), radio button must be "
	       "child of a CCheckListItem" );
    }
    init();
}

/*!
  Constructs a checkable item with parent \a parent, text \a text and type
  \a tt. Note that \a tt must not be RadioButton, if so
  it will not toggle.
 */
CCheckListItem::CCheckListItem( CListView *parent, const QString &text,
				Type tt )
    : CListViewItem( parent, text )
{
    myType = tt;
    if ( tt == RadioButton )
	qWarning( "CCheckListItem::CCheckListItem(), radio button must be "
		 "child of a CCheckListItem" );
    init();
}

/*!
  Constructs a Controller item with parent \a parent, text \a text and pixmap
  \a p.
 */
CCheckListItem::CCheckListItem( CListView *parent, const QString &text,
				const QPixmap & p )
    : CListViewItem( parent, text )
{
    myType = Controller;
    setPixmap( 0, p );
    init();
}

/*!
  Constructs a Controller item with parent \a parent, text \a text and pixmap
  \a p.
 */
CCheckListItem::CCheckListItem( CListViewItem *parent, const QString &text,
				const QPixmap & p )
    : CListViewItem( parent, text )
{
    myType = Controller;
    setPixmap( 0, p );
    init();
}

void CCheckListItem::init()
{
    on = FALSE;
    reserved = 0;
    if ( !defaultIcon )
	defaultIcon = new QPixmap( (const char **)def_item_xpm );
    if ( myType == Controller ) {
	if ( !pixmap(0) )
	    setPixmap( 0, *defaultIcon );
    }
    exclusive = 0;
}


/*! \fn CCheckListItem::Type CCheckListItem::type() const

  Returns the type of this item.
*/

/*! \fn  bool CCheckListItem::isOn() const
  Returns TRUE if this item is toggled on, FALSE otherwise.
*/


/*! \fn QString CCheckListItem::text() const

  Returns the text of this item.
*/


/*!
  If this is a Controller that has RadioButton children, turn off the
  child that is on.
 */
void CCheckListItem::turnOffChild()
{
    if ( myType == Controller && exclusive )
	exclusive->setOn( FALSE );
}

/*!
  Toggle checkbox, or set radio button on.
 */
void CCheckListItem::activate()
{
    QPoint pos;
    if ( activatedPos( pos ) ) {
	//ignore clicks outside the box
	if ( pos.x() < 0 || pos.x() >= BoxSize )
	    return;
    }
    if ( myType == CheckBox ) {
	setOn( !on );
    } else if ( myType == RadioButton ) {
	setOn( TRUE );
    }
}

/*!
  Sets this button on if \a b is TRUE, off otherwise. Maintains radio button
  exclusivity.
 */
void CCheckListItem::setOn( bool b  )
{
    if ( listView() && !listView()->isEnabled() )
	return;

    if ( b == on )
	return;
    if ( myType == CheckBox ) {
	on = b;
	stateChange( b );
    } else if ( myType == RadioButton ) {
	if ( b ) {
	    if ( exclusive && exclusive->exclusive != this )
		exclusive->turnOffChild();
	    on = TRUE;
	    if ( exclusive )
		exclusive->exclusive = this;
	} else {
	    if ( exclusive && exclusive->exclusive == this )
		exclusive->exclusive = 0;
	    on = FALSE;
	}
	stateChange( b );
    }
    repaint();
}


/*!
  This virtual function is called when the item changes its on/off state.
 */
void CCheckListItem::stateChange( bool )
{
}

/*!
  Performs setup.
 */
void CCheckListItem::setup()
{
    CListViewItem::setup();
    int h = height();
    h = QMAX( BoxSize, h );
    setHeight( h );
}

/*!
  \reimp
 */

int CCheckListItem::width( const QFontMetrics& fm, const CListView* lv, int
column) const {
    int r = CListViewItem::width( fm, lv, column );
    if ( column == 0 ) {
	r += lv->itemMargin();
	if ( myType == Controller && pixmap( 0 ) ) {
	    //	     r += 0;
	} else {
	    r += BoxSize + 4;
	}
    }
    return r;
}

/*!
  Paints this item.
 */
void CCheckListItem::paintCell( QPainter * p, const QColorGroup & cg,
			       int column, int width, int align )
{
    if ( !p )
	return;

    p->fillRect( 0, 0, width, height(), cg.brush( QColorGroup::Base ) );

    if ( column != 0 ) {
	// The rest is text, or for subclasses to change.
	CListViewItem::paintCell( p, cg, column, width, align );
	return;
    }

    CListView *lv = listView();
    if ( !lv )
	return;
    int marg = lv->itemMargin();
    int r = marg;

    bool winStyle = lv->style() == WindowsStyle;

    if ( myType == Controller ) {
	if ( !pixmap( 0 ) )
	    r += BoxSize + 4;
    } else {
	ASSERT( lv ); //###
	//	QFontMetrics fm( lv->font() );
	//	int d = fm.height();
	int x = 0;
	int y = (height() - BoxSize) / 2;
	//	p->setPen( QPen( cg.text(), winStyle ? 2 : 1 ) );
	if ( myType == CheckBox ) {
	    p->setPen( QPen( cg.text(), 2 ) );
	    p->drawRect( x+marg, y+2, BoxSize-4, BoxSize-4 );
	    /////////////////////
	    x++;
	    y++;
	    if ( on ) {
		QPointArray a( 7*2 );
		int i, xx, yy;
		xx = x+1+marg;
		yy = y+5;
		for ( i=0; i<3; i++ ) {
		    a.setPoint( 2*i,   xx, yy );
		    a.setPoint( 2*i+1, xx, yy+2 );
		    xx++; yy++;
		}
		yy -= 2;
		for ( i=3; i<7; i++ ) {
		    a.setPoint( 2*i,   xx, yy );
		    a.setPoint( 2*i+1, xx, yy+2 );
		    xx++; yy--;
		}
		p->drawLineSegments( a );
	    }
	    ////////////////////////
	} else { //radio button look
	    if ( winStyle ) {
#define QCOORDARRLEN(x) sizeof(x)/(sizeof(QCOORD)*2)

		static QCOORD pts1[] = {		// dark lines
		    1,9, 1,8, 0,7, 0,4, 1,3, 1,2, 2,1, 3,1, 4,0, 7,0, 8,1, 9,1 };
		static QCOORD pts2[] = {		// black lines
		    2,8, 1,7, 1,4, 2,3, 2,2, 3,2, 4,1, 7,1, 8,2, 9,2 };
		static QCOORD pts3[] = {		// background lines
		    2,9, 3,9, 4,10, 7,10, 8,9, 9,9, 9,8, 10,7, 10,4, 9,3 };
		static QCOORD pts4[] = {		// white lines
		    2,10, 3,10, 4,11, 7,11, 8,10, 9,10, 10,9, 10,8, 11,7,
		    11,4, 10,3, 10,2 };
		// static QCOORD pts5[] = {		// inner fill
		//    4,2, 7,2, 9,4, 9,7, 7,9, 4,9, 2,7, 2,4 };
		//QPointArray a;
		//	p->eraseRect( x, y, w, h );

		p->setPen( cg.text() );
		QPointArray a( QCOORDARRLEN(pts1), pts1 );
		a.translate( x, y );
		//p->setPen( cg.dark() );
		p->drawPolyline( a );
		a.setPoints( QCOORDARRLEN(pts2), pts2 );
		a.translate( x, y );
		p->drawPolyline( a );
		a.setPoints( QCOORDARRLEN(pts3), pts3 );
		a.translate( x, y );
		//		p->setPen( black );
		p->drawPolyline( a );
		a.setPoints( QCOORDARRLEN(pts4), pts4 );
		a.translate( x, y );
		//			p->setPen( blue );
		p->drawPolyline( a );
		//		a.setPoints( QCOORDARRLEN(pts5), pts5 );
		//		a.translate( x, y );
		//	QColor fillColor = isDown() ? g.background() : g.base();
		//	p->setPen( fillColor );
		//	p->setBrush( fillColor );
		//	p->drawPolygon( a );
		if ( on     ) {
		    p->setPen( NoPen );
		    p->setBrush( cg.text() );
		    p->drawRect( x+5, y+4, 2, 4 );
		    p->drawRect( x+4, y+5, 4, 2 );
		}

	    } else { //motif
		p->setPen( QPen( cg.text() ) );
		QPointArray a;
		int cx = BoxSize/2 - 1;
		int cy = height()/2;
		int e = BoxSize/2 - 1;
		for ( int i = 0; i < 3; i++ ) { //penWidth 2 doesn't quite work
		    a.setPoints( 4, cx-e, cy, cx, cy-e,  cx+e, cy,  cx, cy+e );
		    p->drawPolygon( a );
		    e--;
		}
		if ( on ) {
		    p->setPen( QPen( cg.text()) );
		    QBrush   saveBrush = p->brush();
		    p->setBrush( cg.text() );
		    e = e - 2;
		    a.setPoints( 4, cx-e, cy, cx, cy-e,  cx+e, cy,  cx, cy+e );
		    p->drawPolygon( a );
		    p->setBrush( saveBrush );
		}
	    }
	}
	r += BoxSize + 4;
    }

    p->translate( r, 0 );
    CListViewItem::paintCell( p, cg, column, width - r, align );
}

/*!
  Draws the focus rectangle
*/
void CCheckListItem::paintFocus( QPainter *p, const QColorGroup & cg,
				 const QRect & r )
{
    bool intersect = TRUE;
    CListView *lv = listView();
    if ( lv && lv->header()->mapToActual( 0 ) != 0 ) {
	int xdepth = lv->treeStepSize() * ( depth() + ( lv->rootIsDecorated() ? 1 : 0) ) + lv->itemMargin();
	int p = lv->header()->cellPos( lv->header()->mapToActual( 0 ) );
	xdepth += p;
	intersect = r.intersects( QRect( p, r.y(), xdepth - p + 1, r.height() ) );
    }
    if ( myType != Controller && intersect ) {
	QRect rect( r.x() + BoxSize + 5, r.y(), r.width() - BoxSize - 5, r.height() );
	CListViewItem::paintFocus(p, cg, rect);
    } else {
	CListViewItem::paintFocus(p, cg, r);
    }
}

/*!
  Fills the rectangle. No decoration is drawn.
 */
void CCheckListItem::paintBranches( QPainter * p, const QColorGroup & cg,
			    int w, int, int h, GUIStyle)
{
    p->fillRect( 0, 0, w, h, cg.brush( QColorGroup::Base ) );
}


/*!\reimp
*/
QSize CListView::sizeHint() const
{
    //    This is as wide as CHeader::sizeHint() recommends and tall
    //    enough for perhaps 10 items.

    constPolish();
    if ( !isVisible() &&
	 (!d->drawables || d->drawables->isEmpty()) )
	// force the column widths to sanity, if possible
	buildDrawableList();

    QSize s( d->h->sizeHint() );
    s.setWidth( s.width() + style().scrollBarExtent().width() );
    s += QSize(frameWidth()*2,frameWidth()*2);
    CListViewItem * l = d->r;
    while( l && !l->height() )
	l = l->childItem ? l->childItem : l->siblingItem;

    if ( l && l->height() )
	s.setHeight( s.height() + 10 * l->height() );
    else
	s.setHeight( s.height() + 140 );

    if ( s.width() > s.height() * 3 )
	s.setHeight( s.width() / 3 );
    else if ( s.width() *3 < s.height() )
	s.setHeight( s.width() * 3 );

    return s;
}


/*!
  \reimp
*/

QSize CListView::minimumSizeHint() const
{
    //###should be implemented
    return QScrollView::minimumSizeHint();
}



/*!  Sets \a item to be open if \a open is TRUE and \a item is
  expandable, and to be closed if \a open is FALSE.  Repaints
  accordingly.

  Does nothing if \a item is not expandable.

  \sa CListViewItem::setOpen() CListViewItem::setExpandable()
*/

void CListView::setOpen( CListViewItem * item, bool open )
{
    if ( !item ||
	 item->isOpen() == open ||
	 (open && !item->childCount() && !item->isExpandable()) )
	return;

    item->setOpen( open );

    if ( d->drawables )
	d->drawables->clear();
    buildDrawableList();

    CListViewPrivate::DrawableItem * c = d->drawables->first();

    while( c && c->i && c->i != item )
	c = d->drawables->next();

    if ( c && c->i == item ) {
	d->dirtyItemTimer->start( 0, TRUE );
	if ( !d->dirtyItems )
	    d->dirtyItems = new QPtrDict<void>();
	while( c && c->i ) {
	    d->dirtyItems->insert( (void *)(c->i), (void *)(c->i) );
	    c = d->drawables->next();
	}
    }
}


/*!  Identical to \a item->isOpen().  Provided for completeness.

  \sa setOpen()
*/

bool CListView::isOpen( const CListViewItem * item ) const
{
    return item->isOpen();
}


/*!  Sets this list view to show open/close signs on root items if \a
  enable is TRUE, and to not show such signs if \a enable is FALSE.

  Open/close signs is a little + or - in windows style, an arrow in
  Motif style.
*/

void CListView::setRootIsDecorated( bool enable )
{
    if ( enable != (bool)d->rootIsExpandable ) {
	d->rootIsExpandable = enable;
	if ( isVisible() )
	    triggerUpdate();
    }
}


/*!  Returns TRUE if root items can be opened and closed by the user,
  FALSE if not.
*/

bool CListView::rootIsDecorated() const
{
    return d->rootIsExpandable;
}


/*!  Ensures that \a i is made visible, scrolling the list view
  vertically as required.

  \sa itemRect() QScrollView::ensureVisible()
*/

void CListView::ensureItemVisible( const CListViewItem * i )
{
    if ( !i )
	return;
    if ( d->r->maybeTotalHeight < 0 )
	updateGeometries();
    int y = itemPos( i );
    int h = i->height();
    ensureVisible( contentsX(), y + h / 2, 0, h / 2 );
}


/*! \fn QString CCheckListItem::text( int n ) const

  \reimp
*/

/*!  Returns a pointer to the CHeader object that manages this list
  view's columns.  Please don't modify the header behind the list
  view's back.

  Acceptable methods to call are:
  <ul>
    <li>void CHeader::setClickEnabled( bool, int logIdx = -1 );
    <li>void CHeader::setResizeEnabled( bool, int logIdx = -1 );
    <li>void CHeader::setMovingEnabled( bool );
  </ul>
*/

CHeader * CListView::header() const
{
    return d->h;
}


/*!  Returns the current number of parentless CListViewItem objects in
  this CListView, like CListViewItem::childCount() returns the number
  of child items for a CListViewItem.

  \sa CListViewItem::childCount()
*/

int CListView::childCount() const
{
    return d->r->childCount();
}


/*!  Moves this item to just after \a olderSibling.  \a olderSibling
  and this object must have the same parent.
*/

void CListViewItem::moveToJustAfter( CListViewItem * olderSibling )
{
    if ( parentItem && olderSibling &&
	 olderSibling->parentItem == parentItem && olderSibling != this ) {
	if ( parentItem->childItem == this ) {
	    parentItem->childItem = siblingItem;
	} else {
	    CListViewItem * i = parentItem->childItem;
	    while( i && i->siblingItem != this )
		i = i->siblingItem;
	    if ( i )
		i->siblingItem = siblingItem;
	}
	siblingItem = olderSibling->siblingItem;
	olderSibling->siblingItem = this;
    }
}


/*!  \reimp */

void CListView::showEvent( QShowEvent * )
{
    if ( d->drawables )
	d->drawables->clear();
    delete d->dirtyItems;
    d->dirtyItems = 0;
    d->dirtyItemTimer->stop();

    updateGeometries();
}


/*!  Returns the y coordinate of \a item in the list view's
  coordinate system.  This functions is normally much slower than
  CListView::itemAt(), but it works for all items, while
  CListView::itemAt() normally works only for items on the screen.

  \sa CListView::itemAt() CListView::itemRect() CListView::itemPos()
*/

int CListViewItem::itemPos() const
{

    int a = 0;
    if (listView()->iconView())
    {
        QListIterator<CListViewPrivate::DrawableItem>
            it(*(listView()->d->drawables));
        CListViewPrivate::DrawableItem * current = 0;
        CListViewPrivate::DrawableItem * found = 0;

        while ((current = it.current()) != 0)
        {
            ++it;
            if (current->i == this)
                found = current;
        }
        if (found)
            a = found->y;
    }
    else
    {
        QStack<CListViewItem> s;
        CListViewItem * i = (CListViewItem *)this;
        while( i ) {
            s.push( i );
            i = i->parentItem;
        }

        CListViewItem * p = 0;
        while( s.count() ) {
            i = s.pop();
            if ( p ) {
                if ( !p->configured ) {
                    p->configured = TRUE;
                    p->setup(); // ### virtual non-const function called in const
                }
                a += p->height();
                CListViewItem * s = p->firstChild();
                if (listView()->iconView())
                {
                }
                else
                {
                    while( s && s != i ) {
                        a += s->totalHeight();
                        s = s->nextSibling();
                    }
                }
            }
            p = i;
        }
    }

    return a;
}


/*!\obsolete

  This function has been renamed takeItem().
*/

void CListView::removeItem( CListViewItem * i )
{
    takeItem( i );
}

/*!  Removes \a i from the list view; \a i must be a top-level item.
  The warnings regarding CListViewItem::takeItem( i ) apply to this
  function too.

  \sa CListViewItem::takeItem() (important) insertItem()
*/
void CListView::takeItem( CListViewItem * i )
{
    d->r->takeItem( i );
}


/**********************************************************************
 *
 * Class CListViewItemIterator
 *
 **********************************************************************/


/*! \class CListViewItemIterator qlistview.h

  \brief The CListViewItemIterator class provides an iterator for collections of CListViewItems

  Construct an instance of a CListViewItemIterator with either a
  CListView* or a CListViewItem* as argument, to operate on the tree
  of CListViewItems.

  A CListViewItemIterator iterates over all items of a listview. This means ++it makes always
  the first child of the current item the new current one. If there is no child, the next sibling
  gets the new current item, and if there is no next sibling, the next sibling of the parent is
  set to current.

  Example:

  Often you want to get all items, which were selected by a user. Here is
  an example which does this and stores the pointers to all selected items
  in a CList.

  \code

  // Somewhere a listview is generated like this
  CListView *lv = new CListView(this);
  // Enable multiselection
  lv->setMultiSelection( TRUE );

  // Insert the items here

  // ...

  // This function is called to get a list of the selected items of a listview
  QList<CListViewItem> * getSelectedItems( CListView *lv ) {
    if ( !lv )
      return 0;

    // Create the list
    QList<CListViewItem> *lst = new QList<CListViewItem>;
    lst->setAutoDelete( FALSE );

    // Create an iterator and give the listview as argument
    CListViewItemIterator it( lv );
    // iterate through all items of the listview
    for ( ; it.current(); ++it ) {
      if ( it.current()->isSelected() )
	lst->append( it.current() );
    }

    return lst;
  }

  \endcode

  Using a CListViewItemIterator is a convinient way to traverse the
  tree of CListViewItems of a CListView. It makes especially operating
  on a hirarchical CListView easy.

  Also, multiple CListViewItemIterators can operate on the tree of
  CListViewItems.  A CListView knows about all iterators which are
  operating on its CListViewItems.  So when a CListViewItem gets
  removed, all iterators that point to this item get updated and point
  to the new current item after that.

  \sa CListView, CListViewItem
*/

/*!  Constructs an empty iterator. */

CListViewItemIterator::CListViewItemIterator()
    : curr( 0 ), listView( 0 )
{
}

/*! Constructs an iterator for the CListView of the \e item. The
  current iterator item is set to point on the \e item.
*/

CListViewItemIterator::CListViewItemIterator( CListViewItem *item )
    : curr( item ), listView( 0 )
{
    if ( item )
	listView = item->listView();
    addToListView();
}

/*! Constructs an iterator for the same CListView as \e it. The
  current iterator item is set to point on the current item of \e it.
*/

CListViewItemIterator::CListViewItemIterator( const CListViewItemIterator& it )
    : curr( it.curr ), listView( it.listView )
{
    addToListView();
}

/*! Constructs an iterator for the CListView \e lv. The current
  iterator item is set to point on the first child ( CListViewItem )
  of \e lv.
*/

CListViewItemIterator::CListViewItemIterator( CListView *lv )
    : curr( lv->firstChild() ), listView( lv )
{
    addToListView();
}

/*!  Assignment. Makes a copy of \e it and returns a reference to its
  iterator.
*/

CListViewItemIterator &CListViewItemIterator::operator=( const CListViewItemIterator &it )
{
    if ( listView ) {
	if ( listView->d->iterators->removeRef( this ) ) {
	    if ( listView->d->iterators->count() == 0 ) {
		delete listView->d->iterators;
		listView->d->iterators = 0;
	    }
	}
    }

    listView = it.listView;
    addToListView();
    curr = it.curr;

    return *this;
}

/*!
  Destroys the iterator.
*/

CListViewItemIterator::~CListViewItemIterator()
{
    if ( listView ) {
	if ( listView->d->iterators->removeRef( this ) ) {
	    if ( listView->d->iterators->count() == 0 ) {
		delete listView->d->iterators;
		listView->d->iterators = 0;
	    }
	}
    }
}

/*!
  Prefix ++ makes the next item in the CListViewItem tree of the
  CListView of the iterator the current item and returns it. If the
  current item was the last item in the CListView or null, null is
  returned.
*/

CListViewItemIterator &CListViewItemIterator::operator++()
{
    if ( !curr )
	return *this;

    CListViewItem *item = curr->firstChild();
    if ( item ) {
	curr = item;
	return *this;
    }

    item = curr->nextSibling();
    if ( item ) {
	curr = item;
	return *this;
    }

    CListViewItem *p = curr->parent();
    bool found = FALSE;
    while ( p ) {
	if ( p->nextSibling() ) {
	    curr = p->nextSibling();
	    found = TRUE;
	    break;
	}
	p = p->parent();
    }

    if ( !found )
	curr = 0;

    return *this;
}

/*!
  Postfix ++ makes the next item in the CListViewItem tree of the
  CListView of the iterator the current item and returns the item,
  which was the current one before.
*/

const CListViewItemIterator CListViewItemIterator::operator++( int )
{
    CListViewItemIterator oldValue = *this;
    ++( *this );
    return oldValue;
}

/*!
  Sets the current item to the item \e j positions after the current
  item in the CListViewItem hirarchie. If this item is beyond the last
  item, the current item is set to null.

  The new current item (or null, if the new current item is null) is returned.
*/

CListViewItemIterator &CListViewItemIterator::operator+=( int j )
{
    while ( curr && j-- )
	++( *this );

    return *this;
}

/*!
  Prefix -- makes the previous item in the CListViewItem tree of the
  CListView of the iterator the current item and returns it. If the
  current item was the last first in the CListView or null, null is
  returned.
*/

CListViewItemIterator &CListViewItemIterator::operator--()
{
    if ( !curr )
	return *this;

    if ( !curr->parent() ) {
	// we are in the first depth
       if ( curr->listView() ) {
	    if ( curr->listView()->firstChild() != curr ) {
		// go the previous sibling
		CListViewItem *i = curr->listView()->firstChild();
		while ( i && i->siblingItem != curr )
		    i = i->siblingItem;

		curr = i;

		if ( i && i->firstChild() ) {
		    // go to the last child of this item
		    CListViewItemIterator it( curr->firstChild() );
		    for ( ; it.current() && it.current()->parent(); ++it )
			curr = it.current();
		}

		return *this;
	    } else {
		// we are already the first child of the listview, so it's over
		curr = 0;
		return *this;
	    }
	} else
	    return *this;
    } else {
	CListViewItem *parent = curr->parent();

	if ( curr != parent->firstChild() ) {
	    // go to the previous sibling
	    CListViewItem *i = parent->firstChild();
	    while ( i && i->siblingItem != curr )
		i = i->siblingItem;

	    curr = i;

	    if ( i && i->firstChild() ) {
		// go to the last child of this item
		CListViewItemIterator it( curr->firstChild() );
		for ( ; it.current() && it.current()->parent() != parent; ++it )
		    curr = it.current();
	    }

	    return *this;
	} else {
	    // make our parent the current item
	    curr = parent;
	    return *this;
	}
    }
}

/*!  Postfix -- makes the previous item in the CListViewItem tree of
  the CListView of the iterator the current item and returns the item,
  which was the current one before.
*/

const CListViewItemIterator CListViewItemIterator::operator--( int )
{
    CListViewItemIterator oldValue = *this;
    --( *this );
    return oldValue;
}

/*!  Sets the current item to the item \e j positions before the
  current item in the CListViewItem hirarchie. If this item is above
  the first item, the current item is set to null.  The new current
  item (or null, if the new current item is null) is returned.
*/

CListViewItemIterator &CListViewItemIterator::operator-=( int j )
{
    while ( curr && j-- )
	--( *this );

    return *this;
}

/*!
  Returns a pointer to the current item of the iterator.
*/

CListViewItem *CListViewItemIterator::current() const
{
    return curr;
}

/*!
  Adds the iterator to the list of iterators of the iterator's CListViewItem.
*/

void CListViewItemIterator::addToListView()
{
    if ( listView ) {
	if ( !listView->d->iterators ) {
	    listView->d->iterators = new QList<CListViewItemIterator>;
	    CHECK_PTR( listView->d->iterators );
	}
	listView->d->iterators->append( this );
    }
}

/*!
  This methode is called to notify the iterator that the current item
  gets deleted, and lets the current item point to another (valid)
  item.
*/

void CListViewItemIterator::currentRemoved()
{
    if ( !curr ) return;

    if ( curr->parent() )
	curr = curr->parent();
    else if ( curr->nextSibling() )
	curr = curr->nextSibling();
    else if ( listView && listView->firstChild() &&
	      listView->firstChild() != curr )
	curr = listView->firstChild();
    else
	curr = 0;
}

void CListView::handleItemChange( CListViewItem *old, bool shift, bool control )
{
    if ( d->selectionMode == Single ) {
	// nothing
    } else if ( d->selectionMode == Extended ) {
	if ( control ) {
	    // nothing
	} else if ( shift ) {
	    selectRange( d->selectAnchor ? d->selectAnchor : old,
			 d->focusItem, FALSE, TRUE, d->selectAnchor ? TRUE : FALSE );
	} else {
	    blockSignals( TRUE );
	    selectAll( FALSE );
	    blockSignals( FALSE );
	    setSelected( d->focusItem, TRUE );
	}
    } else if ( d->selectionMode == Multi ) {
	if ( shift )
	    selectRange( old, d->focusItem, TRUE, FALSE );
    }
}

void CListView::selectRange( CListViewItem *from, CListViewItem *to, bool invert, bool includeFirst, bool clearSel )
{
    if ( !from || !to )
	return;
    bool swap = FALSE;
    if ( to == from->itemAbove() )
	swap = TRUE;
    if ( !swap && from != to && from != to->itemAbove() ) {
	CListViewItemIterator it( from );
	bool found = FALSE;
	for ( ; it.current(); ++it ) {
	    if ( it.current() == to ) {
		found = TRUE;
		break;
	    }
	}
	if ( !found )
	    swap = TRUE;
    }
    if ( swap ) {
	CListViewItem *i = from;
	from = to;
	to = i;
	if ( !includeFirst )
	    to = to->itemAbove();
    } else {
	if ( !includeFirst )
	    from = from->itemBelow();
    }

    bool changed = FALSE;
    if ( clearSel ) {
	CListViewItemIterator it( firstChild() );
	for ( ; it.current(); ++it ) {
	    if ( it.current()->selected ) {
		it.current()->setSelected( FALSE );
		changed = TRUE;
		repaintItem( it.current() );
	    }
	}
	it = CListViewItemIterator( to );
	for ( ; it.current(); ++it ) {
	    if ( it.current()->selected ) {
		it.current()->setSelected( FALSE );
		changed = TRUE;
		repaintItem( it.current() );
	    }
	}
    }

    for ( CListViewItem *i = from; i; i = i->itemBelow() ) {
	if ( !invert ) {
	    if ( !i->selected && i->isSelectable() ) {
		i->setSelected( TRUE );
		changed = TRUE;
		repaintItem( i );
	    }
	} else {
	    bool sel = !i->selected;
	    if ( (bool)i->selected != sel && sel && i->isSelectable() || !sel ) {
		i->setSelected( sel );
		changed = TRUE;
		repaintItem( i );
	    }
	}
	if ( i == to )
	    break;
    }
    if ( changed ) {
	emit selectionChanged();
    }
}

void CListView::setIconView(bool bOn)
{
	static ScrollBarMode scrollBarMode = hScrollBarMode();
	
	if (m_bIconView != bOn)
	{
		m_bIconView = bOn;
		CListViewItem *current = firstChild();
		while (current)
		{
			current->setup();
			current = current->nextSibling();
		}
		if (bOn)
		{
			scrollBarMode = hScrollBarMode();
			setHScrollBarMode(AlwaysOff);
		
			header()->hide();
			if (m_bUseStationaryHeader && m_pStationaryHeader)
			{
				m_pStationaryHeader->hide();
			}
			
			buildDrawableList();
		}
		else
		{
			setHScrollBarMode(scrollBarMode);
		
			header()->show();
			if (m_bUseStationaryHeader && m_pStationaryHeader)
			{
				m_pStationaryHeader->show();
			}
			
			d->r->maybeTotalHeight = -1;
		}
		triggerUpdate();
	}
	m_bIconView = bOn;
}

bool CListView::moveColumn(int nFromIndex, int nToIndex)
{
	return d->h->moveHeader(nFromIndex, nToIndex);
}

void CListView::resetColumnPositions()
{
	d->h->resetHeaderPositions();
}

void CListView::setNumCols(int nCols)
{
	if (nCols >=0)
	{
		int nColsNow = d->h->count();
		int nDiff = nColsNow - nCols;
		if (nDiff > 0)
		{
			resetColumnPositions();
			for (int nLoop = 0; nLoop < nDiff; nLoop++)
			{
				removeColumn(d->h->count() - 1);
			}
		}// if
		else if(nDiff < 0)
		{
			resetColumnPositions();
			for (int nLoop = 0; nLoop < -nDiff; nLoop++)
			{
				addColumn("");
			}
		}// else
	}// if
}

void CListView::stationaryHeader(bool bUse, QString szLabel,
		bool bCloseButton, const QPixmap *pPixmap)
{
	if (m_bUseStationaryHeader && m_pStationaryHeader)
	{
		delete m_pStationaryHeader;
		m_pStationaryHeader = 0;
	}
	m_bUseStationaryHeader = bUse;
	if (m_bUseStationaryHeader)
	{
		m_pStationaryHeader = new CStationaryHeader(szLabel, this);
		if (bCloseButton)
		{
			if (pPixmap)
			{
				m_pStationaryHeader->addCloseButton(*pPixmap);
			}
			else
			{
				m_pStationaryHeader->addCloseButton(QPixmap());
			}
			connect(m_pStationaryHeader->button(), SIGNAL(clicked()),
							this, SIGNAL(stationaryHeaderClosed()));
		}
		else if (!bCloseButton)
		{
			m_pStationaryHeader->removeCloseButton();
		}
		updateGeometries();
	}// if
}

int CListView::sortColumn()
{
	return d->sortcolumn;
}

bool CListView::sortAscending()
{
	return d->ascending;
}

void CListView::switchToIconView( void )
{
	setIconView( true );
}

void CListView::switchToNormalView( void )
{
	setIconView( false );
}

/////////////////////////////// CMultiListView ////////////////////////////////
//
// This class derives from CListView and is useful for a multi-selection
// style that follows the Windows selection style.
//
// With CListview (and QListView), if you select an item (left click) and then
// select another with a left-click, the first remains selected. In
// CMultiListView, the first will be deselected, unless the control key was
// held down as well. There is also support for the shift-mouse-click
// directional selection.
//
// The Extended selection mode still does not work exactly as in Windows.
// CMultiListView fixes the last few behaviour differences.  A CMultiListView
// in Single or Multiple mode behaves identically to a CListView
//
// The magic is done in mouseReleaseEvent so that drags can be allowed.
// So in mousePressEvent, we ignore the left button but allow the right-button
// functionality to remain.
//
///////////////////////////////////////////////////////////////////////////////

void CMultiListView::contentsMousePressEvent(QMouseEvent *e)
{
  // does selection depending on these rules:
  // if it's the right mouse button, select the current and,
  // if it wasn't already selected, deselect the others.
  // Rationale: if it was already selected, this means someone probably wants
  // to do file operations on a bunch of selected files. But if it was
  // not selected, it's one its own.
  //
  // if it's the left button alone, select current and deselect others
  // if it's the left button with control, deselect all others
  // if it's the left button with shift, let contentsMouseReleaseEvent look at
  //  the former current item and select all the ones between it and
  //  the new current item.
  //

  if (selectionMode() != Extended)
  {
    CListView::contentsMousePressEvent( e );
    return;
  }

  bool bControl = (ControlButton == (e->state() & ControlButton));
  bool bShift = (ShiftButton == (e->state() & ShiftButton));

  CListViewItem *pItem = itemAt(e->pos());
  setCurrentItem(pItem);
  if (!bShift) m_pLastCurrentItem = currentItem();

  if (e->button() == RightButton)
  {
    if (!isSelected(pItem))
      SelectCurrentOnly();
    CListView::contentsMousePressEvent(e);
  }
  else
  {
    if (e->button() == LeftButton)
    {
      if (!bControl && !bShift)
      {
	SelectCurrentOnly();
      }
      if (bControl)
      {
      	setSelected(pItem, !isSelected(pItem));
      }
    }
  }
}

void CMultiListView::contentsMouseReleaseEvent(QMouseEvent *e)
{
  //based on code from CorelExplorer/rightpanel.cpp
  //
  // Deals with shift range selection among other related tasks.
  //

  if (selectionMode() != Extended)
  {
    CListView::contentsMouseReleaseEvent( e );
    return;
  }


  if (e->button() == LeftButton || e->button() == RightButton)
  {
    CListViewItem *pNewCurrentItem = itemAt(e->pos());

    //    const QPoint pPoint(viewport()->mapToGlobal(e->pos()));
    //		
    //    if (NULL == GetItemAt(pPoint))
    if (NULL == pNewCurrentItem)
    {
      setSelected(currentItem(), false);
      CListView::contentsMouseReleaseEvent(e);
      return;
    }

    if (e->button() == LeftButton)
    {
      //CListViewItem *pOldCurrentItem = currentItem();
      CListViewItem *pOldCurrentItem = m_pLastCurrentItem;
      setCurrentItem(pNewCurrentItem);
	
      if (ShiftButton == (e->state() & ShiftButton))
      {
	CListViewItem *pI;
        CListViewItem *pFirstSentinel;
        bool bInSelectedArea = false;
	for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
        {
            if (!bInSelectedArea &&
                   (pI == pNewCurrentItem || pI == pOldCurrentItem))
            {
                bInSelectedArea = true;
                pFirstSentinel = pI;
            }

            if (bInSelectedArea)
                setSelected( pI, true );
            else if (!(ControlButton == (e->state() & ControlButton)))
                setSelected( pI, false );

            if (bInSelectedArea &&
                   (pI == pNewCurrentItem || pI == pOldCurrentItem) &&
                   pI != pFirstSentinel)
            {
                // if Control is held we don't have to deselect the rest
                if ((ControlButton == (e->state() & ControlButton)))
                    break;
                bInSelectedArea = false;
            }
        }
      }
    }
    else
    {
      CListView::contentsMouseReleaseEvent(e);
    }
  }
  else
  {
    CListView::contentsMouseReleaseEvent(e);
  }
}


CListViewItem *CMultiListView::GetItemAt(const QPoint& p)
{
  QPoint pos = viewport()->mapFromGlobal(p);

  CListViewItem *pItem = itemAt(pos);
	
  if (NULL == pItem || iconView())
  {
    return pItem;
  }

  if (allColumnsShowFocus())
    return pItem;

  // otherwise make sure p is on the item
  int x = pos.x();

  if (x < header()->cellPos(header()->mapToActual(0)) ||
      x > header()->cellPos(header()->mapToActual(0)) + columnWidth(0))
  {
    return NULL;
  }

  QPainter painter;
  painter.begin(this);
  int w = painter.boundingRect(0, 0, columnWidth(0), 500, AlignLeft, pItem->text(0)).width();
  painter.end();

  const QPixmap *pPixmap = pItem->pixmap(0);
  int nPixWidth = 0;
  if (pPixmap != NULL)
  {
    nPixWidth = pPixmap->width();
  }
  if (x > header()->cellPos(header()->mapToActual(0)) + w + itemMargin()*2 +
      nPixWidth)
  {
    return NULL;
  }

  return pItem;
}


void CMultiListView::SelectCurrentOnly()
{
  CListViewItem *pI;

  for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
  {
    if (isSelected(pI) && pI != currentItem())
      setSelected(pI, false);
  }

  if (!isSelected(currentItem()))
    setSelected(currentItem(), true);
}
