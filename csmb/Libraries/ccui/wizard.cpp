/*

   wizard.cpp

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


#include <kapp.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qframe.h>
#include <qpixmap.h>
#include <qaccel.h>
#include <qfontmetrics.h>
#include <qobjectlist.h>
#include <qwindowdefs.h>
#include <qfont.h>
#include <qapplication.h>
#include <iostream.h>
#include <X11/Xlib.h>

#include "wizard.h"
#include "wizardpage.h"

//transient decorations disable
bool gbTransient = false;

CCorelWizard::CCorelWizard( int nButton1, int nButton2, int nButton3,
			    int nButton4, int nButton5, bool bSpacing,
			    QWidget *parent, const char *name, bool bModal )
  : QDialog(parent, name, bModal), m_buttonOrderList(NUM_BUTTONS),
    m_bSpacing(bSpacing), m_bDoneReplacesNextOnFinalPage(true)
{
  initWidget( IDS_PREV1, IDS_NEXT1, IDS_DONE1, IDS_HELP1, IDS_CANCEL1,
	      nButton1, nButton2, nButton3, nButton4, nButton5);
}

CCorelWizard::CCorelWizard( const char* szPrev, const char* szNext,
			    const char* szDone, const char* szHelp,
			    const char* szCancel,
			    int nButton1, int nButton2, int nButton3,
			    int nButton4, int nButton5, bool bSpacing,
			    QWidget *parent, const char *name, bool bModal)
  : QDialog(parent, name, bModal), m_buttonOrderList(NUM_BUTTONS),
    m_bSpacing(bSpacing), m_bDoneReplacesNextOnFinalPage(true)

{
  initWidget( szPrev, szNext, szDone, szHelp, szCancel,
	      nButton1, nButton2, nButton3, nButton4, nButton5);
}

void CCorelWizard::setActivePageEvent( int /*nPage*/, int /*nButton*/ )
{
}

void CCorelWizard::show(int nPage)
{
  QDialog::show();
  //you must call move AFTER dialog show because otherwise
  //position hints are ignored by window manager
  move(m_npx,m_npy);
  //  printf("x:%d,y:%d\n",m_npx,m_npy);
  //force no resize = min = max = default size
  setMinimumSize( m_ncx, m_ncy );
  setMaximumSize( m_ncx, m_ncy );
  ASSERT( m_pageList.count() != 0 );
  setActivePage( nPage );
}

