
#ifndef CHEADER_H
#define CHEADER_H

#ifndef QT_H
#include <qwidget.h>
#include <qstring.h>
#include <qiconset.h>
#endif // QT_H

struct CHeaderData;

class Q_EXPORT CHeader : public QWidget
{
    Q_OBJECT
    Q_PROPERTY( Orientation orientation READ orientation WRITE setOrientation )
    Q_PROPERTY( bool tracking READ tracking WRITE setTracking )
    Q_PROPERTY( int count READ count )
    Q_PROPERTY( int offset READ offset WRITE setOffset )
    Q_PROPERTY( bool moving READ isMovingEnabled WRITE setMovingEnabled )

public:
    CHeader( QWidget *parent=0, const char *name=0,
	bool bUseHeaderExtender = true );
    CHeader( int, QWidget *parent=0, const char *name=0,
	bool bUseHeaderExtender = true );
    ~CHeader();

    int		addLabel( const QString &, int size = -1 );
    int		addLabel( const QIconSet&, const QString &, int size = -1 );
    void 	removeLabel( int section );
    virtual void setLabel( int, const QString &, int size = -1 );
    virtual void setLabel( int, const QIconSet&, const QString &, int size = -1 );
    QString 	label( int section ) const;
    QIconSet* 	iconSet( int section ) const;

    virtual void setOrientation( Orientation );
    Orientation orientation() const;
    virtual void setTracking( bool enable );
    bool	tracking() const;

    virtual void setClickEnabled( bool, int section = -1 );
    virtual void setResizeEnabled( bool, int section = -1 );
    virtual void setMovingEnabled( bool );
    bool isClickEnabled( int section = -1 ) const;
    bool isResizeEnabled( int section = -1 ) const;
    bool isMovingEnabled() const;

    void 	resizeSection( int section, int s );
    int		sectionSize( int section ) const;
    int		sectionPos( int section ) const;
    int		sectionAt( int pos ) const;
    int		count() const;

    virtual void setCellSize( int , int ); // obsolete, do not use
    int		cellSize( int ) const; // obsolete, do not use
    int		cellPos( int ) const; // obsolete, do not use
    int		cellAt( int ) const; // obsolete, do not use

    int 	offset() const;

    QSize	sizeHint() const;
    QSizePolicy sizePolicy() const;

    int		mapToSection( int index ) const;
    int		mapToIndex( int section ) const;
    int		mapToLogical( int ) const; // obsolete, do not use
    int		mapToActual( int ) const; // obsolete, do not use

    void 	moveSection( int section, int toIndex );
    virtual void moveCell( int, int); // obsolete, do not use

    void 	setSortIndicator( int section, bool increasing = TRUE );

// New for CHeader

    bool moveHeader( int nFromIndex, int nToIndex );
    void resetHeaderPositions();

public slots:
    virtual void setOffset( int pos );

signals:
    void	clicked( int section );
    void	pressed( int section );
    void	released( int section );
    void	sizeChange( int section, int oldSize, int newSize );
    void	indexChange( int section, int fromIndex, int toIndex );
    void	sectionClicked( int ); // obsolete, do not use
    void	moved( int, int ); // obsolete, do not use

protected:
    void	paintEvent( QPaintEvent * );
    QRect	sRect( int index );

    void	paintSection( QPainter *p, int index, QRect fr); // ### const QRect& ## virtual 3.0
    void	paintSectionLabel( QPainter* p, int index, const QRect& fr ); //## virtual 3.0

    void	mousePressEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );

private:
    void	init( int, bool bUseHeaderExtender );

    void	paintRect( int p, int s );
    void	markLine( int idx );
    void	unMarkLine( int idx );
    int		pPos( int i ) const;
    int		pSize( int i ) const;
    int		pHeight( int i ) const;
    void	setPHeight( int i, int h );
    int 	findLine( int );

    void	handleColumnResize(int, int, bool);

    int		offs;
    int		handleIdx;
    int		oldHIdxSize;
    int		moveToIdx;
    enum State { Idle, Sliding, Pressed, Moving, Blocked };
    State	state;
    QCOORD	clickPos;
    bool	trackingIsOn;
    int       cachedIdx; // not used
    int	cachedPos; // not used
    Orientation orient;

    CHeaderData *d;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    CHeader( const CHeader & );
    CHeader &operator=( const CHeader & );
#endif
};


inline CHeader::Orientation CHeader::orientation() const
{
    return orient;
}

inline void CHeader::setTracking( bool enable ) { trackingIsOn = enable; }
inline bool CHeader::tracking() const { return trackingIsOn; }

#endif //CHEADER_H
