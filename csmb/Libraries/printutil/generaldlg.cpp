/* Name: generaldlg.cpp
            
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

//
//	Jun 19, 2000 by olegn: reformatted according to Corel Coding Standard

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "stdio.h"
#include "stdlib.h"
#include "qfile.h"
#include <qstrlist.h>
#include <qfile.h>
#include <qtstream.h>
#include <qfileinfo.h>
#include <qapp.h>
#include <X11/Xlib.h>
//#include <kmsgbox.h>
#include <klocale.h>
#include <kapp.h>
#include "constText.h"
#include "generaldlg.h"
#include "commonfunc.h"

/************************* General Base Dialog *************************/

KPrinterGeneralDlg::KPrinterGeneralDlg(
	bool bReadOnly,
	QWidget *parent,
	const char *name,
	KPrinterObject *printer,
  WFlags f,
	const QStrList* fontlist) :
	QWidget(parent, name, f)
{
	setReadOnly(bReadOnly);

	// Create the label/combo combinations

	pName_leb = new QLineEdit(this, "name");
	pName_lbl = new QLabel(pName_leb, NAME, this);

	// Create the file limit widgets

	pLimit_btn = new QCheckBox(this, "limit");
	pLimit_btn->setText(LIMIT);
	pKb_spb = new QSpinBox(1, 99999, 1, this, "kb");
	pKb_spb->setEnabled(FALSE);
	pKb_lbl = new QLabel("kB", this);

	// Setup the default connections

	connect(pLimit_btn, SIGNAL(toggled(bool)), this, SLOT(slot_enableLimit(bool)));

	pName_leb->setEnabled(!readOnly());
	pLimit_btn->setEnabled(!readOnly());

	setTabOrder(pLimit_btn, pKb_lbl);
	setTabOrder(pLimit_btn, pKb_spb);

	setMinimumSize(400, 320);
	m_printer = printer;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralDlg::setName(const char *name)
{
	pName_leb->setText(name);
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterGeneralDlg::getName()
{
	return (const char *)pName_leb->text()
#ifdef QT_20
  .latin1()
#endif
                          ;
}

////////////////////////////////////////////////////////////////////////////

QLineEdit * KPrinterGeneralDlg::getNameField()
{
	return pName_leb;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralDlg::setLimitPrintSize(bool flag)
{
	pLimit_btn->setChecked(flag);
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralDlg::getLimitPrintSize()
{
	return pLimit_btn->isChecked();
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralDlg::setSizeLimit(int value)
{
	pKb_spb->setValue(value);
}

////////////////////////////////////////////////////////////////////////////

long KPrinterGeneralDlg::getSizeLimit()
{
	return atol((const char *)pKb_spb->text()
#ifdef QT_20
  .latin1()
#endif
  );
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralDlg::slot_enableLimit(bool flag)
{
	pKb_spb->setEnabled((!readOnly()) && flag);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralDlg::invalidEditField(QLineEdit *field)
{
	field->selectAll();
	field->setFocus();
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralDlg::resizeEvent(QResizeEvent *e)
{
}

/************************* General Local Dialog *************************/

KPrinterGeneralLocalDlg::KPrinterGeneralLocalDlg(
	bool bReadOnly,
	QWidget *parent,
	const char *name,
	KPrinterObject *printer,
	bool modal,
	const QStrList* fontlist) :
	KPrinterGeneralDlg(bReadOnly, parent, name,printer)//, modal,fontlist)
{
	// Create the label/combo combinations
	pDevice_leb = new QLineEdit(this, "device");
	pDevice_lbl = new QLabel(pDevice_leb, DEVICES, this);

	pDevice_leb->setEnabled(!readOnly());

	setTabOrder(pName_leb, pDevice_leb);
	setTabOrder(pDevice_leb, pLimit_btn);
	setGeneralDlg(m_printer);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralLocalDlg::setDeviceName(const char *name)
{
	pDevice_leb->setText(name);
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterGeneralLocalDlg::getDeviceName()
{
	return (const char *)pDevice_leb->text()
#ifdef QT_20
  .latin1()
#endif
                            ;
}

////////////////////////////////////////////////////////////////////////////

QLineEdit *KPrinterGeneralLocalDlg::getDeviceNameField()
{
	return pDevice_leb;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralLocalDlg::resizeEvent(QResizeEvent *e)
{
	QSize label2 = pKb_lbl->sizeHint();
	QSize button = pLimit_btn->sizeHint();
	QSize spin = pKb_spb->sizeHint();

	int nOffset = 14;
	int nVertSpacing = 45;

	// Place the label/combobox combinations

	int nLabelWidth = pName_lbl->sizeHint().width();

	if (nLabelWidth < pDevice_lbl->sizeHint().width())
		nLabelWidth = pDevice_lbl->sizeHint().width();

	int nEditWidth = width() - nLabelWidth - 28 - 5;

	if (nEditWidth > 250)
		nEditWidth = 250;

	pName_lbl->setGeometry(nOffset, nOffset, nLabelWidth, 25);
	pName_leb->setGeometry(nOffset + nLabelWidth + 5, nOffset, nEditWidth, 25);

	pDevice_lbl->setGeometry(nOffset, nOffset + nVertSpacing, nLabelWidth, 25);
	pDevice_leb->setGeometry(nOffset + nLabelWidth + 5, nOffset + nVertSpacing, nEditWidth, 25);

	// Place the file cap line widgets
	pLimit_btn->setGeometry(nOffset, nOffset + nVertSpacing*2, button.width(), button.height());
	pKb_spb->setGeometry(nOffset + button.width() + 5, nOffset + nVertSpacing*2, spin.width(), spin.height());
	pKb_lbl->setGeometry(pKb_spb->x() + pKb_spb->width() + 5, nOffset + nVertSpacing*2, label2.width(), spin.height());
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralLocalDlg::setGeneralDlg(KPrinterObject *fromAprinter)
{
	setName((const char*)fromAprinter->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
                                                      );
	setLimitPrintSize(fromAprinter->getFileLimit() > 0);
	setSizeLimit(fromAprinter->getFileLimit());

	// local specific options
	setDeviceName((const char*)fromAprinter->getLocation()
#ifdef QT_20
  .latin1()
#endif
                                                         );
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralLocalDlg::setPrinterGeneralProperty(KPrinterObject *aPrinter)
{
  Aps_PrinterHandle printerHandle;
  Aps_Result result;
  QString loc;
	QString OldPrinterName = aPrinter->getPrinterName();

	if (APS_SUCCESS == (result = Aps_OpenPrinter((const char*)OldPrinterName
#ifdef QT_20
  .latin1()
#endif
                          , &printerHandle)))
	{
		QString NewPrinterName = getName();

		if (OldPrinterName != NewPrinterName)
		{
			aPrinter->setPrinterName(NewPrinterName);

			//reset name

			if (APS_SUCCESS != (result = Aps_PrinterRename(printerHandle, (const char*)NewPrinterName
#ifdef QT_20
  .latin1()
#endif
      )))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr, "\n Err, Aps_PrinterRename() = %d",result);
					return false;
				}
			}
		}

		// Set file size (limit)

		long NewLimit = getLimitPrintSize() ? getSizeLimit() : 0;

		if (NewLimit != aPrinter->getFileLimit())
		{
      aPrinter->setFileLimit(NewLimit);

			if (APS_SUCCESS != (result = Aps_PrinterSetMaxJobSize(printerHandle, aPrinter->getFileLimit()*1000)))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr,"\nErr, Aps_PrinterSetMaxJobSize()=%d",result);
					return false;
				}
			}
		}

		// Location (connection info)

		if (aPrinter->getLocation() != getDeviceName())
		{
      aPrinter->setLocation(getDeviceName());

			if (APS_SUCCESS != (result = Aps_PrinterSetConnectInfo(printerHandle, APS_CONNECT_LOCAL, (char*)getDeviceName())))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr,"\nErr,Aps_PrinterSetConnectInfo()=%d",result);
					return false;
				}
			}
		}

		Aps_ReleaseHandle(printerHandle);

		return true;
	}
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralLocalDlg::validate()
{
	static QString fieldValue;
	static QString vError(i18n("Validation Error"));
	static QString tmpError;
	static QFileInfo fiTestInfo;

  // Validate the fields

  // Ensure that the user supplied something to each field
	fieldValue = getName();

	if (!validFileName(fieldValue))
	{
#ifndef QT_20
		KMsgBox::message(this, vError.data(), WARNING1, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
		invalidEditField(getNameField());
    return false;
	}
	else
	{
		if (!uniqueNewName((const char*)fieldValue
#ifdef QT_20
  .latin1()
#endif
    , (const char*)m_printer->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
                                                                              ))
		{
#ifndef QT_20
			KMsgBox::message(this, vError.data(), WARNING2, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
			invalidEditField(getNameField());
			return false;
		}
		else
		{
			if (fieldValue == DEFAULT_PRINTER_SYMBOL)
			{
#ifndef QT_20
				KMsgBox::message(this, vError.data(), WARNING3, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
				invalidEditField(getNameField());
				return false;
			}
		}
	}

	fieldValue = getDeviceName();
	fiTestInfo.setFile(fieldValue);

	if (!fiTestInfo.exists() || fiTestInfo.isDir() || fiTestInfo.isFile())
	{
#ifndef QT_20
		KMsgBox::message(this, vError.data(), WARNING4, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
		invalidEditField(getNameField());
		return false;
	}

	return true;
}

/*********************** General Network Dialog *************************/

KPrinterGeneralNetworkDlg::KPrinterGeneralNetworkDlg(
	bool bReadOnly,
	QWidget *parent,const char *name,
	KPrinterObject *printer,
	bool modal,
	const QStrList* fontlist) :
  KPrinterGeneralDlg(bReadOnly, parent, name, printer)//,modal, fontlist)
{
	// Create the label/combo combinations
	pHost_leb = new QLineEdit(this, "hostname");
	pHost_lbl = new QLabel(pHost_leb, HOSTNAME1, this);

	pHost_leb->setEnabled(!readOnly());

	setTabOrder(pName_leb, pHost_leb);
	setTabOrder(pHost_leb, pLimit_btn);
	setGeneralDlg(m_printer);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralNetworkDlg::setHostName(const char *name)
{
	pHost_leb->setText(name);
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterGeneralNetworkDlg::getHostName()
{
	return (const char *)pHost_leb->text()
#ifdef QT_20
  .latin1()
#endif
                              ;
}

////////////////////////////////////////////////////////////////////////////

QLineEdit *KPrinterGeneralNetworkDlg::getHostNameField()
{
	return pHost_leb;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralNetworkDlg::resizeEvent(QResizeEvent *e)
{
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralNetworkDlg::setGeneralDlg(KPrinterObject *fromAprinter)
{
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralNetworkDlg::setPrinterGeneralProperty(KPrinterObject *toPrinter)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralNetworkDlg::validate()
{
	return false;
}

/************************* General Unix Dialog *************************/

KPrinterGeneralUnixDlg::KPrinterGeneralUnixDlg(
	bool bReadOnly,
  QWidget *parent,
	const char *name,
	KPrinterObject* printer,
	bool modal,
	const QStrList* fontlist) :
	KPrinterGeneralNetworkDlg(bReadOnly, parent,name,printer)//,modal,fontlist)
{
	pQueue_lbl = new QLabel(this, QUEUE, this);
  pQueue_leb = new QLineEdit(this, "queue");
	pQueue_lbl->setBuddy(pQueue_leb);

	pQueue_leb->setEnabled(!readOnly());

	setTabOrder(pHost_leb, pQueue_leb);
	setTabOrder(pQueue_leb, pLimit_btn);
	setGeneralDlg(m_printer);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralUnixDlg::setQueueName(const char * name)
{
	pQueue_leb->setText(name);
}

////////////////////////////////////////////////////////////////////////////

const char * KPrinterGeneralUnixDlg::getQueueName()
{
	return (const char *)pQueue_leb->text()
#ifdef QT_20
  .latin1()
#endif
                              ;
}

////////////////////////////////////////////////////////////////////////////

QLineEdit * KPrinterGeneralUnixDlg::getQueueNameField()
{
	return pQueue_leb;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralUnixDlg::resizeEvent(QResizeEvent *e)
{
  QSize label2 = pKb_lbl->sizeHint();
  QSize button = pLimit_btn->sizeHint();
  QSize spin = pKb_spb->sizeHint();
	int nOffset = 14;
	int nVertSpacing = 45;

	// Place the label/combobox combinations

	int nLabelWidth = pName_lbl->sizeHint().width();

	if (nLabelWidth < pHost_lbl->sizeHint().width())
		nLabelWidth = pHost_lbl->sizeHint().width();

	if (nLabelWidth < pQueue_lbl->sizeHint().width())
		nLabelWidth = pQueue_lbl->sizeHint().width();

	int nEditWidth = width() - nLabelWidth - 28 - 5;

	if (nEditWidth > 250)
		nEditWidth = 250;

	pName_lbl->setGeometry(nOffset, nOffset, nLabelWidth, 25);
	pName_leb->setGeometry(nOffset + nLabelWidth + 5, nOffset, nEditWidth, 25);

	pHost_lbl->setGeometry(nOffset, nOffset + nVertSpacing, nLabelWidth, 25);
	pHost_leb->setGeometry(nOffset + nLabelWidth + 5, nOffset + nVertSpacing, nEditWidth, 25);

  pQueue_lbl->setGeometry(nOffset, nOffset + nVertSpacing * 2, nLabelWidth, 25);
  pQueue_leb->setGeometry(nOffset + nLabelWidth + 5, nOffset + nVertSpacing * 2, nEditWidth, 25);

	// Place the file cap line widgets
	pLimit_btn->setGeometry(nOffset, nOffset + nVertSpacing*3, button.width(), button.height());
	pKb_spb->setGeometry(nOffset + button.width() + 5, nOffset + nVertSpacing*3, spin.width(), spin.height());
	pKb_lbl->setGeometry(pKb_spb->x() + pKb_spb->width() + 5, nOffset + nVertSpacing*3, label2.width(), spin.height());
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralUnixDlg::setGeneralDlg(KPrinterObject *fromAprinter)
{
	QString hostName;
	QString strHostName;
	QString remotePrinter;
	setName((const char *)fromAprinter->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
  );
	setLimitPrintSize(fromAprinter->getFileLimit() > 0);
	setSizeLimit(fromAprinter->getFileLimit());
	strHostName = fromAprinter->getLocation().copy();

	if (strHostName[0] == '/')
  	 strHostName.remove(0, 1);

	if (strHostName[0] == '/')
  	 strHostName.remove(0, 1);

	hostName = strHostName;

	uint i;

	for (i = 0; i<strHostName.length(); i++)
	{
		if (strHostName.at(i) == '/')
		{
			hostName.truncate(i);
			break;
		}
	}

	remotePrinter = strHostName.remove(0, i+1);
  setHostName((const char *)hostName
#ifdef QT_20
  .latin1()
#endif
  );
  setQueueName((const char *)remotePrinter
#ifdef QT_20
  .latin1()
#endif
  );
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralUnixDlg::setPrinterGeneralProperty(KPrinterObject *aPrinter)
{
	Aps_PrinterHandle printerHandle;
  Aps_Result result;
  QString loc;
	QString OldPrinterName = aPrinter->getPrinterName();

	if (APS_SUCCESS == (result = Aps_OpenPrinter((const char *)OldPrinterName
#ifdef QT_20
  .latin1()
#endif

  , &printerHandle)))
	{
		QString NewPrinterName = getName();

		if (OldPrinterName != NewPrinterName)
		{
			aPrinter->setPrinterName(NewPrinterName);

			//reset name

			if (APS_SUCCESS != (result = Aps_PrinterRename(printerHandle, (const char*)NewPrinterName
#ifdef QT_20
  .latin1()
#endif
                                                                                                )))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr, "\n Err, Aps_PrinterRename() = %d",result);
					return false;
				}
			}
		}

		// Set file size (limit)

		long NewLimit = getLimitPrintSize() ? getSizeLimit() : 0;

		if (NewLimit != aPrinter->getFileLimit())
		{
      aPrinter->setFileLimit(NewLimit);

			if (APS_SUCCESS != (result = Aps_PrinterSetMaxJobSize(printerHandle, aPrinter->getFileLimit()*1000)))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr,"\nErr, Aps_PrinterSetMaxJobSize()=%d",result);
					return false;
				}
			}
		}

		// Unix specific options

		QString NewLocation(QByteArray(1024));
		QString NewQueueName(QByteArray(1024));

		NewLocation.sprintf("//%s/%s", getHostName(), getQueueName());
		NewQueueName.sprintf("%s", getQueueName());

		if (NewLocation != aPrinter->getLocation() ||
				NewQueueName != aPrinter->getQueue())
		{
      aPrinter->setLocation(NewLocation);
			aPrinter->setQueue(NewQueueName);

			//set connection

			if (APS_SUCCESS != (result = Aps_PrinterSetConnectInfo(printerHandle, APS_CONNECT_NETWORK_LPD, (const char*)NewLocation
#ifdef QT_20
  .latin1()
#endif
                                                                                                                                )))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr,"\nErr,Aps_PrinterSetConnectInfo()=%d",result);
					return false;
				}
			}
		}

		Aps_ReleaseHandle(printerHandle);
		return true;
	}
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralUnixDlg::validate()
{
	static QString fieldValue;
	static QString vError(i18n("Validation Error"));
	static QString tmpError;
	static QFileInfo fiTestInfo;

	fieldValue = getName();

	if (!strlen(fieldValue
#ifdef QT_20
  .latin1()
#endif
  ))
	{
#ifndef QT_20
		KMsgBox::message(this, vError.data(), WARNING1, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif

		invalidEditField(getNameField());
		return false;
	}

	if (!validFileName(fieldValue))
	{
#ifndef QT_20
		KMsgBox::message(this, vError.data(), WARNING1, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
		invalidEditField(getNameField());
		return false;
	}
	else
	{
		if (!uniqueNewName((const char *)fieldValue
#ifdef QT_20
  .latin1()
#endif
    , (const char*)m_printer->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
                                                  ))
		{
#ifndef QT_20
			KMsgBox::message(this, vError.data(), WARNING2, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
			invalidEditField(getNameField());
			return false;
		}
		else
		{
			if (fieldValue == DEFAULT_PRINTER_SYMBOL)
			{
#ifndef QT_20
				KMsgBox::message(this, vError.data(), WARNING3, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif

				invalidEditField(getNameField());
				return false;
      }
		}
	}

	fieldValue = getHostName();

	if (fieldValue.isEmpty())
	{
#ifndef QT_20
		KMsgBox::message(this, vError.data(), WARNING5, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
		invalidEditField(getHostNameField());
		return false;
	}

	fieldValue = getQueueName();

	if (fieldValue.isEmpty())
	{
#ifndef QT_20
		KMsgBox::message(this, vError.data(), WARNING_Q1,KMsgBox::EXCLAMATION,OK_BUTTON);
//commented by alexandrm
#endif

		invalidEditField(getQueueNameField());
		return false;
	}

	if (!validFileName(fieldValue))
	{
#ifndef QT_20
		KMsgBox::message(this, vError.data(), WARNING_Q2, KMsgBox::EXCLAMATION,OK_BUTTON);
//commented by alexandrm
#endif
		invalidEditField(getQueueNameField());
		return false;
	}

	return true;
}

/************************* General Samba Dialog *************************/

KPrinterGeneralSambaDlg::KPrinterGeneralSambaDlg
	(bool bReadOnly,
   QWidget *parent,
   const char *name,
   KPrinterObject* printer,
   bool modal,
	 const QStrList* fontlist) :
	KPrinterGeneralNetworkDlg(bReadOnly, parent, name, printer) //,modal,fontlist)
{
	// Create the label/combo combinations
	pPrinter_leb = new QLineEdit(this, "printer");
	pPrinter_lbl = new QLabel(pPrinter_leb, PRINTER1, this);

	pPrinter_leb->setEnabled(!readOnly());

	setTabOrder(pName_leb, pHost_leb);
	setTabOrder(pHost_leb, pPrinter_leb);
	setTabOrder(pPrinter_leb, pLimit_btn);
	setGeneralDlg(m_printer);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralSambaDlg::setPrinterName(const char * name)
{
  pPrinter_leb->setText(name);
}

////////////////////////////////////////////////////////////////////////////

const char * KPrinterGeneralSambaDlg::getPrinterName()
{
  return pPrinter_leb->text()
#ifdef QT_20
  .latin1()
#endif
                              ;
}

////////////////////////////////////////////////////////////////////////////

QLineEdit * KPrinterGeneralSambaDlg::getPrinterNameField()
{
  return pPrinter_leb;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralSambaDlg::resizeEvent(QResizeEvent *e)
{
  QSize label2 = pKb_lbl->sizeHint();
  QSize button = pLimit_btn->sizeHint();
  QSize spin = pKb_spb->sizeHint();
	int nOffset = 14;
	int nVertSpacing = 45;

	// Place the label/combobox combinations

	int nLabelWidth = pName_lbl->sizeHint().width();

	if (nLabelWidth < pHost_lbl->sizeHint().width())
		nLabelWidth = pHost_lbl->sizeHint().width();

	if (nLabelWidth < pPrinter_lbl->sizeHint().width())
		nLabelWidth = pPrinter_lbl->sizeHint().width();

	int nEditWidth = width() - nLabelWidth - 28 - 5;

	if (nEditWidth > 250)
		nEditWidth = 250;

	pName_lbl->setGeometry(nOffset, nOffset, nLabelWidth, 25);
	pName_leb->setGeometry(nOffset + nLabelWidth + 5, nOffset, nEditWidth, 25);

	pHost_lbl->setGeometry(nOffset, nOffset + nVertSpacing, nLabelWidth, 25);
	pHost_leb->setGeometry(nOffset + nLabelWidth + 5, nOffset + nVertSpacing, nEditWidth, 25);

  pPrinter_lbl->setGeometry(nOffset, nOffset + nVertSpacing * 2, nLabelWidth, 25);
  pPrinter_leb->setGeometry(nOffset + nLabelWidth + 5, nOffset + nVertSpacing * 2, nEditWidth, 25);

	// Place the file cap line widgets
	pLimit_btn->setGeometry(nOffset, nOffset + nVertSpacing*3, button.width(), button.height());
	pKb_spb->setGeometry(nOffset + button.width() + 5, nOffset + nVertSpacing*3, spin.width(), spin.height());
	pKb_lbl->setGeometry(pKb_spb->x() + pKb_spb->width() + 5, nOffset + nVertSpacing*3, label2.width(), spin.height());
}

////////////////////////////////////////////////////////////////////////////

void KPrinterGeneralSambaDlg::setGeneralDlg(KPrinterObject *fromAprinter)
{
  uint i;
	QString hostName;
	QString remotePrinter;
	QString strHostName;
  setName((const char *)fromAprinter->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
  );
  setLimitPrintSize(fromAprinter->getFileLimit() > 0);
  setSizeLimit(fromAprinter->getFileLimit());

  // Samba specific options
  strHostName = fromAprinter->getLocation().copy();

  if (strHostName[0] == '/')
  	  strHostName.remove(0, 1);

  if (strHostName[0] == '/')
  	  strHostName.remove(0, 1);

  hostName = strHostName;

  for (i = 0; i < strHostName.length(); i++)
	{
		if (strHostName.at(i) == '/')
		{
			hostName.truncate(i);
			break;
		}
	}

  remotePrinter = strHostName.remove(0, i+1);
  setHostName((const char *)hostName
#ifdef QT_20
  .latin1()
#endif
                                    );
  setPrinterName((const char *)remotePrinter
#ifdef QT_20
  .latin1()
#endif
                              );
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralSambaDlg::setPrinterGeneralProperty(KPrinterObject *aPrinter)
{
	Aps_PrinterHandle printerHandle;
  Aps_Result result;
	QString OldPrinterName = aPrinter->getPrinterName();

  if (APS_SUCCESS == (result = Aps_OpenPrinter((const char *)OldPrinterName
#ifdef QT_20
  .latin1()
#endif
  , &printerHandle)))
	{
		QString NewPrinterName = getName();

		if (OldPrinterName != NewPrinterName)
		{
			aPrinter->setPrinterName(NewPrinterName);

			//reset name

			if (APS_SUCCESS != (result = Aps_PrinterRename(printerHandle, (const char*)NewPrinterName
#ifdef QT_20
  .latin1()
#endif
      )))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr, "\n Err, Aps_PrinterRename() = %d",result);
					return false;
				}
			}
		}

		// Set file size (limit)

		long NewLimit = getLimitPrintSize() ? getSizeLimit() : 0;

		if (NewLimit != aPrinter->getFileLimit())
		{
      aPrinter->setFileLimit(NewLimit);

			if (APS_SUCCESS != (result = Aps_PrinterSetMaxJobSize(printerHandle, aPrinter->getFileLimit()*1000)))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr,"\nErr, Aps_PrinterSetMaxJobSize()=%d",result);
					return false;
				}
			}
		}

    // Samba specific options
    QString NewLocation;
    NewLocation.sprintf("//%s/%s", getHostName(), getPrinterName());

		if (NewLocation != aPrinter->getLocation())
		{
			aPrinter->setLocation(NewLocation);

			if (APS_SUCCESS != (result = Aps_PrinterSetConnectInfo(printerHandle, APS_CONNECT_NETWORK_SMB, (const char*)NewLocation
#ifdef QT_20
  .latin1()
#endif
      )))
			{
				if (result != APS_NOT_FOUND && result != APS_NO_CHANGE)
				{
					//fprintf(stderr,"\nErr,Aps_PrinterSetConnectInfo()=%d",result);
					return false;
				}
			}
		}

		Aps_ReleaseHandle(printerHandle);
		return true;
	}
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterGeneralSambaDlg::validate()
{
  static QString fieldValue;
  static QString vError(i18n("Validation Error"));
  static QString tmpError;
  static QFileInfo fiTestInfo;

  fieldValue = getName();

  if (!validFileName(fieldValue))
  {
#ifndef QT_20
    KMsgBox::message(this, vError.data(), WARNING1, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
    invalidEditField(getNameField());
    return false;
  }
  else
  {
    if (!uniqueNewName((const char *)fieldValue
#ifdef QT_20
  .latin1()
#endif
    , (const char*)m_printer->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
    ))
    {
#ifndef QT_20
      KMsgBox::message(this, vError.data(), WARNING2, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif

      invalidEditField(getNameField());
      return false;
    }
    else
    {
      if (fieldValue == DEFAULT_PRINTER_SYMBOL)
      {
#ifndef QT_20
        KMsgBox::message(this, vError.data(),WARNING3, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif

        invalidEditField(getNameField());
        return false;
      }
    }
  }

  fieldValue = getHostName();

  if (fieldValue.isEmpty())
  {
#ifndef QT_20
    KMsgBox::message(this, vError.data(),WARNING5, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif

    invalidEditField(getHostNameField());
    return false;
  }

  fieldValue = getPrinterName();

  if (fieldValue.isEmpty())
  {
#ifndef QT_20
    KMsgBox::message(this, vError.data(), WARNING6, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
    invalidEditField(getPrinterNameField());
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////
