/* Name: PromptDialogData.cpp

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
#include "PromptDialogData.h"

#define Inherited QDialog


CPromptDialogData::CPromptDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pLabel1 = new QLabel( this, "Label_1" );
	m_pLabel1->setGeometry( 10, 13, 366, 28 );
	m_pLabel1->setMinimumSize( 0, 0 );
	m_pLabel1->setMaximumSize( 32767, 32767 );
	m_pLabel1->setFocusPolicy( QWidget::NoFocus );
	m_pLabel1->setBackgroundMode( QWidget::PaletteBackground );
	m_pLabel1->setFontPropagation( QWidget::NoChildren );
	m_pLabel1->setPalettePropagation( QWidget::NoChildren );
	m_pLabel1->setFrameStyle( 0 );
	m_pLabel1->setLineWidth( 1 );
	m_pLabel1->setMidLineWidth( 0 );
	m_pLabel1->QFrame::setMargin( 0 );
	m_pLabel1->setText( "---" );
	m_pLabel1->setAlignment( 289 );
	m_pLabel1->setMargin( -1 );

	m_pLabel2 = new QLabel( this, "Label_2" );
	m_pLabel2->setGeometry( 10, 52, 90, 20 );
	m_pLabel2->setMinimumSize( 0, 0 );
	m_pLabel2->setMaximumSize( 32767, 32767 );
	m_pLabel2->setFocusPolicy( QWidget::NoFocus );
	m_pLabel2->setBackgroundMode( QWidget::PaletteBackground );
	m_pLabel2->setFontPropagation( QWidget::NoChildren );
	m_pLabel2->setPalettePropagation( QWidget::NoChildren );
	m_pLabel2->setFrameStyle( 0 );
	m_pLabel2->setLineWidth( 1 );
	m_pLabel2->setMidLineWidth( 0 );
	m_pLabel2->QFrame::setMargin( 0 );
	m_pLabel2->setText( "---" );
	m_pLabel2->setAlignment( 289 );
	m_pLabel2->setMargin( -1 );

	m_pEdit = new QLineEdit( this, "LineEdit_1" );
	m_pEdit->setGeometry( 100, 47, 280, 26 );
	m_pEdit->setMinimumSize( 0, 0 );
	m_pEdit->setMaximumSize( 32767, 32767 );
	m_pEdit->setFocusPolicy( QWidget::StrongFocus );
	m_pEdit->setBackgroundMode( QWidget::PaletteBase );
	m_pEdit->setFontPropagation( QWidget::NoChildren );
	m_pEdit->setPalettePropagation( QWidget::NoChildren );
	m_pEdit->setText( "" );
	m_pEdit->setMaxLength( 32767 );
	m_pEdit->setFrame( QLineEdit::Password );
	m_pEdit->setFrame( TRUE );

	m_pOKButton = new QPushButton( this, "PushButton_1" );
	m_pOKButton->setGeometry( 210, 88, 80, 26 );
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
	m_pCancelButton->setGeometry( 298, 88, 80, 26 );
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

	resize( 390,125 );
	setMinimumSize( 390, 125 );
	setMaximumSize( 390, 125 );
}


CPromptDialogData::~CPromptDialogData()
{
}