void CCorelWizard::initWidget( const char* szPrev, const char* szNext,
			       const char* szDone, const char* szHelp,
			       const char* szCancel,
			       int nButton1, int nButton2, int nButton3,
			       int nButton4, int nButton5)
{
  ASSERT( szPrev );
  ASSERT( szNext );
  ASSERT( szDone );
  ASSERT( szHelp );
  ASSERT( szCancel );

  // order of buttons is reversed here because we display them right-to-left.
  // The order therefore appears the same in the constructor call as onscreen.
  m_buttonOrderList[0] = nButton5;
  m_buttonOrderList[1] = nButton4;
  m_buttonOrderList[2] = nButton3;
  m_buttonOrderList[3] = nButton2;
  m_buttonOrderList[4] = nButton1;

  m_nDefaultButton = -1;

  //	SetDisplay(qt_xdisplay());
  //take all possible decorations (works with kde wm)
  if (gbTransient)
  {
    XSetTransientForHint(qt_xdisplay(),winId(),winId());
  }

  m_bLeftImage = true;
  m_bCustomPos = false;

#ifndef QT_2
  //turn off minimize button-not working
  GUIStyle gstyle = style();
  gstyle = (GUIStyle) (gstyle & ~WStyle_Minimize);
  setStyle( gstyle );
#endif

  //decorations removing on MWM and Motif compatible wm
  //	ChangeDecorations( winId(), qt_xdisplay(), 0 );

  setGeometry(80,40,480,360);
  //setGeometry call fixes also below problem also

  //I have tried to add below code so that you dont have to specify
  //position and size of wizard prior to calling show function
  //for some reason this modification does not work now
  //so I had to comment it out back again
  m_npx = PX;
  m_npy = PY;

  m_ncx = CX;
  m_ncy = CY;
  //default image size
  m_nix = m_ncx/3;
  m_niy = m_ncy;

  //save done,cancel strings
  m_strDone = szDone;
  m_strCancel = szCancel;

  m_bButtonHidden = false;

  m_bImageHidden = true;
  m_bTitleHidden = true;

  m_pImage = new QLabel( this, "image" );
  m_pImage->setBackgroundColor( gray );
  m_pImage->hide();

  m_pTitle = new QLabel( this, "title" );
  QFont f( "times", 24 );
  m_pTitle->setFont( f );
  m_pTitle->setBackgroundColor( gray );
  m_pTitle->hide();

  m_pageList.setAutoDelete( true );
  m_pageButtonEnableAttributeList.setAutoDelete( true );

  //initialize page
  m_nCurrentPage = -1;
  m_nCurrentPageEnableAttribute = 0;


#ifdef DEBUG1
  cerr << "Set up button pointer array" << endl;
#endif

  // create buttons
  m_pPrev = new QPushButton( szPrev, this, "prev" );
  connect( m_pPrev, SIGNAL(clicked()), this, SLOT(prevClicked()) );

  m_pNext = new QPushButton( szNext, this, "next" );
  connect( m_pNext, SIGNAL(clicked()), this, SLOT(nextClicked()) );

  m_pHelp = new QPushButton( szHelp, this, "help" );
  connect( m_pHelp, SIGNAL(clicked()), this, SLOT(helpClicked()) );

  m_pCancel = new QPushButton( szCancel, this, "cancel" );
  connect( m_pCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()) );

  m_pDone = new QPushButton( szDone, this, "done" );
  connect( m_pDone, SIGNAL(clicked()), this, SLOT(doneClicked()) );


  // set up button pointer array to match order of m_buttonOrderList
  m_aButtons[m_buttonOrderList.find(eButtonHelp)] = m_pHelp;
  m_aButtons[m_buttonOrderList.find(eButtonDone)] = m_pDone;
  m_aButtons[m_buttonOrderList.find(eButtonCancel)] = m_pCancel;
  m_aButtons[m_buttonOrderList.find(eButtonNext)] = m_pNext;
  m_aButtons[m_buttonOrderList.find(eButtonPrev)] = m_pPrev;


  // set up tabbing to follow that order
  // don't forget they're in reverse order...
  setTabOrder(m_aButtons[4], m_aButtons[3]);
  setTabOrder(m_aButtons[3], m_aButtons[2]);
  setTabOrder(m_aButtons[2], m_aButtons[1]);
  setTabOrder(m_aButtons[1], m_aButtons[0]);
  

  m_pFrame = new QFrame( this, "frame" , 0, TRUE );
  m_pFrame->setFrameStyle(QFrame::HLine+QFrame::Sunken);
  m_pFrame->setFixedHeight(FY);

  connect( this, SIGNAL(exit()), qApp, SLOT(quit()) );

  //add accelerator keys
  //first figure out a letter key to add to accelerator
  //for following:next,prev,help
  int nHelp = ALT;
  int nPrev = ALT;
  int nNext = ALT;
  //right now just pick first letter from string
  //there may be colliding - just assert to that
  //skip until finding first letter case
  char c = 0;
  for (unsigned int i = 0; i < strlen(szHelp); i++)
  {
    c = szHelp[i] & ~('a'-'A');
    if ((c >= 'A') && (c <= 'Z'))
    {
      break;
    }
  }
  nHelp += c;
  for (unsigned int i = 0; i < strlen(szPrev); i++)
  {
    c = szPrev[i] & ~('a'-'A');
    if ((c >= 'A') && (c <= 'Z'))
    {
      break;
    }
  }
  nPrev += c;
  for (unsigned int i = 0; i < strlen(szNext); i++)
  {
    c = szNext[i] & ~('a'-'A');
    if ((c >= 'A') && (c <= 'Z'))
    {
      break;
    }
  }
  nNext += c;
  ASSERT( nHelp != nPrev );
  ASSERT( nPrev != nNext );
  ASSERT( nNext != nHelp);

  m_pAccel = new QAccel( this , "accel" );
  m_pAccel->connectItem(m_pAccel->insertItem( ALT+Key_Left ),
			this, SLOT(prevKeyClicked()));
  //m_pAccel->connectItem(m_pAccel->insertItem( nPrev ),
  //  this,SLOT(prevKeyClicked()));
  m_pAccel->connectItem(m_pAccel->insertItem( ALT+Key_Right ),
			this,SLOT(nextKeyClicked()));
  //m_pAccel->connectItem(m_pAccel->insertItem( nNext ),
  //  this,SLOT(nextKeyClicked()));
  m_pAccel->connectItem(m_pAccel->insertItem( Key_F1 ),
			this, SLOT(helpKeyClicked()));
  //m_pAccel->connectItem(m_pAccel->insertItem( nHelp ),
  //   this, SLOT(helpKeyClicked()));
  m_pAccel->connectItem(m_pAccel->insertItem( Key_Return ),
			this, SLOT(retKeyClicked()));
  m_pAccel->connectItem(m_pAccel->insertItem( Key_Escape ),
			this, SLOT(escKeyClicked()));
}

