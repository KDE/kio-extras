/* Name: mapdialogdata.cpp

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

#include <qpixmap.h>
#include <qlayout.h>

//-----------------------------------

#include "qlistview.h"

//#if (QT_VERSION < 200)
#define QListView CListView
//#endif

#include "listview.h"

//-----------------------------------
#include "mapdialogdata.h"

#define Inherited QDialog


CMapDialogData::CMapDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pShareToMountLabel = new QLabel( this, "Label_1" );
	m_pShareToMountLabel->setGeometry( 11, 15, 120, 50 );
	m_pShareToMountLabel->setMinimumSize( 0, 0 );
	m_pShareToMountLabel->setMaximumSize( 32767, 32767 );
	m_pShareToMountLabel->setFocusPolicy( QWidget::NoFocus );
	m_pShareToMountLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pShareToMountLabel->setFontPropagation( QWidget::NoChildren );
	m_pShareToMountLabel->setPalettePropagation( QWidget::NoChildren );
	m_pShareToMountLabel->setFrameStyle( 0 );
	m_pShareToMountLabel->setLineWidth( 1 );
	m_pShareToMountLabel->setMidLineWidth( 0 );
	m_pShareToMountLabel->QFrame::setMargin( 0 );
	m_pShareToMountLabel->setText( "---" );
	m_pShareToMountLabel->setAlignment( 289 );
	m_pShareToMountLabel->setMargin( -1 );
	m_pShareToMountLabel->setAutoResize( true );

	m_pUNCPath = new QLineEdit( this, "LineEdit_1" );
	m_pUNCPath->setGeometry( 120, 15, 278, 22 );
	m_pUNCPath->setMinimumSize( 0, 0 );
	m_pUNCPath->setMaximumSize( 32767, 32767 );
	m_pUNCPath->setFocusPolicy( QWidget::StrongFocus );
	m_pUNCPath->setBackgroundMode( QWidget::PaletteBase );
	m_pUNCPath->setFontPropagation( QWidget::NoChildren );
	m_pUNCPath->setPalettePropagation( QWidget::NoChildren );
	m_pUNCPath->setText( "" );
	m_pUNCPath->setMaxLength( 32767 );
	m_pUNCPath->setFrame( QLineEdit::Normal );
	m_pUNCPath->setFrame( TRUE );

	m_pMountPointLabel = new QLabel( this, "Label_2" );
	m_pMountPointLabel->setGeometry( 11, 48, 99, 22 );
	m_pMountPointLabel->setMinimumSize( 0, 0 );
	m_pMountPointLabel->setMaximumSize( 32767, 32767 );
	m_pMountPointLabel->setFocusPolicy( QWidget::NoFocus );
	m_pMountPointLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pMountPointLabel->setFontPropagation( QWidget::NoChildren );
	m_pMountPointLabel->setPalettePropagation( QWidget::NoChildren );
	m_pMountPointLabel->setFrameStyle( 0 );
	m_pMountPointLabel->setLineWidth( 1 );
	m_pMountPointLabel->setMidLineWidth( 0 );
	m_pMountPointLabel->QFrame::setMargin( 0 );
	m_pMountPointLabel->setText( "---" );
	m_pMountPointLabel->setAlignment( 289 );
	m_pMountPointLabel->setMargin( -1 );
	m_pMountPointLabel->setAutoResize( true );

	m_pMountPoint = new QLineEdit( this, "LineEdit_2" );
	m_pMountPoint->setGeometry( 120, 45, 278, 22 );
	m_pMountPoint->setMinimumSize( 0, 0 );
	m_pMountPoint->setMaximumSize( 32767, 32767 );
	m_pMountPoint->setFocusPolicy( QWidget::StrongFocus );
	m_pMountPoint->setBackgroundMode( QWidget::PaletteBase );
	m_pMountPoint->setFontPropagation( QWidget::NoChildren );
	m_pMountPoint->setPalettePropagation( QWidget::NoChildren );
	m_pMountPoint->setText( "" );
	m_pMountPoint->setMaxLength( 32767 );
	m_pMountPoint->setFrame( QLineEdit::Normal );
	m_pMountPoint->setFrame( TRUE );

	m_pBrowseButton = new QPushButton( this, "PushButton_2" );
	m_pBrowseButton->setGeometry( 313, 72, 85, 26 );
	m_pBrowseButton->setMinimumSize( 0, 0 );
	m_pBrowseButton->setMaximumSize( 32767, 32767 );
	connect( m_pBrowseButton, SIGNAL(clicked()), SLOT(OnBrowse()) );
	m_pBrowseButton->setFocusPolicy( QWidget::TabFocus );
	m_pBrowseButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pBrowseButton->setFontPropagation( QWidget::NoChildren );
	m_pBrowseButton->setPalettePropagation( QWidget::NoChildren );
	m_pBrowseButton->setText( "---" );
	m_pBrowseButton->setAutoRepeat( FALSE );
	m_pBrowseButton->setAutoResize( FALSE );
	m_pBrowseButton->setToggleButton( FALSE );
	m_pBrowseButton->setDefault( FALSE );
	m_pBrowseButton->setAutoDefault( FALSE );
	m_pBrowseButton->setIsMenuButton( FALSE );

	m_pConnectAsLabel = new QLabel( this, "Label_3" );
	m_pConnectAsLabel->setGeometry( 11, 112, 99, 22 );
	m_pConnectAsLabel->setMinimumSize( 0, 0 );
	m_pConnectAsLabel->setMaximumSize( 32767, 32767 );
	m_pConnectAsLabel->setFocusPolicy( QWidget::NoFocus );
	m_pConnectAsLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pConnectAsLabel->setFontPropagation( QWidget::NoChildren );
	m_pConnectAsLabel->setPalettePropagation( QWidget::NoChildren );
	m_pConnectAsLabel->setFrameStyle( 0 );
	m_pConnectAsLabel->setLineWidth( 1 );
	m_pConnectAsLabel->setMidLineWidth( 0 );
	m_pConnectAsLabel->QFrame::setMargin( 0 );
	m_pConnectAsLabel->setText( "---" );
	m_pConnectAsLabel->setAlignment( 289 );
	m_pConnectAsLabel->setMargin( -1 );
	m_pConnectAsLabel->setAutoResize( true );

	m_pConnectAs = new QLineEdit( this, "LineEdit_3" );
	m_pConnectAs->setGeometry( 120, 107, 278, 22 );
	m_pConnectAs->setMinimumSize( 0, 0 );
	m_pConnectAs->setMaximumSize( 32767, 32767 );
	m_pConnectAs->setFocusPolicy( QWidget::StrongFocus );
	m_pConnectAs->setBackgroundMode( QWidget::PaletteBase );
	m_pConnectAs->setFontPropagation( QWidget::NoChildren );
	m_pConnectAs->setPalettePropagation( QWidget::NoChildren );
	m_pConnectAs->setText( "" );
	m_pConnectAs->setMaxLength( 32767 );
	m_pConnectAs->setFrame( QLineEdit::Normal );
	m_pConnectAs->setFrame( TRUE );

	m_pReconnectAtLogon = new QCheckBox( this, "CheckBox_1" );
	m_pReconnectAtLogon->setGeometry( 120, 138, 278, 22 );
	m_pReconnectAtLogon->setMinimumSize( 0, 0 );
	m_pReconnectAtLogon->setMaximumSize( 32767, 32767 );
	m_pReconnectAtLogon->setFocusPolicy( QWidget::TabFocus );
	m_pReconnectAtLogon->setBackgroundMode( QWidget::PaletteBackground );
	m_pReconnectAtLogon->setFontPropagation( QWidget::NoChildren );
	m_pReconnectAtLogon->setPalettePropagation( QWidget::NoChildren );
	m_pReconnectAtLogon->setText( "---" );
	m_pReconnectAtLogon->setAutoRepeat( FALSE );
	m_pReconnectAtLogon->setAutoResize( true );
	m_pReconnectAtLogon->setChecked( FALSE );

	m_Tree = new CListView( this, "ListView_1" );
	m_Tree->setGeometry( 11, 187, 387, 202 );
	m_Tree->setMinimumSize( 0, 0 );
	m_Tree->setMaximumSize( 32767, 32767 );
	m_Tree->setFocusPolicy( QWidget::StrongFocus );
	m_Tree->setBackgroundMode( QWidget::PaletteBackground );
	m_Tree->setFontPropagation( QWidget::NoChildren );
	m_Tree->setPalettePropagation( QWidget::NoChildren );
	m_Tree->setFrameStyle( 50 );
	m_Tree->setLineWidth( 1 );
	m_Tree->setMidLineWidth( 0 );
	m_Tree->QFrame::setMargin( 0 );
	m_Tree->setResizePolicy( QScrollView::Manual );
	m_Tree->setVScrollBarMode( QScrollView::Auto );
	m_Tree->setHScrollBarMode( QScrollView::Auto );
	m_Tree->setTreeStepSize( 20 );
	m_Tree->setMultiSelection( FALSE );
	m_Tree->setAllColumnsShowFocus( FALSE );
	m_Tree->setItemMargin( 1 );
	m_Tree->setRootIsDecorated( FALSE );
	m_Tree->addColumn( "Sample items", -1 );
	m_Tree->setColumnWidthMode( 0, QListView::Maximum );
	m_Tree->setColumnAlignment( 0, 1 );

	m_OKButton = new QPushButton( this, "PushButton_3" );
	m_OKButton->setGeometry( 230, 400, 80, 26 );
	m_OKButton->setMinimumSize( 0, 0 );
	m_OKButton->setMaximumSize( 32767, 32767 );
	connect( m_OKButton, SIGNAL(clicked()), SLOT(accept()) );
	m_OKButton->setFocusPolicy( QWidget::TabFocus );
	m_OKButton->setBackgroundMode( QWidget::PaletteBackground );
	m_OKButton->setFontPropagation( QWidget::NoChildren );
	m_OKButton->setPalettePropagation( QWidget::NoChildren );
	m_OKButton->setText( "---" );
	m_OKButton->setAutoRepeat( FALSE );
	m_OKButton->setAutoResize( FALSE );
	m_OKButton->setToggleButton( FALSE );
	m_OKButton->setDefault( FALSE );
	m_OKButton->setAutoDefault( TRUE );
	m_OKButton->setIsMenuButton( FALSE );

	m_pCancelButton = new QPushButton( this, "PushButton_4" );
	m_pCancelButton->setGeometry( 318, 400, 80, 26 );
	m_pCancelButton->setMinimumSize( 0, 0 );
	m_pCancelButton->setMaximumSize( 32767, 32767 );
	connect( m_pCancelButton, SIGNAL(clicked()), SLOT(reject()) );
	m_pCancelButton->setFocusPolicy( QWidget::TabFocus );
	m_pCancelButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pCancelButton->setFontPropagation( QWidget::NoChildren );
	m_pCancelButton->setPalettePropagation( QWidget::NoChildren );
	m_pCancelButton->setText( "---" );
	m_pCancelButton->setAutoRepeat( FALSE );
	m_pCancelButton->setAutoResize( FALSE );
	m_pCancelButton->setToggleButton( FALSE );
	m_pCancelButton->setDefault( FALSE );
	m_pCancelButton->setAutoDefault( TRUE );
	m_pCancelButton->setIsMenuButton( FALSE );

	m_pSharedDirectoriesLabel = new QLabel( this, "Label_4" );
	m_pSharedDirectoriesLabel->setGeometry( 11, 164, 377, 22 );
	m_pSharedDirectoriesLabel->setMinimumSize( 0, 0 );
	m_pSharedDirectoriesLabel->setMaximumSize( 32767, 32767 );
	m_pSharedDirectoriesLabel->setFocusPolicy( QWidget::NoFocus );
	m_pSharedDirectoriesLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pSharedDirectoriesLabel->setFontPropagation( QWidget::NoChildren );
	m_pSharedDirectoriesLabel->setPalettePropagation( QWidget::NoChildren );
	m_pSharedDirectoriesLabel->setFrameStyle( 0 );
	m_pSharedDirectoriesLabel->setLineWidth( 1 );
	m_pSharedDirectoriesLabel->setMidLineWidth( 0 );
	m_pSharedDirectoriesLabel->QFrame::setMargin( 0 );
	m_pSharedDirectoriesLabel->setText( "---" );
	m_pSharedDirectoriesLabel->setAlignment( 289 );
	m_pSharedDirectoriesLabel->setMargin( -1 );

	resize( 409,437 );
	setMinimumSize( 409, 437 );
	setMaximumSize( 409, 437 );
}


CMapDialogData::~CMapDialogData()
{
}
void CMapDialogData::OnBrowse()
{
}
