/* Name: ShareLevelAccessDialogData.cpp

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
#include "ShareLevelAccessDialogData.h"

#define Inherited QDialog


CShareLevelAccessDialogData::CShareLevelAccessDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pTopLabel = new QLabel( this, "Label_1" );
	m_pTopLabel->setGeometry( 10, 10, 360, 20 );
	m_pTopLabel->setMinimumSize( 0, 0 );
	m_pTopLabel->setMaximumSize( 32767, 32767 );
	m_pTopLabel->setFocusPolicy( QWidget::NoFocus );
	m_pTopLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pTopLabel->setFontPropagation( QWidget::NoChildren );
	m_pTopLabel->setPalettePropagation( QWidget::NoChildren );
	m_pTopLabel->setFrameStyle( 0 );
	m_pTopLabel->setLineWidth( 1 );
	m_pTopLabel->setMidLineWidth( 0 );
	m_pTopLabel->QFrame::setMargin( 0 );
	m_pTopLabel->setText( "---" );
	m_pTopLabel->setAlignment( 289 );
	m_pTopLabel->setMargin( -1 );

	m_pAllowAnonymousAccessRadio = new QRadioButton( this, "RadioButton_1" );
	m_pAllowAnonymousAccessRadio->setGeometry( 10, 35, 370, 20 );
	m_pAllowAnonymousAccessRadio->setMinimumSize( 0, 0 );
	m_pAllowAnonymousAccessRadio->setMaximumSize( 32767, 32767 );
	connect( m_pAllowAnonymousAccessRadio, SIGNAL(clicked()), SLOT(OnAnonymousAccess()) );
	m_pAllowAnonymousAccessRadio->setFocusPolicy( QWidget::TabFocus );
	m_pAllowAnonymousAccessRadio->setBackgroundMode( QWidget::PaletteBackground );
	m_pAllowAnonymousAccessRadio->setFontPropagation( QWidget::NoChildren );
	m_pAllowAnonymousAccessRadio->setPalettePropagation( QWidget::NoChildren );
	m_pAllowAnonymousAccessRadio->setText( "---" );
	m_pAllowAnonymousAccessRadio->setAutoRepeat( FALSE );
	m_pAllowAnonymousAccessRadio->setAutoResize( FALSE );
	m_pAllowAnonymousAccessRadio->setChecked( FALSE );

	m_pAllowAccessUsingPasswordRadio = new QRadioButton( this, "RadioButton_2" );
	m_pAllowAccessUsingPasswordRadio->setGeometry( 10, 65, 370, 20 );
	m_pAllowAccessUsingPasswordRadio->setMinimumSize( 0, 0 );
	m_pAllowAccessUsingPasswordRadio->setMaximumSize( 32767, 32767 );
	connect( m_pAllowAccessUsingPasswordRadio, SIGNAL(clicked()), SLOT(OnPasswordAccess()) );
	m_pAllowAccessUsingPasswordRadio->setFocusPolicy( QWidget::TabFocus );
	m_pAllowAccessUsingPasswordRadio->setBackgroundMode( QWidget::PaletteBackground );
	m_pAllowAccessUsingPasswordRadio->setFontPropagation( QWidget::NoChildren );
	m_pAllowAccessUsingPasswordRadio->setPalettePropagation( QWidget::NoChildren );
	m_pAllowAccessUsingPasswordRadio->setText( "---" );
	m_pAllowAccessUsingPasswordRadio->setAutoRepeat( FALSE );
	m_pAllowAccessUsingPasswordRadio->setAutoResize( FALSE );
	m_pAllowAccessUsingPasswordRadio->setChecked( FALSE );

	m_pPasswordEdit = new QLineEdit( this, "LineEdit_1" );
	m_pPasswordEdit->setGeometry( 40, 95, 340, 25 );
	m_pPasswordEdit->setMinimumSize( 0, 0 );
	m_pPasswordEdit->setMaximumSize( 32767, 32767 );
	m_pPasswordEdit->setFocusPolicy( QWidget::StrongFocus );
	m_pPasswordEdit->setBackgroundMode( QWidget::PaletteBase );
	m_pPasswordEdit->setFontPropagation( QWidget::NoChildren );
	m_pPasswordEdit->setPalettePropagation( QWidget::NoChildren );
	m_pPasswordEdit->setText( "" );
	m_pPasswordEdit->setMaxLength( 32767 );
	m_pPasswordEdit->setFrame( QLineEdit::Password );
	m_pPasswordEdit->setFrame( TRUE );

	m_pConfirmPasswordLabel = new QLabel( this, "Label_2" );
	m_pConfirmPasswordLabel->setGeometry( 40, 128, 340, 20 );
	m_pConfirmPasswordLabel->setMinimumSize( 0, 0 );
	m_pConfirmPasswordLabel->setMaximumSize( 32767, 32767 );
	m_pConfirmPasswordLabel->setFocusPolicy( QWidget::NoFocus );
	m_pConfirmPasswordLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pConfirmPasswordLabel->setFontPropagation( QWidget::NoChildren );
	m_pConfirmPasswordLabel->setPalettePropagation( QWidget::NoChildren );
	m_pConfirmPasswordLabel->setFrameStyle( 0 );
	m_pConfirmPasswordLabel->setLineWidth( 1 );
	m_pConfirmPasswordLabel->setMidLineWidth( 0 );
	m_pConfirmPasswordLabel->QFrame::setMargin( 0 );
	m_pConfirmPasswordLabel->setText( "---" );
	m_pConfirmPasswordLabel->setAlignment( 289 );
	m_pConfirmPasswordLabel->setMargin( -1 );

	m_pConfirmPasswordEdit = new QLineEdit( this, "LineEdit_2" );
	m_pConfirmPasswordEdit->setGeometry( 40, 159, 340, 25 );
	m_pConfirmPasswordEdit->setMinimumSize( 0, 0 );
	m_pConfirmPasswordEdit->setMaximumSize( 32767, 32767 );
	m_pConfirmPasswordEdit->setFocusPolicy( QWidget::StrongFocus );
	m_pConfirmPasswordEdit->setBackgroundMode( QWidget::PaletteBase );
	m_pConfirmPasswordEdit->setFontPropagation( QWidget::NoChildren );
	m_pConfirmPasswordEdit->setPalettePropagation( QWidget::NoChildren );
	m_pConfirmPasswordEdit->setText( "" );
	m_pConfirmPasswordEdit->setMaxLength( 32767 );
	m_pConfirmPasswordEdit->setFrame( QLineEdit::Password );
	m_pConfirmPasswordEdit->setFrame( TRUE );

	m_pSeparator1 = new QFrame( this, "Frame_1" );
	m_pSeparator1->setGeometry( 10, 194, 379, 5 );
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

	m_pReadOnlyAccessCheckbox = new QCheckBox( this, "CheckBox_1" );
	m_pReadOnlyAccessCheckbox->setGeometry( 10, 206, 360, 17 );
	m_pReadOnlyAccessCheckbox->setMinimumSize( 0, 0 );
	m_pReadOnlyAccessCheckbox->setMaximumSize( 32767, 32767 );
	m_pReadOnlyAccessCheckbox->setFocusPolicy( QWidget::TabFocus );
	m_pReadOnlyAccessCheckbox->setBackgroundMode( QWidget::PaletteBackground );
	m_pReadOnlyAccessCheckbox->setFontPropagation( QWidget::NoChildren );
	m_pReadOnlyAccessCheckbox->setPalettePropagation( QWidget::NoChildren );
	m_pReadOnlyAccessCheckbox->setText( "---" );
	m_pReadOnlyAccessCheckbox->setAutoRepeat( FALSE );
	m_pReadOnlyAccessCheckbox->setAutoResize( FALSE );
	m_pReadOnlyAccessCheckbox->setChecked( FALSE );

	m_pOKButton = new QPushButton( this, "PushButton_1" );
	m_pOKButton->setGeometry( 218, 230, 80, 26 );
	m_pOKButton->setMinimumSize( 0, 0 );
	m_pOKButton->setMaximumSize( 32767, 32767 );
	connect( m_pOKButton, SIGNAL(clicked()), SLOT(accept()) );
	m_pOKButton->setFocusPolicy( QWidget::TabFocus );
	m_pOKButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pOKButton->setFontPropagation( QWidget::NoChildren );
	m_pOKButton->setPalettePropagation( QWidget::NoChildren );
	m_pOKButton->setText( "" );
	m_pOKButton->setAutoRepeat( FALSE );
	m_pOKButton->setAutoResize( FALSE );
	m_pOKButton->setToggleButton( FALSE );
	m_pOKButton->setDefault( FALSE );
	m_pOKButton->setAutoDefault( FALSE );
	m_pOKButton->setIsMenuButton( FALSE );

	m_pCancelButton = new QPushButton( this, "PushButton_2" );
	m_pCancelButton->setGeometry( 309, 230, 80, 26 );
	m_pCancelButton->setMinimumSize( 0, 0 );
	m_pCancelButton->setMaximumSize( 32767, 32767 );
	connect( m_pCancelButton, SIGNAL(clicked()), SLOT(reject()) );
	m_pCancelButton->setFocusPolicy( QWidget::TabFocus );
	m_pCancelButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pCancelButton->setFontPropagation( QWidget::NoChildren );
	m_pCancelButton->setPalettePropagation( QWidget::NoChildren );
	m_pCancelButton->setText( "" );
	m_pCancelButton->setAutoRepeat( FALSE );
	m_pCancelButton->setAutoResize( FALSE );
	m_pCancelButton->setToggleButton( FALSE );
	m_pCancelButton->setDefault( FALSE );
	m_pCancelButton->setAutoDefault( FALSE );
	m_pCancelButton->setIsMenuButton( FALSE );

	resize( 400,266 );
	setMinimumSize( 400, 266 );
	setMaximumSize( 400, 266 );
}


CShareLevelAccessDialogData::~CShareLevelAccessDialogData()
{
}
void CShareLevelAccessDialogData::OnAnonymousAccess()
{
}
void CShareLevelAccessDialogData::OnPasswordAccess()
{
}
