/* Name: pageOne.cpp
            
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

#include "constText.h"
#include "pageOne.h"
#include <wizard.h>

////////////////////////////////////////////////////////////////////////////////

pageOne::pageOne(QWidget* parent, const char* name) :
	CWizardPage( parent, name, 0)
{
}

////////////////////////////////////////////////////////////////////////////////

pageOne::pageOne(int nPage,
								 bool bFinalPage,
								 int nNextPage,
								 int nPrevPage,
								 const char* pszImage,
								 const char* pszTitle,
								 bool bCheckPage,
								 QWidget *parent,
								 const char *name)
 : CWizardPage(nPage,
							 bFinalPage,
							 nNextPage,
							 nPrevPage,
							 pszImage,
							 pszTitle,
							 bCheckPage,
							 parent,
							 name)
{
	// Description label

	m_pmainMsg = new QLabel(this, "Label_1");
	m_pmainMsg->setGeometry(5, 3, 273, 70 );
	m_pmainMsg->setMinimumSize(0, 0);
	m_pmainMsg->setMaximumSize(32767, 32767);
	m_pmainMsg->setFocusPolicy(QWidget::NoFocus );
	m_pmainMsg->setBackgroundMode(QWidget::PaletteBackground );
	m_pmainMsg->setFontPropagation(QWidget::NoChildren );
	m_pmainMsg->setPalettePropagation(QWidget::NoChildren );
	m_pmainMsg->setText( LABEL1 );
	m_pmainMsg->setAlignment( AlignLeft | AlignTop | ExpandTabs | WordBreak ); //289
	m_pmainMsg->setMargin( -1 );

	// Invisible group box

	m_pGroup = new QButtonGroup( this, "ButtonGroup_1" );
	m_pGroup->setGeometry( 0, 0, 0, 0 );
	m_pGroup->setMinimumSize( 0, 0 );
	m_pGroup->setMaximumSize( 32767, 32767 );
	m_pGroup->setFocusPolicy( QWidget::NoFocus );
	m_pGroup->setBackgroundMode( QWidget::PaletteBackground );
	m_pGroup->setFontPropagation( QWidget::NoChildren );
	m_pGroup->setPalettePropagation( QWidget::NoChildren );
	m_pGroup->setExclusive(true);
	m_pGroup->setFrameStyle( 49 );
	m_pGroup->setTitle( "" );
	m_pGroup->setAlignment( 1 );

  // "Local" radio

	m_plocalPrn	= new QRadioButton(this, "RadioButton_1" );
	m_plocalPrn->setGeometry( 10, 70, 200, 30 );
	m_plocalPrn->setMinimumSize( 0, 0 );
	m_plocalPrn->setMaximumSize( 32767, 32767 );
	m_plocalPrn->setFocusPolicy( QWidget::TabFocus );
	m_plocalPrn->setBackgroundMode( QWidget::PaletteBackground );
	m_plocalPrn->setFontPropagation( QWidget::NoChildren );
	m_plocalPrn->setPalettePropagation( QWidget::NoChildren );
	m_plocalPrn->setText(LOCALLY);
	m_plocalPrn->setAutoRepeat(FALSE);
	m_plocalPrn->setAutoResize(FALSE);
  m_plocalPrn->setChecked( TRUE);
	m_pGroup->insert(m_plocalPrn);

	// "Network" radio

	m_pnetPrn = new QRadioButton(this, "RadioButton_2" );
	m_pnetPrn->setGeometry(10, 120, 200, 30);
	m_pnetPrn->setMinimumSize(0, 0);
	m_pnetPrn->setMaximumSize(32767, 32767);
	m_pnetPrn->setFocusPolicy(QWidget::TabFocus);
	m_pnetPrn->setBackgroundMode(QWidget::PaletteBackground);
	m_pnetPrn->setFontPropagation(QWidget::NoChildren);
	m_pnetPrn->setPalettePropagation(QWidget::NoChildren);
	m_pnetPrn->setText(REMOTELY);
	m_pnetPrn->setAutoRepeat(FALSE);
	m_pnetPrn->setAutoResize(FALSE);
	m_pGroup->insert(m_pnetPrn);
}

////////////////////////////////////////////////////////////////////////////////

pageOne::~pageOne()
{
	delete m_pmainMsg;
	delete m_pnetPrn;
	delete m_plocalPrn;
	delete m_pGroup;
}

////////////////////////////////////////////////////////////////////////////////

int pageOne::GetTypeOfPrinter()
{
	return m_pnetPrn->isChecked() ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////

void pageOne::pressedButton( int nButton, int nPage )
{
  if ((nButton == eButtonHelp) && (nPage == m_nPage))
  {
		QString helpfileName;
  	helpfileName = "cs_print_add.htm";
#ifndef QT_20
  	showClientHelp(helpfileName, "");
//commented by alexandrm
#endif
	}
}

////////////////////////////////////////////////////////////////////////////////

