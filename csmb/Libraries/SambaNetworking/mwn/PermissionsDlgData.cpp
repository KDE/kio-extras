/* Name: PermissionsDlgData.cpp

   Description: This file is a part of the libmwn library.

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
#include "PermissionsDlgData.h"

#define Inherited QDialog

#include <qframe.h>

CPermissionsDlgData::CPermissionsDlgData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pAccessThroughShareLabel = new QLabel( this, "Label_1" );
	m_pAccessThroughShareLabel->setGeometry( 9, 10, 127, 15 );
	m_pAccessThroughShareLabel->setMinimumSize( 0, 0 );
	m_pAccessThroughShareLabel->setMaximumSize( 32767, 32767 );
	m_pAccessThroughShareLabel->setFocusPolicy( QWidget::NoFocus );
	m_pAccessThroughShareLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pAccessThroughShareLabel->setFontPropagation( QWidget::NoChildren );
	m_pAccessThroughShareLabel->setPalettePropagation( QWidget::NoChildren );
	m_pAccessThroughShareLabel->setFrameStyle( 0 );
	m_pAccessThroughShareLabel->setLineWidth( 1 );
	m_pAccessThroughShareLabel->setMidLineWidth( 0 );
	m_pAccessThroughShareLabel->QFrame::setMargin( 0 );
	m_pAccessThroughShareLabel->setText( "---" );
	m_pAccessThroughShareLabel->setAlignment( 289 );
	m_pAccessThroughShareLabel->setMargin( -1 );
	m_pAccessThroughShareLabel->setAutoResize( true );

	m_ShareName = new QLabel( this, "Label_2" );
	m_ShareName->setGeometry( 174, 10, 241, 15 );
	m_ShareName->setMinimumSize( 0, 0 );
	m_ShareName->setMaximumSize( 32767, 32767 );
	m_ShareName->setFocusPolicy( QWidget::NoFocus );
	m_ShareName->setBackgroundMode( QWidget::PaletteBackground );
	m_ShareName->setFontPropagation( QWidget::NoChildren );
	m_ShareName->setPalettePropagation( QWidget::NoChildren );
	m_ShareName->setFrameStyle( 0 );
	m_ShareName->setLineWidth( 1 );
	m_ShareName->setMidLineWidth( 0 );
	m_ShareName->QFrame::setMargin( 0 );
	m_ShareName->setText( "---" );
	m_ShareName->setAlignment( 289 );
	m_ShareName->setMargin( -1 );

	m_pNameLabel = new QLabel( this, "Label_3" );
	m_pNameLabel->setGeometry( 10, 38, 36, 15 );
	m_pNameLabel->setMinimumSize( 0, 0 );
	m_pNameLabel->setMaximumSize( 32767, 32767 );
	m_pNameLabel->setFocusPolicy( QWidget::NoFocus );
	m_pNameLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pNameLabel->setFontPropagation( QWidget::NoChildren );
	m_pNameLabel->setPalettePropagation( QWidget::NoChildren );
	m_pNameLabel->setFrameStyle( 0 );
	m_pNameLabel->setLineWidth( 1 );
	m_pNameLabel->setMidLineWidth( 0 );
	m_pNameLabel->QFrame::setMargin( 0 );
	m_pNameLabel->setText( "---" );
	m_pNameLabel->setAlignment( 289 );
	m_pNameLabel->setMargin( -1 );
	m_pNameLabel->setAutoResize( true );

	m_List = new QListBox( this, "ListBox_1" );
	m_List->setGeometry( 10, 58, 481, 165 );
	m_List->setMinimumSize( 0, 0 );
	m_List->setMaximumSize( 32767, 32767 );
	connect( m_List, SIGNAL(highlighted(int)), SLOT(OnListSelChanged(int)) );
	m_List->setFocusPolicy( QWidget::StrongFocus );
	m_List->setBackgroundMode( QWidget::PaletteBase );
	m_List->setFontPropagation( QWidget::SameFont );
	m_List->setPalettePropagation( QWidget::SameFont );
	m_List->setFrameStyle( 51 );
	m_List->setLineWidth( 2 );
	m_List->setMidLineWidth( 0 );
	m_List->QFrame::setMargin( 0 );
	m_List->setDragSelect( TRUE );
	m_List->setAutoScroll( TRUE );
	m_List->setScrollBar( FALSE );
	m_List->setAutoScrollBar( TRUE );
	m_List->setBottomScrollBar( FALSE );
	m_List->setAutoBottomScrollBar( TRUE );
	m_List->setSmoothScrolling( TRUE );
	m_List->setMultiSelection( FALSE );
	m_List->setAutoUpdate( TRUE );

	m_pAccessTypeLabel = new QLabel( this, "Label_4" );
	m_pAccessTypeLabel->setGeometry( 10, 233, 100, 21 );
	m_pAccessTypeLabel->setMinimumSize( 0, 0 );
	m_pAccessTypeLabel->setMaximumSize( 32767, 32767 );
	m_pAccessTypeLabel->setFocusPolicy( QWidget::NoFocus );
	m_pAccessTypeLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pAccessTypeLabel->setFontPropagation( QWidget::NoChildren );
	m_pAccessTypeLabel->setPalettePropagation( QWidget::NoChildren );
	m_pAccessTypeLabel->setFrameStyle( 0 );
	m_pAccessTypeLabel->setLineWidth( 1 );
	m_pAccessTypeLabel->setMidLineWidth( 0 );
	m_pAccessTypeLabel->QFrame::setMargin( 0 );
	m_pAccessTypeLabel->setText( "---" );
	m_pAccessTypeLabel->setAlignment( 289 );
	m_pAccessTypeLabel->setMargin( -1 );

	m_AccessType = new QComboBox( FALSE, this, "ComboBox_1" );
	m_AccessType->setGeometry( 115, 230, 180, 26 );
	m_AccessType->setMinimumSize( 0, 0 );
	m_AccessType->setMaximumSize( 32767, 32767 );
	connect( m_AccessType, SIGNAL(activated(int)), SLOT(OnAccessTypeComboChanged(int)) );
	m_AccessType->setFocusPolicy( QWidget::StrongFocus );
	m_AccessType->setBackgroundMode( QWidget::PaletteBackground );
	m_AccessType->setFontPropagation( QWidget::AllChildren );
	m_AccessType->setPalettePropagation( QWidget::AllChildren );
	m_AccessType->setSizeLimit( 10 );
	m_AccessType->setAutoResize( FALSE );
	m_AccessType->setMaxCount( 2147483647 );
	m_AccessType->setAutoCompletion( FALSE );

	m_AddButton = new QPushButton( this, "PushButton_1" );
	m_AddButton->setGeometry( 306, 230, 90, 26 );
	m_AddButton->setMinimumSize( 0, 0 );
	m_AddButton->setMaximumSize( 32767, 32767 );
	connect( m_AddButton, SIGNAL(clicked()), SLOT(OnAdd()) );
	m_AddButton->setFocusPolicy( QWidget::TabFocus );
	m_AddButton->setBackgroundMode( QWidget::PaletteBackground );
	m_AddButton->setFontPropagation( QWidget::NoChildren );
	m_AddButton->setPalettePropagation( QWidget::NoChildren );
	m_AddButton->setText( "---" );
	m_AddButton->setAutoRepeat( FALSE );
	m_AddButton->setAutoResize( FALSE );
	m_AddButton->setToggleButton( FALSE );
	m_AddButton->setDefault( FALSE );
	m_AddButton->setAutoDefault( FALSE );
	m_AddButton->setIsMenuButton( FALSE );

	m_RemoveButton = new QPushButton( this, "PushButton_2" );
	m_RemoveButton->setGeometry( 402, 230, 90, 26 );
	m_RemoveButton->setMinimumSize( 0, 0 );
	m_RemoveButton->setMaximumSize( 32767, 32767 );
	connect( m_RemoveButton, SIGNAL(clicked()), SLOT(OnRemove()) );
	m_RemoveButton->setFocusPolicy( QWidget::TabFocus );
	m_RemoveButton->setBackgroundMode( QWidget::PaletteBackground );
	m_RemoveButton->setFontPropagation( QWidget::NoChildren );
	m_RemoveButton->setPalettePropagation( QWidget::NoChildren );
	m_RemoveButton->setText( "---" );
	m_RemoveButton->setAutoRepeat( FALSE );
	m_RemoveButton->setAutoResize( FALSE );
	m_RemoveButton->setToggleButton( FALSE );
	m_RemoveButton->setDefault( FALSE );
	m_RemoveButton->setAutoDefault( FALSE );
	m_RemoveButton->setIsMenuButton( FALSE );

	m_OKButton = new QPushButton( this, "PushButton_3" );
	m_OKButton->setGeometry( 306, 274, 90, 26 );
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
	m_OKButton->setAutoDefault( FALSE );
	m_OKButton->setIsMenuButton( FALSE );

	m_CancelButton = new QPushButton( this, "PushButton_4" );
	m_CancelButton->setGeometry( 402, 274, 90, 26 );
	m_CancelButton->setMinimumSize( 0, 0 );
	m_CancelButton->setMaximumSize( 32767, 32767 );
	connect( m_CancelButton, SIGNAL(clicked()), SLOT(reject()) );
	m_CancelButton->setFocusPolicy( QWidget::TabFocus );
	m_CancelButton->setBackgroundMode( QWidget::PaletteBackground );
	m_CancelButton->setFontPropagation( QWidget::NoChildren );
	m_CancelButton->setPalettePropagation( QWidget::NoChildren );
	m_CancelButton->setText( "---" );
	m_CancelButton->setAutoRepeat( FALSE );
	m_CancelButton->setAutoResize( FALSE );
	m_CancelButton->setToggleButton( FALSE );
	m_CancelButton->setDefault( FALSE );
	m_CancelButton->setAutoDefault( FALSE );
	m_CancelButton->setIsMenuButton( FALSE );

	QFrame* qtarch_Frame_1;
	qtarch_Frame_1 = new QFrame( this, "Frame_1" );
	qtarch_Frame_1->setGeometry( 10, 264, 481, 4 );
	qtarch_Frame_1->setMinimumSize( 0, 0 );
	qtarch_Frame_1->setMaximumSize( 32767, 32767 );
	qtarch_Frame_1->setFocusPolicy( QWidget::NoFocus );
	qtarch_Frame_1->setBackgroundMode( QWidget::PaletteBackground );
	qtarch_Frame_1->setFontPropagation( QWidget::NoChildren );
	qtarch_Frame_1->setPalettePropagation( QWidget::NoChildren );
	qtarch_Frame_1->setFrameStyle( 52 );
	qtarch_Frame_1->setLineWidth( 1 );
	qtarch_Frame_1->setMidLineWidth( 0 );
	qtarch_Frame_1->QFrame::setMargin( 0 );

	resize( 500,309 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


CPermissionsDlgData::~CPermissionsDlgData()
{
}
void CPermissionsDlgData::OnListSelChanged(int)
{
}
void CPermissionsDlgData::OnAccessTypeComboChanged(int)
{
}
void CPermissionsDlgData::OnAdd()
{
}
void CPermissionsDlgData::OnRemove()
{
}
