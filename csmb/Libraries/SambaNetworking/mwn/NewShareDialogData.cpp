/* Name: NewShareDialogData.cpp

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
#include "NewShareDialogData.h"

#define Inherited QDialog

#include <qlabel.h>
#include <qpushbt.h>

CNewShareDialogData::CNewShareDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_ButtonGroup = new QButtonGroup( this, "ButtonGroup_1" );
	m_ButtonGroup->setGeometry( 10, 115, 380, 102 );
	m_ButtonGroup->setMinimumSize( 0, 0 );
	m_ButtonGroup->setMaximumSize( 32767, 32767 );
	m_ButtonGroup->setFocusPolicy( QWidget::NoFocus );
	m_ButtonGroup->setBackgroundMode( QWidget::PaletteBackground );
	m_ButtonGroup->setFontPropagation( QWidget::NoChildren );
	m_ButtonGroup->setPalettePropagation( QWidget::NoChildren );
	m_ButtonGroup->setFrameStyle( 49 );
	m_ButtonGroup->setTitle( "---" );
	m_ButtonGroup->setAlignment( 1 );

	m_pShareNameLabel = new QLabel( this, "Label_1" );
	m_pShareNameLabel->setGeometry( 10, 13, 100, 15 );
	m_pShareNameLabel->setMinimumSize( 0, 0 );
	m_pShareNameLabel->setMaximumSize( 32767, 32767 );
	m_pShareNameLabel->setFocusPolicy( QWidget::NoFocus );
	m_pShareNameLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pShareNameLabel->setFontPropagation( QWidget::NoChildren );
	m_pShareNameLabel->setPalettePropagation( QWidget::NoChildren );
	m_pShareNameLabel->setText( "---" );
	m_pShareNameLabel->setAlignment( 289 );
	m_pShareNameLabel->setMargin( -1 );

	m_ShareName = new QLineEdit( this, "LineEdit_1" );
	m_ShareName->setGeometry( 110, 10, 300, 26 );
	m_ShareName->setMinimumSize( 0, 0 );
	m_ShareName->setMaximumSize( 32767, 32767 );
	m_ShareName->setFocusPolicy( QWidget::StrongFocus );
	m_ShareName->setBackgroundMode( QWidget::PaletteBase );
	m_ShareName->setFontPropagation( QWidget::NoChildren );
	m_ShareName->setPalettePropagation( QWidget::NoChildren );
	m_ShareName->setText( "" );
	m_ShareName->setMaxLength( 32767 );
	m_ShareName->setEchoMode( QLineEdit::Normal );
	m_ShareName->setFrame( TRUE );

	m_pCommentLabel = new QLabel( this, "Label_2" );
	m_pCommentLabel->setGeometry( 10, 48, 80, 15 );
	m_pCommentLabel->setMinimumSize( 0, 0 );
	m_pCommentLabel->setMaximumSize( 32767, 32767 );
	m_pCommentLabel->setFocusPolicy( QWidget::NoFocus );
	m_pCommentLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pCommentLabel->setFontPropagation( QWidget::NoChildren );
	m_pCommentLabel->setPalettePropagation( QWidget::NoChildren );
	m_pCommentLabel->setText( "---" );
	m_pCommentLabel->setAlignment( 289 );
	m_pCommentLabel->setMargin( -1 );

	m_Comment = new QLineEdit( this, "LineEdit_2" );
	m_Comment->setGeometry( 110, 45, 300, 26 );
	m_Comment->setMinimumSize( 0, 0 );
	m_Comment->setMaximumSize( 32767, 32767 );
	m_Comment->setFocusPolicy( QWidget::StrongFocus );
	m_Comment->setBackgroundMode( QWidget::PaletteBase );
	m_Comment->setFontPropagation( QWidget::NoChildren );
	m_Comment->setPalettePropagation( QWidget::NoChildren );
	m_Comment->setText( "" );
	m_Comment->setMaxLength( 32767 );
	m_Comment->setEchoMode( QLineEdit::Normal );
	m_Comment->setFrame( TRUE );

	m_MaximumAllowed = new QRadioButton( this, "RadioButton_1" );
	m_MaximumAllowed->setGeometry( 24, 138, 121, 19 );
	m_MaximumAllowed->setMinimumSize( 0, 0 );
	m_MaximumAllowed->setMaximumSize( 32767, 32767 );
	m_MaximumAllowed->setFocusPolicy( QWidget::TabFocus );
	m_MaximumAllowed->setBackgroundMode( QWidget::PaletteBackground );
	m_MaximumAllowed->setFontPropagation( QWidget::NoChildren );
	m_MaximumAllowed->setPalettePropagation( QWidget::NoChildren );
	m_MaximumAllowed->setText( "---" );
	m_MaximumAllowed->setAutoRepeat( FALSE );
	m_MaximumAllowed->setAutoResize( TRUE );

	m_Allow = new QRadioButton( this, "RadioButton_2" );
	m_Allow->setGeometry( 24, 174, 170, 19 );
	m_Allow->setMinimumSize( 0, 0 );
	m_Allow->setMaximumSize( 32767, 32767 );
	m_Allow->setFocusPolicy( QWidget::TabFocus );
	m_Allow->setBackgroundMode( QWidget::PaletteBackground );
	m_Allow->setFontPropagation( QWidget::NoChildren );
	m_Allow->setPalettePropagation( QWidget::NoChildren );
	m_Allow->setText( "---" );
	m_Allow->setAutoRepeat( FALSE );
	m_Allow->setAutoResize( TRUE );

	m_UsersMax = new QSpinBox( this, "SpinBox_1" );
	m_UsersMax->setGeometry( 190, 171, 85, 26 );
	m_UsersMax->setMinimumSize( 0, 0 );
	m_UsersMax->setMaximumSize( 32767, 32767 );
	m_UsersMax->setFocusPolicy( QWidget::StrongFocus );
	m_UsersMax->setBackgroundMode( QWidget::PaletteBackground );
	m_UsersMax->setFontPropagation( QWidget::NoChildren );
	m_UsersMax->setPalettePropagation( QWidget::NoChildren );
	m_UsersMax->setFrameStyle( 50 );
	m_UsersMax->setLineWidth( 2 );
	m_UsersMax->setRange( 1, 10 );
	m_UsersMax->setSteps( 1, 0 );
	m_UsersMax->setPrefix( "" );
	m_UsersMax->setSuffix( "" );
	m_UsersMax->setSpecialValueText( "" );
	m_UsersMax->setWrapping( FALSE );

	m_pUsersLabel = new QLabel( this, "Label_4" );
	m_pUsersLabel->setGeometry( 285, 179, 80, 10 );
	m_pUsersLabel->setMinimumSize( 0, 0 );
	m_pUsersLabel->setMaximumSize( 32767, 32767 );
	m_pUsersLabel->setFocusPolicy( QWidget::NoFocus );
	m_pUsersLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pUsersLabel->setFontPropagation( QWidget::NoChildren );
	m_pUsersLabel->setPalettePropagation( QWidget::NoChildren );
	m_pUsersLabel->setText( "---" );
	m_pUsersLabel->setAlignment( 289 );
	m_pUsersLabel->setMargin( -1 );

	m_pOKButton = new QPushButton( this, "PushButton_1" );
	m_pOKButton->setGeometry( 420, 10, 100, 30 );
	m_pOKButton->setMinimumSize( 0, 0 );
	m_pOKButton->setMaximumSize( 32767, 32767 );
	connect(m_pOKButton, SIGNAL(clicked()), SLOT(accept()) );
	m_pOKButton->setFocusPolicy( QWidget::TabFocus );
	m_pOKButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pOKButton->setFontPropagation( QWidget::NoChildren );
	m_pOKButton->setPalettePropagation( QWidget::NoChildren );
	m_pOKButton->setText( "---" );
	m_pOKButton->setAutoRepeat( FALSE );
	m_pOKButton->setAutoResize( FALSE );

	m_pCancelButton = new QPushButton( this, "PushButton_2" );
	m_pCancelButton->setGeometry( 421, 45, 100, 30 );
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

	m_pPermissionsButton = new QPushButton( this, "PushButton_3" );
	m_pPermissionsButton->setGeometry( 422, 82, 100, 30 );
	m_pPermissionsButton->setMinimumSize( 0, 0 );
	m_pPermissionsButton->setMaximumSize( 32767, 32767 );
	connect( m_pPermissionsButton, SIGNAL(clicked()), SLOT(OnPermissions()) );
	m_pPermissionsButton->setFocusPolicy( QWidget::TabFocus );
	m_pPermissionsButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pPermissionsButton->setFontPropagation( QWidget::NoChildren );
	m_pPermissionsButton->setPalettePropagation( QWidget::NoChildren );
	m_pPermissionsButton->setText( "---" );
	m_pPermissionsButton->setAutoRepeat( FALSE );
	m_pPermissionsButton->setAutoResize( FALSE );

	m_Enabled = new QCheckBox( this, "CheckBox_1" );
	m_Enabled->setGeometry( 10, 77, 400, 30 );
	m_Enabled->setMinimumSize( 0, 0 );
	m_Enabled->setMaximumSize( 32767, 32767 );
	m_Enabled->setFocusPolicy( QWidget::TabFocus );
	m_Enabled->setBackgroundMode( QWidget::PaletteBackground );
	m_Enabled->setFontPropagation( QWidget::NoChildren );
	m_Enabled->setPalettePropagation( QWidget::NoChildren );
	m_Enabled->setText( "---" );
	m_Enabled->setAutoRepeat( FALSE );
	m_Enabled->setAutoResize( FALSE );

	m_ButtonGroup->insert( m_MaximumAllowed );
	m_ButtonGroup->insert( m_Allow );

	resize( 532,229 );
	setMinimumSize( 532,229 );
	setMaximumSize( 532,229 );
}


CNewShareDialogData::~CNewShareDialogData()
{
}
void CNewShareDialogData::OnPermissions()
{
}
