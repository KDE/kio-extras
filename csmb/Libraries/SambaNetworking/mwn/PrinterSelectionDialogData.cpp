/* Name: PrinterSelectionDialogData.cpp

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
#include "PrinterSelectionDialogData.h"

#define Inherited QDialog


CPrinterSelectionDialogData::CPrinterSelectionDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pPrinterLabel = new QLabel( this, "Label_1" );
	m_pPrinterLabel->setGeometry( 13, 15, 142, 22 );
	m_pPrinterLabel->setMinimumSize( 0, 0 );
	m_pPrinterLabel->setMaximumSize( 32767, 32767 );
	m_pPrinterLabel->setFocusPolicy( QWidget::NoFocus );
	m_pPrinterLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pPrinterLabel->setFontPropagation( QWidget::NoChildren );
	m_pPrinterLabel->setPalettePropagation( QWidget::NoChildren );
	m_pPrinterLabel->setFrameStyle( 0 );
	m_pPrinterLabel->setLineWidth( 1 );
	m_pPrinterLabel->setMidLineWidth( 0 );
	m_pPrinterLabel->QFrame::setMargin( 0 );
	m_pPrinterLabel->setText( "---" );
	m_pPrinterLabel->setAlignment( 289 );
	m_pPrinterLabel->setMargin( -1 );

	m_pPath = new QLineEdit( this, "LineEdit_1" );
	m_pPath->setGeometry( 165, 12, 240, 25 );
	m_pPath->setMinimumSize( 0, 0 );
	m_pPath->setMaximumSize( 32767, 32767 );
	m_pPath->setFocusPolicy( QWidget::StrongFocus );
	m_pPath->setBackgroundMode( QWidget::PaletteBase );
	m_pPath->setFontPropagation( QWidget::NoChildren );
	m_pPath->setPalettePropagation( QWidget::NoChildren );
	m_pPath->setText( "" );
	m_pPath->setMaxLength( 32767 );
	m_pPath->setFrame( QLineEdit::Normal );
	m_pPath->setFrame( TRUE );

	m_pOKButton = new QPushButton( this, "PushButton_1" );
	m_pOKButton->setGeometry( 420, 12, 80, 25 );
	m_pOKButton->setMinimumSize( 0, 0 );
	m_pOKButton->setMaximumSize( 32767, 32767 );
	connect( m_pOKButton, SIGNAL(clicked()), SLOT(accept()) );
	m_pOKButton->setFocusPolicy( QWidget::TabFocus );
	m_pOKButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pOKButton->setFontPropagation( QWidget::NoChildren );
	m_pOKButton->setPalettePropagation( QWidget::NoChildren );
	m_pOKButton->setText( "---" );
	m_pOKButton->setAutoRepeat( FALSE );
	m_pOKButton->setAutoResize( FALSE );
	m_pOKButton->setToggleButton( FALSE );
	m_pOKButton->setDefault( FALSE );
	m_pOKButton->setAutoDefault( FALSE );
	m_pOKButton->setIsMenuButton( FALSE );

	m_pCancelButton = new QPushButton( this, "PushButton_2" );
	m_pCancelButton->setGeometry( 420, 47, 80, 25 );
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
	m_pCancelButton->setAutoDefault( FALSE );
	m_pCancelButton->setIsMenuButton( FALSE );

	m_pTree = new QListView( this, "ListView_1" );
	m_pTree->setGeometry( 13, 83, 392, 228 );
	m_pTree->setMinimumSize( 0, 0 );
	m_pTree->setMaximumSize( 32767, 32767 );
	connect( m_pTree, SIGNAL(doubleClicked(QListViewItem*)), SLOT(OnDoubleClicked(QListViewItem*)) );
	m_pTree->setFocusPolicy( QWidget::StrongFocus );
	m_pTree->setBackgroundMode( QWidget::PaletteBackground );
	m_pTree->setFontPropagation( QWidget::NoChildren );
	m_pTree->setPalettePropagation( QWidget::NoChildren );
	m_pTree->setFrameStyle( 50 );
	m_pTree->setLineWidth( 2 );
	m_pTree->setMidLineWidth( 0 );
	m_pTree->QFrame::setMargin( 0 );
	m_pTree->setResizePolicy( QScrollView::Manual );
	m_pTree->setVScrollBarMode( QScrollView::Auto );
	m_pTree->setHScrollBarMode( QScrollView::Auto );
	m_pTree->setTreeStepSize( 20 );
	m_pTree->setMultiSelection( FALSE );
	m_pTree->setAllColumnsShowFocus( FALSE );
	m_pTree->setItemMargin( 1 );
	m_pTree->setRootIsDecorated( FALSE );
	m_pTree->addColumn( "Sample items", -1 );
	m_pTree->setColumnWidthMode( 0, QListView::Maximum );
	m_pTree->setColumnAlignment( 0, 1 );

	m_pConnectAsLabel = new QLabel( this, "Label_2" );
	m_pConnectAsLabel->setGeometry( 13, 49, 142, 22 );
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

	m_pConnectAs = new QLineEdit( this, "LineEdit_2" );
	m_pConnectAs->setGeometry( 165, 47, 240, 25 );
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

	resize( 512,324 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


CPrinterSelectionDialogData::~CPrinterSelectionDialogData()
{
}
void CPrinterSelectionDialogData::OnDoubleClicked(QListViewItem*)
{
}
