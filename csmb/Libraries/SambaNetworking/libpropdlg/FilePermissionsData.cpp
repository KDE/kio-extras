/* Name: FilePermissionsData.cpp

   Description: This file is a part of the Corel File Manager application.

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
#include "FilePermissionsData.h"

#define Inherited QDialog


CFilePermissionsData::CFilePermissionsData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_pOwnershipGroupBox = new QGroupBox( this, "GroupBox_2" );
	m_pOwnershipGroupBox->setGeometry( 15, 221, 461, 121 );
	m_pOwnershipGroupBox->setMinimumSize( 0, 0 );
	m_pOwnershipGroupBox->setMaximumSize( 32767, 32767 );
	m_pOwnershipGroupBox->setFocusPolicy( QWidget::NoFocus );
	m_pOwnershipGroupBox->setBackgroundMode( QWidget::PaletteBackground );
	m_pOwnershipGroupBox->setFontPropagation( QWidget::NoChildren );
	m_pOwnershipGroupBox->setPalettePropagation( QWidget::NoChildren );
	m_pOwnershipGroupBox->setFrameStyle( 49 );
	m_pOwnershipGroupBox->setLineWidth( 1 );
	m_pOwnershipGroupBox->setMidLineWidth( 0 );
	m_pOwnershipGroupBox->QFrame::setMargin( 0 );
	m_pOwnershipGroupBox->setTitle( "---" );
	m_pOwnershipGroupBox->setAlignment( 1 );

	m_pAccessPermissionsGroupBox = new QGroupBox( this, "GroupBox_1" );
	m_pAccessPermissionsGroupBox->setGeometry( 15, 63, 461, 144 );
	m_pAccessPermissionsGroupBox->setMinimumSize( 0, 0 );
	m_pAccessPermissionsGroupBox->setMaximumSize( 32767, 32767 );
	m_pAccessPermissionsGroupBox->setFocusPolicy( QWidget::NoFocus );
	m_pAccessPermissionsGroupBox->setBackgroundMode( QWidget::PaletteBackground );
	m_pAccessPermissionsGroupBox->setFontPropagation( QWidget::NoChildren );
	m_pAccessPermissionsGroupBox->setPalettePropagation( QWidget::NoChildren );
	m_pAccessPermissionsGroupBox->setFrameStyle( 49 );
	m_pAccessPermissionsGroupBox->setLineWidth( 1 );
	m_pAccessPermissionsGroupBox->setMidLineWidth( 0 );
	m_pAccessPermissionsGroupBox->QFrame::setMargin( 0 );
	m_pAccessPermissionsGroupBox->setTitle( "---" );
	m_pAccessPermissionsGroupBox->setAlignment( 1 );

	m_pReadLabel = new QLabel( this, "Label_31" );
	m_pReadLabel->setGeometry( 77, 87, 65, 26 );
	m_pReadLabel->setMinimumSize( 0, 0 );
	m_pReadLabel->setMaximumSize( 32767, 32767 );
	m_pReadLabel->setFocusPolicy( QWidget::NoFocus );
	m_pReadLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pReadLabel->setFontPropagation( QWidget::NoChildren );
	m_pReadLabel->setPalettePropagation( QWidget::NoChildren );
	m_pReadLabel->setFrameStyle( 0 );
	m_pReadLabel->setLineWidth( 1 );
	m_pReadLabel->setMidLineWidth( 0 );
	m_pReadLabel->QFrame::setMargin( 0 );
	m_pReadLabel->setText( "---" );
	m_pReadLabel->setAlignment( 292 );
	m_pReadLabel->setMargin( -1 );

	m_pWriteLabel = new QLabel( this, "Label_32" );
	m_pWriteLabel->setGeometry( 142, 87, 65, 26 );
	m_pWriteLabel->setMinimumSize( 0, 0 );
	m_pWriteLabel->setMaximumSize( 32767, 32767 );
	m_pWriteLabel->setFocusPolicy( QWidget::NoFocus );
	m_pWriteLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pWriteLabel->setFontPropagation( QWidget::NoChildren );
	m_pWriteLabel->setPalettePropagation( QWidget::NoChildren );
	m_pWriteLabel->setFrameStyle( 0 );
	m_pWriteLabel->setLineWidth( 1 );
	m_pWriteLabel->setMidLineWidth( 0 );
	m_pWriteLabel->QFrame::setMargin( 0 );
	m_pWriteLabel->setText( "---" );
	m_pWriteLabel->setAlignment( 292 );
	m_pWriteLabel->setMargin( -1 );

	m_pExecLabel = new QLabel( this, "Label_33" );
	m_pExecLabel->setGeometry( 207, 87, 65, 26 );
	m_pExecLabel->setMinimumSize( 0, 0 );
	m_pExecLabel->setMaximumSize( 32767, 32767 );
	m_pExecLabel->setFocusPolicy( QWidget::NoFocus );
	m_pExecLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pExecLabel->setFontPropagation( QWidget::NoChildren );
	m_pExecLabel->setPalettePropagation( QWidget::NoChildren );
	m_pExecLabel->setFrameStyle( 0 );
	m_pExecLabel->setLineWidth( 1 );
	m_pExecLabel->setMidLineWidth( 0 );
	m_pExecLabel->QFrame::setMargin( 0 );
	m_pExecLabel->setText( "---" );
	m_pExecLabel->setAlignment( 292 );
	m_pExecLabel->setMargin( -1 );

	m_pSpecialLabel = new QLabel( this, "Label_34" );
	m_pSpecialLabel->setGeometry( 272, 87, 74, 26 );
	m_pSpecialLabel->setMinimumSize( 0, 0 );
	m_pSpecialLabel->setMaximumSize( 32767, 32767 );
	m_pSpecialLabel->setFocusPolicy( QWidget::NoFocus );
	m_pSpecialLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pSpecialLabel->setFontPropagation( QWidget::NoChildren );
	m_pSpecialLabel->setPalettePropagation( QWidget::NoChildren );
	m_pSpecialLabel->setFrameStyle( 0 );
	m_pSpecialLabel->setLineWidth( 1 );
	m_pSpecialLabel->setMidLineWidth( 0 );
	m_pSpecialLabel->QFrame::setMargin( 0 );
	m_pSpecialLabel->setText( "---" );
	m_pSpecialLabel->setAlignment( 292 );
	m_pSpecialLabel->setMargin( -1 );

	m_pUserLabel = new QLabel( this, "Label_35" );
	m_pUserLabel->setGeometry( 30, 122, 65, 16 );
	m_pUserLabel->setMinimumSize( 0, 0 );
	m_pUserLabel->setMaximumSize( 32767, 32767 );
	m_pUserLabel->setFocusPolicy( QWidget::NoFocus );
	m_pUserLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pUserLabel->setFontPropagation( QWidget::NoChildren );
	m_pUserLabel->setPalettePropagation( QWidget::NoChildren );
	m_pUserLabel->setFrameStyle( 0 );
	m_pUserLabel->setLineWidth( 1 );
	m_pUserLabel->setMidLineWidth( 0 );
	m_pUserLabel->QFrame::setMargin( 0 );
	m_pUserLabel->setText( "---" );
	m_pUserLabel->setAlignment( 289 );
	m_pUserLabel->setMargin( -1 );

	m_pGroupLabel = new QLabel( this, "Label_36" );
	m_pGroupLabel->setGeometry( 29, 148, 65, 16 );
	m_pGroupLabel->setMinimumSize( 0, 0 );
	m_pGroupLabel->setMaximumSize( 32767, 32767 );
	m_pGroupLabel->setFocusPolicy( QWidget::NoFocus );
	m_pGroupLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pGroupLabel->setFontPropagation( QWidget::NoChildren );
	m_pGroupLabel->setPalettePropagation( QWidget::NoChildren );
	m_pGroupLabel->setFrameStyle( 0 );
	m_pGroupLabel->setLineWidth( 1 );
	m_pGroupLabel->setMidLineWidth( 0 );
	m_pGroupLabel->QFrame::setMargin( 0 );
	m_pGroupLabel->setText( "---" );
	m_pGroupLabel->setAlignment( 289 );
	m_pGroupLabel->setMargin( -1 );

	m_pOthersLabel = new QLabel( this, "Label_37" );
	m_pOthersLabel->setGeometry( 30, 177, 65, 16 );
	m_pOthersLabel->setMinimumSize( 0, 0 );
	m_pOthersLabel->setMaximumSize( 32767, 32767 );
	m_pOthersLabel->setFocusPolicy( QWidget::NoFocus );
	m_pOthersLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pOthersLabel->setFontPropagation( QWidget::NoChildren );
	m_pOthersLabel->setPalettePropagation( QWidget::NoChildren );
	m_pOthersLabel->setFrameStyle( 0 );
	m_pOthersLabel->setLineWidth( 1 );
	m_pOthersLabel->setMidLineWidth( 0 );
	m_pOthersLabel->QFrame::setMargin( 0 );
	m_pOthersLabel->setText( "---" );
	m_pOthersLabel->setAlignment( 289 );
	m_pOthersLabel->setMargin( -1 );

	m_pSetUIDLabel = new QLabel( this, "Label_38" );
	m_pSetUIDLabel->setGeometry( 325, 122, 140, 16 );
	m_pSetUIDLabel->setMinimumSize( 0, 0 );
	m_pSetUIDLabel->setMaximumSize( 32767, 32767 );
	m_pSetUIDLabel->setFocusPolicy( QWidget::NoFocus );
	m_pSetUIDLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pSetUIDLabel->setFontPropagation( QWidget::NoChildren );
	m_pSetUIDLabel->setPalettePropagation( QWidget::NoChildren );
	m_pSetUIDLabel->setFrameStyle( 0 );
	m_pSetUIDLabel->setLineWidth( 1 );
	m_pSetUIDLabel->setMidLineWidth( 0 );
	m_pSetUIDLabel->QFrame::setMargin( 0 );
	m_pSetUIDLabel->setText( "---" );
	m_pSetUIDLabel->setAlignment( 289 );
	m_pSetUIDLabel->setMargin( -1 );

	m_pSetGIDLabel = new QLabel( this, "Label_39" );
	m_pSetGIDLabel->setGeometry( 325, 148, 140, 16 );
	m_pSetGIDLabel->setMinimumSize( 0, 0 );
	m_pSetGIDLabel->setMaximumSize( 32767, 32767 );
	m_pSetGIDLabel->setFocusPolicy( QWidget::NoFocus );
	m_pSetGIDLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pSetGIDLabel->setFontPropagation( QWidget::NoChildren );
	m_pSetGIDLabel->setPalettePropagation( QWidget::NoChildren );
	m_pSetGIDLabel->setFrameStyle( 0 );
	m_pSetGIDLabel->setLineWidth( 1 );
	m_pSetGIDLabel->setMidLineWidth( 0 );
	m_pSetGIDLabel->QFrame::setMargin( 0 );
	m_pSetGIDLabel->setText( "---" );
	m_pSetGIDLabel->setAlignment( 289 );
	m_pSetGIDLabel->setMargin( -1 );

	m_pStickyLabel = new QLabel( this, "Label_40" );
	m_pStickyLabel->setGeometry( 325, 177, 140, 16 );
	m_pStickyLabel->setMinimumSize( 0, 0 );
	m_pStickyLabel->setMaximumSize( 32767, 32767 );
	m_pStickyLabel->setFocusPolicy( QWidget::NoFocus );
	m_pStickyLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pStickyLabel->setFontPropagation( QWidget::NoChildren );
	m_pStickyLabel->setPalettePropagation( QWidget::NoChildren );
	m_pStickyLabel->setFrameStyle( 0 );
	m_pStickyLabel->setLineWidth( 1 );
	m_pStickyLabel->setMidLineWidth( 0 );
	m_pStickyLabel->QFrame::setMargin( 0 );
	m_pStickyLabel->setText( "---" );
	m_pStickyLabel->setAlignment( 289 );
	m_pStickyLabel->setMargin( -1 );

	m_IRUSR = new QCheckBox( this, "CheckBox_1" );
	m_IRUSR->setGeometry( 104, 124, 12, 12 );
	m_IRUSR->setMinimumSize( 0, 0 );
	m_IRUSR->setMaximumSize( 32767, 32767 );
	m_IRUSR->setFocusPolicy( QWidget::TabFocus );
	m_IRUSR->setBackgroundMode( QWidget::PaletteBackground );
	m_IRUSR->setFontPropagation( QWidget::NoChildren );
	m_IRUSR->setPalettePropagation( QWidget::NoChildren );
	m_IRUSR->setText( "" );
	m_IRUSR->setAutoRepeat( FALSE );
	m_IRUSR->setAutoResize( FALSE );
	m_IRUSR->setChecked( FALSE );

	m_IWUSR = new QCheckBox( this, "CheckBox_2" );
	m_IWUSR->setGeometry( 169, 124, 12, 12 );
	m_IWUSR->setMinimumSize( 0, 0 );
	m_IWUSR->setMaximumSize( 32767, 32767 );
	m_IWUSR->setFocusPolicy( QWidget::TabFocus );
	m_IWUSR->setBackgroundMode( QWidget::PaletteBackground );
	m_IWUSR->setFontPropagation( QWidget::NoChildren );
	m_IWUSR->setPalettePropagation( QWidget::NoChildren );
	m_IWUSR->setText( "" );
	m_IWUSR->setAutoRepeat( FALSE );
	m_IWUSR->setAutoResize( FALSE );
	m_IWUSR->setChecked( FALSE );

	m_IXUSR = new QCheckBox( this, "CheckBox_3" );
	m_IXUSR->setGeometry( 234, 124, 12, 12 );
	m_IXUSR->setMinimumSize( 0, 0 );
	m_IXUSR->setMaximumSize( 32767, 32767 );
	m_IXUSR->setFocusPolicy( QWidget::TabFocus );
	m_IXUSR->setBackgroundMode( QWidget::PaletteBackground );
	m_IXUSR->setFontPropagation( QWidget::NoChildren );
	m_IXUSR->setPalettePropagation( QWidget::NoChildren );
	m_IXUSR->setText( "" );
	m_IXUSR->setAutoRepeat( FALSE );
	m_IXUSR->setAutoResize( FALSE );
	m_IXUSR->setChecked( FALSE );

	m_ISUID = new QCheckBox( this, "CheckBox_4" );
	m_ISUID->setGeometry( 299, 124, 12, 12 );
	m_ISUID->setMinimumSize( 0, 0 );
	m_ISUID->setMaximumSize( 32767, 32767 );
	m_ISUID->setFocusPolicy( QWidget::TabFocus );
	m_ISUID->setBackgroundMode( QWidget::PaletteBackground );
	m_ISUID->setFontPropagation( QWidget::NoChildren );
	m_ISUID->setPalettePropagation( QWidget::NoChildren );
	m_ISUID->setText( "" );
	m_ISUID->setAutoRepeat( FALSE );
	m_ISUID->setAutoResize( FALSE );
	m_ISUID->setChecked( FALSE );

	m_IRGRP = new QCheckBox( this, "CheckBox_5" );
	m_IRGRP->setGeometry( 104, 150, 12, 12 );
	m_IRGRP->setMinimumSize( 0, 0 );
	m_IRGRP->setMaximumSize( 32767, 32767 );
	m_IRGRP->setFocusPolicy( QWidget::TabFocus );
	m_IRGRP->setBackgroundMode( QWidget::PaletteBackground );
	m_IRGRP->setFontPropagation( QWidget::NoChildren );
	m_IRGRP->setPalettePropagation( QWidget::NoChildren );
	m_IRGRP->setText( "" );
	m_IRGRP->setAutoRepeat( FALSE );
	m_IRGRP->setAutoResize( FALSE );
	m_IRGRP->setChecked( FALSE );

	m_IROTH = new QCheckBox( this, "CheckBox_6" );
	m_IROTH->setGeometry( 104, 179, 12, 12 );
	m_IROTH->setMinimumSize( 0, 0 );
	m_IROTH->setMaximumSize( 32767, 32767 );
	m_IROTH->setFocusPolicy( QWidget::TabFocus );
	m_IROTH->setBackgroundMode( QWidget::PaletteBackground );
	m_IROTH->setFontPropagation( QWidget::NoChildren );
	m_IROTH->setPalettePropagation( QWidget::NoChildren );
	m_IROTH->setText( "" );
	m_IROTH->setAutoRepeat( FALSE );
	m_IROTH->setAutoResize( FALSE );
	m_IROTH->setChecked( FALSE );

	m_IWGRP = new QCheckBox( this, "CheckBox_7" );
	m_IWGRP->setGeometry( 169, 150, 12, 12 );
	m_IWGRP->setMinimumSize( 0, 0 );
	m_IWGRP->setMaximumSize( 32767, 32767 );
	m_IWGRP->setFocusPolicy( QWidget::TabFocus );
	m_IWGRP->setBackgroundMode( QWidget::PaletteBackground );
	m_IWGRP->setFontPropagation( QWidget::NoChildren );
	m_IWGRP->setPalettePropagation( QWidget::NoChildren );
	m_IWGRP->setText( "" );
	m_IWGRP->setAutoRepeat( FALSE );
	m_IWGRP->setAutoResize( FALSE );
	m_IWGRP->setChecked( FALSE );

	m_IXGRP = new QCheckBox( this, "CheckBox_8" );
	m_IXGRP->setGeometry( 234, 150, 12, 12 );
	m_IXGRP->setMinimumSize( 0, 0 );
	m_IXGRP->setMaximumSize( 32767, 32767 );
	m_IXGRP->setFocusPolicy( QWidget::TabFocus );
	m_IXGRP->setBackgroundMode( QWidget::PaletteBackground );
	m_IXGRP->setFontPropagation( QWidget::NoChildren );
	m_IXGRP->setPalettePropagation( QWidget::NoChildren );
	m_IXGRP->setText( "" );
	m_IXGRP->setAutoRepeat( FALSE );
	m_IXGRP->setAutoResize( FALSE );
	m_IXGRP->setChecked( FALSE );

	m_IWOTH = new QCheckBox( this, "CheckBox_9" );
	m_IWOTH->setGeometry( 169, 179, 12, 12 );
	m_IWOTH->setMinimumSize( 0, 0 );
	m_IWOTH->setMaximumSize( 32767, 32767 );
	m_IWOTH->setFocusPolicy( QWidget::TabFocus );
	m_IWOTH->setBackgroundMode( QWidget::PaletteBackground );
	m_IWOTH->setFontPropagation( QWidget::NoChildren );
	m_IWOTH->setPalettePropagation( QWidget::NoChildren );
	m_IWOTH->setText( "" );
	m_IWOTH->setAutoRepeat( FALSE );
	m_IWOTH->setAutoResize( FALSE );
	m_IWOTH->setChecked( FALSE );

	m_IXOTH = new QCheckBox( this, "CheckBox_10" );
	m_IXOTH->setGeometry( 234, 179, 12, 12 );
	m_IXOTH->setMinimumSize( 0, 0 );
	m_IXOTH->setMaximumSize( 32767, 32767 );
	m_IXOTH->setFocusPolicy( QWidget::TabFocus );
	m_IXOTH->setBackgroundMode( QWidget::PaletteBackground );
	m_IXOTH->setFontPropagation( QWidget::NoChildren );
	m_IXOTH->setPalettePropagation( QWidget::NoChildren );
	m_IXOTH->setText( "" );
	m_IXOTH->setAutoRepeat( FALSE );
	m_IXOTH->setAutoResize( FALSE );
	m_IXOTH->setChecked( FALSE );

	m_ISGID = new QCheckBox( this, "CheckBox_11" );
	m_ISGID->setGeometry( 299, 150, 12, 12 );
	m_ISGID->setMinimumSize( 0, 0 );
	m_ISGID->setMaximumSize( 32767, 32767 );
	m_ISGID->setFocusPolicy( QWidget::TabFocus );
	m_ISGID->setBackgroundMode( QWidget::PaletteBackground );
	m_ISGID->setFontPropagation( QWidget::NoChildren );
	m_ISGID->setPalettePropagation( QWidget::NoChildren );
	m_ISGID->setText( "" );
	m_ISGID->setAutoRepeat( FALSE );
	m_ISGID->setAutoResize( FALSE );
	m_ISGID->setChecked( FALSE );

	m_ISVTX = new QCheckBox( this, "CheckBox_12" );
	m_ISVTX->setGeometry( 299, 179, 12, 12 );
	m_ISVTX->setMinimumSize( 0, 0 );
	m_ISVTX->setMaximumSize( 32767, 32767 );
	m_ISVTX->setFocusPolicy( QWidget::TabFocus );
	m_ISVTX->setBackgroundMode( QWidget::PaletteBackground );
	m_ISVTX->setFontPropagation( QWidget::NoChildren );
	m_ISVTX->setPalettePropagation( QWidget::NoChildren );
	m_ISVTX->setText( "" );
	m_ISVTX->setAutoRepeat( FALSE );
	m_ISVTX->setAutoResize( FALSE );
	m_ISVTX->setChecked( FALSE );

	m_pOwnerLabel = new QLabel( this, "Label_41" );
	m_pOwnerLabel->setGeometry( 30, 248, 76, 26 );
	m_pOwnerLabel->setMinimumSize( 0, 0 );
	m_pOwnerLabel->setMaximumSize( 32767, 32767 );
	m_pOwnerLabel->setFocusPolicy( QWidget::NoFocus );
	m_pOwnerLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pOwnerLabel->setFontPropagation( QWidget::NoChildren );
	m_pOwnerLabel->setPalettePropagation( QWidget::NoChildren );
	m_pOwnerLabel->setFrameStyle( 0 );
	m_pOwnerLabel->setLineWidth( 1 );
	m_pOwnerLabel->setMidLineWidth( 0 );
	m_pOwnerLabel->QFrame::setMargin( 0 );
	m_pOwnerLabel->setText( "---" );
	m_pOwnerLabel->setAlignment( 289 );
	m_pOwnerLabel->setMargin( -1 );

	m_pGroupLabel2 = new QLabel( this, "Label_42" );
	m_pGroupLabel2->setGeometry( 30, 296, 73, 26 );
	m_pGroupLabel2->setMinimumSize( 0, 0 );
	m_pGroupLabel2->setMaximumSize( 32767, 32767 );
	m_pGroupLabel2->setFocusPolicy( QWidget::NoFocus );
	m_pGroupLabel2->setBackgroundMode( QWidget::PaletteBackground );
	m_pGroupLabel2->setFontPropagation( QWidget::NoChildren );
	m_pGroupLabel2->setPalettePropagation( QWidget::NoChildren );
	m_pGroupLabel2->setFrameStyle( 0 );
	m_pGroupLabel2->setLineWidth( 1 );
	m_pGroupLabel2->setMidLineWidth( 0 );
	m_pGroupLabel2->QFrame::setMargin( 0 );
	m_pGroupLabel2->setText( "---" );
	m_pGroupLabel2->setAlignment( 289 );
	m_pGroupLabel2->setMargin( -1 );

	m_Group = new QComboBox( FALSE, this, "ComboBox_1" );
	m_Group->setGeometry( 113, 296, 230, 26 );
	m_Group->setMinimumSize( 0, 0 );
	m_Group->setMaximumSize( 32767, 32767 );
	m_Group->setFocusPolicy( QWidget::StrongFocus );
	m_Group->setBackgroundMode( QWidget::PaletteBackground );
	m_Group->setFontPropagation( QWidget::AllChildren );
	m_Group->setPalettePropagation( QWidget::AllChildren );
	m_Group->setSizeLimit( 10 );
	m_Group->setAutoResize( FALSE );
	m_Group->setMaxCount( 2147483647 );
	m_Group->setAutoCompletion( FALSE );

	m_Owner = new QComboBox( FALSE, this, "ComboBox_2" );
	m_Owner->setGeometry( 113, 248, 230, 26 );
	m_Owner->setMinimumSize( 0, 0 );
	m_Owner->setMaximumSize( 32767, 32767 );
	m_Owner->setFocusPolicy( QWidget::StrongFocus );
	m_Owner->setBackgroundMode( QWidget::PaletteBackground );
	m_Owner->setFontPropagation( QWidget::AllChildren );
	m_Owner->setPalettePropagation( QWidget::AllChildren );
	m_Owner->setSizeLimit( 10 );
	m_Owner->setAutoResize( FALSE );
	m_Owner->setMaxCount( 2147483647 );
	m_Owner->setAutoCompletion( FALSE );

	m_pIconLabel = new QLabel( this, "Label_43" );
	m_pIconLabel->setGeometry( 10, 10, 60, 40 );
	m_pIconLabel->setMinimumSize( 60, 40 );
	m_pIconLabel->setMaximumSize( 60, 40 );
	m_pIconLabel->setFocusPolicy( QWidget::NoFocus );
	m_pIconLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pIconLabel->setFontPropagation( QWidget::NoChildren );
	m_pIconLabel->setPalettePropagation( QWidget::NoChildren );
	m_pIconLabel->setFrameStyle( 0 );
	m_pIconLabel->setLineWidth( 1 );
	m_pIconLabel->setMidLineWidth( 0 );
	m_pIconLabel->QFrame::setMargin( 0 );
	m_pIconLabel->setText( "---" );
	m_pIconLabel->setAlignment( 289 );
	m_pIconLabel->setMargin( -1 );

	m_Name = new QLabel( this, "Label_52" );
	m_Name->setGeometry( 80, 20, 395, 32 );
	m_Name->setMinimumSize( 0, 0 );
	m_Name->setMaximumSize( 32767, 32767 );
	m_Name->setFocusPolicy( QWidget::NoFocus );
	m_Name->setBackgroundMode( QWidget::PaletteBackground );
	m_Name->setFontPropagation( QWidget::NoChildren );
	m_Name->setPalettePropagation( QWidget::NoChildren );
	m_Name->setFrameStyle( 0 );
	m_Name->setLineWidth( 1 );
	m_Name->setMidLineWidth( 0 );
	m_Name->QFrame::setMargin( 0 );
	m_Name->setText( "---" );
	m_Name->setAlignment( 289 );
	m_Name->setMargin( -1 );

	resize( 489,350 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


CFilePermissionsData::~CFilePermissionsData()
{
}
