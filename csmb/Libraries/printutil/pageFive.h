/* Name: pageFive.h
            
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

#ifndef pageFive_include
#define pageFive_include

#include "wizardpage.h"
#include <stdio.h>
#include <string.h>
//#include <kmsgbox.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

class pageFive : public CWizardPage
{
    Q_OBJECT

public:

   
    pageFive(
        int nPage, bool bFinalPage = false, int nNextPage = -1, int nPrevPage = -1,
	    const char* pszImage = 0, const char* pszTitle = 0, bool bCheckPage = true,
	    QWidget *parent=0, const char *name=0 );

 //   virtual ~pageFive();

public slots:
    void shareButtonClicked();
		void noShareButtonClicked();

	  bool getShare()const {return isShare;};
protected:
    QButtonGroup *qtarch_group;
		QRadioButton *m_pShare, *m_pNoShare;
    QLabel* q_InfoLabel;
    QLabel* q_label_2;
    QLineEdit* q_Lineedit;
    bool isShare;
		
public:

};

#endif // pageThree_included
