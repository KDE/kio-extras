/* Name: ExportsEntryDialogData.cpp

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
#include "ExportsEntryDialogData.h"

#define Inherited QDialog


CExportsEntryDialogData::CExportsEntryDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pHostnameLabel = new QLabel( this, "Label_1" );
	m_pHostnameLabel->setGeometry( 6, 6, 130, 25 );
	m_pHostnameLabel->setMinimumSize( 0, 0 );
	m_pHostnameLabel->setMaximumSize( 32767, 32767 );
	m_pHostnameLabel->setFocusPolicy( QWidget::NoFocus );
	m_pHostnameLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pHostnameLabel->setFontPropagation( QWidget::NoChildren );
	m_pHostnameLabel->setPalettePropagation( QWidget::NoChildren );
	m_pHostnameLabel->setFrameStyle( 0 );
	m_pHostnameLabel->setLineWidth( 1 );
	m_pHostnameLabel->setMidLineWidth( 0 );
	m_pHostnameLabel->QFrame::setMargin( 0 );
	m_pHostnameLabel->setText( "---" );
	m_pHostnameLabel->setAlignment( 289 );
	m_pHostnameLabel->setMargin( -1 );

	m_pHostnameEdit = new QLineEdit( this, "LineEdit_1" );
	m_pHostnameEdit->setGeometry( 140, 6, 290, 25 );
	m_pHostnameEdit->setMinimumSize( 0, 0 );
	m_pHostnameEdit->setMaximumSize( 32767, 32767 );
	m_pHostnameEdit->setFocusPolicy( QWidget::StrongFocus );
	m_pHostnameEdit->setBackgroundMode( QWidget::PaletteBase );
	m_pHostnameEdit->setFontPropagation( QWidget::NoChildren );
	m_pHostnameEdit->setPalettePropagation( QWidget::NoChildren );
	m_pHostnameEdit->setText( "" );
	m_pHostnameEdit->setMaxLength( 32767 );
	m_pHostnameEdit->setFrame( QLineEdit::Normal );
	m_pHostnameEdit->setFrame( TRUE );

	m_pAccessTypeLabel = new QLabel( this, "Label_2" );
	m_pAccessTypeLabel->setGeometry( 6, 39, 130, 25 );
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

	m_pAccessType = new QComboBox( FALSE, this, "ComboBox_1" );
	m_pAccessType->setGeometry( 140, 39, 290, 25 );
	m_pAccessType->setMinimumSize( 0, 0 );
	m_pAccessType->setMaximumSize( 32767, 32767 );
	m_pAccessType->setFocusPolicy( QWidget::StrongFocus );
	m_pAccessType->setBackgroundMode( QWidget::PaletteBackground );
	m_pAccessType->setFontPropagation( QWidget::AllChildren );
	m_pAccessType->setPalettePropagation( QWidget::AllChildren );
	m_pAccessType->setSizeLimit( 10 );
	m_pAccessType->setAutoResize( FALSE );
	m_pAccessType->setMaxCount( 2147483647 );
	m_pAccessType->setAutoCompletion( FALSE );

	m_pOptionsLabel = new QLabel( this, "Label_3" );
	m_pOptionsLabel->setGeometry( 6, 72, 130, 25 );
	m_pOptionsLabel->setMinimumSize( 0, 0 );
	m_pOptionsLabel->setMaximumSize( 32767, 32767 );
	m_pOptionsLabel->setFocusPolicy( QWidget::NoFocus );
	m_pOptionsLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pOptionsLabel->setFontPropagation( QWidget::NoChildren );
	m_pOptionsLabel->setPalettePropagation( QWidget::NoChildren );
	m_pOptionsLabel->setFrameStyle( 0 );
	m_pOptionsLabel->setLineWidth( 1 );
	m_pOptionsLabel->setMidLineWidth( 0 );
	m_pOptionsLabel->QFrame::setMargin( 0 );
	m_pOptionsLabel->setText( "---" );
	m_pOptionsLabel->setAlignment( 289 );
	m_pOptionsLabel->setMargin( -1 );

	m_pOptionsEdit = new QLineEdit( this, "LineEdit_2" );
	m_pOptionsEdit->setGeometry( 140, 72, 290, 25 );
	m_pOptionsEdit->setMinimumSize( 0, 0 );
	m_pOptionsEdit->setMaximumSize( 32767, 32767 );
	m_pOptionsEdit->setFocusPolicy( QWidget::StrongFocus );
	m_pOptionsEdit->setBackgroundMode( QWidget::PaletteBase );
	m_pOptionsEdit->setFontPropagation( QWidget::NoChildren );
	m_pOptionsEdit->setPalettePropagation( QWidget::NoChildren );
	m_pOptionsEdit->setText( "" );
	m_pOptionsEdit->setMaxLength( 32767 );
	m_pOptionsEdit->setFrame( QLineEdit::Normal );
	m_pOptionsEdit->setFrame( TRUE );

	m_pOKButton = new QPushButton( this, "PushButton_1" );
	m_pOKButton->setGeometry( 260, 106, 80, 26 );
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
	m_pOKButton->setAutoDefault( TRUE );
	m_pOKButton->setIsMenuButton( FALSE );

	m_pCancelButton = new QPushButton( this, "PushButton_2" );
	m_pCancelButton->setGeometry( 350, 106, 80, 26 );
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

	resize( 436,137 );
	setMinimumSize( 436, 137 );
	setMaximumSize( 436, 137 );
}


CExportsEntryDialogData::~CExportsEntryDialogData()
{
}
