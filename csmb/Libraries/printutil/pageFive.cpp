/* Name: pageFive.cpp
            
    Description: This file is a part of the printutil shared library.

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

/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#include <qpixmap.h>
#include <qlayout.h>

//#include <kmsgbox.h>
#include "pageFive.h"
#include "constText.h"
#include "outputdlg.h"
#include "pageFive.h"
#define Inherited CWizardPage

#include <qlabel.h>
#include <qcombobox.h>
#include <qbutton.h>

#define TRACE printf("file:%s, ------ line:%d\n", __FILE__, __LINE__);

pageFive::pageFive
(
	int nPage, bool bFinalPage, int nNextPage, int nPrevPage ,
	const char* pszImage, const char* pszTitle, bool bCheckPage,
	QWidget *parent, const char *name

)
	:
	CWizardPage( nPage, bFinalPage, nNextPage, nPrevPage, pszImage,
	pszTitle, bCheckPage,parent, name )
{
	
//	printf("pageFive constructor................................\n");
	
	q_InfoLabel = new QLabel( this, "Label_1" );
	q_InfoLabel->setGeometry( 5, 3, 360, 50 );
	q_InfoLabel->setMinimumSize( 0, 0 );
	q_InfoLabel->setMaximumSize( 32767, 32767 );
	q_InfoLabel->setFocusPolicy( QWidget::NoFocus );
	q_InfoLabel->setBackgroundMode( QWidget::PaletteBackground );
	q_InfoLabel->setFontPropagation( QWidget::NoChildren );
	q_InfoLabel->setPalettePropagation( QWidget::NoChildren );
	q_InfoLabel->setFrameStyle( 0 );
	q_InfoLabel->setLineWidth( 1 );
	q_InfoLabel->setMidLineWidth( 0 );
	q_InfoLabel->QFrame::setMargin( 0 );
	q_InfoLabel->setText( LABEL5);
	q_InfoLabel->setAlignment( 1289 );
	q_InfoLabel->setMargin( -1 );

/*	qtarch_group = new QButtonGroup( this, "Network_group" );
  qtarch_group->setGeometry( 15, 65, 250, 55 );
  qtarch_group->setMinimumSize( 0, 0 );
  qtarch_group->setMaximumSize( 32767, 32767 );
  qtarch_group->setFocusPolicy( QWidget::NoFocus );
  qtarch_group->setBackgroundMode( QWidget::PaletteBackground );
  qtarch_group->setFontPropagation( QWidget::NoChildren );
  qtarch_group->setPalettePropagation( QWidget::NoChildren );
  qtarch_group->setFrameStyle( 49 );
  qtarch_group->setLineWidth( 1 );
  qtarch_group->setMidLineWidth( 0 );
  qtarch_group->QFrame::setMargin( 0 );
  qtarch_group->setTitle( );
  qtarch_group->setAlignment( 1 );
  qtarch_group->setExclusive( FALSE );
 */
    m_pShare = new QRadioButton( this, "windows" );
    m_pShare->setGeometry( 35, 60, 100, 20 );
    m_pShare->setMinimumSize( 0, 0 );
    m_pShare->setMaximumSize( 32767, 32767 );
    m_pShare->setFocusPolicy( QWidget::TabFocus );
    m_pShare->setBackgroundMode( QWidget::PaletteBackground );
    m_pShare->setFontPropagation( QWidget::NoChildren );
    m_pShare->setPalettePropagation( QWidget::NoChildren );
    m_pShare->setText( SHARE);
    m_pShare->setAutoRepeat( FALSE );
    m_pShare->setAutoResize( FALSE );
    m_pShare->setChecked( FALSE );

    m_pNoShare = new QRadioButton( this, "unix" );
    m_pNoShare->setGeometry( 35, 85, 100, 20 );
    m_pNoShare->setMinimumSize( 0, 0 );
    m_pNoShare->setMaximumSize( 32767, 32767 );
    m_pNoShare->setFocusPolicy( QWidget::TabFocus );
    m_pNoShare->setBackgroundMode( QWidget::PaletteBackground );
    m_pNoShare->setFontPropagation( QWidget::NoChildren );
    m_pNoShare->setPalettePropagation( QWidget::NoChildren );
    m_pNoShare->setText( NOSHARE );
    m_pNoShare->setAutoRepeat( FALSE );
    m_pNoShare->setAutoResize( FALSE );
    m_pNoShare->setChecked( TRUE );

 //   qtarch_group->insert( m_pShare );
 //   qtarch_group->insert( m_pNoShare );
    isShare = false;
    // set up signal and slot for radio buttons to hide
    // and display the required stuff...
    connect( m_pNoShare, SIGNAL(clicked()), this,SLOT(noShareButtonClicked()));
		connect( m_pShare, SIGNAL(clicked()),this,SLOT(shareButtonClicked())); 	
  q_label_2 = new QLabel( this, "label_2" );
  q_label_2->setGeometry( 5, 115, 100, 20 );
  q_label_2->setMinimumSize( 0, 0 );
  q_label_2->setMaximumSize( 32767, 32767 );
  q_label_2->setFocusPolicy( QWidget::NoFocus );
  q_label_2->setBackgroundMode( QWidget::PaletteBackground );
  q_label_2->setFontPropagation( QWidget::NoChildren );
  q_label_2->setPalettePropagation( QWidget::NoChildren );
  q_label_2->setText( SHARENAME );
  q_label_2->setAlignment( AlignLeft | AlignTop | ExpandTabs | WordBreak);
	q_Lineedit = new QLineEdit(this, "share name" );
	q_Lineedit->setGeometry( 105, 115, 150, 20 );
	q_Lineedit->setMinimumSize( 0, 0 );
	q_Lineedit->setMaximumSize( 32767, 32767 );
	q_Lineedit->setFocusPolicy( QWidget::StrongFocus );
	q_Lineedit->setBackgroundMode( QWidget::PaletteBackground );
	q_Lineedit->setFontPropagation( QWidget::AllChildren );
	q_Lineedit->setPalettePropagation( QWidget::AllChildren );
  q_Lineedit->setEnabled(false);
		
	resize( 400,300 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );

}

void pageFive::shareButtonClicked()
{
	m_pShare->setChecked(true);
  m_pNoShare->setChecked(false);
	isShare = true;
	q_Lineedit->setEnabled(true);
	q_Lineedit->setFocus();
}

void pageFive::noShareButtonClicked()
{
	m_pNoShare->setChecked(true);
	m_pShare->setChecked(false);
	isShare = false;
	q_Lineedit->clear();
	q_Lineedit->setEnabled(false);
}

