

#ifndef CLISTVIEW_H
#define CLISTVIEW_H

#include "ccui_common.h"

class QPixmap;
class QFont;
class CHeader;
class QIconSet;
class CStationaryHeader;
class QMouseEvent;

class CListView;
struct CListViewPrivate;
class CListViewItemIterator;

#ifndef QT_H
#include "qscrollview.h"
#endif // QT_H

// Settings for Corel's icon view
#define ICON_VIEW_WIDTH                               90
#define ICON_VIEW_MARGIN                               6


class Q_EXPORT CListViewItem: public Qt
{
    friend class CListViewItemIterator;
#if defined(_CC_MSVC_)
    friend class CListViewItem;
#endif

public:
    CListViewItem( CListView * parent );
    CListViewItem( CListViewItem * parent );
    CListViewItem( CListView * parent, CListViewItem * after );
    CListViewItem( CListViewItem * parent, CListViewItem * after );

    CListViewItem( CListView * parent,
		   QString,     QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null );
    CListViewItem( CListViewItem * parent,
		   QString,     QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null );

    CListViewItem( CListView * parent, CListViewItem * after,
		   QString,     QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null );
    CListViewItem( CListViewItem * parent, CListViewItem * after,
		   QString,     QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null,
		   QString = QString::null, QString = QString::null );
    virtual ~CListViewItem();

    virtual void insertItem( CListViewItem * );
    virtual void takeItem( CListViewItem * );
    virtual void removeItem( CListViewItem * ); //obsolete, use takeItem instead

    int height() const;
    virtual void invalidateHeight();
    int totalHeight() const;
    virtual int width( const QFontMetrics&,
		       const CListView*, int column) const;
    void widthChanged(int column=-1) const;
    int depth() const;

    virtual void setText( int, const QString &);
    virtual QString text( int ) const;

    virtual void setPixmap( int, const QPixmap & );
    virtual const QPixmap * pixmap( int ) const;

    virtual QString key( int, bool ) const;
    virtual void sortChildItems( int, bool );

    int childCount() const { return nChildren; }

    bool isOpen() const { return open; }
    virtual void setOpen( bool );
    virtual void setup();

    virtual void setSelected( bool );
    bool isSelected() const { return selected; }

    virtual void paintCell( QPainter *, const QColorGroup & cg,
			    int column, int width, int alignment );
    virtual void paintBranches( QPainter * p, const QColorGroup & cg,
				int w, int y, int h, GUIStyle s );
    virtual void paintFocus( QPainter *, const QColorGroup & cg,
			     const QRect & r );

    CListViewItem * firstChild() const;
    CListViewItem * nextSibling() const { return siblingItem; }
    CListViewItem * prevSibling() const { return prevSiblingItem; }
        // new for Corel's icon view
    CListViewItem * parent() const;

    CListViewItem * itemAbove();
    CListViewItem * itemBelow();
    CListViewItem * itemLeft();     // new for Corel's icon view
    CListViewItem * itemRight();    // new for Corel's icon view

    int itemPos() const;

    CListView *listView() const;

    virtual void setSelectable( bool enable );
    bool isSelectable() const { return selectable; }

    virtual void setExpandable( bool );
    bool isExpandable() const { return expandable; }

    void repaint() const;

    void sort(); // ######## make virtual in next major release

    // new for Corel's icon view
    virtual void setIconViewPixmap( const QPixmap & );
    virtual const QPixmap * iconViewPixmap( void ) const;


protected:
    virtual void enforceSortOrder() const;
    virtual void setHeight( int );
    virtual void activate();

    bool activatedPos( QPoint & );

    // new for Corel's icon view
    virtual void paintCellIconView(QPainter *, const QColorGroup &,
        int column, int width, int align);
    virtual void paintCellNormalView(QPainter *, const QColorGroup &,
        int column, int width, int align);

private:
    void init();
    void moveToJustAfter( CListViewItem * );
    int ownHeight;
    int maybeTotalHeight;
    int nChildren;

