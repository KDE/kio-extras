/* Name: ServPropGeneralData.cpp

   Description: This file is a part of the Corel File Manager application.

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
#include "ServPropGeneralData.h"

#define Inherited QDialog

#include <qframe.h>

CServPropGeneralData::CServPropGeneralData
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

	m_Name = new QLabel( this, "Label_12" );
	m_Name->setGeometry( 80, 10, 310, 20 );
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

	QFrame* qtarch_Frame_4;
	qtarch_Frame_4 = new QFrame( this, "Frame_4" );
	qtarch_Frame_4->setGeometry( 10, 55, 464, 13 );
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

	m_pCommentLabel = new QLabel( this, "Label_13" );
	m_pCommentLabel->setGeometry( 10, 75, 80, 20 );
	m_pCommentLabel->setMinimumSize( 0, 0 );
	m_pCommentLabel->setMaximumSize( 32767, 32767 );
	m_pCommentLabel->setFocusPolicy( QWidget::NoFocus );
	m_pCommentLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_pCommentLabel->setFontPropagation( QWidget::NoChildren );
	m_pCommentLabel->setPalettePropagation( QWidget::NoChildren );
	m_pCommentLabel->setFrameStyle( 0 );
	m_pCommentLabel->setLineWidth( 1 );
	m_pCommentLabel->setMidLineWidth( 0 );
	m_pCommentLabel->QFrame::setMargin( 0 );
	m_pCommentLabel->setText( "---" );
	m_pCommentLabel->setAlignment( 289 );
	m_pCommentLabel->setMargin( -1 );

	m_Comment = new QLabel( this, "Label_15" );
	m_Comment->setGeometry( 100, 75, 375, 20 );
	m_Comment->setMinimumSize( 0, 0 );
	m_Comment->setMaximumSize( 32767, 32767 );
	m_Comment->setFocusPolicy( QWidget::NoFocus );
	m_Comment->setBackgroundMode( QWidget::PaletteBackground );
	m_Comment->setFontPropagation( QWidget::NoChildren );
	m_Comment->setPalettePropagation( QWidget::NoChildren );
	m_Comment->setFrameStyle( 0 );
	m_Comment->setLineWidth( 1 );
	m_Comment->setMidLineWidth( 0 );
	m_Comment->QFrame::setMargin( 0 );
	m_Comment->setText( "---" );
	m_Comment->setAlignment( 289 );
	m_Comment->setMargin( -1 );

	m_pDomainLabel = new QLabel( this, "Label_16" );
	m_pDomainLabel->setGeometry( 10, 105, 60, 18 );
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

	m_Domain = new QLabel( this, "Label_18" );
	m_Domain->setGeometry( 100, 105, 375, 20 );
	m_Domain->setMinimumSize( 0, 0 );
	m_Domain->setMaximumSize( 32767, 32767 );
	m_Domain->setFocusPolicy( QWidget::NoFocus );
	m_Domain->setBackgroundMode( QWidget::PaletteBackground );
	m_Domain->setFontPropagation( QWidget::NoChildren );
	m_Domain->setPalettePropagation( QWidget::NoChildren );
	m_Domain->setFrameStyle( 0 );
	m_Domain->setLineWidth( 1 );
	m_Domain->setMidLineWidth( 0 );
	m_Domain->QFrame::setMargin( 0 );
	m_Domain->setText( "---" );
	m_Domain->setAlignment( 289 );
	m_Domain->setMargin( -1 );

	resize( 489,350 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


CServPropGeneralData::~CServPropGeneralData()
{
}