void CCorelWizard::setLeftImage( bool bLeftImage )
{
  m_bLeftImage = bLeftImage;
}

void CCorelWizard::setSize( int w, int h )
{
  //set dimension of wizard on x,y
  m_ncx = w;
  m_ncy = h;

  //calculate where to position wizard
  //it has to be at middle of screen (root window)
  //get get display resolution by checking root window width, height
  //	GetWindowSize( qt_xrootwin(), qt_xdisplay(), &m_npx, &m_npy);
  m_npx = QApplication::desktop()->width();
  m_npy = QApplication::desktop()->height();

  m_npx = m_npx/2-m_ncx/2;
  m_npy = m_npy/2-m_ncy/2;

  //force no resize = min = max = default size
  //	setMinimumSize( m_ncx, m_ncy );
  //	setMaximumSize( m_ncx, m_ncy );

	//set size of image to 1/3 if text is displayed
  m_nix = w/3;
  m_niy = h;
}

void CCorelWizard::setPos( int x, int y)
{
  m_npx = x;
  m_npy = y;
}

QWidget* CCorelWizard::getPage( int nPage )
{
  ASSERT( nPage < getCount() );
  return m_pageList.at( nPage );
}

int CCorelWizard::getIndex( QWidget* pWidget )
{
  ASSERT( pWidget );
  int nRet =  m_pageList.find( pWidget );
  ASSERT( nRet != -1 );

  return nRet;
}

QWidget* CCorelWizard::getActivePage()
{
  ASSERT( m_nCurrentPage != -1 );
  return m_pageList.at(m_nCurrentPage);
}

int CCorelWizard::getPageEnableAttribute( int nPage )
{
  ASSERT( nPage < getCount() );
  return *m_pageButtonEnableAttributeList.at( nPage);
}

int CCorelWizard::getPageShowAttribute( int nPage )
{
  ASSERT( nPage < getCount() );
  return *m_pageButtonShowAttributeList.at( nPage);
}


int CCorelWizard::getActivePageAttribute()
{
  ASSERT( m_nCurrentPage != -1 );
  return *m_pageButtonEnableAttributeList.at(m_nCurrentPage);
}

int CCorelWizard::getActiveIndex()
{
  ASSERT( m_nCurrentPage != -1 );

  return m_nCurrentPage;
}

int CCorelWizard::getCount()
{
  int nRet = m_pageList.count();
  ASSERT( nRet != 0 );

  return nRet;
}