    uint lsc: 14;
    uint lso: 1;
    uint open : 1;
    uint selected : 1;
    uint selectable: 1;
    uint configured: 1;
    uint expandable: 1;
    uint is_root: 1;

    CListViewItem * parentItem;
    CListViewItem * siblingItem;
    CListViewItem * prevSiblingItem;    // new for Corel's icon view
    CListViewItem * childItem;

    void * columns;

    friend class CListView;

    // new for Corel's icon view
    QPixmap *   m_pIconViewPixmap;
    int         m_nIconViewTextWidth;
};


class Q_EXPORT CListView: public QScrollView
{
    friend class CListViewItemIterator;
    friend class CListViewItem;

    Q_OBJECT
    Q_ENUMS( SelectionMode )
    Q_PROPERTY( int columns READ columns WRITE setNumCols )
    Q_PROPERTY( bool multiSelection READ isMultiSelection WRITE setMultiSelection )
    Q_PROPERTY( SelectionMode selectionMode READ selectionMode WRITE setSelectionMode )
    Q_PROPERTY( int childCount READ childCount )
    Q_PROPERTY( bool allColumnsShowFocus READ allColumnsShowFocus WRITE setAllColumnsShowFocus )
    Q_PROPERTY( int itemMargin READ itemMargin WRITE setItemMargin )
    Q_PROPERTY( bool rootIsDecorated READ rootIsDecorated WRITE setRootIsDecorated )
    Q_PROPERTY( bool iconView READ iconView WRITE setIconView )

public:
    CListView( QWidget * parent = 0, const char *name = 0,
        bool bUseHeaderExtender = true );       // new for Corel
    ~CListView();

    int treeStepSize() const;
    virtual void setTreeStepSize( int );

    virtual void insertItem( CListViewItem * );
    virtual void takeItem( CListViewItem * );
    virtual void removeItem( CListViewItem * ); // obsolete, use takeItem instead

    virtual void clear();

    CHeader * header() const;

    virtual int addColumn( const QString &label, int size = -1);
    virtual int addColumn( const QIconSet& iconset, const QString &label, int size = -1);
    void removeColumn( int index ); // #### make virtual in next major release!
    virtual void setColumnText( int column, const QString &label );
    virtual void setColumnText( int column, const QIconSet& iconset, const QString &label );
    QString columnText( int column ) const;
    virtual void setColumnWidth( int column, int width );
    int columnWidth( int column ) const;
    enum WidthMode { Manual, Maximum };
    virtual void setColumnWidthMode( int column, WidthMode );
    WidthMode columnWidthMode( int column ) const;
    int columns() const;

    virtual void setColumnAlignment( int, int );
    int columnAlignment( int ) const;

    void show();

    CListViewItem * itemAt( const QPoint & screenPos ) const;
    QRect itemRect( const CListViewItem * ) const;
    int itemPos( const CListViewItem * );

    void ensureItemVisible( const CListViewItem * );

    void repaintItem( const CListViewItem * ) const;

    virtual void setMultiSelection( bool enable );
    bool isMultiSelection() const;

    enum SelectionMode { Single, Multi, Extended, NoSelection  };
    void setSelectionMode( SelectionMode mode );
    SelectionMode selectionMode() const;

    virtual void clearSelection();
    virtual void setSelected( CListViewItem *, bool );
    bool isSelected( const CListViewItem * ) const;
    CListViewItem * selectedItem() const;
    virtual void setOpen( CListViewItem *, bool );
    bool isOpen( const CListViewItem * ) const;

    virtual void setCurrentItem( CListViewItem * );
    CListViewItem * currentItem() const;

    CListViewItem * firstChild() const;

    int childCount() const;

    virtual void setAllColumnsShowFocus( bool );
    bool allColumnsShowFocus() const;

    virtual void setItemMargin( int );
    int itemMargin() const;

    virtual void setRootIsDecorated( bool );
    bool rootIsDecorated() const;

    virtual void setSorting( int column, bool increasing = TRUE );
    void sort(); // #### make virtual in next major release

    virtual void setFont( const QFont & );
    virtual void setPalette( const QPalette & );

