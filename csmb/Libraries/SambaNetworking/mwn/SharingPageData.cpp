/* Name: SharingPageData.cpp

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
#include "SharingPageData.h"

#define Inherited QDialog

#include <qframe.h>

CSharingPageData::CSharingPageData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_IconLabel = new QLabel( this, "Label_1" );
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

	m_PathLabel = new QLabel( this, "Label_2" );
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

	QFrame* qtarch_Frame_1;
	qtarch_Frame_1 = new QFrame( this, "Frame_1" );
	qtarch_Frame_1->setGeometry( 12, 66, 466, 10 );
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

	m_SharedAs = new QCheckBox( this, "CheckBox_1" );
	m_SharedAs->setGeometry( 12, 84, 449, 12 );
	m_SharedAs->setMinimumSize( 0, 0 );
	m_SharedAs->setMaximumSize( 32767, 32767 );
	connect( m_SharedAs, SIGNAL(clicked()), SLOT(OnSharedAs()) );
	m_SharedAs->setFocusPolicy( QWidget::TabFocus );
	m_SharedAs->setBackgroundMode( QWidget::PaletteBackground );
	m_SharedAs->setFontPropagation( QWidget::NoChildren );
	m_SharedAs->setPalettePropagation( QWidget::NoChildren );
	m_SharedAs->setText( "---" );
	m_SharedAs->setAutoRepeat( FALSE );
	m_SharedAs->setAutoResize( FALSE );
	m_SharedAs->setChecked( FALSE );

	m_ShareNameLabel = new QLabel( this, "Label_3" );
	m_ShareNameLabel->setGeometry( 20, 110, 115, 26 );
	m_ShareNameLabel->setMinimumSize( 0, 0 );
	m_ShareNameLabel->setMaximumSize( 32767, 32767 );
	m_ShareNameLabel->setFocusPolicy( QWidget::NoFocus );
	m_ShareNameLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_ShareNameLabel->setFontPropagation( QWidget::NoChildren );
	m_ShareNameLabel->setPalettePropagation( QWidget::NoChildren );
	m_ShareNameLabel->setFrameStyle( 0 );
	m_ShareNameLabel->setLineWidth( 1 );
	m_ShareNameLabel->setMidLineWidth( 0 );
	m_ShareNameLabel->QFrame::setMargin( 0 );
	m_ShareNameLabel->setText( "---" );
	m_ShareNameLabel->setAlignment( 289 );
	m_ShareNameLabel->setMargin( -1 );

	m_ShareName = new QComboBox( FALSE, this, "ComboBox_1" );
	m_ShareName->setGeometry( 158, 107, 320, 26 );
	m_ShareName->setMinimumSize( 0, 0 );
	m_ShareName->setMaximumSize( 32767, 32767 );
	connect( m_ShareName, SIGNAL(activated(int)), SLOT(OnComboChange(int)) );
	m_ShareName->setFocusPolicy( QWidget::StrongFocus );
	m_ShareName->setBackgroundMode( QWidget::PaletteBackground );
	m_ShareName->setFontPropagation( QWidget::AllChildren );
	m_ShareName->setPalettePropagation( QWidget::AllChildren );
	m_ShareName->setSizeLimit( 10 );
	m_ShareName->setAutoResize( FALSE );
	m_ShareName->setMaxCount( 2147483647 );
	m_ShareName->setAutoCompletion( FALSE );

	m_ShareNameEdit = new QLineEdit( this, "LineEdit_3" );
	m_ShareNameEdit->setGeometry( 158, 107, 320, 26 );
	m_ShareNameEdit->setMinimumSize( 0, 0 );
	m_ShareNameEdit->setMaximumSize( 32767, 32767 );
	m_ShareNameEdit->setFocusPolicy( QWidget::StrongFocus );
	m_ShareNameEdit->setBackgroundMode( QWidget::PaletteBase );
	m_ShareNameEdit->setFontPropagation( QWidget::NoChildren );
	m_ShareNameEdit->setPalettePropagation( QWidget::NoChildren );
	m_ShareNameEdit->setText( "" );
	m_ShareNameEdit->setMaxLength( 32767 );
	m_ShareNameEdit->setFrame( QLineEdit::Normal );
	m_ShareNameEdit->setFrame( TRUE );

	m_CommentLabel = new QLabel( this, "Label_4" );
	m_CommentLabel->setGeometry( 20, 146, 115, 26 );
	m_CommentLabel->setMinimumSize( 0, 0 );
	m_CommentLabel->setMaximumSize( 32767, 32767 );
	m_CommentLabel->setFocusPolicy( QWidget::NoFocus );
	m_CommentLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_CommentLabel->setFontPropagation( QWidget::NoChildren );
	m_CommentLabel->setPalettePropagation( QWidget::NoChildren );
	m_CommentLabel->setFrameStyle( 0 );
	m_CommentLabel->setLineWidth( 1 );
	m_CommentLabel->setMidLineWidth( 0 );
	m_CommentLabel->QFrame::setMargin( 0 );
	m_CommentLabel->setText( "---" );
	m_CommentLabel->setAlignment( 289 );
	m_CommentLabel->setMargin( -1 );

	m_Comment = new QLineEdit( this, "LineEdit_2" );
	m_Comment->setGeometry( 158, 143, 320, 26 );
	m_Comment->setMinimumSize( 0, 0 );
	m_Comment->setMaximumSize( 32767, 32767 );
	m_Comment->setFocusPolicy( QWidget::StrongFocus );
	m_Comment->setBackgroundMode( QWidget::PaletteBase );
	m_Comment->setFontPropagation( QWidget::NoChildren );
	m_Comment->setPalettePropagation( QWidget::NoChildren );
	m_Comment->setText( "" );
	m_Comment->setMaxLength( 32767 );
	m_Comment->setFrame( QLineEdit::Normal );
	m_Comment->setFrame( TRUE );

	m_NewShareButton = new QPushButton( this, "PushButton_2" );
	m_NewShareButton->setGeometry( 302, 184, 82, 26 );
	m_NewShareButton->setMinimumSize( 0, 0 );
	m_NewShareButton->setMaximumSize( 32767, 32767 );
	connect( m_NewShareButton, SIGNAL(clicked()), SLOT(OnNewShare()) );
	m_NewShareButton->setFocusPolicy( QWidget::TabFocus );
	m_NewShareButton->setBackgroundMode( QWidget::PaletteBackground );
	m_NewShareButton->setFontPropagation( QWidget::NoChildren );
	m_NewShareButton->setPalettePropagation( QWidget::NoChildren );
	m_NewShareButton->setText( "---" );
	m_NewShareButton->setAutoRepeat( FALSE );
	m_NewShareButton->setAutoResize( TRUE );
	m_NewShareButton->setToggleButton( FALSE );
	m_NewShareButton->setDefault( FALSE );
	m_NewShareButton->setAutoDefault( FALSE );
	m_NewShareButton->setIsMenuButton( FALSE );

	m_RemoveShareButton = new QPushButton( this, "PushButton_3" );
	m_RemoveShareButton->setGeometry( 396, 184, 82, 26 );
	m_RemoveShareButton->setMinimumSize( 0, 0 );
	m_RemoveShareButton->setMaximumSize( 32767, 32767 );
	connect( m_RemoveShareButton, SIGNAL(clicked()), SLOT(OnRemoveShare()) );
	m_RemoveShareButton->setFocusPolicy( QWidget::TabFocus );
	m_RemoveShareButton->setBackgroundMode( QWidget::PaletteBackground );
	m_RemoveShareButton->setFontPropagation( QWidget::NoChildren );
	m_RemoveShareButton->setPalettePropagation( QWidget::NoChildren );
	m_RemoveShareButton->setText( "---" );
	m_RemoveShareButton->setAutoRepeat( FALSE );
	m_RemoveShareButton->setAutoResize( TRUE );
	m_RemoveShareButton->setToggleButton( FALSE );
	m_RemoveShareButton->setDefault( FALSE );
	m_RemoveShareButton->setAutoDefault( FALSE );
	m_RemoveShareButton->setIsMenuButton( FALSE );

	m_UserLimitLabel = new QLabel( this, "Label_5" );
	m_UserLimitLabel->setGeometry( 20, 213, 90, 26 );
	m_UserLimitLabel->setMinimumSize( 0, 0 );
	m_UserLimitLabel->setMaximumSize( 32767, 32767 );
	m_UserLimitLabel->setFocusPolicy( QWidget::NoFocus );
	m_UserLimitLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_UserLimitLabel->setFontPropagation( QWidget::NoChildren );
	m_UserLimitLabel->setPalettePropagation( QWidget::NoChildren );
	m_UserLimitLabel->setFrameStyle( 0 );
	m_UserLimitLabel->setLineWidth( 1 );
	m_UserLimitLabel->setMidLineWidth( 0 );
	m_UserLimitLabel->QFrame::setMargin( 0 );
	m_UserLimitLabel->setText( "---" );
	m_UserLimitLabel->setAlignment( 289 );
	m_UserLimitLabel->setMargin( -1 );

	m_pSeparator3 = new QFrame( this, "Frame_2" );
	m_pSeparator3->setGeometry( 99, 223, 379, 4 );
	m_pSeparator3->setMinimumSize( 0, 0 );
	m_pSeparator3->setMaximumSize( 32767, 32767 );
	m_pSeparator3->setFocusPolicy( QWidget::NoFocus );
	m_pSeparator3->setBackgroundMode( QWidget::PaletteBackground );
	m_pSeparator3->setFontPropagation( QWidget::NoChildren );
	m_pSeparator3->setPalettePropagation( QWidget::NoChildren );
	m_pSeparator3->setFrameStyle( 52 );
	m_pSeparator3->setLineWidth( 1 );
	m_pSeparator3->setMidLineWidth( 0 );
	m_pSeparator3->QFrame::setMargin( 0 );

	m_MaximumAllowed = new QRadioButton( this, "RadioButton_1" );
	m_MaximumAllowed->setGeometry( 79, 236, 180, 20 );
	m_MaximumAllowed->setMinimumSize( 0, 0 );
	m_MaximumAllowed->setMaximumSize( 32767, 32767 );
	connect( m_MaximumAllowed, SIGNAL(clicked()), SLOT(OnMaximumAllowed()) );
	m_MaximumAllowed->setFocusPolicy( QWidget::TabFocus );
	m_MaximumAllowed->setBackgroundMode( QWidget::PaletteBackground );
	m_MaximumAllowed->setFontPropagation( QWidget::NoChildren );
	m_MaximumAllowed->setPalettePropagation( QWidget::NoChildren );
	m_MaximumAllowed->setText( "---" );
	m_MaximumAllowed->setAutoRepeat( FALSE );
	m_MaximumAllowed->setAutoResize( FALSE );
	m_MaximumAllowed->setChecked( FALSE );

	m_Allow = new QRadioButton( this, "RadioButton_2" );
	m_Allow->setGeometry( 79, 266, 163, 20 );
	m_Allow->setMinimumSize( 0, 0 );
	m_Allow->setMaximumSize( 32767, 32767 );
	connect( m_Allow, SIGNAL(clicked()), SLOT(OnAllow()) );
	m_Allow->setFocusPolicy( QWidget::TabFocus );
	m_Allow->setBackgroundMode( QWidget::PaletteBackground );
	m_Allow->setFontPropagation( QWidget::NoChildren );
	m_Allow->setPalettePropagation( QWidget::NoChildren );
	m_Allow->setText( "---" );
	m_Allow->setAutoRepeat( FALSE );
	m_Allow->setAutoResize( FALSE );
	m_Allow->setChecked( FALSE );

	m_UsersMax = new QSpinBox( this, "SpinBox_1" );
	m_UsersMax->setGeometry( 250, 263, 86, 26 );
	m_UsersMax->setMinimumSize( 0, 0 );
	m_UsersMax->setMaximumSize( 32767, 32767 );
	m_UsersMax->setFocusPolicy( QWidget::StrongFocus );
	m_UsersMax->setBackgroundMode( QWidget::PaletteBackground );
	m_UsersMax->setFontPropagation( QWidget::NoChildren );
	m_UsersMax->setPalettePropagation( QWidget::NoChildren );
	m_UsersMax->setFrameStyle( 50 );
	m_UsersMax->setLineWidth( 2 );
	m_UsersMax->setMidLineWidth( 0 );
	m_UsersMax->QFrame::setMargin( 0 );
	m_UsersMax->setRange( 0, 99 );
	m_UsersMax->setSteps( 1, 0 );
	m_UsersMax->setPrefix( "" );
	m_UsersMax->setSuffix( "" );
	m_UsersMax->setSpecialValueText( "" );
	m_UsersMax->setWrapping( FALSE );

	m_UsersLabel = new QLabel( this, "Label_6" );
	m_UsersLabel->setGeometry( 350, 263, 120, 26 );
	m_UsersLabel->setMinimumSize( 0, 0 );
	m_UsersLabel->setMaximumSize( 32767, 32767 );
	m_UsersLabel->setFocusPolicy( QWidget::NoFocus );
	m_UsersLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_UsersLabel->setFontPropagation( QWidget::NoChildren );
	m_UsersLabel->setPalettePropagation( QWidget::NoChildren );
	m_UsersLabel->setFrameStyle( 0 );
	m_UsersLabel->setLineWidth( 1 );
	m_UsersLabel->setMidLineWidth( 0 );
	m_UsersLabel->QFrame::setMargin( 0 );
	m_UsersLabel->setText( "---" );
	m_UsersLabel->setAlignment( 289 );
	m_UsersLabel->setMargin( -1 );

	QFrame* qtarch_Frame_3;
	qtarch_Frame_3 = new QFrame( this, "Frame_3" );
	qtarch_Frame_3->setGeometry( 20, 300, 457, 4 );
	qtarch_Frame_3->setMinimumSize( 0, 0 );
	qtarch_Frame_3->setMaximumSize( 32767, 32767 );
	qtarch_Frame_3->setFocusPolicy( QWidget::NoFocus );
	qtarch_Frame_3->setBackgroundMode( QWidget::PaletteBackground );
	qtarch_Frame_3->setFontPropagation( QWidget::NoChildren );
	qtarch_Frame_3->setPalettePropagation( QWidget::NoChildren );
	qtarch_Frame_3->setFrameStyle( 52 );
	qtarch_Frame_3->setLineWidth( 1 );
	qtarch_Frame_3->setMidLineWidth( 0 );
	qtarch_Frame_3->QFrame::setMargin( 0 );

	m_PermissionsButton = new QPushButton( this, "PushButton_1" );
	m_PermissionsButton->setGeometry( 20, 312, 100, 26 );
	m_PermissionsButton->setMinimumSize( 0, 0 );
	m_PermissionsButton->setMaximumSize( 32767, 32767 );
	connect( m_PermissionsButton, SIGNAL(clicked()), SLOT(OnPermissions()) );
	m_PermissionsButton->setFocusPolicy( QWidget::TabFocus );
	m_PermissionsButton->setBackgroundMode( QWidget::PaletteBackground );
	m_PermissionsButton->setFontPropagation( QWidget::NoChildren );
	m_PermissionsButton->setPalettePropagation( QWidget::NoChildren );
	m_PermissionsButton->setText( "---" );
	m_PermissionsButton->setAutoRepeat( FALSE );
	m_PermissionsButton->setAutoResize( FALSE );
	m_PermissionsButton->setToggleButton( FALSE );
	m_PermissionsButton->setDefault( FALSE );
	m_PermissionsButton->setAutoDefault( FALSE );
	m_PermissionsButton->setIsMenuButton( FALSE );

	m_Enabled = new QCheckBox( this, "CheckBox_2" );
	m_Enabled->setGeometry( 216, 312, 260, 26 );
	m_Enabled->setMinimumSize( 0, 0 );
	m_Enabled->setMaximumSize( 32767, 32767 );
	m_Enabled->setFocusPolicy( QWidget::TabFocus );
	m_Enabled->setBackgroundMode( QWidget::PaletteBackground );
	m_Enabled->setFontPropagation( QWidget::NoChildren );
	m_Enabled->setPalettePropagation( QWidget::NoChildren );
	m_Enabled->setText( "---" );
	m_Enabled->setAutoRepeat( FALSE );
	m_Enabled->setAutoResize( FALSE );
	m_Enabled->setChecked( FALSE );

	resize( 489,371 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


CSharingPageData::~CSharingPageData()
{
}
void CSharingPageData::OnSharedAs()
{
}
void CSharingPageData::OnComboChange(int)
{
}
void CSharingPageData::OnNewShare()
{
}
void CSharingPageData::OnRemoveShare()
{
}
void CSharingPageData::OnMaximumAllowed()
{
}
void CSharingPageData::OnAllow()
{
}
void CSharingPageData::OnPermissions()
{
}