bool CCorelWizard::setActivePage( int nPage, int nButton )
{
  //  printf("active page %d(button %d)\n",nPage,nButton);
  if (m_nCurrentPage == -1)
  {
    m_pPrev->hide();
    m_pNext->hide();
    m_pHelp->hide();
    m_pCancel->hide();
    m_pDone->hide();

    m_pFrame->hide();
    m_bButtonHidden = true;
    m_pTitle->hide();
    m_pImage->hide();
    m_bImageHidden = true;
    m_bTitleHidden = true;
    //  printf("nothing to show\n");

    return false;
  }

  if (m_bButtonHidden)
  {
    m_bButtonHidden = false;

    int nShowAttribs = getPageShowAttribute(nPage);

    if (nShowAttribs & eButtonPrev)
      m_pPrev->show();
    if (nShowAttribs & eButtonNext)
      m_pNext->show();
    if (nShowAttribs & eButtonHelp)
      m_pHelp->show();
    if (nShowAttribs & eButtonCancel)
      m_pCancel->show();
    if (nShowAttribs & eButtonDone)
    {
      //      cerr << "Showing final button" << endl;
      m_pDone->show();
    }

    m_pFrame->show();
  }

  int dx = DX;
  int dy = DY;

  int hButton = HY;
  int wButton = HX;
  int hFrame = FY;

  //over-ride button height
  QString s = "XXXX";
  QFontMetrics fm = fontMetrics();
  QSize sz = fm.size(ShowPrefix,s);

  int h = sz.height()+sz.height()/8+8;
  //printf("button %d(%d)\n",h,hButton);
  hButton = h;

  int hCustom = IY;

  //printf("x:%d,y:%d\n",width(),height());

  bool bRet = true;
  ASSERT( nPage < getCount() );

  //get current page-that we hide
  CWizardPage* pPage1 = (CWizardPage*) getPage( m_nCurrentPage );

  //get next page-that we show
  CWizardPage* pPage = (CWizardPage*) getPage( nPage );

  // this prevents people typing stuff on page two that goes into a
  // focused widget on page one... basically a clearFocus on whatever
  // widget had it on the page from which we're coming.
  pPage->setFocus();

  // setFocus on the first child of the page instead... but doesn't work!
  // const QObjectList *pChildList = pPage->children();
#if 0
  QWidget *pWidget = NULL;
  if (pChildList)
  {
    pWidget = (QWidget *)pChildList->getLast();
  }
  if (pWidget)
  {
    pWidget->setFocus();
  }
  else
  {
    pPage->setFocus();
  }
#endif

  int nLastPage = m_nCurrentPage;
  bool bCheck = true;

  //allow over-ride of check page if corel wizard page
  if (pPage1->inherits("CWizardPage"))
  {
    // was    bCheck = pPage1->checkPage(nButton == eButtonNext);
    bCheck = pPage1->checkPage(nButton);
  }

  if (!bCheck)
  {
    if (m_nCurrentPage != nPage && m_nCurrentPage != -1)
    {
#ifdef DEBUG1
      //change page not permitted
      cerr << "change page " << m_nCurrentPage << " not permitted" << endl ;
#endif
      return false;
    }
  }

  bool bImage = false;
  bool bTitle = false;
  bool bFinalPage = false;

  const char* pTitle = 0;
  const char* pImage = 0;

  if (pPage->inherits("CWizardPage"))
  {
    //prepare page
    pPage->preparePage();

    //check now for image and title
    bImage = pPage->getImage( &pImage );
    bTitle = pPage->getTitle( &pTitle );

    //see if final page
    bFinalPage = pPage->isFinalPage();
  }

  //hide current page widget
  pPage1->hide();

  //show next page widget
  pPage->show();

  //save nPage
  m_nCurrentPage = nPage;
  m_nCurrentPageEnableAttribute = getPageEnableAttribute( nPage );

  int ix = 0;
  int iy = 0;

  if (bImage)
  {
    ASSERT( pImage );
    //check to see if we really have image or text
    //check image to have ".xpm" extension
    unsigned int l = strlen( pImage );
    if ( pImage[l-4] == '.' && pImage[l-3] == 'x' &&
	 pImage[l-2] == 'p' && pImage[l-1] == 'm')
    {
      QPixmap pix( pImage );

      //get size of image
      ix = pix.width();
      iy = pix.height();

      m_pImage->setPixmap( pix );
    }
    else
    {
      //it's text
      ix = m_nix;
      iy = height()-dy-hButton-dy-hFrame-dy-dy;//m_niy;
      char szBuffer[256] = {0};
      strncpy( szBuffer, pImage, 256 );
      //just take out "\n" and replace with lf char
      for (unsigned int j = 0; j < l; j++)
      {
	if ((szBuffer[j] == '\\') && (szBuffer[j+1] == 'n'))
	{
	  szBuffer[j] = ' ';
	  szBuffer[j+1] = '\n';
	  j++;//skip two chars
	}
      }
      m_pImage->setText( szBuffer );
    }
    
    if (m_bImageHidden)
    {
      //printf("show image\n");
      
      m_pImage->show();
      m_bImageHidden = false;
    }
  }
  else
  {
    if (!m_bImageHidden)
    {
      //printf("hide image\n");

      m_pImage->hide();
      m_bImageHidden = true;
    }

    ix = -dx;
  }

  int ty = 0;

  if (bTitle)
  {
    ASSERT( *pTitle );
    ty = LY;

    m_pTitle->setText( pTitle );

    if (m_bTitleHidden)
    {
      //printf("show title\n");

      m_pTitle->show();
      m_bTitleHidden = false;
    }
  }
  else
  {
    if (!m_bTitleHidden)
    {
      //printf("hide title\n");

      m_pTitle->hide();
      m_bTitleHidden = true;
    }

    ty = -dx;
  }

  //draw image
  if (bImage)
  {
    if (m_bLeftImage)
    {
      m_pImage->setGeometry( dx, dy, ix, iy );
    }
    else
    {
      m_pImage->setGeometry( width()-dx-ix, dy, ix, iy );
    }
  }

  //draw title
  if (bTitle)
  {
    if (m_bLeftImage)
    {
      m_pTitle->setGeometry( dx+ix+dx, dy, width()-dx-ix-dx-dx, ty);
    }
    else
    {
      m_pTitle->setGeometry( dx, dy, width()-dx-ix-dx-dx, ty);
    }
  }

  //if next,prev buttons are custom positioned

  if (m_bCustomPos)
  {
    //draw widget
    if (m_bLeftImage)
    {
      pPage->setGeometry(dx+ix+dx, dy+ty+dy, width()-dx-ix-dx-dx,
			 height()-dy-hButton-dy-hFrame-dy-ty-dy-dy-hCustom-dy);
    }
    else
    {
      pPage->setGeometry(dx, dy+ty+dy, width()-dx-ix-dx-dx,
			 height()-dy-hButton-dy-hFrame-dy-ty-dy-dy-hCustom-dy);
    }
  }
  else
  {
    //draw widget
    if (m_bLeftImage)
    {
      pPage->setGeometry( dx+ix+dx, dy+ty+dy,
			  width()-dx-ix-dx-dx,
			  height()-dy-hButton-dy-hFrame-dy-ty-dy-dy);
    }
    else
    {
      pPage->setGeometry( dx, dy+ty+dy,
			  width()-dx-ix-dx-dx,
			  height()-dy-hButton-dy-hFrame-dy-ty-dy-dy);
    }
  }

  if (m_bCustomPos)
  {
    //draw frame
    m_pFrame->setGeometry(dx,height()-dy-hCustom-dy-hFrame,
			  width()-2*dx,hFrame);
  }
  else
  {
    //draw frame
    m_pFrame->setGeometry(dx,height()-dy-hButton-dy-hFrame,
			  width()-2*dx,hFrame);
  }

  int nDisplacement = 0;
  if (m_bCustomPos)      // move buttons into wizardpage area.
  {
    nDisplacement = hFrame+dy+hCustom+dy;
  }

  // traverse list of pointers to buttons and show or hide as necessary

  int nLoopIndex, nButtonNumber;
  int nAttrib =  getPageShowAttribute(nPage);
  int nButtonType;

  for (nLoopIndex = 0, nButtonNumber = 1; nLoopIndex < NUM_BUTTONS;
       ++nLoopIndex)
  {
    // nButtonType will have a value like eButtonPrev, eButtonNext, etc.
    nButtonType = m_buttonOrderList[nLoopIndex];

    if (nAttrib & nButtonType)  // if button should be shown
    {
      if (doneReplacesNextOnFinalPage() &&
	  (bFinalPage || (nPage == getCount()-1)) &&
	  ((nButtonType == eButtonNext) || (nButtonType == eButtonDone)))
      {
	if (nButtonType == eButtonNext)
	{
	  // hide the next button
	  m_aButtons[m_buttonOrderList.find(eButtonNext)]->hide();

	  // put the done button in the location of the hidden next
	  int nTempIndex = m_buttonOrderList.find(eButtonDone);
	  m_aButtons[nTempIndex]->setGeometry(width() - nButtonNumber*dx -
					      nButtonNumber*wButton,
					      height()-dy-hButton-
					      nDisplacement, wButton, hButton);
	  m_aButtons[nTempIndex]->show();
	  ++nButtonNumber;
	}
	else if (nButtonType == eButtonDone)
	{
	  // don't need to do anything - see other branch
	}
      }
      else // show the button normally
      {
	m_aButtons[nLoopIndex]->setGeometry(width() - nButtonNumber*dx -
					    nButtonNumber*wButton,
					    height()-dy-hButton-nDisplacement,
					    wButton, hButton);
	m_aButtons[nLoopIndex]->show();
	++nButtonNumber;
      }
    }
    else  // don't show button
    {
      m_aButtons[nLoopIndex]->hide();
      if (isSpaced())
      {
	++nButtonNumber;  // leave a blank space for this button
      }
    }
  }

  //derived call positioning
  setActivePageEvent( nPage, nButton );

  if (nButton == eButtonNext)
  {

    //re-enable back button if we are now on first page
    if (nLastPage == 0)
    {
      enableButton( eButtonPrev, true );
    }

    //if we are on last page of wizard or if virtual last page
    //disable next button or show Done button there...
    if ((bFinalPage || (nPage == getCount()-1)))
    {
      if (! doneReplacesNextOnFinalPage())
      {
	enableButton( eButtonNext, false );
      }

      if ( ! (m_nCurrentPageEnableAttribute & eButtonDone))
      {
	cerr << "It looks like you forgot to enable the Done button on the";
	cerr << " final page,\nso I've enabled it. If you meant to do that,";
	cerr << " then please override the\nfunction prepareButtons() to";
	cerr << " specify the conditions under which the\nDone button";
	cerr << " will be enabled.\n";
      }
      enableButton( eButtonDone, true);
    }
    else
    {
      enableButton(eButtonCancel,
		   m_nCurrentPageEnableAttribute & eButtonCancel?true:false);
      enableButton(eButtonDone,
		   m_nCurrentPageEnableAttribute & eButtonDone?true:false);

    }
  }
  else if (nButton == eButtonPrev)
  {
    //re-enable next button if currently disabled
    enableButton( eButtonNext, true );

    //if first page disable back
    if (nPage == 0)
    {
      enableButton( eButtonPrev, false );
      //      m_nCurrentPageEnableAttribute &= ~eButtonPrev;
    }

    enableButton( eButtonCancel,
		  m_nCurrentPageEnableAttribute & eButtonCancel ? true : false);
      enableButton(eButtonDone,
		   m_nCurrentPageEnableAttribute & eButtonDone?true:false);
  }
  else
  {
    //bug fix
    //if jump from last to first page next button remains disabled
    //re-enable next button if currently disabled
    enableButton( eButtonNext, true );

    //bug fix
    //if jump from first to last page prev button remains disabled
    //re-enable prev button if currently disabled
    if (m_nCurrentPageEnableAttribute & eButtonPrev)
    {
      enableButton( eButtonPrev, true );
    }

    ASSERT( nButton == 0);
    //if we have only one page disable next,prev
    //if first page disable back
    if (nPage == 0)
    {
      enableButton( eButtonPrev, false );
      //      m_nCurrentPageEnableAttribute &= ~eButtonPrev;
    }
    //if we are on last page of wizard or if virtual last page
    //disable next button
    if ( bFinalPage || (nPage == getCount()-1))
    {
      if (bFinalPage)
      {
	//printf("final page\n");
      }
      enableButton( eButtonNext, false );

      enableButton( eButtonDone, true);

    }
    else
    {
      enableButton( eButtonCancel,
		    m_nCurrentPageEnableAttribute & eButtonCancel ? true : false);
      enableButton(eButtonDone,
		   m_nCurrentPageEnableAttribute & eButtonDone?true:false);
    }
  }

  enableButton( eButtonHelp,
		m_nCurrentPageEnableAttribute & eButtonHelp ? true : false);


  if (pPage->inherits("CWizardPage"))
  {
    pPage->prepareButtons();  // for changing disable/enable rules
  }

  return bRet;
}

