/* Name: FileReplaceDialogData.cpp

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
#include "FileReplaceDialogData.h"

#define Inherited QDialog


CFileReplaceDialogData::CFileReplaceDialogData
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name, TRUE, 0 )
{
	m_FileReplaceIconLabel = new QLabel( this, "Label_1" );
	m_FileReplaceIconLabel->setGeometry( 10, 10, 32, 32 );
	m_FileReplaceIconLabel->setMinimumSize( 0, 0 );
	m_FileReplaceIconLabel->setMaximumSize( 32767, 32767 );
	m_FileReplaceIconLabel->setFocusPolicy( QWidget::NoFocus );
	m_FileReplaceIconLabel->setBackgroundMode( QWidget::PaletteBackground );
	m_FileReplaceIconLabel->setFontPropagation( QWidget::NoChildren );
	m_FileReplaceIconLabel->setPalettePropagation( QWidget::NoChildren );
	m_FileReplaceIconLabel->setFrameStyle( 0 );
	m_FileReplaceIconLabel->setLineWidth( 1 );
	m_FileReplaceIconLabel->setMidLineWidth( 0 );
	m_FileReplaceIconLabel->QFrame::setMargin( 0 );
	m_FileReplaceIconLabel->setText( "---" );
	m_FileReplaceIconLabel->setAlignment( 289 );
	m_FileReplaceIconLabel->setMargin( -1 );

	m_TopText = new QLabel( this, "Label_2" );
	m_TopText->setGeometry( 83, 23, 527, 32 );
	m_TopText->setMinimumSize( 0, 0 );
	m_TopText->setMaximumSize( 32767, 32767 );
	m_TopText->setFocusPolicy( QWidget::NoFocus );
	m_TopText->setBackgroundMode( QWidget::PaletteBackground );
	m_TopText->setFontPropagation( QWidget::NoChildren );
	m_TopText->setPalettePropagation( QWidget::NoChildren );
	m_TopText->setFrameStyle( 0 );
	m_TopText->setLineWidth( 1 );
	m_TopText->setMidLineWidth( 0 );
	m_TopText->QFrame::setMargin( 0 );
	m_TopText->setText( "This folder already contains a file named 'xxx'" );
	m_TopText->setAlignment( 289 );
	m_TopText->setMargin( -1 );

	m_pLabel2 = new QLabel( this, "Label_3" );
	m_pLabel2->setGeometry( 83, 67, 512, 15 );
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

	m_FileIcon1 = new QLabel( this, "Label_4" );
	m_FileIcon1->setGeometry( 83, 91, 35, 32 );
	m_FileIcon1->setMinimumSize( 0, 0 );
	m_FileIcon1->setMaximumSize( 32767, 32767 );
	m_FileIcon1->setFocusPolicy( QWidget::NoFocus );
	m_FileIcon1->setBackgroundMode( QWidget::PaletteBackground );
	m_FileIcon1->setFontPropagation( QWidget::NoChildren );
	m_FileIcon1->setPalettePropagation( QWidget::NoChildren );
	m_FileIcon1->setFrameStyle( 0 );
	m_FileIcon1->setLineWidth( 1 );
	m_FileIcon1->setMidLineWidth( 0 );
	m_FileIcon1->QFrame::setMargin( 0 );
	m_FileIcon1->setText( "---" );
	m_FileIcon1->setAlignment( 289 );
	m_FileIcon1->setMargin( -1 );

	m_File1Text1 = new QLabel( this, "Label_5" );
	m_File1Text1->setGeometry( 144, 93, 394, 16 );
	m_File1Text1->setMinimumSize( 0, 0 );
	m_File1Text1->setMaximumSize( 32767, 32767 );
	m_File1Text1->setFocusPolicy( QWidget::NoFocus );
	m_File1Text1->setBackgroundMode( QWidget::PaletteBackground );
	m_File1Text1->setFontPropagation( QWidget::NoChildren );
	m_File1Text1->setPalettePropagation( QWidget::NoChildren );
	m_File1Text1->setFrameStyle( 0 );
	m_File1Text1->setLineWidth( 1 );
	m_File1Text1->setMidLineWidth( 0 );
	m_File1Text1->QFrame::setMargin( 0 );
	m_File1Text1->setText( "XXX bytes" );
	m_File1Text1->setAlignment( 289 );
	m_File1Text1->setMargin( -1 );

	m_File1Text2 = new QLabel( this, "Label_6" );
	m_File1Text2->setGeometry( 144, 119, 440, 14 );
	m_File1Text2->setMinimumSize( 0, 0 );
	m_File1Text2->setMaximumSize( 32767, 32767 );
	m_File1Text2->setFocusPolicy( QWidget::NoFocus );
	m_File1Text2->setBackgroundMode( QWidget::PaletteBackground );
	m_File1Text2->setFontPropagation( QWidget::NoChildren );
	m_File1Text2->setPalettePropagation( QWidget::NoChildren );
	m_File1Text2->setFrameStyle( 0 );
	m_File1Text2->setLineWidth( 1 );
	m_File1Text2->setMidLineWidth( 0 );
	m_File1Text2->QFrame::setMargin( 0 );
	m_File1Text2->setText( "modified on Monday, June 28, 1999, 2:17:06 PM" );
	m_File1Text2->setAlignment( 289 );
	m_File1Text2->setMargin( -1 );

	m_FileIcon2 = new QLabel( this, "Label_7" );
	m_FileIcon2->setGeometry( 83, 186, 35, 32 );
	m_FileIcon2->setMinimumSize( 0, 0 );
	m_FileIcon2->setMaximumSize( 32767, 32767 );
	m_FileIcon2->setFocusPolicy( QWidget::NoFocus );
	m_FileIcon2->setBackgroundMode( QWidget::PaletteBackground );
	m_FileIcon2->setFontPropagation( QWidget::NoChildren );
	m_FileIcon2->setPalettePropagation( QWidget::NoChildren );
	m_FileIcon2->setFrameStyle( 0 );
	m_FileIcon2->setLineWidth( 1 );
	m_FileIcon2->setMidLineWidth( 0 );
	m_FileIcon2->QFrame::setMargin( 0 );
	m_FileIcon2->setText( "---" );
	m_FileIcon2->setAlignment( 289 );
	m_FileIcon2->setMargin( -1 );

	m_File2Text1 = new QLabel( this, "Label_8" );
	m_File2Text1->setGeometry( 144, 173, 446, 16 );
	m_File2Text1->setMinimumSize( 0, 0 );
	m_File2Text1->setMaximumSize( 32767, 32767 );
	m_File2Text1->setFocusPolicy( QWidget::NoFocus );
	m_File2Text1->setBackgroundMode( QWidget::PaletteBackground );
	m_File2Text1->setFontPropagation( QWidget::NoChildren );
	m_File2Text1->setPalettePropagation( QWidget::NoChildren );
	m_File2Text1->setFrameStyle( 0 );
	m_File2Text1->setLineWidth( 1 );
	m_File2Text1->setMidLineWidth( 0 );
	m_File2Text1->QFrame::setMargin( 0 );
	m_File2Text1->setText( "xxx bytes" );
	m_File2Text1->setAlignment( 289 );
	m_File2Text1->setMargin( -1 );

	m_File2Text2 = new QLabel( this, "Label_9" );
	m_File2Text2->setGeometry( 144, 203, 416, 15 );
	m_File2Text2->setMinimumSize( 0, 0 );
	m_File2Text2->setMaximumSize( 32767, 32767 );
	m_File2Text2->setFocusPolicy( QWidget::NoFocus );
	m_File2Text2->setBackgroundMode( QWidget::PaletteBackground );
	m_File2Text2->setFontPropagation( QWidget::NoChildren );
	m_File2Text2->setPalettePropagation( QWidget::NoChildren );
	m_File2Text2->setFrameStyle( 0 );
	m_File2Text2->setLineWidth( 1 );
	m_File2Text2->setMidLineWidth( 0 );
	m_File2Text2->QFrame::setMargin( 0 );
	m_File2Text2->setText( "modified on Friday, April 02, 1999, 11:30:32 AM" );
	m_File2Text2->setAlignment( 289 );
	m_File2Text2->setMargin( -1 );

	m_pYesButton = new QPushButton( this, "PushButton_1" );
	m_pYesButton->setGeometry( 188, 242, 100, 30 );
	m_pYesButton->setMinimumSize( 0, 0 );
	m_pYesButton->setMaximumSize( 32767, 32767 );
	connect( m_pYesButton, SIGNAL(clicked()), SLOT(OnYes()) );
	m_pYesButton->setFocusPolicy( QWidget::TabFocus );
	m_pYesButton->setBackgroundMode( QWidget::PaletteBackground );
	m_pYesButton->setFontPropagation( QWidget::NoChildren );
	m_pYesButton->setPalettePropagation( QWidget::NoChildren );
	m_pYesButton->setText( "---" );
	m_pYesButton->setAutoRepeat( FALSE );
	m_pYesButton->setAutoResize( FALSE );
	m_pYesButton->setToggleButton( FALSE );
	m_pYesButton->setDefault( FALSE );
	m_pYesButton->setAutoDefault( FALSE );
	m_pYesButton->setIsMenuButton( FALSE );

	m_Button2 = new QPushButton( this, "PushButton_2" );
	m_Button2->setGeometry( 295, 242, 100, 30 );
	m_Button2->setMinimumSize( 0, 0 );
	m_Button2->setMaximumSize( 32767, 32767 );
	connect( m_Button2, SIGNAL(clicked()), SLOT(OnButton2()) );
	m_Button2->setFocusPolicy( QWidget::TabFocus );
	m_Button2->setBackgroundMode( QWidget::PaletteBackground );
	m_Button2->setFontPropagation( QWidget::NoChildren );
	m_Button2->setPalettePropagation( QWidget::NoChildren );
	m_Button2->setText( "---" );
	m_Button2->setAutoRepeat( FALSE );
	m_Button2->setAutoResize( FALSE );
	m_Button2->setToggleButton( FALSE );
	m_Button2->setDefault( FALSE );
	m_Button2->setAutoDefault( FALSE );
	m_Button2->setIsMenuButton( FALSE );

	m_Button3 = new QPushButton( this, "PushButton_3" );
	m_Button3->setGeometry( 403, 242, 100, 30 );
	m_Button3->setMinimumSize( 0, 0 );
	m_Button3->setMaximumSize( 32767, 32767 );
	connect( m_Button3, SIGNAL(clicked()), SLOT(OnButton3()) );
	m_Button3->setFocusPolicy( QWidget::TabFocus );
	m_Button3->setBackgroundMode( QWidget::PaletteBackground );
	m_Button3->setFontPropagation( QWidget::NoChildren );
	m_Button3->setPalettePropagation( QWidget::NoChildren );
	m_Button3->setText( "---" );
	m_Button3->setAutoRepeat( FALSE );
	m_Button3->setAutoResize( FALSE );
	m_Button3->setToggleButton( FALSE );
	m_Button3->setDefault( FALSE );
	m_Button3->setAutoDefault( FALSE );
	m_Button3->setIsMenuButton( FALSE );

	m_Cancel = new QPushButton( this, "PushButton_4" );
	m_Cancel->setGeometry( 511, 242, 100, 30 );
	m_Cancel->setMinimumSize( 0, 0 );
	m_Cancel->setMaximumSize( 32767, 32767 );
	connect( m_Cancel, SIGNAL(clicked()), SLOT(reject()) );
	m_Cancel->setFocusPolicy( QWidget::TabFocus );
	m_Cancel->setBackgroundMode( QWidget::PaletteBackground );
	m_Cancel->setFontPropagation( QWidget::NoChildren );
	m_Cancel->setPalettePropagation( QWidget::NoChildren );
	m_Cancel->setText( "---" );
	m_Cancel->setAutoRepeat( FALSE );
	m_Cancel->setAutoResize( FALSE );
	m_Cancel->setToggleButton( FALSE );
	m_Cancel->setDefault( FALSE );
	m_Cancel->setAutoDefault( FALSE );
	m_Cancel->setIsMenuButton( FALSE );

	m_pLabel3 = new QLabel( this, "Label_10" );
	m_pLabel3->setGeometry( 83, 147, 512, 15 );
	m_pLabel3->setMinimumSize( 0, 0 );
	m_pLabel3->setMaximumSize( 32767, 32767 );
	m_pLabel3->setFocusPolicy( QWidget::NoFocus );
	m_pLabel3->setBackgroundMode( QWidget::PaletteBackground );
	m_pLabel3->setFontPropagation( QWidget::NoChildren );
	m_pLabel3->setPalettePropagation( QWidget::NoChildren );
	m_pLabel3->setFrameStyle( 0 );
	m_pLabel3->setLineWidth( 1 );
	m_pLabel3->setMidLineWidth( 0 );
	m_pLabel3->QFrame::setMargin( 0 );
	m_pLabel3->setText( "---" );
	m_pLabel3->setAlignment( 289 );
	m_pLabel3->setMargin( -1 );

	resize( 621,282 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}


CFileReplaceDialogData::~CFileReplaceDialogData()
{
}
void CFileReplaceDialogData::OnYes()
{
}
void CFileReplaceDialogData::OnButton2()
{
}
void CFileReplaceDialogData::OnButton3()
{
}
