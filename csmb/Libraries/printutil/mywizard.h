/* Name: mywizard.h
            
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

#ifndef PPRINTERWIZARD_H
#define PPRINTERWIZARD_H

#include <qdialog.h>
#include <qlist.h>
#include "constText.h"
#include <wizard.h>

class CPrinterWizard : public CCorelWizard //alexandrm
{
	Q_OBJECT
public:

//  bSpacing = false, make sure there not space for hided key
// bModel = true, because QDialog::done(r) only work for model
// dialog and useless for modelless dialog.
  CPrinterWizard( int nButton1=eButtonPrev, int nButton2=eButtonNext,
	     int nButton3=eButtonCancel, int nButton4=eButtonDone,
	     int nButton5=eButtonHelp, bool bSpacing= false,
 	     QWidget *parent=0, const char *name=0,bool bModal=true  )
    : CCorelWizard(nButton1, nButton2, nButton3, nButton4, nButton5, bSpacing, //alexandrm
		   parent, name, bModal)
		{
			//m_pTitle->setBackgroundColor(gray);
 		};

  CPrinterWizard( const char* szPrev, const char* szNext,
		const char* szDone, const char* szHelp, const char* szCancel,
		int nButton1=eButtonPrev, int nButton2=eButtonNext,
		int nButton3=eButtonCancel, int nButton4=eButtonDone,
		int nButton5=eButtonHelp, bool bSpacing= false,
		QWidget *parent=0, const char *name=0, bool bModal=true)
    : CCorelWizard(szPrev, szNext, szDone, szHelp, szCancel, nButton1,
		   nButton2, nButton3, nButton4, nButton5, bSpacing, parent,
		   name, bModal)  //alexandrm
		{
     // m_pTitle->setBackgroundColor(gray);
		};


public slots:

//	void prevClicked();
//	void nextClicked();
//  void helpClicked();
//  void doneClicked();

protected:
	void done(int r);

public: // new interface for new database

	// Interface for reteriving information
	QString getNickName();
	QString getDeviceName();
	int	getPrinterType(); // type of printer 0-local, 1-Unix, 2-Windows
//	QString findTitle();
	QString getHostName(); // for Network Printer(Unix/Windows)
	QString getPrinterName(); // for Windows only
	QString getQueueName(); // for unix only
	QString getPrinterTypeKey(); // return data from printerObject.magicFilter
	QString getCommand(); //return command for printing testpage.
  bool getPrintTestPage();
	QString getPaperSize();
	// interface for setting the information
	void setNumberOfPrinterObjects(int);
	int setPrinterObject(const char* item, const char* itemKey);

	// interface to update printer list
	void 	updatePrinterCombos();
	bool  varifyNickName(const QString& strName);
	QString getManufacture();
	QString getModel();
	QString getLocation();
};


#endif // PPRINTERWIZARD_H
