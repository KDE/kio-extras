/*

   wizardpage.cpp

   Author: Florin Epure
   Revisor 1: Emily Ezust
   Revisor 2: Joe Mason

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


#include <stdio.h>

#include <qfont.h>
#include <qpainter.h>
#include <qmessagebox.h>
#include "wizard.h"
#include "wizardpage.h"

CWizardPage::CWizardPage( QWidget *parent, const char *name, WFlags f)
  : QWidget( parent, name, f ), m_bFinalPage( false ), 
    m_nNextPage( -1 ), m_nPrevPage( -1 ),
    m_pszImage( 0 ), m_pszTitle( 0 ), m_pWizard((CCorelWizard *)parent),
    m_bCheckPage( true ) 
{
  //you can set page to real value with set page
  //after it was added to wizard
  m_nPage = 0; 
}

CWizardPage::CWizardPage(int nPage, bool bFinalPage, int nNextPage,
			 int nPrevPage, const char* pszImage,
			 const char* pszTitle, bool bCheckPage,
			 QWidget *parent, const char *name)
  : QWidget(parent, name), m_bFinalPage(bFinalPage), m_nNextPage(nNextPage),
    m_nPrevPage(nPrevPage), m_pszImage(pszImage), m_pszTitle(pszTitle),
    m_nPage(nPage), m_pWizard((CCorelWizard *)parent),
    m_bCheckPage(bCheckPage) 
{ 
#ifdef	WIZARDPAGE_TEST
  setBackgroundColor( white );
#endif//WIZARDPAGE_TEST 
}

void CWizardPage::setPage(int nPage, bool bFinalPage,
			  int nNextPage, int nPrevPage,
			  const char* pszImage, const char* pszTitle,
			  bool bCheckPage)
{
  m_nPage = nPage;
  m_bFinalPage = bFinalPage;
  m_nNextPage = nNextPage;
  m_nPrevPage = nPrevPage;
  m_pszImage = pszImage;
  m_pszTitle = pszTitle;
  m_bCheckPage = bCheckPage;
}

void CWizardPage::pressedButton( int nButton, int nPage )
{
  //  	printf("on page %d buttonpress %d sent to page %d\n",nPage,
  //	       nButton,m_nPage);

  if ((nButton == eButtonHelp) && (nPage == m_nPage))
  {
    QMessageBox::information(this, "help dialog",
			     "Help has not yet been implemented.");
  }
}

void CWizardPage::paintEvent( QPaintEvent* event )
{
  QWidget::paintEvent( event );
#ifdef WIZARDPAGE_TEST
  QPainter p( this );
  p.setFont( "times" );
  p.drawText(10, 10, (m_pszTitle.length() == 0) ?
	     "no title" : (const char*)m_pszTitle );
#endif//WIZARDPAGE_TEST
}

void CWizardPage::enableButton( int nButton, bool bEnable )  // slot
{
  ASSERT (m_pWizard != 0);
  // pass on the enable command to the wizard, where the buttons live
  m_pWizard->enableButton(nButton, bEnable);
}