bool CCorelWizard::setActivePage( int nPage )
{
  return setActivePage( nPage, 0 );
}

bool CCorelWizard::setActivePage( QWidget* pWidget )
{
  ASSERT( pWidget );

  int nPage = getIndex( pWidget );

  return setActivePage( nPage );
}

int CCorelWizard::addPage( QWidget* pWidget, int nPageButtonEnableAttribute,
			   int nPageButtonShowAttribute, bool bActivePage )
{
  int nRet = -1;

  //  printf("add page %x\n",pWidget);

  ASSERT( pWidget );
  ASSERT( pWidget->inherits("QWidget") );

  m_pageList.append( pWidget );

  //hide widget
  pWidget->hide();

  // save attributes

  int* pnPageButtonEnableAttribute = new int;
  *pnPageButtonEnableAttribute = nPageButtonEnableAttribute;
  
  ASSERT( nPageButtonEnableAttribute & (eButtonPrev | eButtonNext) );
  m_pageButtonEnableAttributeList.append( pnPageButtonEnableAttribute );
  

  // in case the adder of the page forgot to add eButtonDone to the 
  // show attributes for the final page
  if (pWidget->inherits("CWizardPage"))
  {
    if ( ((CWizardPage *)pWidget)->isFinalPage())
    {
      if ( ! (nPageButtonShowAttribute & eButtonDone))
      {
	nPageButtonShowAttribute |= eButtonDone;
	cerr << "You forgot to add eButtonDone on page " ;
	cerr << ((CWizardPage *)pWidget)->getPageNumber() ;
	cerr << "; I've done it anyway.";
	cerr << endl;
      }
    }
  }
  int* pnPageButtonShowAttribute = new int;
  *pnPageButtonShowAttribute = nPageButtonShowAttribute;

  ASSERT( nPageButtonShowAttribute & (eButtonPrev | eButtonNext) );
  m_pageButtonShowAttributeList.append( pnPageButtonShowAttribute );

  //return index of this page - it is last element added to list
  nRet = getCount() - 1;

  //set active page if first page added or bActivePage is true
  //use bActivePage to start wizard activation 
  //at page different from first page
  if (bActivePage || (getCount() == 1))
  {
    //      printf("set active page\n");
    m_nCurrentPage = getCount() - 1;
    m_nCurrentPageEnableAttribute = nPageButtonEnableAttribute;
  }

  //if our wizard page connect page signal
  CWizardPage* pPage = (CWizardPage*) pWidget;

  if (pPage->inherits("CWizardPage"))
  {
    connect( this, SIGNAL(clickedButton(int,int)), pPage,
	     SLOT(pressedButton(int,int)) );
  }
  return nRet;
}

