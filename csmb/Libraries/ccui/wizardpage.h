/*

   wizardpage.h

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


#ifndef WIZARDPAGE_H
#define WIZARDPAGE_H
#include <qwidget.h>

class CCorelWizard;

//wizard page
class CWizardPage : public QWidget
{
  Q_OBJECT
public:
  CWizardPage( QWidget *parent=0, const char *name=0, WFlags f=0 );

  CWizardPage( int nPage, bool bFinalPage = false, int nNextPage = -1,
	       int nPrevPage = -1, const char* pszImage = 0,
	       const char* pszTitle = 0, bool bCheckPage = true,
		QWidget *parent=0, const char *name=0 );

  //set current wizard page
  void setPage( int nPage, bool bFinalPage = false, int nNextPage = -1,
		int nPrevPage = -1, const char* pszImage = 0,
		const char* pszTitle = 0, bool bCheckPage = true );
	
  //virtual override to allow for wizards with mulitple "final" pages
  //if you return true for isFinalPage, wizard will disable "next" button
  //even if this is not last page on the wizard
  virtual bool isFinalPage() { return m_bFinalPage; }
	
  //if you want to set "custom" page number for "next" or "previous" page
  //return TRUE and set correct page number in pointer variable
  //return FALSE will implement the default next/previous scheme
  virtual bool getNextPage( int* pnNextPage )
    { *pnNextPage = m_nNextPage; return m_nNextPage == -1 ? false : true ; }
  virtual bool getPrevPage( int* pnPrevPage )
    { *pnPrevPage = m_nPrevPage; return m_nPrevPage == -1 ? false : true; }

  //if you have an image that you want to left of wizard
  //return pointer to pixmap file name
  virtual bool getImage(const char** ppImage)
  {
    *ppImage = m_pszImage;
    return m_pszImage == 0 ? false : (m_pszImage[0] == '\0' ? false : true);
  }

  //if you have a title that you want to upper side of wizard
  //return pointer to title - font size is set by wizard
  virtual bool getTitle( const char** ppTitle )
  {
    *ppTitle = m_pszTitle;
    return m_pszTitle == 0 ? false : (m_pszTitle[0] == '\0' ? false : true);
  }

  int getPageNumber() { return m_nPage; }

  //if you want to check contents of current page over-ride this function.
  //It used to be called with a bool telling you whether eButtonNext had been
  //clicked, but now it is called with nButton.

  virtual bool checkPage(int /*nButton*/) { return m_bCheckPage; }

  //prepare contents of current page before displaying
  virtual void preparePage() { }
  virtual void prepareButtons() {}

public slots:
  void pressedButton( int nButton, int nPage );

  //enable/grey-out a button display in wizard
  void enableButton( int nButton, bool bEnable );

protected:
  void paintEvent( QPaintEvent* event );
protected:
  bool m_bFinalPage;
  int m_nNextPage;
  int m_nPrevPage;
  QString m_pszImage;
  QString m_pszTitle;
  int m_nPage;   // which page the wizardpage was installed as
  CCorelWizard *m_pWizard;  // pointer to the wizard
  bool m_bCheckPage;
};

//prototypes for shared library wizards functions

typedef CWizardPage* (*PGetClassPtr)(int nPage, bool bFinalPage = false,
				     int nNextPage = -1, int nPrevPage = -1,
				     const char* pImage = 0,
				     const char* pTitle = 0,
				     bool bCheckPage = true,
				     QWidget *parent=0, const char *name=0 );

//create a structure with function pointers on these functions

typedef struct _SWIZARDPAGE
{
  void* pHandle;
  PGetClassPtr pGetClassPtr;
} SWIZARDPAGE, *PSWIZARDPAGE;

#define IDS_WIZARD_FILE		"config.ini"
#define IDS_WIZARD_INIT		"WizardInit"
#define IDS_WIZARD_COUNT	"PageCount"
#define IDS_WIZARD_PAGE		"WizardPage%d"
#define IDS_WIZARD_LIBRARY	"Library"
#define IDS_WIZARD_NEXT		"NextPage"
#define IDS_WIZARD_PREV		"PrevPage"
#define IDS_WIZARD_IMAGE	"Image"
#define IDS_WIZARD_TITLE	"Title"
#define IDS_WIZARD_ATTRIBUTE	"PageAttribute"
#define IDS_WIZARD_ACTIVE	"ActivePage"
#define IDS_WIZARD_FINAL	"FinalPage"
#define IDS_WIZARD_CHECK	"CheckPage"

#endif//WIZARDPAGE_H
