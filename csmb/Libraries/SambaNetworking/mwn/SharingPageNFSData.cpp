/* Name: SharingPageNFSData.cpp

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
#include "SharingPageNFSData.h"

#define Inherited QDialog


CSharingPageNFSData::CSharingPageNFSData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_IconLabel = new QLabel( this, "Label_7" );
	m_IconLabel->setGeometry( 10, 10, 60, 40 );
	m_IconLabel->setMinimumSize( 0, 0 );
	m_IconLabel->setMaximumSize( 32767, 32767 );
	m_IconLabel->setFocusPolicy( QWidget::NoFocus );
	m_IconLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_IconLabel->setFontPropagation( QWidget::NoChildren );
	m_IconLabel->setPalettePropagation( QWidget::NoChildren );
	m_IconLabel->setFrameStyle( 0 );
	m_IconLabel->setLineWidth( 1 );
	m_IconLabel->setMidLineWidth( 0 );
	m_IconLabel->QFrame::setMargin( 0 );
	m_IconLabel->setText( "---" );
	m_IconLabel->setAlignment( 289 );
	m_IconLabel->setMargin( -1 );

	m_PathLabel = new QLabel( this, "Label_8" );
	m_PathLabel->setGeometry( 80, 10, 395, 52 );
	m_PathLabel->setMinimumSize( 0, 0 );
	m_PathLabel->setMaximumSize( 32767, 32767 );
	m_PathLabel->setFocusPolicy( QWidget::NoFocus );
	m_PathLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_PathLabel->setFontPropagation( QWidget::NoChildren );
	m_PathLabel->setPalettePropagation( QWidget::NoChildren );
	m_PathLabel->setFrameStyle( 0 );
	m_PathLabel->setLineWidth( 1 );
	m_PathLabel->setMidLineWidth( 0 );
	m_PathLabel->QFrame::setMargin( 0 );
	m_PathLabel->setText( "---" );
	m_PathLabel->setAlignment( 289 );
	m_PathLabel->setMargin( -1 );

	m_pSeparator1 = new QFrame( this, "Frame_4" );
	m_pSeparator1->setGeometry( 12, 66, 466, 10 );
	m_pSeparator1->setMinimumSize( 0, 0 );
	m_pSeparator1->setMaximumSize( 32767, 32767 );
	m_pSeparator1->setFocusPolicy( QWidget::NoFocus );
	m_pSeparator1->setBackgroundMode( QWidget::PaletteBackground );
	m_pSeparator1->setFontPropagation( QWidget::NoChildren );
	m_pSeparator1->setPalettePropagation( QWidget::NoChildren );
	m_pSeparator1->setFrameStyle( 52 );
	m_pSeparator1->setLineWidth( 1 );
	m_pSeparator1->setMidLineWidth( 0 );
	m_pSeparator1->QFrame::setMargin( 0 );

	m_pIsSharedCheckbox = new QCheckBox( this, "CheckBox_3" );
	m_pIsSharedCheckbox->setGeometry( 12, 80, 449, 20 );
	m_pIsSharedCheckbox->setMinimumSize( 0, 0 );
	m_pIsSharedCheckbox->setMaximumSize( 32767, 32767 );
	connect( m_pIsSharedCheckbox, SIGNAL(clicked()), SLOT(OnIsShared()) );
	m_pIsSharedCheckbox->setFocusPolicy( QWidget::TabFocus );
	m_pIsSharedCheckbox->setBackgroundMode( QWidget::PaletteBackground );
	m_pIsSharedCheckbox->setFontPropagation( QWidget::NoChildren );
	m_pIsSharedCheckbox->setPalettePropagation( QWidget::NoChildren );
	m_pIsSharedCheckbox->setText( "---" );
	m_pIsSharedCheckbox->setAutoRepeat( FALSE );
	m_pIsSharedCheckbox->setAutoResize( FALSE );
	m_pIsSharedCheckbox->setChecked( FALSE );

	m_pListView = new CListView( this, "User_1" );
	m_pListView->setGeometry( 10, 110, 380, 250 );
	m_pListView->setMinimumSize( 0, 0 );
	m_pListView->setMaximumSize( 32767, 32767 );
	m_pListView->setFocusPolicy( QWidget::NoFocus );
	m_pListView->setBackgroundMode( QWidget::PaletteBackground );
	m_pListView->setFontPropagation( QWidget::NoChildren );
	m_pListView->setPalettePropagation( QWidget::AllChildren );

	m_pAddButton = new QPushButton( this, "PushButton_4" );
	m_pAddButton->setGeometry( 400, 110, 80, 26 );
	m_pAddButton->setMinimumSize( 0, 0 );
	m_pAddButton->setMaximumSize( 32767, 32767 );
	connect( m_pAddButton, SIGNAL(clicked()), SLOT(OnAdd()) );
	m_pAddButton->setFocusPolicy( QWidget::TabFocus );
	m_pAddButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pAddButton->setFontPropagation( QWidget::NoChildren );
	m_pAddButton->setPalettePropagation( QWidget::NoChildren );
	m_pAddButton->setText( "---" );
	m_pAddButton->setAutoRepeat( FALSE );
	m_pAddButton->setAutoResize( FALSE );
	m_pAddButton->setToggleButton( FALSE );
	m_pAddButton->setDefault( FALSE );
	m_pAddButton->setAutoDefault( FALSE );
	m_pAddButton->setIsMenuButton( FALSE );

	m_pEditButton = new QPushButton( this, "PushButton_5" );
	m_pEditButton->setGeometry( 400, 150, 80, 26 );
	m_pEditButton->setMinimumSize( 0, 0 );
	m_pEditButton->setMaximumSize( 32767, 32767 );
	connect( m_pEditButton, SIGNAL(clicked()), SLOT(OnEdit()) );
	m_pEditButton->setFocusPolicy( QWidget::TabFocus );
	m_pEditButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pEditButton->setFontPropagation( QWidget::NoChildren );
	m_pEditButton->setPalettePropagation( QWidget::NoChildren );
	m_pEditButton->setText( "---" );
	m_pEditButton->setAutoRepeat( FALSE );
	m_pEditButton->setAutoResize( FALSE );
	m_pEditButton->setToggleButton( FALSE );
	m_pEditButton->setDefault( FALSE );
	m_pEditButton->setAutoDefault( FALSE );
	m_pEditButton->setIsMenuButton( FALSE );

	m_pDeleteButton = new QPushButton( this, "PushButton_6" );
	m_pDeleteButton->setGeometry( 400, 190, 80, 26 );
	m_pDeleteButton->setMinimumSize( 0, 0 );
	m_pDeleteButton->setMaximumSize( 32767, 32767 );
	connect( m_pDeleteButton, SIGNAL(clicked()), SLOT(OnRemove()) );
	m_pDeleteButton->setFocusPolicy( QWidget::TabFocus );
	m_pDeleteButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pDeleteButton->setFontPropagation( QWidget::NoChildren );
	m_pDeleteButton->setPalettePropagation( QWidget::NoChildren );
	m_pDeleteButton->setText( "---" );
	m_pDeleteButton->setAutoRepeat( FALSE );
	m_pDeleteButton->setAutoResize( FALSE );
	m_pDeleteButton->setToggleButton( FALSE );
	m_pDeleteButton->setDefault( FALSE );
	m_pDeleteButton->setAutoDefault( FALSE );
	m_pDeleteButton->setIsMenuButton( FALSE );

	resize( 489,371 );
	setMinimumSize( 489, 371 );
	setMaximumSize( 489, 371 );
}


CSharingPageNFSData::~CSharingPageNFSData()
{
}
void CSharingPageNFSData::OnIsShared()
{
}
void CSharingPageNFSData::OnAdd()
{
}
void CSharingPageNFSData::OnEdit()
{
}
void CSharingPageNFSData::OnRemove()
{
}
