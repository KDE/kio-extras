/* Name: pageFour.cpp
            
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
#include "pageFour.h"
#include "constText.h"
#include "outputdlg.h"
#define Inherited CWizardPage

#include <qlabel.h>
#include <qcombobox.h>

#define TRACE printf("file:%s, ------ line:%d\n", __FILE__, __LINE__);

pageFour::pageFour
(
	int nPage, bool bFinalPage = false, int nNextPage = -1, int nPrevPage = -1,
	const char* pszImage = 0, const char* pszTitle = 0, bool bCheckPage = true,
	QWidget *parent=0, const char *name=0

)
	:
	Inherited( nPage, bFinalPage, 
		nNextPage, nPrevPage, pszImage, pszTitle, bCheckPage,
		parent, name )
{
	qtarch_Label_1 = new QLabel( this, "Label_1" );
	qtarch_Label_1->setGeometry( 5, 3, 360, 50 );
	qtarch_Label_1->setMinimumSize( 0, 0 );
	qtarch_Label_1->setMaximumSize( 32767, 32767 );
	qtarch_Label_1->setFocusPolicy( QWidget::NoFocus );
	qtarch_Label_1->setBackgroundMode( QWidget::PaletteBackground );
	qtarch_Label_1->setFontPropagation( QWidget::NoChildren );
	qtarch_Label_1->setPalettePropagation( QWidget::NoChildren );
	qtarch_Label_1->setFrameStyle( 0 );
	qtarch_Label_1->setLineWidth( 1 );
	qtarch_Label_1->setMidLineWidth( 0 );
	qtarch_Label_1->QFrame::setMargin( 0 );
	qtarch_Label_1->setText( LABEL4);
	qtarch_Label_1->setAlignment( 1289 );
	qtarch_Label_1->setMargin( -1 );

	m_pYes = new QRadioButton( this, "windows" );
	m_pYes->setGeometry( 35, 60, 100, 20 );
	m_pYes->setMinimumSize( 0, 0 );
	m_pYes->setMaximumSize( 32767, 32767 );
	m_pYes->setFocusPolicy( QWidget::TabFocus );
	m_pYes->setBackgroundMode( QWidget::PaletteBackground );
	m_pYes->setFontPropagation( QWidget::NoChildren );
	m_pYes->setPalettePropagation( QWidget::NoChildren );
	m_pYes->setText( YESPRINT);
	m_pYes->setAutoRepeat( FALSE );
	m_pYes->setAutoResize( FALSE );
	m_pYes->setChecked( FALSE );
	
	m_pNo = new QRadioButton( this, "unix" );
	m_pNo->setGeometry( 35, 85, 100, 20 );
	m_pNo->setMinimumSize( 0, 0 );
	m_pNo->setMaximumSize( 32767, 32767 );
	m_pNo->setFocusPolicy( QWidget::TabFocus );
	m_pNo->setBackgroundMode( QWidget::PaletteBackground );
	m_pNo->setFontPropagation( QWidget::NoChildren );
	m_pNo->setPalettePropagation( QWidget::NoChildren );
	m_pNo->setText( NOPRINT );
	m_pNo->setAutoRepeat( FALSE );
	m_pNo->setAutoResize( FALSE );
	m_pNo->setChecked( TRUE );

	printTestPage = false;
    
	// set up signal and slot for radio buttons to hide
  // and display the required stuff...
	connect( m_pNo, SIGNAL(clicked()), this, SLOT(noButtonClicked()));
	connect( m_pYes, SIGNAL(clicked()), this,SLOT(yesButtonClicked()));
	
  qtarch_label_2 = new QLabel( this, "label_2" );
  qtarch_label_2->setGeometry( 5, 115, 100, 20 );
  qtarch_label_2->setMinimumSize( 0, 0 );
  qtarch_label_2->setMaximumSize( 32767, 32767 );
  qtarch_label_2->setFocusPolicy( QWidget::NoFocus );
  qtarch_label_2->setBackgroundMode( QWidget::PaletteBackground );
  qtarch_label_2->setFontPropagation( QWidget::NoChildren );
  qtarch_label_2->setPalettePropagation( QWidget::NoChildren );
  qtarch_label_2->setText( i18n("Paper size:") );
  qtarch_label_2->setAlignment( AlignLeft | AlignTop | ExpandTabs | WordBreak);
	qtarch_ComboBox_1 = new QComboBox( FALSE, this, "ComboBox_1" );
	qtarch_ComboBox_1->setGeometry( 105, 115, 150, 20 );
	qtarch_ComboBox_1->setMinimumSize( 0, 0 );
	qtarch_ComboBox_1->setMaximumSize( 32767, 32767 );
	qtarch_ComboBox_1->setFocusPolicy( QWidget::StrongFocus );
	qtarch_ComboBox_1->setBackgroundMode( QWidget::PaletteBackground );
	qtarch_ComboBox_1->setFontPropagation( QWidget::AllChildren );
	qtarch_ComboBox_1->setPalettePropagation( QWidget::AllChildren );
	qtarch_ComboBox_1->setSizeLimit( 10 );
	qtarch_ComboBox_1->setAutoResize( FALSE );
	qtarch_ComboBox_1->setMaxCount( 2147483647 );
	qtarch_ComboBox_1->setAutoCompletion( FALSE );
	qtarch_label_2->hide();
  qtarch_ComboBox_1->hide();
  
	// connecting the two comboboxes
  //connect( qtarch_ComboBox_1, SIGNAL(highlighted(const char*)), this, SLOT(updateList(const char*)));

	resize( 400,300 );
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
}

void pageFour::yesButtonClicked()
{
	m_pYes->setChecked(true);
  m_pNo->setChecked(false);
	printTestPage = true;
	qtarch_ComboBox_1->setEnabled(true);
}

void pageFour::noButtonClicked()
{
	m_pNo->setChecked(true);
	m_pYes->setChecked(false);
	printTestPage = false;
	qtarch_ComboBox_1->setEnabled(false);
}

QString pageFour::getPaperSize()const
{
	return qtarch_ComboBox_1->currentText();//paperSize;
}
