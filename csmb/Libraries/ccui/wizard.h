/*

   wizard.h

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

#ifndef WIZARD_H
#define WIZARD_H

#include "ccui_common.h"

#ifdef QT_2
#include <klocale.h>
#endif

#include <qdialog.h>
#include <qlist.h>

//default button strings
#define IDS_PREV1	i18n("<Back")
#define IDS_NEXT1	i18n("Next>")
#define IDS_HELP1	i18n("Help")
#define IDS_CANCEL1	i18n("Cancel")
#define IDS_DONE1	i18n("Finish")


#define	eButtonNext	1
#define	eButtonDone	2
#define eButtonCancel   4
#define eButtonHelp	8
#define	eButtonPrev	16


#define NUM_BUTTONS 5

//default size
#define CX		480
#define CY		360
//default position
#define PX		50
#define PY		50

//constants
#define DX	10
#define	DY	10
#define	HX	80//was 50
#define	HY	25
#define FX	10//not used
#define FY	10//35//10
#define LX	10//not used
#define LY	40
#define IX	10//not used
#define	IY	32

class QWidget;
class QLabel;
class QPushButton;
class QFrame;
class QAccel;
class CWizardPage;

class CCorelWizard : public QDialog
{
  Q_OBJECT
public:
  // nButton1 through 5 describe the order of the buttons in the wizard.
  // Use the constants eButtonHelp, eButtonCancel, etc. to initialize them.
  // bSpacing is true if, when buttons are hidden, there's an empty space
  // rather than a different button shown in its place.
  CCorelWizard(int nButton1=eButtonPrev, int nButton2=eButtonNext,
	       int nButton3=eButtonCancel, int nButton4=eButtonDone,
	       int nButton5=eButtonHelp, bool bSpacing=true,
	       QWidget *parent=0, const char *name=0, bool bModal=false);

  CCorelWizard(const char* szPrev, const char* szNext,
	       const char* szDone, const char* szHelp, const char* szCancel,
	       int nButton1=eButtonPrev, int nButton2=eButtonNext,
	       int nButton3=eButtonCancel, int nButton4=eButtonDone,
	       int nButton5=eButtonHelp, bool bSpacing=true,
	       QWidget *parent=0, const char *name=0, bool bModal=true);

  //default button
  void setDefaultButton( int nButton ) { m_nDefaultButton = nButton; }
  int getDefaultButton() { return m_nDefaultButton; }
	
  //set position of image on widget
  void setLeftImage( bool bLeftImage );
  bool getLeftImage() { return m_bLeftImage; }
	
  //set custom next,prev buttons position
  void setCustomPos( bool bCustomPos ) {m_bCustomPos = bCustomPos; }
  bool getCustomPos() { return m_bCustomPos; }
	
  //set widget position and size
  void setSize( int w, int h );	
  void setPos( int x, int y );
	
  //returns where to position wizard
  //it calculates position to middle of screen
  //based on screen display resolution
  int cx() { return m_ncx; }
  int cy() { return m_ncy; }
  int px() { return m_npx; }
  int py() { return m_npy; }

  //show widget - activates nPage		
  void show(int nPage=0);

  //returns buttons attributes for the page
  //one bit set for each button that is enabled per page
  int getPageEnableAttribute( int nPage);

  //returns buttons attributes for the page
  //one bit set for each button that is shown per page
  int getPageShowAttribute( int nPage);

  int getActivePageAttribute();

  //return page, index of page		
  QWidget* getPage( int nPage );
  int getIndex( QWidget* pWidget );

  //return active page, index of active page
  QWidget* getActivePage();
  int getActiveIndex();

  //return number of pages in wizard
  int getCount();

  //selects which active page to show and displays it
  bool setActivePage( int nPage );
  bool setActivePage( QWidget* pWidget );

  //add a page to wizard
  // Attribute buttons are initial bit sets for each button. There are two:
  // enable attributes and show attributes. For disable/enable rules while
  // on a wizard page, override the wizard page method prepareButtons() to
  // do some checking and button enabling.
  // Set last parameter to set the first page to be displayed when show() is
  // called
  int addPage(QWidget* pWidget, int nPageButtonEnableAttribute = eButtonPrev |
	      eButtonNext | eButtonCancel | eButtonHelp,
	      int nPageButtonShowAttribute = eButtonPrev | eButtonNext |
	      eButtonCancel | eButtonHelp, bool bActivePage = false );

  //remove a page from the wizard
  bool removePage(QWidget* pWidget);
  bool removePage(int nPage);

  // return how many pages are currently managed by the wizard
  int numPages() { return m_pageList.count(); }

  // returns true if hidden buttons leave a blank space in their place
  bool isSpaced() { return m_bSpacing; }

  // set to true if you want hidden buttons to leave a blank space in their
  // place; false to have buttons move over to take up the slack.
  void setSpacing(bool bSpacing) { m_bSpacing = bSpacing; }

  // Self-explanatory. Note: does not preclude use of Done buttons
  // on other pages.
  bool doneReplacesNextOnFinalPage() { return m_bDoneReplacesNextOnFinalPage; }
  void setDoneReplacesNextOnFinalPage(bool bVal)
    { m_bDoneReplacesNextOnFinalPage = bVal; }

public slots:	
  //simulate button press
  //check to see if corresponding button is enabled for specific page
  void pressButton( int nButton );

  //enable/grey-out a button display on current page
  void enableButton( int nButton, bool bEnable );

private slots:

  //button press callbacks
  void prevClicked();
  void nextClicked();
  void helpClicked();
  void doneClicked();
  void cancelClicked();

	
  //accelerator key callbacks
  void prevKeyClicked();
  void nextKeyClicked();
  void helpKeyClicked();
  void retKeyClicked();
  void escKeyClicked();

  //initialize widget parameters
  void initWidget( const char* szPrev, const char* szNext,
		   const char* szDone, const char* szHelp,
		   const char* szCancel, int nButton1, int nButton2,
		   int nButton3, int nButton4, int nButton5);
signals:
  //signal emitted when specified button is pressed
  //for pages derived from CWizardPage a connection is done to their callback
  void clickedButton( int nButton, int nPage );

  //signal emitted when wizard is terminating
  void exit();
protected:
  //displays active page in wizard
  //second parameter tells how to invoke it
  //i.e. from previous,next buttons or just a call to change page
  bool setActivePage( int nPage, int nButton );

  virtual void setActivePageEvent( int /* nPage */, int /* nButton */ );

  virtual bool checkLast( int /* r */ );
		
  void resizeEvent( QResizeEvent *event );

  void keyPressEvent( QKeyEvent* event );

  void closeEvent( QCloseEvent* event );
		
  void done( int /* r */ );
