/* Name: mywizard.cpp
            
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

#include <qpushbutton.h>
#include <qlabel.h>
#include <qframe.h>
#include <qpixmap.h>
#include <qaccel.h>
#include <qwindowdefs.h>
#include <qfont.h>
#include <qapplication.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include "wizardpage.h"
#include <string.h>
#include <qdialog.h>
#include "pageOne.h"
#include "pageTwo.h"
#include "pageThree.h"
#include "pageFour.h"
#include "pageFive.h"
#include "mywizard.h"
#include "constText.h"
#include "commonfunc.h"

////////////////////////////////////////////////////////////////////////////////

void CPrinterWizard::done( int r )
{
	QDialog::done(r);
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getNickName()
{
	char temp[1024];

	((pageTwo*)getPage(1))->GetPrinterName(temp);

	QString nickName(temp);

	return nickName;
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getDeviceName()
{
	char temp[1024];

	((pageTwo*)getPage(1))->GetPrinterPort_Device(temp);

	QString devicePort(temp);

	if (devicePort.contains("usb"))
		devicePort = "/dev/usb/" + devicePort;
	else
	 devicePort = "/dev/"+ devicePort;

	return devicePort;
}

////////////////////////////////////////////////////////////////////////////////

int	CPrinterWizard::getPrinterType() // type of printer 0-local, 1-Unix,2-Windows
{
	int type = ((pageOne*)getPage(0))->GetTypeOfPrinter();

	if (type == 1) //network printer
	{
		if (((pageTwo*)getPage(1))->GetNetworkType()) // windows printer
			type = 2;
		else
      type = 1;
	}

	return type;
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getHostName() // for Network Printer(Unix/Windows)
{
	QString Hostname;
	QString testing;
  char temp[1024];

	switch(getPrinterType())
	{
		case 0:
			return QString("");

		case 1:
			// printer type is Unix Network
			((pageTwo*)getPage(1))->GetHostName(temp);
      Hostname = temp;
    break;

		case 2:
			((pageTwo*)getPage(1))->GetHostName(temp);
      Hostname = temp;
			// extract hostname from the printer
			//removing '\\' from the name
			Hostname.remove( 0, 2);
			// looking for remaining '\' and removing it along with printer name
			Hostname.remove( Hostname.find('\\'), 1000);//len is too large to remove rest of the string
		break;

		default:
		return 0;
	}

	return Hostname;
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getLocation()
{
	QString location;
	char temp[1024];

	switch( getPrinterType())
	{
		case 0:
			location = getDeviceName();
		break;

		case 1:
			// printer type is Unix Network
			((pageTwo*)getPage(1))->GetHostName(temp);
			location = "\\\\";
			location += QString(temp);
			location += "\\";
			location += getQueueName();
		break;

		case 2:
			((pageTwo*)getPage(1))->GetHostName(temp);
      location = temp;
		break;

		default:
		return 0;
	}

	return location;
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getPrinterName() // for Windows only
{
	char temp[1024];

	if (getPrinterType() != 2)
		return 0;

	//printer type is Windows

	((pageTwo*)getPage(1))->GetHostName(temp);

	QString printer(temp);

	// extract hostname from the printer
	//removing '\\' from the name

	printer.remove( 0, 2);
	// looking for remaining '\' and removing it along with hostname
	printer.remove( 0, printer.find('\\')+1);

	return printer;
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getQueueName() // for unix only
{
	char temp[1024];

	if (getPrinterType() != 1)
		return 0;

	//printer type is Windows

	((pageTwo*)getPage(1))->GetQueueName(temp);

	QString queue(temp);

	return queue;
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getPrinterTypeKey()
{
	char temp[1024];

	((pageThree*)getPage(2))->getMagicFilter(temp);

	QString typeKey(temp);
	return typeKey;
}

////////////////////////////////////////////////////////////////////////////////

bool CPrinterWizard::varifyNickName(const QString& strName)
{
	static QString fieldValue;
  static QString vError(WARNING_CAPTION);
  static QString tmpError;
	int  j;
 // KPrinterObjects *allprinters = pr->getAllPrinters();
 /////////////this part need reimplementation
  if (!validFileName(strName))
  {
  	tmpError.sprintf((const char *)i18n("The name for the printer contains illegal characters.\n Please try again.")
#ifdef QT_20
  .latin1()
#endif
                                                                                                      );
#ifndef QT_20
       KMsgBox::message(this, vError.data(), WARNING1, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif

   return false;
  }

  if(!uniquePrinterName((const char*)strName
#ifdef QT_20
  .latin1()
#endif
                                              ))
  {
#ifndef QT_20
   	KMsgBox::message(this, vError.data(),WARNING2, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
    return false;
  }

 	char array[7][5] = {"lp", "lpr", "lpq", "lprm", "lp0", "lp1", "lp2"};
  for (j =0; j < 7; j++)
  {
  	if (!strcmp((const char *)strName
#ifdef QT_20
  .latin1()
#endif
                                      , array[j]))
  	{
  		tmpError.sprintf((const char *)i18n("The name for the printer can not be lp\\lpq\\lpr\\lprm\\lp0\\lp1\\lp2.\n Please try again.")
#ifdef QT_20
  .latin1()
#endif
                                );
#ifndef QT_20
    	KMsgBox::message(this, vError.data(), WARNING3, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif

   	 return false;
   	 }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// interface for setting the information

void CPrinterWizard::setNumberOfPrinterObjects(int count)
{
	// third page in the wizard has the printerdatabase object
	// passing the information to the third page...
 ((pageThree*)getPage(2))->setNumberOfPrinterObjects(count);
}

////////////////////////////////////////////////////////////////////////////////

int CPrinterWizard::setPrinterObject(const char* item, const char* itemKey)
{
	return ((pageThree*)getPage(2))->setPrinterObject( item, itemKey);
}

////////////////////////////////////////////////////////////////////////////////

void CPrinterWizard::updatePrinterCombos()
{
	((pageThree*)getPage(2))->init_comboboxes();

	// setting focus to the first page
	setActivePage(0, 1);
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getCommand()
{
	QString command1("lpr -P");

	if (getPrintTestPage())
	{
		QString size_paper(getPaperSize());
		command1 += getNickName();

		if (size_paper.contains("Letter") ||size_paper.contains("Legal"))
		{
			command1 += QString(" /etc/printfilters/testpage.ps");
		}
		else
		{
			command1 += QString(" /etc/printfilters/testpage-a4.ps");
		}

		return command1;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool CPrinterWizard::getPrintTestPage()
{
  return ((pageFour*)getPage(3))->getPrintTestPage();
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getPaperSize()
{
	QString size_paper(((pageFour*)getPage(3))->getPaperSize());

	return size_paper;
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getManufacture()
{
  char temp[1024];

	((pageThree*)getPage(2))->getManufacture(temp);

	return QString(temp);
}

////////////////////////////////////////////////////////////////////////////////

QString CPrinterWizard::getModel()
{
  char temp[1024];

	((pageThree*)getPage(2))->getPrinterModel(temp);

	return QString(temp);
}

////////////////////////////////////////////////////////////////////////////////

