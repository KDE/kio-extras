/* Name: CorelFileDialogData.cpp

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
#include "CorelFileDialogData.h"

#define Inherited QDialog


CCorelFileDialogData::CCorelFileDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pLookInLabel = new QLabel( this, "Label_1" );
	m_pLookInLabel->setGeometry( 10, 10, 85, 25 );
	m_pLookInLabel->setMinimumSize( 0, 0 );
	m_pLookInLabel->setMaximumSize( 32767, 32767 );
	m_pLookInLabel->setFocusPolicy( QWidget::NoFocus );
	m_pLookInLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pLookInLabel->setFontPropagation( QWidget::NoChildren );
	m_pLookInLabel->setPalettePropagation( QWidget::NoChildren );
	m_pLookInLabel->setFrameStyle( 0 );
	m_pLookInLabel->setLineWidth( 1 );
	m_pLookInLabel->setMidLineWidth( 0 );
	m_pLookInLabel->QFrame::setMargin( 0 );
	m_pLookInLabel->setText( "---" );
	m_pLookInLabel->setAlignment( 289 );
	m_pLookInLabel->setMargin( -1 );

	m_pLookInCombo = new CPixmapCombo( this, "User_1" );
	m_pLookInCombo->setGeometry( 103, 10, 275, 25 );
	m_pLookInCombo->setMinimumSize( 0, 0 );
	m_pLookInCombo->setMaximumSize( 32767, 32767 );
	m_pLookInCombo->setFocusPolicy( QWidget::StrongFocus );
	m_pLookInCombo->setBackgroundMode( QWidget::PaletteBackground );
	m_pLookInCombo->setFontPropagation( QWidget::NoChildren );
	m_pLookInCombo->setPalettePropagation( QWidget::NoChildren );

	m_pTopButton1 = new QPushButton( this, "PushButton_2" );
	m_pTopButton1->setGeometry( 389, 11, 23, 22 );
	m_pTopButton1->setMinimumSize( 0, 0 );
	m_pTopButton1->setMaximumSize( 32767, 32767 );
	m_pTopButton1->setFocusPolicy( QWidget::TabFocus );
	m_pTopButton1->setBackgroundMode( QWidget::PaletteBackground );
	m_pTopButton1->setFontPropagation( QWidget::NoChildren );
	m_pTopButton1->setPalettePropagation( QWidget::NoChildren );
	m_pTopButton1->setText( "" );
	m_pTopButton1->setAutoRepeat( FALSE );
	m_pTopButton1->setAutoResize( FALSE );
	m_pTopButton1->setToggleButton( FALSE );
	m_pTopButton1->setDefault( FALSE );
	m_pTopButton1->setAutoDefault( FALSE );
	m_pTopButton1->setIsMenuButton( FALSE );

	m_pTopButton2 = new QPushButton( this, "PushButton_3" );
	m_pTopButton2->setGeometry( 420, 11, 23, 22 );
	m_pTopButton2->setMinimumSize( 0, 0 );
	m_pTopButton2->setMaximumSize( 32767, 32767 );
	m_pTopButton2->setFocusPolicy( QWidget::TabFocus );
	m_pTopButton2->setBackgroundMode( QWidget::PaletteBackground );
	m_pTopButton2->setFontPropagation( QWidget::NoChildren );
	m_pTopButton2->setPalettePropagation( QWidget::NoChildren );
	m_pTopButton2->setText( "" );
	m_pTopButton2->setAutoRepeat( FALSE );
	m_pTopButton2->setAutoResize( FALSE );
	m_pTopButton2->setToggleButton( FALSE );
	m_pTopButton2->setDefault( FALSE );
	m_pTopButton2->setAutoDefault( FALSE );
	m_pTopButton2->setIsMenuButton( FALSE );

	m_pTopButton3 = new QPushButton( this, "PushButton_1" );
	m_pTopButton3->setGeometry( 451, 11, 23, 22 );
	m_pTopButton3->setMinimumSize( 0, 0 );
	m_pTopButton3->setMaximumSize( 32767, 32767 );
	m_pTopButton3->setFocusPolicy( QWidget::TabFocus );
	m_pTopButton3->setBackgroundMode( QWidget::PaletteBackground );
	m_pTopButton3->setFontPropagation( QWidget::NoChildren );
	m_pTopButton3->setPalettePropagation( QWidget::NoChildren );
	m_pTopButton3->setText( "" );
	m_pTopButton3->setAutoRepeat( FALSE );
	m_pTopButton3->setAutoResize( FALSE );
	m_pTopButton3->setToggleButton( FALSE );
	m_pTopButton3->setDefault( FALSE );
	m_pTopButton3->setAutoDefault( FALSE );
	m_pTopButton3->setIsMenuButton( FALSE );

	m_pTopButton4 = new QPushButton( this, "PushButton_4" );
	m_pTopButton4->setGeometry( 476, 11, 23, 22 );
	m_pTopButton4->setMinimumSize( 0, 0 );
	m_pTopButton4->setMaximumSize( 32767, 32767 );
	m_pTopButton4->setFocusPolicy( QWidget::TabFocus );
	m_pTopButton4->setBackgroundMode( QWidget::PaletteBackground );
	m_pTopButton4->setFontPropagation( QWidget::NoChildren );
	m_pTopButton4->setPalettePropagation( QWidget::NoChildren );
	m_pTopButton4->setText( "" );
	m_pTopButton4->setAutoRepeat( FALSE );
	m_pTopButton4->setAutoResize( FALSE );
	m_pTopButton4->setToggleButton( FALSE );
	m_pTopButton4->setDefault( FALSE );
	m_pTopButton4->setAutoDefault( FALSE );
	m_pTopButton4->setIsMenuButton( FALSE );

	m_pListView = new CListView( this, "User_2" );
	m_pListView->setGeometry( 10, 46, 544, 168 );
	m_pListView->setMinimumSize( 0, 0 );
	m_pListView->setMaximumSize( 32767, 32767 );
	m_pListView->setFocusPolicy( QWidget::StrongFocus );
	m_pListView->setBackgroundMode( QWidget::PaletteBackground );
	m_pListView->setFontPropagation( QWidget::NoChildren );
	m_pListView->setPalettePropagation( QWidget::NoChildren );

	m_pFileNameLabel = new QLabel( this, "Label_3" );
	m_pFileNameLabel->setGeometry( 11, 223, 95, 25 );
	m_pFileNameLabel->setMinimumSize( 0, 0 );
	m_pFileNameLabel->setMaximumSize( 32767, 32767 );
	m_pFileNameLabel->setFocusPolicy( QWidget::NoFocus );
	m_pFileNameLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pFileNameLabel->setFontPropagation( QWidget::NoChildren );
	m_pFileNameLabel->setPalettePropagation( QWidget::NoChildren );
	m_pFileNameLabel->setFrameStyle( 0 );
	m_pFileNameLabel->setLineWidth( 1 );
	m_pFileNameLabel->setMidLineWidth( 0 );
	m_pFileNameLabel->QFrame::setMargin( 0 );
	m_pFileNameLabel->setText( "File name:" );
	m_pFileNameLabel->setAlignment( 289 );
	m_pFileNameLabel->setMargin( -1 );

	m_pFileNameEdit = new QLineEdit( this, "LineEdit_1" );
	m_pFileNameEdit->setGeometry( 112, 223, 331, 25 );
	m_pFileNameEdit->setMinimumSize( 0, 0 );
	m_pFileNameEdit->setMaximumSize( 32767, 32767 );
	m_pFileNameEdit->setFocusPolicy( QWidget::StrongFocus );
	m_pFileNameEdit->setBackgroundMode( QWidget::PaletteBase );
	m_pFileNameEdit->setFontPropagation( QWidget::NoChildren );
	m_pFileNameEdit->setPalettePropagation( QWidget::NoChildren );
	m_pFileNameEdit->setText( "" );
	m_pFileNameEdit->setMaxLength( 32767 );
	m_pFileNameEdit->setFrame( QLineEdit::Normal );
	m_pFileNameEdit->setFrame( TRUE );

	m_pFileTypeLabel = new QLabel( this, "Label_4" );
	m_pFileTypeLabel->setGeometry( 11, 256, 89, 25 );
	m_pFileTypeLabel->setMinimumSize( 0, 0 );
	m_pFileTypeLabel->setMaximumSize( 32767, 32767 );
	m_pFileTypeLabel->setFocusPolicy( QWidget::NoFocus );
	m_pFileTypeLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pFileTypeLabel->setFontPropagation( QWidget::NoChildren );
	m_pFileTypeLabel->setPalettePropagation( QWidget::NoChildren );
	m_pFileTypeLabel->setFrameStyle( 0 );
	m_pFileTypeLabel->setLineWidth( 1 );
	m_pFileTypeLabel->setMidLineWidth( 0 );
	m_pFileTypeLabel->QFrame::setMargin( 0 );
	m_pFileTypeLabel->setText( "Files of type:" );
	m_pFileTypeLabel->setAlignment( 289 );
	m_pFileTypeLabel->setMargin( -1 );

	m_pFileTypeCombo = new QComboBox( FALSE, this, "ComboBox_2" );
	m_pFileTypeCombo->setGeometry( 112, 256, 332, 25 );
	m_pFileTypeCombo->setMinimumSize( 0, 0 );
	m_pFileTypeCombo->setMaximumSize( 32767, 32767 );
	m_pFileTypeCombo->setFocusPolicy( QWidget::StrongFocus );
	m_pFileTypeCombo->setBackgroundMode( QWidget::PaletteBackground );
	m_pFileTypeCombo->setFontPropagation( QWidget::AllChildren );
	m_pFileTypeCombo->setPalettePropagation( QWidget::AllChildren );
	m_pFileTypeCombo->setSizeLimit( 10 );
	m_pFileTypeCombo->setAutoResize( FALSE );
	m_pFileTypeCombo->setMaxCount( 2147483647 );
	m_pFileTypeCombo->setAutoCompletion( FALSE );

	m_pAcceptButton = new QPushButton( this, "PushButton_5" );
	m_pAcceptButton->setGeometry( 454, 223, 100, 28 );
	m_pAcceptButton->setMinimumSize( 0, 0 );
	m_pAcceptButton->setMaximumSize( 32767, 32767 );
	connect( m_pAcceptButton, SIGNAL(clicked()), SLOT(OnAcceptClicked()) );
	m_pAcceptButton->setFocusPolicy( QWidget::TabFocus );
	m_pAcceptButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pAcceptButton->setFontPropagation( QWidget::NoChildren );
	m_pAcceptButton->setPalettePropagation( QWidget::NoChildren );
	m_pAcceptButton->setText( "---" );
	m_pAcceptButton->setAutoRepeat( FALSE );
	m_pAcceptButton->setAutoResize( FALSE );
	m_pAcceptButton->setToggleButton( FALSE );
	m_pAcceptButton->setDefault( FALSE );
	m_pAcceptButton->setAutoDefault( FALSE );
	m_pAcceptButton->setIsMenuButton( FALSE );

	m_pCancelButton = new QPushButton( this, "PushButton_6" );
	m_pCancelButton->setGeometry( 455, 257, 100, 28 );
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

	resize( 565,295 );
	setMinimumSize( 565, 318 );
	setMaximumSize( 565, 318 );
}


CCorelFileDialogData::~CCorelFileDialogData()
{
}
void CCorelFileDialogData::OnAcceptClicked()
{
}