    bool eventFilter( QObject * o, QEvent * );

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    void setShowSortIndicator( bool show );
    bool showSortIndicator() const;

    // new for Corel

    bool moveColumn(int nFromIndex, int nToIndex);
        // wrapper for CHeader->moveHeader()
    void setNumCols(int nCols);
        // adds empty columns or deletes columns, starting at right
    void stationaryHeader(bool bUse = true, QString szLabel = QString::null,
        bool bCloseButton = false, const QPixmap * = 0);
        // turn on and off stationary header
    int sortColumn();
        // return current column used as sort key
    bool sortAscending();
        // return whether using ascending or descending sort

    void setIconView(bool bOn = true);
    bool iconView() const {return m_bIconView;}

public slots:
    void invertSelection(); // ###### make virtual
    void selectAll( bool select ); // make virtual
    void triggerUpdate();
    void setContentsPos( int x, int y );

    // new for Corel
    void switchToIconView( void );
    void switchToNormalView( void );
    void resetColumnPositions();

signals:
    void selectionChanged();
    void selectionChanged( CListViewItem * );
    void currentChanged( CListViewItem * );
    void clicked( CListViewItem * );
    void clicked( CListViewItem *, const QPoint &, int );
    void pressed( CListViewItem * );
    void pressed( CListViewItem *, const QPoint &, int );

    void doubleClicked( CListViewItem * );
    void returnPressed( CListViewItem * );
    void rightButtonClicked( CListViewItem *, const QPoint&, int );
    void rightButtonPressed( CListViewItem *, const QPoint&, int );
    void mouseButtonPressed( int, CListViewItem *, const QPoint& , int );
    void mouseButtonClicked( int, CListViewItem *,  const QPoint&, int );

    void onItem( CListViewItem *item );
    void onViewport();

    void expanded( CListViewItem *item );
    void collapsed( CListViewItem *item );

    // new for Corel
    void stationaryHeaderClosed( void );

protected:
    virtual void contentsMousePressEvent( QMouseEvent * e );
    virtual void contentsMouseReleaseEvent( QMouseEvent * e );
    virtual void contentsMouseMoveEvent( QMouseEvent * e );
    virtual void contentsMouseDoubleClickEvent( QMouseEvent * e );

    void focusInEvent( QFocusEvent * e );
    void focusOutEvent( QFocusEvent * e );

    void keyPressEvent( QKeyEvent *e );

    void resizeEvent( QResizeEvent *e );

    void showEvent( QShowEvent * );

    void drawContentsOffset( QPainter *, int ox, int oy,
			     int cx, int cy, int cw, int ch );
        // now wraps NormalView or IconView version, depending on mode

    virtual void paintEmptyArea( QPainter *, const QRect & );
    void enabledChange( bool );
    void styleChange( QStyle& );

    // new for Corel
    void drawContentsOffsetIconView(QPainter *, int ox, int oy,
                 int cx, int cy, int cw, int ch);
    void drawContentsOffsetNormalView(QPainter *, int ox, int oy,
                 int cx, int cy, int cw, int ch);

    // return item at given screen position, depending on current mode
    CListViewItem * itemAtNormalView(const QPoint & screenPos) const;
    CListViewItem * itemAtIconView(const QPoint & screenPos) const;

protected slots:
    void updateContents();
    void doAutoScroll();

private slots:
    void changeSortColumn( int );
    void updateDirtyItems();
    void makeVisible();
    void handleSizeChange( int, int, int );

private:
    void updateGeometries();
    void buildDrawableList() const;
        // now wraps NormalView or IconView version, depending on mode
    void reconfigureItems();
    void widthChanged(const CListViewItem*, int c);
    void handleItemChange( CListViewItem *old, bool shift, bool control );
    void selectRange( CListViewItem *from, CListViewItem *to, bool invert, bool includeFirst, bool clearSel = FALSE );

    CListViewPrivate * d;

    // new for Corel
    CStationaryHeader *m_pStationaryHeader;
    bool m_bUseStationaryHeader;
    bool m_bIconView;
    QRect m_iconViewTextRect;