private:
  //wizard buttons
  QPushButton* m_pPrev;
  QPushButton* m_pNext;
  QPushButton* m_pCancel;
  QPushButton* m_pHelp;
  QPushButton* m_pDone;

  // pointers to the buttons for ease of group use.
  // The order of the buttons is determined by m_buttonOrderList.
  QPushButton *m_aButtons[NUM_BUTTONS];


  //frame displayed between user widget and buttons
  QFrame* m_pFrame;

  //image, title for wizard page - one per page	
  QLabel* m_pImage;
  QLabel* m_pTitle;

  //key accelerator
  //default keys ALT <- prev, ALT -> next, F1 help, esc - cancel, cr - done
  //for prev,next,alt another accelerator is used
  //based on the string for that particular button
  //i.e. for <"Prev" button ALT P is used
  QAccel* m_pAccel;

  // lists of data for the pages
  QList<QWidget> m_pageList;
  QList<int> m_pageButtonEnableAttributeList;
  QList<int> m_pageButtonShowAttributeList;

  //cancel, finish string
  QString m_strCancel;
  QString m_strDone;

  //current page index in list	
  int m_nCurrentPage;
  int m_nCurrentPageEnableAttribute;

  bool m_bButtonHidden;
  bool m_bImageHidden;
  bool m_bTitleHidden;

  //size of wizard initially
  int m_ncx;
  int m_ncy;

  //position where to display wizard initially
  int m_npx;
  int m_npy;

  //initial image size for each page - if no image is displayed
  //we use 1/3 of width for image "text" area
  int m_nix;
  int m_niy;

  //flag where to display image on left of wizard or right	
  bool m_bLeftImage;

  //flag to put custom position for buttons
  bool m_bCustomPos;

  //which button is default
  int m_nDefaultButton;

  QArray<int> m_buttonOrderList;    // order of buttons
  bool m_bSpacing;     // true if should leave a space when a button is hidden

  // true if the Next button becomes the Done button on the final page(s).
  // initialized to false. Set with setDoneReplacesNextOnFinalPage(bool)
  bool m_bDoneReplacesNextOnFinalPage;


};

#endif // WIZARD_H
