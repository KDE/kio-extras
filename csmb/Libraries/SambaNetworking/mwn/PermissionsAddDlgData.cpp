/* Name: PermissionsAddDlgData.cpp

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
#include "PermissionsAddDlgData.h"

#define Inherited QDialog


CPermissionsAddDlgData::CPermissionsAddDlgData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pButtonGroup = new QButtonGroup( this, "ButtonGroup_1" );
	m_pButtonGroup->setGeometry( 10, 10, 380, 120 );
	m_pButtonGroup->setMinimumSize( 0, 0 );
	m_pButtonGroup->setMaximumSize( 32767, 32767 );
	m_pButtonGroup->setFocusPolicy( QWidget::NoFocus );
	m_pButtonGroup->setBackgroundMode( QWidget::PaletteBackground );
	m_pButtonGroup->setFontPropagation( QWidget::NoChildren );
	m_pButtonGroup->setPalettePropagation( QWidget::NoChildren );
	m_pButtonGroup->setFrameStyle( 49 );
	m_pButtonGroup->setLineWidth( 1 );
	m_pButtonGroup->setMidLineWidth( 0 );
	m_pButtonGroup->QFrame::setMargin( 0 );
	m_pButtonGroup->setTitle( "---" );
	m_pButtonGroup->setAlignment( 1 );
	m_pButtonGroup->setExclusive( FALSE );

	m_User = new QRadioButton( this, "RadioButton_1" );
	m_User->setGeometry( 20, 34, 90, 20 );
	m_User->setMinimumSize( 0, 0 );
	m_User->setMaximumSize( 32767, 32767 );
	connect( m_User, SIGNAL(clicked()), SLOT(OnUser()) );
	m_User->setFocusPolicy( QWidget::TabFocus );
	m_User->setBackgroundMode( QWidget::PaletteBackground );
	m_User->setFontPropagation( QWidget::NoChildren );
	m_User->setPalettePropagation( QWidget::NoChildren );
	m_User->setText( "---" );
	m_User->setAutoRepeat( FALSE );
	m_User->setAutoResize( FALSE );
	m_User->setChecked( FALSE );

	m_UserCombo = new QComboBox( FALSE, this, "ComboBox_1" );
	m_UserCombo->setGeometry( 110, 32, 270, 25 );
	m_UserCombo->setMinimumSize( 0, 0 );
	m_UserCombo->setMaximumSize( 32767, 32767 );
	m_UserCombo->setFocusPolicy( QWidget::StrongFocus );
	m_UserCombo->setBackgroundMode( QWidget::PaletteBackground );
	m_UserCombo->setFontPropagation( QWidget::AllChildren );
	m_UserCombo->setPalettePropagation( QWidget::AllChildren );
	m_UserCombo->setSizeLimit( 10 );
	m_UserCombo->setAutoResize( FALSE );
	m_UserCombo->setMaxCount( 2147483647 );
	m_UserCombo->setAutoCompletion( FALSE );

	m_Group = new QRadioButton( this, "RadioButton_2" );
	m_Group->setGeometry( 20, 65, 89, 20 );
	m_Group->setMinimumSize( 0, 0 );
	m_Group->setMaximumSize( 32767, 32767 );
	connect( m_Group, SIGNAL(clicked()), SLOT(OnGroup()) );
	m_Group->setFocusPolicy( QWidget::TabFocus );
	m_Group->setBackgroundMode( QWidget::PaletteBackground );
	m_Group->setFontPropagation( QWidget::NoChildren );
	m_Group->setPalettePropagation( QWidget::NoChildren );
	m_Group->setText( "---" );
	m_Group->setAutoRepeat( FALSE );
	m_Group->setAutoResize( FALSE );
	m_Group->setChecked( FALSE );

	m_GroupCombo = new QComboBox( FALSE, this, "ComboBox_2" );
	m_GroupCombo->setGeometry( 110, 63, 270, 25 );
	m_GroupCombo->setMinimumSize( 0, 0 );
	m_GroupCombo->setMaximumSize( 32767, 32767 );
	m_GroupCombo->setFocusPolicy( QWidget::StrongFocus );
	m_GroupCombo->setBackgroundMode( QWidget::PaletteBackground );
	m_GroupCombo->setFontPropagation( QWidget::AllChildren );
	m_GroupCombo->setPalettePropagation( QWidget::AllChildren );
	m_GroupCombo->setSizeLimit( 10 );
	m_GroupCombo->setAutoResize( FALSE );
	m_GroupCombo->setMaxCount( 2147483647 );
	m_GroupCombo->setAutoCompletion( FALSE );

	m_Everyone = new QRadioButton( this, "RadioButton_3" );
	m_Everyone->setGeometry( 20, 96, 280, 20 );
	m_Everyone->setMinimumSize( 0, 0 );
	m_Everyone->setMaximumSize( 32767, 32767 );
	connect( m_Everyone, SIGNAL(clicked()), SLOT(OnEveryone()) );
	m_Everyone->setFocusPolicy( QWidget::TabFocus );
	m_Everyone->setBackgroundMode( QWidget::PaletteBackground );
	m_Everyone->setFontPropagation( QWidget::NoChildren );
	m_Everyone->setPalettePropagation( QWidget::NoChildren );
	m_Everyone->setText( "---" );
	m_Everyone->setAutoRepeat( FALSE );
	m_Everyone->setAutoResize( FALSE );
	m_Everyone->setChecked( FALSE );

	m_pAccessTypeLabel = new QLabel( this, "Label_1" );
	m_pAccessTypeLabel->setGeometry( 10, 140, 110, 30 );
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

	m_AccessType = new QComboBox( FALSE, this, "ComboBox_3" );
	m_AccessType->setGeometry( 120, 143, 270, 25 );
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

	m_pOKButton = new QPushButton( this, "PushButton_1" );
	m_pOKButton->setGeometry( 223, 180, 80, 26 );
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
	m_pCancelButton->setGeometry( 310, 180, 80, 26 );
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

	m_pButtonGroup->insert( m_User );
	m_pButtonGroup->insert( m_Group );
	m_pButtonGroup->insert( m_Everyone );

	resize( 400,215 );
	setMinimumSize( 400, 225 );
	setMaximumSize( 400, 225 );
}


CPermissionsAddDlgData::~CPermissionsAddDlgData()
{
}
void CPermissionsAddDlgData::OnUser()
{
}
void CPermissionsAddDlgData::OnGroup()
{
}
void CPermissionsAddDlgData::OnEveryone()
{
}
void CPermissionsAddDlgData::OnAccessTypeComboChanged(int)
{
}
