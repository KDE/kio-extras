/* Name: pageFour.h
            
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

#ifndef pageFour_included
#define pageFour_included

#include "wizardpage.h"
#include <qlabel.h>
#include <qcombobox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <stdio.h>
#include <string.h>
//#include <kmsgbox.h>

class pageFour : public CWizardPage
{
    Q_OBJECT

public:

   
    pageFour(
        int nPage, bool bFinalPage = false, int nNextPage = -1, int nPrevPage = -1,
	    const char* pszImage = 0, const char* pszTitle = 0, bool bCheckPage = true,
	    QWidget *parent=0, const char *name=0 );

 //   virtual ~pageFour();

public slots:
    void yesButtonClicked();
		void noButtonClicked();
    QString getPaperSize()const;
	  bool getPrintTestPage()const {return printTestPage;};
protected:
    QButtonGroup *qtarch_group;
		QRadioButton *m_pYes, *m_pNo;
    QLabel* qtarch_Label_1;
    QLabel* qtarch_label_2;
    QComboBox* qtarch_ComboBox_1;
    bool printTestPage;
		QString paperSize;
public:

};

#endif // pageThree_included