    int nOldWidth;
    bool iconWidthChange(bool bUpdateStatus = true);
    int numIconWidth() const;

    void buildDrawableListIconView() const;
    void buildDrawableListNormalView() const;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    CListView( const QWidget & );
    CListView &operator=( const QWidget & );
#endif
};


class Q_EXPORT CCheckListItem : public CListViewItem
{
public:
    enum Type { RadioButton, CheckBox, Controller };

    CCheckListItem( CCheckListItem *parent, const QString &text,
		    Type = Controller );
    CCheckListItem( CListViewItem *parent, const QString &text,
		    Type = Controller );
    CCheckListItem( CListView *parent, const QString &text,
		    Type = Controller );
    CCheckListItem( CListViewItem *parent, const QString &text,
		    const QPixmap & );
    CCheckListItem( CListView *parent, const QString &text,
		    const QPixmap & );

    void paintCell( QPainter *,  const QColorGroup & cg,
		    int column, int width, int alignment );
    virtual void paintFocus( QPainter *, const QColorGroup & cg,
			     const QRect & r );
    int width( const QFontMetrics&, const CListView*, int column) const;
    void setup();

    virtual void setOn( bool );
    bool isOn() const { return on; }
    Type type() const { return myType; }
    QString text() const { return CListViewItem::text( 0 ); }
    QString text( int n ) const { return CListViewItem::text( n ); }

protected:
    void paintBranches( QPainter * p, const QColorGroup & cg,
			int w, int y, int h, GUIStyle s );

    void activate();
    void turnOffChild();
    virtual void stateChange( bool );

private:
    void init();
    Type myType;
    bool on;
    CCheckListItem *exclusive;

    void *reserved;
};

class Q_EXPORT CListViewItemIterator
{
    friend struct CListViewPrivate;
    friend class CListView;
    friend class CListViewItem;

public:
    CListViewItemIterator();
    CListViewItemIterator( CListViewItem *item );
    CListViewItemIterator( const CListViewItemIterator &it );
    CListViewItemIterator( CListView *lv );

    CListViewItemIterator &operator=( const CListViewItemIterator &it );

    ~CListViewItemIterator();

    CListViewItemIterator &operator++();
    const CListViewItemIterator operator++( int );
    CListViewItemIterator &operator+=( int j );

    CListViewItemIterator &operator--();
    const CListViewItemIterator operator--( int );
    CListViewItemIterator &operator-=( int j );

    CListViewItem *current() const;

protected:
    CListViewItem *curr;
    CListView *listView;

private:
    void addToListView();
    void currentRemoved();

};

////////////////////////////////////////////////////////////////////////////
// This class derives from CListView and is useful for a multi-selection
// style that follows the Windows selection style.
//
// In CListview (and QListView), if you select one item and then select
// another with a left-click, the first remains selected. In CMultiListView,
// the first is deselected, unless the control key was held down as well. 
// There is also support for the shift-mouse-click range selection.
//
// The Extended selection mode still does not work exactly as in Windows.
// CMultiListView fixes the last few behaviour differences.  A CMultiListView
// in Single or Multiple mode behaves identically to a CListView
//
// The magic is done in mouseReleaseEvent so that drags can be allowed.
// In mousePressEvent, we do the selection only.
//////////////////////////////////////////////////////////////////////////

class Q_EXPORT CMultiListView: public CListView
{
  Q_OBJECT
public:
  CMultiListView(QWidget *parent = 0, const char *name = 0,
                bool bUseHeaderExtender = true,
                bool bAllColumnsShowFocus = false)
    : CListView(parent, name, bUseHeaderExtender), m_pLastCurrentItem(0)
    {setSelectionMode(Extended); setAllColumnsShowFocus(bAllColumnsShowFocus);}

  CListViewItem *GetItemAt(const QPoint& );
protected:
  virtual void contentsMouseReleaseEvent(QMouseEvent *e);
  virtual void contentsMousePressEvent(QMouseEvent *e);
private:
  void SelectCurrentOnly();
  CListViewItem *m_pLastCurrentItem;
};

#endif // CLISTVIEW_H