bool CCorelWizard::removePage( QWidget* pWidget )
{
  ASSERT( pWidget );

  int nPage = getIndex( pWidget );

  return removePage( nPage ); 
}

bool CCorelWizard::removePage( int nPage )
{
  ASSERT( nPage < getCount() );

  //  printf("remove page %d (count %d)\n",nPage,getCount());

  bool bRet = false;

  if (getCount() == 1)
  {
    //      printf("remove last page\n");
    //if remove last element
    m_nCurrentPage = -1;
    //next if will not be executed and no set active page will be called
    setActivePage( m_nCurrentPage );//will clear buttons
  }

  if (nPage == m_nCurrentPage)
  {
    //      printf("remove current page\n");

    //we have at least two elements here
    int nNextPage = nPage+1;

    if (nNextPage > getCount()-1)
    {
      //	  printf("adjust last page\n");
      //just stay on last element
      if (nNextPage == getCount())
      {
	nNextPage = getCount()-1-1;
      }
      else
      {
	nNextPage = getCount()-1;
      }
    }
    bool bRet = setActivePage( nNextPage );
    ASSERT( bRet );
  }

  bRet = m_pageList.remove( nPage );
  ASSERT( bRet );

  ASSERT( m_nCurrentPage != nPage );
  //if page removed is before current page decrement current page
  if (m_nCurrentPage > nPage)
  {
    //      printf("decrement current page\n");
    m_nCurrentPage--;
  }

  bRet = m_pageButtonEnableAttributeList.remove( nPage );

  return bRet;
}

