/* Name: pageThree.h
            
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

#ifndef pageThree_included
#define pageThree_included

#include "commonfunc.h"

#include <wizardpage.h>
#include <wizard.h>
#include <qpushbutton.h>
#include <stdio.h>
#include <string.h>
#include <qcombobox.h>
#include "manufacture.h"

class pageThree : public CWizardPage
{
    Q_OBJECT

public:

   
    pageThree( 
        int nPage, bool bFinalPage = false, int nNextPage = -1, int nPrevPage = -1,
	    const char* pszImage = 0, const char* pszTitle = 0, bool bCheckPage = true,
	    QWidget *parent=0, const char *name=0 );

    virtual ~pageThree();

public slots:
    void updateList(const char* string);
    void diskButtonClicked();

protected slots:


protected:

	QLabel* m_pDescriptionLabel;
	QLabel* m_pManufacturerLabel;
	QLabel* m_pModelLabel;
	QComboBox* m_pManufacturer;
	QComboBox* m_pModel;
	QPushButton *m_DiskButton;

public:
  
	virtual void show();

	// interface functions
	
	int getManufacture(char* name);
	
	int getPrinterModel(char* name);
	
	int getMagicFilter(char* name);
	void resetCount (){;};
	
	int setNumberOfPrinterObjects(int count)
	{
		return 0;
	};
	
	int setPrinterObject(const char* item, const char* itemKey)
	{
		return 0;
	};
		
	void init_comboboxes();
	int indexOfItem(const char *item, QComboBox *list);
};

#endif // pageThree_included
