/* Name: MessageBoxFourData.cpp

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
#include "MessageBoxFourData.h"

#define Inherited QDialog


CMessageBoxFourData::CMessageBoxFourData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pIcon = new QLabel( this, "Label_1" );
	m_pIcon->setGeometry( 20, 20, 32, 32 );
	m_pIcon->setMinimumSize( 0, 0 );
	m_pIcon->setMaximumSize( 32767, 32767 );
	m_pIcon->setFocusPolicy( QWidget::NoFocus );
	m_pIcon->setBackgroundMode( QWidget::PaletteBackground );
	m_pIcon->setFontPropagation( QWidget::NoChildren );
	m_pIcon->setPalettePropagation( QWidget::NoChildren );
	m_pIcon->setFrameStyle( 0 );
	m_pIcon->setLineWidth( 1 );
	m_pIcon->setMidLineWidth( 0 );
	m_pIcon->QFrame::setMargin( 0 );
	m_pIcon->setText( "---" );
	m_pIcon->setAlignment( 289 );
	m_pIcon->setMargin( -1 );

	m_pText = new QLabel( this, "Label_2" );
	m_pText->setGeometry( 84, 20, 449, 85 );
	m_pText->setMinimumSize( 0, 0 );
	m_pText->setMaximumSize( 32767, 32767 );
	m_pText->setFocusPolicy( QWidget::NoFocus );
	m_pText->setBackgroundMode( QWidget::PaletteBackground );
	m_pText->setFontPropagation( QWidget::NoChildren );
	m_pText->setPalettePropagation( QWidget::NoChildren );
	m_pText->setFrameStyle( 0 );
	m_pText->setLineWidth( 1 );
	m_pText->setMidLineWidth( 0 );
	m_pText->QFrame::setMargin( 0 );
	m_pText->setText( "----" );
	m_pText->setAlignment( 1313 );
	m_pText->setMargin( -1 );

	m_pButton1 = new QPushButton( this, "PushButton_1" );
	m_pButton1->setGeometry( 196, 120, 80, 26 );
	m_pButton1->setMinimumSize( 0, 0 );
	m_pButton1->setMaximumSize( 32767, 32767 );
	connect( m_pButton1, SIGNAL(clicked()), SLOT(OnButton1()) );
	m_pButton1->setFocusPolicy( QWidget::TabFocus );
	m_pButton1->setBackgroundMode( QWidget::PaletteBackground );
	m_pButton1->setFontPropagation( QWidget::NoChildren );
	m_pButton1->setPalettePropagation( QWidget::NoChildren );
	m_pButton1->setText( "---" );
	m_pButton1->setAutoRepeat( FALSE );
	m_pButton1->setAutoResize( FALSE );
	m_pButton1->setToggleButton( FALSE );
	m_pButton1->setDefault( FALSE );
	m_pButton1->setAutoDefault( FALSE );
	m_pButton1->setIsMenuButton( FALSE );

	m_pButton2 = new QPushButton( this, "PushButton_2" );
	m_pButton2->setGeometry( 284, 120, 80, 26 );
	m_pButton2->setMinimumSize( 0, 0 );
	m_pButton2->setMaximumSize( 32767, 32767 );
	connect( m_pButton2, SIGNAL(clicked()), SLOT(OnButton2()) );
	m_pButton2->setFocusPolicy( QWidget::TabFocus );
	m_pButton2->setBackgroundMode( QWidget::PaletteBackground );
	m_pButton2->setFontPropagation( QWidget::NoChildren );
	m_pButton2->setPalettePropagation( QWidget::NoChildren );
	m_pButton2->setText( "---" );
	m_pButton2->setAutoRepeat( FALSE );
	m_pButton2->setAutoResize( FALSE );
	m_pButton2->setToggleButton( FALSE );
	m_pButton2->setDefault( FALSE );
	m_pButton2->setAutoDefault( FALSE );
	m_pButton2->setIsMenuButton( FALSE );

	m_pButton3 = new QPushButton( this, "PushButton_3" );
	m_pButton3->setGeometry( 372, 120, 80, 26 );
	m_pButton3->setMinimumSize( 0, 0 );
	m_pButton3->setMaximumSize( 32767, 32767 );
	connect( m_pButton3, SIGNAL(clicked()), SLOT(OnButton3()) );
	m_pButton3->setFocusPolicy( QWidget::TabFocus );
	m_pButton3->setBackgroundMode( QWidget::PaletteBackground );
	m_pButton3->setFontPropagation( QWidget::NoChildren );
	m_pButton3->setPalettePropagation( QWidget::NoChildren );
	m_pButton3->setText( "---" );
	m_pButton3->setAutoRepeat( FALSE );
	m_pButton3->setAutoResize( FALSE );
	m_pButton3->setToggleButton( FALSE );
	m_pButton3->setDefault( FALSE );
	m_pButton3->setAutoDefault( FALSE );
	m_pButton3->setIsMenuButton( FALSE );

	m_pButton4 = new QPushButton( this, "PushButton_4" );
	m_pButton4->setGeometry( 460, 120, 80, 26 );
	m_pButton4->setMinimumSize( 0, 0 );
	m_pButton4->setMaximumSize( 32767, 32767 );
	connect( m_pButton4, SIGNAL(clicked()), SLOT(OnButton4()) );
	m_pButton4->setFocusPolicy( QWidget::TabFocus );
	m_pButton4->setBackgroundMode( QWidget::PaletteBackground );
	m_pButton4->setFontPropagation( QWidget::NoChildren );
	m_pButton4->setPalettePropagation( QWidget::NoChildren );
	m_pButton4->setText( "---" );
	m_pButton4->setAutoRepeat( FALSE );
	m_pButton4->setAutoResize( FALSE );
	m_pButton4->setToggleButton( FALSE );
	m_pButton4->setDefault( FALSE );
	m_pButton4->setAutoDefault( FALSE );
	m_pButton4->setIsMenuButton( FALSE );

	resize( 552,157 );
	setMinimumSize( 552, 157 );
	setMaximumSize( 552, 157 );
}


CMessageBoxFourData::~CMessageBoxFourData()
{
}
void CMessageBoxFourData::OnButton1()
{
}
void CMessageBoxFourData::OnButton2()
{
}
void CMessageBoxFourData::OnButton3()
{
}
void CMessageBoxFourData::OnButton4()
{
}
