/*************************************************************************
* * * ** $Id$
**
** Copyright (C) 1992-2000 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include "listviews.h"
#include "header.h"

#include <kapp.h>
#include <kcmdlineargs.h>

#include <qlabel.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qobjectlist.h>
#include <qpopupmenu.h>

// -----------------------------------------------------------------

MessageHeader::MessageHeader( const MessageHeader &mh )
{
    msender = mh.msender;
    msubject = mh.msubject;
    mdatetime = mh.mdatetime;
}

MessageHeader &MessageHeader::operator=( const MessageHeader &mh )
{
    msender = mh.msender;
    msubject = mh.msubject;
    mdatetime = mh.mdatetime;

    return *this;
}

// -----------------------------------------------------------------

Folder::Folder( Folder *parent, const QString &name )
    : QObject( parent, name ), fName( name )
{
    lstMessages.setAutoDelete( TRUE );
}

// -----------------------------------------------------------------

FolderListItem::FolderListItem( CKDEListView *parent, Folder *f )
    : CListViewItem( parent )
{
    myFolder = f;
    setText( 0, f->folderName() );

    if ( myFolder->children() )
	insertSubFolders( myFolder->children() );
}

FolderListItem::FolderListItem( FolderListItem *parent, Folder *f )
    : CListViewItem( parent )
{
    myFolder = f;

    setText( 0, f->folderName() );

    if ( myFolder->children() )
	insertSubFolders( myFolder->children() );
}

void FolderListItem::insertSubFolders( const QObjectList *lst )
{
    Folder *f;
    for ( f = ( Folder* )( ( QObjectList* )lst )->first(); f; f = ( Folder* )( ( QObjectList* )lst )->next() )
	(void)new FolderListItem( this, f );
}

// -----------------------------------------------------------------

MessageListItem::MessageListItem( CKDEListView *parent, Message *m )
    : CListViewItem( parent )
{
    myMessage = m;
    setText( 1, myMessage->header().sender() );
    setText( 0, myMessage->header().subject() );
    setText( 2, myMessage->header().datetime().toString() );
    setIconViewPixmap( QPixmap( "default.xpm" ) );
}

void MessageListItem::paintCell( QPainter *p, const QColorGroup &cg,
				 int column, int width, int alignment )
{
    QColorGroup _cg( cg );
    QColor c = _cg.text();

    if ( myMessage->state() == Message::Unread )
	_cg.setColor( QColorGroup::Text, Qt::red );

    CListViewItem::paintCell( p, _cg, column, width, alignment );

    _cg.setColor( QColorGroup::Text, c );
}

// -----------------------------------------------------------------

MessageList::MessageList( QWidget *parent, const char *name )
    : CKDEListView( parent, name ),
	m_bStationaryHeader( false )
{
}

void MessageList::toggleStationaryHeader( void )
{
//    void stationaryHeader(bool bUse = true, const char *szLabel = 0,
//      bool bCloseButton = false, const QPixmap * = 0);
	m_bStationaryHeader = !m_bStationaryHeader;
	stationaryHeader( m_bStationaryHeader, "Dummy", true );
}

void MessageList::moveCols( void )
{
	moveColumn(0, 1);
}

// -----------------------------------------------------------------

ListViews::ListViews( QWidget *parent, const char *name )
    : QSplitter( Qt::Horizontal, parent, name ),
    m_sortCol( -1 ),
    m_sortAscending( true )
{
    lstFolders.setAutoDelete( TRUE );

    folders = new CKDEListView( this );
    folders->header()->setClickEnabled( FALSE );
    folders->addColumn( "Folder" );

    initFolders();
    setupFolders();

    folders->setRootIsDecorated( TRUE );
    setResizeMode( folders, QSplitter::KeepSize );

    QSplitter *vsplitter = new QSplitter( Qt::Vertical, this );

    messages = new MessageList( vsplitter, "" );
    messages->addColumn( "Sender" );
    messages->addColumn( "Subject" );
    messages->addColumn( "Date" );
    messages->setColumnAlignment( 1, Qt::AlignRight );
    messages->setAllColumnsShowFocus( FALSE );
    messages->setShowSortIndicator( TRUE );

    menu = new QPopupMenu( messages );

// add CListView tests here, for convenience
    menu->insertItem( QString( "Icon View" ), messages,
		SLOT(switchToIconView()) );
    menu->insertItem( QString( "Normal View" ), messages,
		SLOT(switchToNormalView()) );
    menu->insertItem( QString( "Rearrange Columns" ), messages,
		SLOT(moveCols()) );
    menu->insertItem( QString( "Reset Columns" ), messages,
		SLOT(resetColumnPositions()) );
    menu->insertItem( QString( "Toggle Stationary Header" ), messages,
		SLOT(toggleStationaryHeader()) );

	connect(messages, SIGNAL( rightButtonPressed( CListViewItem *,
		const QPoint &, int ) ), this, SLOT( slotRMB( CListViewItem *,
		const QPoint &, int ) ) );

    vsplitter->setResizeMode( messages, QSplitter::KeepSize );

    message = new QLabel( vsplitter );
    message->setAlignment( Qt::AlignTop );
    message->setBackgroundMode( PaletteBase );

    connect( folders, SIGNAL( selectionChanged( CListViewItem* ) ),
	     this, SLOT( slotFolderChanged( CListViewItem* ) ) );
    connect( messages, SIGNAL( selectionChanged() ),
	     this, SLOT( slotMessageChanged() ) );
    connect( messages, SIGNAL( currentChanged( CListViewItem * ) ),
	     this, SLOT( slotMessageChanged() ) );
//    connect( messages->header(), SIGNAL( clicked(int) ), this, SLOT(
//	slotHeaderClicked(int) ) );

//    messages->setSelectionMode( CKDEListView::Single );
//    messages->setSelectionMode( CKDEListView::Multi );
    messages->setSelectionMode( CKDEListView::Extended );

    // some preperationes
    folders->firstChild()->setOpen( TRUE );
    folders->firstChild()->firstChild()->setOpen( TRUE );
    folders->setCurrentItem( folders->firstChild()->firstChild()->firstChild() );
    folders->setSelected( folders->firstChild()->firstChild()->firstChild(), TRUE );

    messages->setSelected( messages->firstChild(), TRUE );
    messages->setCurrentItem( messages->firstChild() );
    message->setMargin( 5 );

    QValueList<int> lst;
    lst.append( 170 );
    setSizes( lst );
}

void ListViews::initFolders()
{
    unsigned int mcount = 1;

    for ( unsigned int i = 1; i < 2; i++ ) {
	QString str;
	str = QString( "Folder %1" ).arg( i );
	Folder *f = new Folder( 0, str );
	for ( unsigned int j = 1; j < 2; j++ ) {
	    QString str2;
	    str2 = QString( "Sub Folder %1" ).arg( j );
	    Folder *f2 = new Folder( f, str2 );
	    for ( unsigned int k = 1; k < 2; k++ ) {
		QString str3;
		str3 = QString( "Sub Sub Folder %1" ).arg( k );
		Folder *f3 = new Folder( f2, str3 );
		initFolder( f3, mcount );
	    }
	}
	lstFolders.append( f );
    }
}

void ListViews::initFolder( Folder *folder, unsigned int &count )
{
    for ( unsigned int i = 0; i < 100; i++, count++ ) {
	QString str;
	str = QString( "Message %1  " ).arg( count );
	QDateTime dt = QDateTime::currentDateTime();
	dt = dt.addSecs( 60 * count );
	MessageHeader mh( "Troll Tech <info@trolltech.com>  ", str, dt );

	QString body;
	body = QString( "This is the message number %1 of this application, \n"
			"which shows how to use CListViews, CListViewItems, \n"
			"QSplitters and so on. The code should show how easy\n"
			"this can be done in Qt." ).arg( count );
	Message *msg = new Message( mh, body );
	folder->addMessage( msg );
    }
}

void ListViews::setupFolders()
{
    folders->clear();

    for ( Folder* f = lstFolders.first(); f; f = lstFolders.next() )
	(void)new FolderListItem( folders, f );
}

void ListViews::slotHeaderClicked( int col )
{
  if (m_sortCol == col)
  {
    messages->setSorting( col, !m_sortAscending );
    m_sortCol = col;
    m_sortAscending = !m_sortAscending;
  }
  else
  {
    messages->setSorting( col, true );
    m_sortCol = col;
    m_sortAscending = true;
  }
}

void ListViews::slotRMB( CListViewItem* Item, const QPoint & point, int )
{
    if( Item )
	menu->popup( point );
}


void ListViews::slotFolderChanged( CListViewItem *i )
{
    if ( !i )
	return;
    messages->clear();
    message->setText( "" );

    FolderListItem *item = ( FolderListItem* )i;

    for ( Message* msg = item->folder()->firstMessage(); msg;
	  msg = item->folder()->nextMessage() )
	(void) new MessageListItem( messages, msg );
}

void ListViews::slotMessageChanged()
{
    CListViewItem *i = messages->currentItem();
    if ( !i )
	return;

    if ( !i->isSelected() ) {
	message->setText( "" );
	return;
    }

    MessageListItem *item = ( MessageListItem* )i;
    Message *msg = item->message();

    QString text;
    QString tmp = msg->header().sender();
    tmp = tmp.replace( QRegExp( "[<]" ), "&lt;" );
    tmp = tmp.replace( QRegExp( "[>]" ), "&gt;" );
    text = QString( "<b><i>From:</i></b> <a href=\"mailto:info@trolltech.com\">%1</a><br>"
		    "<b><i>Subject:</i></b> <big><big><b>%2</b></big></big><br>"
		    "<b><i>Date:</i></b> %3<br><br>"
		    "%4" ).
	   arg( tmp ).arg( msg->header().subject() ).
	   arg( msg->header().datetime().toString() ).arg( msg->body() );

    message->setText( text );

    msg->setState( Message::Read );
}

int main( int argc, char **argv )
{
    KCmdLineArgs::init( argc, argv, "listviews", "List View Example", "0.1" );
    KApplication::addCmdLineOptions();
    KApplication a;

    ListViews listViews;
    listViews.resize( 640, 480 );
    listViews.setCaption( "List View Example" );
    a.setMainWidget( &listViews );
    listViews.show();

    return a.exec();
}