void CCorelWizard::enableButton( int nButton, bool bEnable )  // slot
{
  //  printf("button %d enable %d\n",nButton,bEnable);
  QPushButton** ppButton = 0;

  // modify enable attribs
  if (bEnable)
  {
    if ( (numPages() == m_nCurrentPage+1) && (nButton == eButtonNext))
    {
      cerr << "You have Next enabled on the final page, but I'm ignoring it."
	   << endl;
      return;
    }
    else if ((nButton == eButtonPrev) && (m_nCurrentPage == 0))
    {
      cerr << "You have Prev enabled on the first page, but I'm ignoring it."
	   << endl;
      return;
    }
    else
    {
      m_nCurrentPageEnableAttribute |= nButton;
      *(m_pageButtonEnableAttributeList.at(m_nCurrentPage)) |= nButton;
    }
  }
  else
  {
    m_nCurrentPageEnableAttribute &= ~nButton;
    *(m_pageButtonEnableAttributeList.at(m_nCurrentPage)) &= ~nButton;
  }


  switch (nButton)
  {
  case eButtonPrev:
    ppButton = &m_pPrev;
    break;
  case eButtonNext:
    ppButton = &m_pNext;
    break;
  case eButtonHelp:
    ppButton = &m_pHelp;
    break;
  case eButtonCancel:
    ppButton = &m_pCancel;
    break;
  case eButtonDone:
    ppButton = &m_pDone;
    break;
  default:
    ASSERT( 0 );
  }

  if (ppButton)
  {
    (*ppButton)->setEnabled( bEnable );
  }
}

void CCorelWizard::pressButton( int nButton )
{
  // figure out which button was pressed

  switch (nButton)
  {
  case eButtonPrev:
    prevClicked();
    break;
  case eButtonNext:
    nextClicked();
    break;
  case eButtonHelp:
    helpClicked();
    break;
  case eButtonCancel:
    cancelClicked();
    break;
  case eButtonDone:
    doneClicked();
    break;
  default:
    ASSERT( 0 );
  }
}

void CCorelWizard::resizeEvent( QResizeEvent *event )
{
  //  printf("resize x:%d y:%d w:%d h:%d\n",x(),y(),width(),height());

  //dialog will be fixed size so we don't need it
  //if resizable dialog re-enable this code
  //	setActivePage( m_nCurrentPage );
  QDialog::resizeEvent( event );
}

void CCorelWizard::prevClicked()
{
  //  printf("prev\n");

  emit clickedButton( eButtonPrev, m_nCurrentPage );

  CWizardPage* pPage = (CWizardPage*) getPage( m_nCurrentPage );

  //we can safely decrement m_nCurrentPage because for first page
  //prev button is disabled
  ASSERT( m_nCurrentPage >= 1 );
  int nPrevPage = m_nCurrentPage - 1;

  bool bRet = false;

  //allow over-ride of jump page if corel wizard page
  if (pPage->inherits("CWizardPage"))
  {
    int nOverPage = nPrevPage;

    bRet = pPage->getPrevPage( &nOverPage );

    if (bRet)
    {
      //	  printf("virtual jump prev page\n");
      nPrevPage = nOverPage;
    }
  }

  //limit page to our limits
  if (nPrevPage < 0)
  {
    nPrevPage = 0;
  }
  if (nPrevPage > getCount()-1)
  {
    nPrevPage = getCount()-1;
  }
  //set new page
  bRet = setActivePage( nPrevPage, eButtonPrev );
  ASSERT( bRet );
}

