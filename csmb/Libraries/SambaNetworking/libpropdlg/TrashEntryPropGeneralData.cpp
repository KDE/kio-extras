/* Name: TrashEntryPropGeneralData.cpp

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

/**********************************************************************

	--- Qt Architect generated file ---

	File: TrashEntryPropGeneralData.cpp
	Last generated: Sun Nov 28 22:30:11 1999

	DO NOT EDIT!!!  This file will be automatically
	regenerated by qtarch.  All changes will be lost.

 *********************************************************************/

#include <qpixmap.h>
#include <qlayout.h>
#include "TrashEntryPropGeneralData.h"

#define Inherited QDialog

#include <qframe.h>

CTrashEntryPropGeneralData::CTrashEntryPropGeneralData
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

	m_FileName = new QLabel( this, "Label_12" );
	m_FileName->setGeometry( 81, 10, 395, 52 );
	m_FileName->setMinimumSize( 0, 0 );
	m_FileName->setMaximumSize( 32767, 32767 );
	m_FileName->setFocusPolicy( QWidget::NoFocus );
	m_FileName->setBackgroundMode( QWidget::PaletteBackground );
	m_FileName->setFontPropagation( QWidget::NoChildren );
	m_FileName->setPalettePropagation( QWidget::NoChildren );
	m_FileName->setFrameStyle( 0 );
	m_FileName->setLineWidth( 1 );
	m_FileName->setMidLineWidth( 0 );
	m_FileName->QFrame::setMargin( 0 );
	m_FileName->setText( "Filename" );
	m_FileName->setAlignment( 289 );
	m_FileName->setMargin( -1 );

	QFrame* qtarch_Frame_4;
	qtarch_Frame_4 = new QFrame( this, "Frame_4" );
	qtarch_Frame_4->setGeometry( 12, 66, 464, 10 );
	qtarch_Frame_4->setMinimumSize( 0, 0 );
	qtarch_Frame_4->setMaximumSize( 32767, 32767 );
	qtarch_Frame_4->setFocusPolicy( QWidget::NoFocus );
	qtarch_Frame_4->setBackgroundMode( QWidget::PaletteBackground );
	qtarch_Frame_4->setFontPropagation( QWidget::NoChildren );
	qtarch_Frame_4->setPalettePropagation( QWidget::NoChildren );
	qtarch_Frame_4->setFrameStyle( 52 );
	qtarch_Frame_4->setLineWidth( 1 );
	qtarch_Frame_4->setMidLineWidth( 0 );
	qtarch_Frame_4->QFrame::setMargin( 0 );

	m_pOriginalLocationLabel = new QLabel( this, "Label_13" );
	m_pOriginalLocationLabel->setGeometry( 12, 80, 120, 20 );
	m_pOriginalLocationLabel->setMinimumSize( 0, 0 );
	m_pOriginalLocationLabel->setMaximumSize( 32767, 32767 );
	m_pOriginalLocationLabel->setFocusPolicy( QWidget::NoFocus );
	m_pOriginalLocationLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pOriginalLocationLabel->setFontPropagation( QWidget::NoChildren );
	m_pOriginalLocationLabel->setPalettePropagation( QWidget::NoChildren );
	m_pOriginalLocationLabel->setFrameStyle( 0 );
	m_pOriginalLocationLabel->setLineWidth( 1 );
	m_pOriginalLocationLabel->setMidLineWidth( 0 );
	m_pOriginalLocationLabel->QFrame::setMargin( 0 );
	m_pOriginalLocationLabel->setText( "---" );
	m_pOriginalLocationLabel->setAlignment( 289 );
	m_pOriginalLocationLabel->setMargin( -1 );

	m_OriginalLocation = new QLabel( this, "Label_15" );
	m_OriginalLocation->setGeometry( 140, 79, 337, 20 );
	m_OriginalLocation->setMinimumSize( 0, 0 );
	m_OriginalLocation->setMaximumSize( 32767, 32767 );
	m_OriginalLocation->setFocusPolicy( QWidget::NoFocus );
	m_OriginalLocation->setBackgroundMode( QWidget::PaletteBackground );
	m_OriginalLocation->setFontPropagation( QWidget::NoChildren );
	m_OriginalLocation->setPalettePropagation( QWidget::NoChildren );
	m_OriginalLocation->setFrameStyle( 0 );
	m_OriginalLocation->setLineWidth( 1 );
	m_OriginalLocation->setMidLineWidth( 0 );
	m_OriginalLocation->QFrame::setMargin( 0 );
	m_OriginalLocation->setText( "---" );
	m_OriginalLocation->setAlignment( 289 );
	m_OriginalLocation->setMargin( -1 );

	m_pSizeLabel = new QLabel( this, "Label_16" );
	m_pSizeLabel->setGeometry( 12, 108, 60, 20 );
	m_pSizeLabel->setMinimumSize( 0, 0 );
	m_pSizeLabel->setMaximumSize( 32767, 32767 );
	m_pSizeLabel->setFocusPolicy( QWidget::NoFocus );
	m_pSizeLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pSizeLabel->setFontPropagation( QWidget::NoChildren );
	m_pSizeLabel->setPalettePropagation( QWidget::NoChildren );
	m_pSizeLabel->setFrameStyle( 0 );
	m_pSizeLabel->setLineWidth( 1 );
	m_pSizeLabel->setMidLineWidth( 0 );
	m_pSizeLabel->QFrame::setMargin( 0 );
	m_pSizeLabel->setText( "---" );
	m_pSizeLabel->setAlignment( 289 );
	m_pSizeLabel->setMargin( -1 );

	m_Size = new QLabel( this, "Label_18" );
	m_Size->setGeometry( 140, 108, 337, 20 );
	m_Size->setMinimumSize( 0, 0 );
	m_Size->setMaximumSize( 32767, 32767 );
	m_Size->setFocusPolicy( QWidget::NoFocus );
	m_Size->setBackgroundMode( QWidget::PaletteBackground );
	m_Size->setFontPropagation( QWidget::NoChildren );
	m_Size->setPalettePropagation( QWidget::NoChildren );
	m_Size->setFrameStyle( 0 );
	m_Size->setLineWidth( 1 );
	m_Size->setMidLineWidth( 0 );
	m_Size->QFrame::setMargin( 0 );
	m_Size->setText( "---" );
	m_Size->setAlignment( 289 );
	m_Size->setMargin( -1 );

	m_pOriginalModifiedLabel = new QLabel( this, "Label_19" );
	m_pOriginalModifiedLabel->setGeometry( 12, 192, 120, 20 );
	m_pOriginalModifiedLabel->setMinimumSize( 0, 0 );
	m_pOriginalModifiedLabel->setMaximumSize( 32767, 32767 );
	m_pOriginalModifiedLabel->setFocusPolicy( QWidget::NoFocus );
	m_pOriginalModifiedLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pOriginalModifiedLabel->setFontPropagation( QWidget::NoChildren );
	m_pOriginalModifiedLabel->setPalettePropagation( QWidget::NoChildren );
	m_pOriginalModifiedLabel->setFrameStyle( 0 );
	m_pOriginalModifiedLabel->setLineWidth( 1 );
	m_pOriginalModifiedLabel->setMidLineWidth( 0 );
	m_pOriginalModifiedLabel->QFrame::setMargin( 0 );
	m_pOriginalModifiedLabel->setText( "---" );
	m_pOriginalModifiedLabel->setAlignment( 289 );
	m_pOriginalModifiedLabel->setMargin( -1 );

	m_DateDeleted = new QLabel( this, "Label_20" );
	m_DateDeleted->setGeometry( 140, 137, 337, 20 );
	m_DateDeleted->setMinimumSize( 0, 0 );
	m_DateDeleted->setMaximumSize( 32767, 32767 );
	m_DateDeleted->setFocusPolicy( QWidget::NoFocus );
	m_DateDeleted->setBackgroundMode( QWidget::PaletteBackground );
	m_DateDeleted->setFontPropagation( QWidget::NoChildren );
	m_DateDeleted->setPalettePropagation( QWidget::NoChildren );
	m_DateDeleted->setFrameStyle( 0 );
	m_DateDeleted->setLineWidth( 1 );
	m_DateDeleted->setMidLineWidth( 0 );
	m_DateDeleted->QFrame::setMargin( 0 );
	m_DateDeleted->setText( "---" );
	m_DateDeleted->setAlignment( 289 );
	m_DateDeleted->setMargin( -1 );

	m_pOwnerLabel = new QLabel( this, "Label_25" );
	m_pOwnerLabel->setGeometry( 12, 234, 60, 20 );
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

	QFrame* qtarch_Frame_5;
	qtarch_Frame_5 = new QFrame( this, "Frame_5" );
	qtarch_Frame_5->setGeometry( 12, 220, 464, 10 );
	qtarch_Frame_5->setMinimumSize( 0, 0 );
	qtarch_Frame_5->setMaximumSize( 32767, 32767 );
	qtarch_Frame_5->setFocusPolicy( QWidget::NoFocus );
	qtarch_Frame_5->setBackgroundMode( QWidget::PaletteBackground );
	qtarch_Frame_5->setFontPropagation( QWidget::NoChildren );
	qtarch_Frame_5->setPalettePropagation( QWidget::NoChildren );
	qtarch_Frame_5->setFrameStyle( 52 );
	qtarch_Frame_5->setLineWidth( 1 );
	qtarch_Frame_5->setMidLineWidth( 0 );
	qtarch_Frame_5->QFrame::setMargin( 0 );

	m_Owner = new QLabel( this, "Label_26" );
	m_Owner->setGeometry( 102, 234, 375, 20 );
	m_Owner->setMinimumSize( 0, 0 );
	m_Owner->setMaximumSize( 32767, 32767 );
	m_Owner->setFocusPolicy( QWidget::NoFocus );
	m_Owner->setBackgroundMode( QWidget::PaletteBackground );
	m_Owner->setFontPropagation( QWidget::NoChildren );
	m_Owner->setPalettePropagation( QWidget::NoChildren );
	m_Owner->setFrameStyle( 0 );
	m_Owner->setLineWidth( 1 );
	m_Owner->setMidLineWidth( 0 );
	m_Owner->QFrame::setMargin( 0 );
	m_Owner->setText( "---" );
	m_Owner->setAlignment( 289 );
	m_Owner->setMargin( -1 );

	m_pOwnerGroupLabel = new QLabel( this, "Label_27" );
	m_pOwnerGroupLabel->setGeometry( 12, 263, 90, 20 );
	m_pOwnerGroupLabel->setMinimumSize( 0, 0 );
	m_pOwnerGroupLabel->setMaximumSize( 32767, 32767 );
	m_pOwnerGroupLabel->setFocusPolicy( QWidget::NoFocus );
	m_pOwnerGroupLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pOwnerGroupLabel->setFontPropagation( QWidget::NoChildren );
	m_pOwnerGroupLabel->setPalettePropagation( QWidget::NoChildren );
	m_pOwnerGroupLabel->setFrameStyle( 0 );
	m_pOwnerGroupLabel->setLineWidth( 1 );
	m_pOwnerGroupLabel->setMidLineWidth( 0 );
	m_pOwnerGroupLabel->QFrame::setMargin( 0 );
	m_pOwnerGroupLabel->setText( "---" );
	m_pOwnerGroupLabel->setAlignment( 289 );
	m_pOwnerGroupLabel->setMargin( -1 );

	m_OwnerGroup = new QLabel( this, "Label_28" );
	m_OwnerGroup->setGeometry( 101, 263, 375, 20 );
	m_OwnerGroup->setMinimumSize( 0, 0 );
	m_OwnerGroup->setMaximumSize( 32767, 32767 );
	m_OwnerGroup->setFocusPolicy( QWidget::NoFocus );
	m_OwnerGroup->setBackgroundMode( QWidget::PaletteBackground );
	m_OwnerGroup->setFontPropagation( QWidget::NoChildren );
	m_OwnerGroup->setPalettePropagation( QWidget::NoChildren );
	m_OwnerGroup->setFrameStyle( 0 );
	m_OwnerGroup->setLineWidth( 1 );
	m_OwnerGroup->setMidLineWidth( 0 );
	m_OwnerGroup->QFrame::setMargin( 0 );
	m_OwnerGroup->setText( "---" );
	m_OwnerGroup->setAlignment( 289 );
	m_OwnerGroup->setMargin( -1 );

	m_pPermissionsLabel = new QLabel( this, "Label_29" );
	m_pPermissionsLabel->setGeometry( 11, 292, 80, 20 );
	m_pPermissionsLabel->setMinimumSize( 0, 0 );
	m_pPermissionsLabel->setMaximumSize( 32767, 32767 );
	m_pPermissionsLabel->setFocusPolicy( QWidget::NoFocus );
	m_pPermissionsLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pPermissionsLabel->setFontPropagation( QWidget::NoChildren );
	m_pPermissionsLabel->setPalettePropagation( QWidget::NoChildren );
	m_pPermissionsLabel->setFrameStyle( 0 );
	m_pPermissionsLabel->setLineWidth( 1 );
	m_pPermissionsLabel->setMidLineWidth( 0 );
	m_pPermissionsLabel->QFrame::setMargin( 0 );
	m_pPermissionsLabel->setText( "---" );
	m_pPermissionsLabel->setAlignment( 289 );
	m_pPermissionsLabel->setMargin( -1 );

	m_Permissions = new QLabel( this, "Label_30" );
	m_Permissions->setGeometry( 100, 292, 375, 20 );
	m_Permissions->setMinimumSize( 0, 0 );
	m_Permissions->setMaximumSize( 32767, 32767 );
	m_Permissions->setFocusPolicy( QWidget::NoFocus );
	m_Permissions->setBackgroundMode( QWidget::PaletteBackground );
	m_Permissions->setFontPropagation( QWidget::NoChildren );
	m_Permissions->setPalettePropagation( QWidget::NoChildren );
	m_Permissions->setFrameStyle( 0 );
	m_Permissions->setLineWidth( 1 );
	m_Permissions->setMidLineWidth( 0 );
	m_Permissions->QFrame::setMargin( 0 );
	m_Permissions->setText( "---" );
	m_Permissions->setAlignment( 289 );
	m_Permissions->setMargin( -1 );

	m_pOriginalCreatedLabel = new QLabel( this, "Label_31" );
	m_pOriginalCreatedLabel->setGeometry( 12, 164, 120, 20 );
	m_pOriginalCreatedLabel->setMinimumSize( 0, 0 );
	m_pOriginalCreatedLabel->setMaximumSize( 32767, 32767 );
	m_pOriginalCreatedLabel->setFocusPolicy( QWidget::NoFocus );
	m_pOriginalCreatedLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pOriginalCreatedLabel->setFontPropagation( QWidget::NoChildren );
	m_pOriginalCreatedLabel->setPalettePropagation( QWidget::NoChildren );
	m_pOriginalCreatedLabel->setFrameStyle( 0 );
	m_pOriginalCreatedLabel->setLineWidth( 1 );
	m_pOriginalCreatedLabel->setMidLineWidth( 0 );
	m_pOriginalCreatedLabel->QFrame::setMargin( 0 );
	m_pOriginalCreatedLabel->setText( "---" );
	m_pOriginalCreatedLabel->setAlignment( 289 );
	m_pOriginalCreatedLabel->setMargin( -1 );

	m_pDeletedLabel = new QLabel( this, "Label_32" );
	m_pDeletedLabel->setGeometry( 12, 136, 60, 20 );
	m_pDeletedLabel->setMinimumSize( 0, 0 );
	m_pDeletedLabel->setMaximumSize( 32767, 32767 );
	m_pDeletedLabel->setFocusPolicy( QWidget::NoFocus );
	m_pDeletedLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pDeletedLabel->setFontPropagation( QWidget::NoChildren );
	m_pDeletedLabel->setPalettePropagation( QWidget::NoChildren );
	m_pDeletedLabel->setFrameStyle( 0 );
	m_pDeletedLabel->setLineWidth( 1 );
	m_pDeletedLabel->setMidLineWidth( 0 );
	m_pDeletedLabel->QFrame::setMargin( 0 );
	m_pDeletedLabel->setText( "---" );
	m_pDeletedLabel->setAlignment( 289 );
	m_pDeletedLabel->setMargin( -1 );

	m_OriginalCreated = new QLabel( this, "Label_33" );
	m_OriginalCreated->setGeometry( 140, 163, 337, 20 );
	m_OriginalCreated->setMinimumSize( 0, 0 );
	m_OriginalCreated->setMaximumSize( 32767, 32767 );
	m_OriginalCreated->setFocusPolicy( QWidget::NoFocus );
	m_OriginalCreated->setBackgroundMode( QWidget::PaletteBackground );
	m_OriginalCreated->setFontPropagation( QWidget::NoChildren );
	m_OriginalCreated->setPalettePropagation( QWidget::NoChildren );
	m_OriginalCreated->setFrameStyle( 0 );
	m_OriginalCreated->setLineWidth( 1 );
	m_OriginalCreated->setMidLineWidth( 0 );
	m_OriginalCreated->QFrame::setMargin( 0 );
	m_OriginalCreated->setText( "---" );
	m_OriginalCreated->setAlignment( 289 );
	m_OriginalCreated->setMargin( -1 );

	m_OriginalModified = new QLabel( this, "Label_34" );
	m_OriginalModified->setGeometry( 140, 190, 337, 20 );
	m_OriginalModified->setMinimumSize( 0, 0 );
	m_OriginalModified->setMaximumSize( 32767, 32767 );
	m_OriginalModified->setFocusPolicy( QWidget::NoFocus );
	m_OriginalModified->setBackgroundMode( QWidget::PaletteBackground );
	m_OriginalModified->setFontPropagation( QWidget::NoChildren );
	m_OriginalModified->setPalettePropagation( QWidget::NoChildren );
	m_OriginalModified->setFrameStyle( 0 );
	m_OriginalModified->setLineWidth( 1 );
	m_OriginalModified->setMidLineWidth( 0 );
	m_OriginalModified->QFrame::setMargin( 0 );
	m_OriginalModified->setText( "---" );
	m_OriginalModified->setAlignment( 289 );
	m_OriginalModified->setMargin( -1 );

	resize( 489,350 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


CTrashEntryPropGeneralData::~CTrashEntryPropGeneralData()
{
}
