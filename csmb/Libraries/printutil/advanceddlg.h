 /* Name: advancedlg.h
            
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

#ifndef __ADVANCEDDLG_H__
#define __ADVANCEDDLG_H__

#include <qwidget.h>
#include <qpushbt.h>
#include <qchkbox.h>
#include <qlistbox.h>
#include <qlined.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qmsgbox.h>
#include "printerobject.h"

class KPrinterAdvancedDlg : public QWidget //QDialog
{
	Q_OBJECT
public:
	KPrinterAdvancedDlg( bool bReadOnly, QWidget *parent,const char*  name,
												KPrinterObject* printer);
 	~KPrinterAdvancedDlg();
	// access methods
	bool readOnly() const		{ return m_bReadOnly; }

	//Get/Set methods for each gui component
	bool getSendEOF();
	void setSendEOF(bool flag);

	bool getFixText();
	void setFixText(bool flag);

	bool getFastPrint();
	void setFastPrint(bool flag);

	const char* getGhostOptions();
	void setGhostOptions(const char* options);
	void setAdvancedDlg(KPrinterObject * fromPrinter);
	bool setPrinterAdvancedProperty(KPrinterObject *toPrinter);
protected:
	void resizeEvent(QResizeEvent *e);

	void setReadOnly( bool bReadOnly )	{ m_bReadOnly = bReadOnly; }

	// holders for each gui component
	QCheckBox *pEOF_btn, *pFix_btn, *pFast_btn;
	QLabel *pGhost_lbl;
	QLineEdit *pGhost_leb;

private:
// attributes
	bool	m_bReadOnly;
	KPrinterObject *m_printer;	
};

#endif