void CCorelWizard::nextClicked()
{
  //  printf("next\n");

  emit clickedButton( eButtonNext, m_nCurrentPage );

  CWizardPage* pPage = (CWizardPage*) getPage( m_nCurrentPage );

  ASSERT( m_nCurrentPage < getCount()-1 );
  //get next page
  int nNextPage = m_nCurrentPage + 1;

  bool bRet = false;

  //allow over-ride of jump page if corel wizard page
  if (pPage->inherits("CWizardPage"))
  {
    int nOverPage = nNextPage;

    bRet = pPage->getNextPage( &nOverPage );

    if (bRet)
    {
      //	  printf("virtual jump next page\n");
      nNextPage = nOverPage;
    }
  }

  //limit page to our limits
  if (nNextPage < 0)
  {
    nNextPage = 0;
  }
  if (nNextPage > getCount()-1)
  {
    nNextPage = getCount()-1;
  }
  bRet = setActivePage( nNextPage, eButtonNext );
  ASSERT( bRet );
}

void CCorelWizard::helpClicked()
{
  //  printf("help\n");

  emit clickedButton( eButtonHelp, m_nCurrentPage );

  //note:each page may bring up help by intercepting help signal
}

void CCorelWizard::doneClicked()
{
  bool bCheck=true;

  ASSERT( m_nCurrentPageEnableAttribute & eButtonDone );
  emit clickedButton( eButtonDone, m_nCurrentPage );

  CWizardPage* pPage = (CWizardPage*) getPage( m_nCurrentPage );

  if (pPage->inherits("CWizardPage"))
  {
    bCheck = pPage->checkPage(eButtonDone);
  }

  if (bCheck && checkLast(Accepted))
  {
    done( Accepted );
  }
}


void CCorelWizard::cancelClicked()
{

  ASSERT( m_nCurrentPageEnableAttribute & eButtonCancel );

  emit clickedButton( eButtonCancel, m_nCurrentPage );
  if (checkLast(Rejected)) 
  {
    done( Rejected );
  }
}

bool CCorelWizard::checkLast( int /* r */ )
{
  //over-ride in derived classes to enable exit
  return true;
}

void CCorelWizard::done( int /* r */ )
{
  //  printf("done:%d\n",r);

  emit exit();
}

void CCorelWizard::prevKeyClicked()
{
  //  printf("prev key\n");
  pressButton( eButtonPrev );
}

void CCorelWizard::nextKeyClicked()
{
  //  printf("next key\n");
  pressButton( eButtonNext );
}

void CCorelWizard::helpKeyClicked()
{
  //  printf("help key\n");
  pressButton( eButtonHelp );
}

void CCorelWizard::escKeyClicked()
{
  // only works if Cancel is enabled

  //  printf("esc key\n");
  if (m_nCurrentPageEnableAttribute & eButtonCancel)
    pressButton( eButtonCancel );
  
}

void CCorelWizard::retKeyClicked()
{
  //  printf("ret key\n");
  if (m_nDefaultButton != -1)
  {
    //special case added for last page
    //setDefaultButton function works on all pages except last page
    if (m_nCurrentPage == (getCount()-1))
    {
      // printf("last page cr pressed\n");
      pressButton(eButtonDone);
      return;
    }
    switch (m_nDefaultButton)
    {
    case eButtonNext:
    case eButtonPrev:
    case eButtonHelp:
	if (m_nCurrentPageEnableAttribute & m_nDefaultButton)
      pressButton( m_nDefaultButton );
      break;
    default:;
      //we must figure out what to do with cancel button
    }
  }
  else
  {
    //older implementation
    pressButton( eButtonDone );
  }
}

void CCorelWizard::keyPressEvent( QKeyEvent* event )
{
  //  printf("key press\n");
  //filter esc key
  if(event->key() != Key_Escape)
  {
    QDialog::keyPressEvent( event );
  }

}

void CCorelWizard::closeEvent( QCloseEvent* event )
{
  //  printf("close\n");

  //if cancel button disabled filter close event
  //this is when done button is NOT present
  //bug fix:close button was disabled on last page
  //now it is re-enabled close button on last page
  //	if ((((m_nCurrentPageEnableAttribute & eButtonDone) == 0) &&
  //		(m_nCurrentPageEnableAttribute & eButtonCancel)) ||
  //		(getCount() == 0))
  {
    if (checkLast(Rejected))event->accept();
  }
}

