/* Name: generaldlg.h
            
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

#ifndef __GENERALDLG_H_
#define __GENERALDLG_H_
#include <stdio.h>
#include <qmsgbox.h>
#include <qpixmap.h>
#include <qapp.h>
#include <qframe.h> 
#include <qbttngrp.h>
#include <qchkbox.h>
#include <qcombo.h>
#include <qframe.h>
#include <qgrpbox.h>
#include <qlabel.h>
#include <qlined.h>
#include <qlistbox.h>
#include <qpushbt.h>
#include <qradiobt.h>
#include <qscrbar.h>
#include <qtooltip.h>
#include <qspinbox.h>
//#include <kpwdbox.h>
#include <qstring.h>
#include <qfont.h>
#include "printerobject.h"

/**************************** General Base Dialog ***********************/

class KPrinterGeneralDlg : public QWidget
{
	Q_OBJECT

public:
	KPrinterGeneralDlg(bool bReadOnly,
										 QWidget *parent = 0L,
										 const char *name = 0L,
										 KPrinterObject *printer = 0L,
                     WFlags f = 0,
										 const QStrList* fontlist = 0L);

	// access methods
	bool readOnly() const
	{
		return m_bReadOnly;
	}
	
	// get|set the printer name
	
	void setName(const char *);
	const char * getName();
  QLineEdit * getNameField();

	// get|set the flag for that limits the size of printed files
	void setLimitPrintSize(bool);
	bool getLimitPrintSize();
	
	// get|set the max allowed size (if applicable) of a printed file
	void setSizeLimit(int);
	long getSizeLimit();

	// Invalid a field, highlight it, set focus on it
	void invalidEditField(QLineEdit *field);
  virtual void setGeneralDlg(KPrinterObject *fomAprinter)= 0;
	virtual bool setPrinterGeneralProperty(KPrinterObject *toPrinter)=0;
	virtual bool validate()=0;

protected:
	
	void setReadOnly(bool bReadOnly)
	{ 
		m_bReadOnly = bReadOnly;
	}
	
	void resizeEvent(QResizeEvent *e);

	QLabel *pName_lbl, *pKb_lbl;
	QLineEdit *pName_leb;
	QCheckBox *pLimit_btn;
	QSpinBox *pKb_spb;
  KPrinterObject *m_printer;

protected slots:
	
	// Disables pKb_spb is the pLimit_btn is disabled
	void slot_enableLimit(bool);

private:
	// attributes
	bool m_bReadOnly;
};

/*************************** General Local Dialog ***********************/

class KPrinterGeneralLocalDlg : public KPrinterGeneralDlg
{
	Q_OBJECT

public:
	KPrinterGeneralLocalDlg(bool bReadOnly,
                          QWidget *parent = 0L,
													const char *name = 0L,
													KPrinterObject *printer = 0L,
													bool modal = FALSE,
													const QStrList* fontlist = 0L);

	// get|set the printer device name
	void setDeviceName(const char *);
	const char * getDeviceName();
	QLineEdit * getDeviceNameField();
  virtual void setGeneralDlg(KPrinterObject *fromAprinter);
	virtual bool setPrinterGeneralProperty(KPrinterObject *toPrinter);
  virtual bool validate();

protected:
	
	void resizeEvent(QResizeEvent *e);

	QLabel *pDevice_lbl;
	QLineEdit *pDevice_leb;
};

/************************* General Network Dialog ***********************/

class KPrinterGeneralNetworkDlg : public KPrinterGeneralDlg
{
	Q_OBJECT

public:
	KPrinterGeneralNetworkDlg(bool bReadOnly,
														QWidget *parent = 0L,
														const char *name = 0L,
														KPrinterObject *printer = 0L,
														bool modal = FALSE,
														const QStrList* fontlist = 0L);
        
	// set|get the printer's hostname
  
	void setHostName(const char * name);
  const char * getHostName();
	QLineEdit * getHostNameField();
  virtual void setGeneralDlg(KPrinterObject *fromAprinter);
	virtual bool setPrinterGeneralProperty(KPrinterObject *toPrinter);
  virtual bool validate();
protected:
  void resizeEvent(QResizeEvent *e);
	QLabel *pHost_lbl;
  QLineEdit *pHost_leb;
};

/*************************** General Samba Dialog ***********************/

class KPrinterGeneralSambaDlg : public KPrinterGeneralNetworkDlg
{
	Q_OBJECT

public:
	KPrinterGeneralSambaDlg(bool bReadOnly,
													QWidget *parent = 0L,
													const char *name = 0L,
													KPrinterObject* printer = 0L ,
													bool modal = FALSE,
													const QStrList* fontlist = 0L);
	
	// set|get the network name of the printer
	
	void setPrinterName(const char *);
	const char *getPrinterName();
	QLineEdit *getPrinterNameField();
  virtual void setGeneralDlg(KPrinterObject *fromAprinter);
	virtual bool setPrinterGeneralProperty(KPrinterObject *toPrinter);
  virtual bool validate();

protected:
	
	void resizeEvent(QResizeEvent *e);

	QLabel *pPrinter_lbl;
	QLineEdit *pPrinter_leb;
};

/*************************** General Unix Dialog ***********************/

class KPrinterGeneralUnixDlg: public KPrinterGeneralNetworkDlg
{
	Q_OBJECT

public:
	KPrinterGeneralUnixDlg(bool bReadOnly,
												 QWidget *parent = 0L,
												 const char *name = 0L,
												 KPrinterObject *printer = 0L,
												 bool modal = FALSE,
												 const QStrList* fontlist = 0L);
	
	// set|get the printer's queue name
	
	void setQueueName(const char *);
	const char *getQueueName();
	QLineEdit *getQueueNameField();
  virtual void setGeneralDlg(KPrinterObject *fromAprinter);
	virtual bool setPrinterGeneralProperty(KPrinterObject *toPrinter);
  virtual bool validate();
protected:
	void resizeEvent(QResizeEvent *e);
	QLabel *pQueue_lbl;
  QLineEdit *pQueue_leb;
};

#endif

