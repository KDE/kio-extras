/* Name: pageTwo.h
            
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

// page two of Wizard

#ifndef __pageTwo_h__
#define __pageTwo_h__
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlabel.h>
#include "commonfunc.h"
#include "wizardpage.h"

class pageTwo : public CWizardPage
{
	Q_OBJECT

public:

	pageTwo(QWidget* parent = NULL, const char* name = NULL);
	
	pageTwo(int nPage, 
					bool bFinalPage = false, 
					int nNextPage = -1, 
					int nPrevPage = -1,
					const char* pszImage = 0,
					const char* pszTitle = 0,
					bool bCheckPage = true,
					QWidget *parent=0,
					const char *name=0);

	virtual ~pageTwo();
	
	QLineEdit* nickNameEdit()
	{
		return m_pPrinterName;
	}
  
	QLineEdit* hostNameEdit()
	{
		return m_pHostname;
	}

  virtual bool checkPage(int nButton);

public slots:
	
	void UnixButtonClicked();
	void WindowsButtonClicked();
	void BrowseButtonClicked();

protected slots:

	void pressedButton(int nButton, int nPage);

protected:

	QLabel* m_pmainMsg;

	QLabel* m_pLabelDeviceName;
	QComboBox* m_pDeviceName_PortList;

	QLabel* m_pNicknameLabel;
	QLineEdit* m_pPrinterName;
	
  QWidget* m_pParent;

	// super set of controls for handling printer options
  
	QLabel* m_pHostnameLabel;
  QLineEdit* m_pHostname;
  QPushButton* m_pBrowseButton;
  QLabel* m_pQueueLabel;
  QLineEdit* m_pQueue;
	
	// network type selection
	
	QButtonGroup* m_pNetworkTypeGroupBox;
  QRadioButton* m_pWindows;
  QRadioButton* m_pUnix;

public: // interface functions

	int GetPrinterPort_Device(char* name);

	int GetPrinterName(char* name);

  int GetNetworkType(); // 0-UNIX  1-WINDOWS

  int GetHostName(char* name);

	int GetQueueName(char *name);     //only for unix printer

  virtual void show();
};

#endif
		
