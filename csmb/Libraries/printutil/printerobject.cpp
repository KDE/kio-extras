/* Name: printerobject.cpp
            
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

/// a new class for communication with KPrinterInfo and APS
#include "printerobject.h"
#include <stdio.h>

KPrinterObject::KPrinterObject()
{
	m_PrinterType = PRINTER_TYPE_LOCAL;
	m_fileSize = 0;
	m_defaultPrinter = false;
	m_printingTsPage = false;
	m_headerpage = false;
	m_sendEof = false;
	m_fixStair = false;
	m_fastText = false;
	m_isColor = false;
	m_Manufacture = "";
	m_Model = "";
	m_Location = "";
	m_ghostScript = "";
	m_colorDepth = "";
/*
	m_resolution.version = 0;
	m_resolution.horizontalRes = 0;
	m_resolution.verticalRes = 0;   */
  m_resolution = "";
	m_printerName = "";
	m_horizontalMargin = "";
	m_verticalMargin = "";
	m_queue = "";
	m_paperSize = "";
	m_pageFormat = "";
	m_user = "";
	m_passwd = "";
 // m_printerModelType = new KPrinterType("");//NULL;
}

KPrinterObject::~KPrinterObject()
{

}
//KPrinterObjects
KPrinterObjects::KPrinterObjects()
{
//	printerList.clear();
}

KPrinterObjects::~KPrinterObjects()
{
	printerList.clear();
}
void KPrinterObjects::addPrinter(KPrinterObject *ku)
{
  printerList.append(ku);
}

//****************************************************************************
//
// Method:
//
// Purpose:
//
//****************************************************************************
void KPrinterObjects::delPrinter(KPrinterObject *au)
{
  printerList.remove(au);
}

uint KPrinterObjects::getPrintersNumber()
{
 	return printerList.count();
}

KPrinterObject * KPrinterObjects::printer_lookup(const char *name)
{
	if (printerList.count()> 0)
	{
   	for (uint i = 0; i < printerList.count(); i++)
  	{
    	if (!strcmp(name,(const char*)printerList.at(i)->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
      ))
    	{
//fprintf(stderr, "\n !!!printer = %s, name = %s",
//						(char*)printerList.at(i)->getPrinterName(), name);
      	return printerList.at(i);
    	}
		}
	}
  return (NULL);
}

KPrinterObject * KPrinterObjects::getPrinter(uint i)
{
	if (getPrintersNumber()>i)
 		 return printerList.at(i);
	else
		 return NULL;
}

KPrinterObject* KPrinterObjects::defaultPrinter()
{
 	for (uint i = 0; i < printerList.count(); i++)
  {
   	if (printerList.at(i)->isDefaultPrinter())
   	{
      	return printerList.at(i);
   	}
	}
	return NULL;
}

void KPrinterObjects::clearObjects()
{
  printerList.clear();
}

KPrinterObjects& KPrinterObjects::operator=(KPrinterObjects& other)
{
 	if (this != &other)
	{
		this->clearObjects();
		for (uint i = 0; i<other.getPrintersNumber(); i++)
		this->addPrinter(other.getPrinter(i));
  }
	return *this;
}
