/* Name: pageOne.h
            
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

// page one of Wizard
#ifndef __pageOne_h__
#define __pageOne_h__
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <wizardpage.h>
class pageOne : public CWizardPage
{

	Q_OBJECT
public: // construction and destruction
	
	pageOne(QWidget* parent = NULL, const char* name = NULL);
	pageOne( int nPage, bool bFinalPage = false, int nNextPage = -1, int nPrevPage = -1,
		const char* pszImage = 0, const char* pszTitle = 0, bool bCheckPage = true,
		QWidget *parent=0, const char *name=0 );


	virtual ~pageOne();
	
public slots:

void pressedButton( int nButton, int nPage );

protected :

	QButtonGroup* m_pGroup;
	QLabel* m_pmainMsg;
	QRadioButton* m_pnetPrn;
	QRadioButton* m_plocalPrn;

public: // interface functions
	int GetTypeOfPrinter(); // 0 - local, 1 - network

};



#endif
