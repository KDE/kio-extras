/* Name: PasswordDlgData.cpp

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
#include "PasswordDlgData.h"

#define Inherited QDialog


CPasswordDlgData::CPasswordDlgData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pTopLine1 = new QLabel( this, "Label_4" );
	m_pTopLine1->setGeometry( 6, 6, 380, 20 );
	m_pTopLine1->setMinimumSize( 0, 0 );
	m_pTopLine1->setMaximumSize( 32767, 32767 );
	m_pTopLine1->setFocusPolicy( QWidget::NoFocus );
	m_pTopLine1->setBackgroundMode( QWidget::PaletteBackground );
	m_pTopLine1->setFontPropagation( QWidget::NoChildren );
	m_pTopLine1->setPalettePropagation( QWidget::NoChildren );
	m_pTopLine1->setFrameStyle( 0 );
	m_pTopLine1->setLineWidth( 1 );
	m_pTopLine1->setMidLineWidth( 0 );
	m_pTopLine1->QFrame::setMargin( 0 );
	m_pTopLine1->setText( "---" );
	m_pTopLine1->setAlignment( 289 );
	m_pTopLine1->setMargin( -1 );

	m_pTopLine2 = new QLabel( this, "Label_5" );
	m_pTopLine2->setGeometry( 6, 26, 379, 20 );
	m_pTopLine2->setMinimumSize( 0, 0 );
	m_pTopLine2->setMaximumSize( 32767, 32767 );
	m_pTopLine2->setFocusPolicy( QWidget::NoFocus );
	m_pTopLine2->setBackgroundMode( QWidget::PaletteBackground );
	m_pTopLine2->setFontPropagation( QWidget::NoChildren );
	m_pTopLine2->setPalettePropagation( QWidget::NoChildren );
	m_pTopLine2->setFrameStyle( 0 );
	m_pTopLine2->setLineWidth( 1 );
	m_pTopLine2->setMidLineWidth( 0 );
	m_pTopLine2->QFrame::setMargin( 0 );
	m_pTopLine2->setText( "---" );
	m_pTopLine2->setAlignment( 289 );
	m_pTopLine2->setMargin( -1 );

	m_pUsernameLabel = new QLabel( this, "Label_1" );
	m_pUsernameLabel->setGeometry( 6, 50, 110, 25 );
	m_pUsernameLabel->setMinimumSize( 0, 0 );
	m_pUsernameLabel->setMaximumSize( 32767, 32767 );
	m_pUsernameLabel->setFocusPolicy( QWidget::NoFocus );
	m_pUsernameLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pUsernameLabel->setFontPropagation( QWidget::NoChildren );
	m_pUsernameLabel->setPalettePropagation( QWidget::NoChildren );
	m_pUsernameLabel->setFrameStyle( 0 );
	m_pUsernameLabel->setLineWidth( 1 );
	m_pUsernameLabel->setMidLineWidth( 0 );
	m_pUsernameLabel->QFrame::setMargin( 0 );
	m_pUsernameLabel->setText( "---" );
	m_pUsernameLabel->setAlignment( 289 );
	m_pUsernameLabel->setMargin( -1 );

	m_pUsername = new QLineEdit( this, "LineEdit_1" );
	m_pUsername->setGeometry( 136, 50, 250, 25 );
	m_pUsername->setMinimumSize( 0, 0 );
	m_pUsername->setMaximumSize( 32767, 32767 );
	m_pUsername->setFocusPolicy( QWidget::StrongFocus );
	m_pUsername->setBackgroundMode( QWidget::PaletteBase );
	m_pUsername->setFontPropagation( QWidget::NoChildren );
	m_pUsername->setPalettePropagation( QWidget::NoChildren );
	m_pUsername->setText( "" );
	m_pUsername->setMaxLength( 32767 );
	m_pUsername->setFrame( QLineEdit::Normal );
	m_pUsername->setFrame( TRUE );

	m_pPasswordLabel = new QLabel( this, "Label_2" );
	m_pPasswordLabel->setGeometry( 6, 90, 120, 25 );
	m_pPasswordLabel->setMinimumSize( 0, 0 );
	m_pPasswordLabel->setMaximumSize( 32767, 32767 );
	m_pPasswordLabel->setFocusPolicy( QWidget::NoFocus );
	m_pPasswordLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pPasswordLabel->setFontPropagation( QWidget::NoChildren );
	m_pPasswordLabel->setPalettePropagation( QWidget::NoChildren );
	m_pPasswordLabel->setFrameStyle( 0 );
	m_pPasswordLabel->setLineWidth( 1 );
	m_pPasswordLabel->setMidLineWidth( 0 );
	m_pPasswordLabel->QFrame::setMargin( 0 );
	m_pPasswordLabel->setText( "---" );
	m_pPasswordLabel->setAlignment( 289 );
	m_pPasswordLabel->setMargin( -1 );

	m_pPassword = new QLineEdit( this, "LineEdit_2" );
	m_pPassword->setGeometry( 136, 90, 250, 25 );
	m_pPassword->setMinimumSize( 0, 0 );
	m_pPassword->setMaximumSize( 32767, 32767 );
	m_pPassword->setFocusPolicy( QWidget::StrongFocus );
	m_pPassword->setBackgroundMode( QWidget::PaletteBase );
	m_pPassword->setFontPropagation( QWidget::NoChildren );
	m_pPassword->setPalettePropagation( QWidget::NoChildren );
	m_pPassword->setText( "" );
	m_pPassword->setMaxLength( 32767 );
	m_pPassword->setFrame( QLineEdit::Normal );
	m_pPassword->setFrame( TRUE );

	m_pDomainLabel = new QLabel( this, "Label_3" );
	m_pDomainLabel->setGeometry( 6, 130, 120, 25 );
	m_pDomainLabel->setMinimumSize( 0, 0 );
	m_pDomainLabel->setMaximumSize( 32767, 32767 );
	m_pDomainLabel->setFocusPolicy( QWidget::NoFocus );
	m_pDomainLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pDomainLabel->setFontPropagation( QWidget::NoChildren );
	m_pDomainLabel->setPalettePropagation( QWidget::NoChildren );
	m_pDomainLabel->setFrameStyle( 0 );
	m_pDomainLabel->setLineWidth( 1 );
	m_pDomainLabel->setMidLineWidth( 0 );
	m_pDomainLabel->QFrame::setMargin( 0 );
	m_pDomainLabel->setText( "---" );
	m_pDomainLabel->setAlignment( 289 );
	m_pDomainLabel->setMargin( -1 );

	m_pDomain = new CPlainCombo( this, "User_1" );
	m_pDomain->setGeometry( 136, 130, 250, 25 );
	m_pDomain->setMinimumSize( 0, 0 );
	m_pDomain->setMaximumSize( 32767, 32767 );
	m_pDomain->setFocusPolicy( QWidget::StrongFocus );
	m_pDomain->setBackgroundMode( QWidget::PaletteBackground );
	m_pDomain->setFontPropagation( QWidget::NoChildren );
	m_pDomain->setPalettePropagation( QWidget::NoChildren );

	m_pOKButton = new QPushButton( this, "PushButton_1" );
	m_pOKButton->setGeometry( 220, 164, 80, 26 );
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
	m_pCancelButton->setGeometry( 307, 164, 80, 26 );
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

	resize( 394,197 );
	setMinimumSize( 394, 197 );
	setMaximumSize( 394, 197 );
}


CPasswordDlgData::~CPasswordDlgData()
{
}
