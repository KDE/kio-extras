/* Name: printerobject.h
            
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

#ifndef __PRINTEROBJECT_H
#define __PRINTEROBJECT_H
#include <qstring.h>
#include <qlist.h>
#include <aps.h>

#define PRINTER_TYPE_LOCAL 0
#define PRINTER_TYPE_UNIX      1
#define PRINTER_TYPE_SMB   2

class KPrinterObject
{
public:
	KPrinterObject();
	~KPrinterObject();
	
	//get set method
	QString getPrinterName(){return m_printerName;};
	void setPrinterName( QString aName){m_printerName = aName;};

	int getPrinterType(){return m_PrinterType;};
	void setPrinterType(int atype){ m_PrinterType = atype;};

	QString getManufacture(){return m_Manufacture;};
	void setManufacture( QString aManufacture)
	{m_Manufacture= aManufacture;};

	QString getModel(){return m_Model;};
 	void setModel( QString aModel){m_Model = aModel;};

	long int getFileLimit(){return m_fileSize;};
	void setFileLimit(long int  bInt){m_fileSize = bInt;};

	QString getLocation(){return m_Location;};
 	void setLocation( QString alocation){m_Location = alocation;};

	QString getColorDepth(){return m_colorDepth;};
	void setColorDepth(QString aColorDepth){m_colorDepth = aColorDepth;};

  QString getResolution(){return m_resolution;};
	void setResolution(QString res){m_resolution = res;};
	QString getPaperSize(){return m_paperSize;};
	void setPaperSize( QString  aPaperSize){m_paperSize = aPaperSize;};

	QString getFormatPages(){return m_pageFormat;};
	void setFormatPages(QString aFormatPages){m_pageFormat = aFormatPages;};

	QString getHorizontalMargin(){return m_horizontalMargin;};
	void setHorizontalMargin(QString aMargin){m_horizontalMargin = aMargin;};

	QString getVerticalMargin(){return m_verticalMargin;};
	void setVerticalMargin(QString  aMargin){m_verticalMargin = aMargin;};

	QString getQueue(){return m_queue;};
	void setQueue(QString  aQueue){m_queue = aQueue;};

  QString getUser(){return m_user;};
	void setUser(QString  aUser){m_user = aUser;};

	QString getPasswd(){return m_passwd;};
	void setPasswd(QString  apwd){m_passwd = apwd;};

	bool getHeaderPage() {return m_headerpage;};
	void setHeaderPage(bool aBoolean){m_headerpage = aBoolean;};

	bool getSendEof(){return m_sendEof;};
	void setSendEof(bool  aBoolean){m_sendEof = aBoolean;};

	bool getFixStair(){return m_fixStair;};
	void setFixStair(bool  aBoolean){m_fixStair = aBoolean;};

	bool getFastTextPrinting(){return m_fastText;};
	void setFastTextPrinting(bool  aBoolean){m_fastText = aBoolean;};

	bool getIsColor(){return m_isColor;};
	void setIsColor(bool  aBoolean){m_isColor = aBoolean;};

  QString getGhostScript(){return m_ghostScript;};
	void setGhostScript(QString aGhostScript)
	{m_ghostScript =aGhostScript;}; 	

	bool isDefaultPrinter() {return m_defaultPrinter;};
	void setDefaultPrinter(bool b){m_defaultPrinter = b;};

	bool printingTestPage() {return m_printingTsPage;};
	void setPrintingTestPage(bool b){m_printingTsPage = b;};

//	KPrinterType* getPrinterModelType(){return m_printerModelType;};
//	void setPrinterModelType(KPrinterType * kp) {m_printerModelType = kp;};
	
private:
	int m_PrinterType;
	long int m_fileSize;
	bool m_defaultPrinter;
	bool m_printingTsPage;
	bool m_headerpage;
	bool m_sendEof;
	bool m_fixStair;
	bool m_fastText;
	bool m_isColor;
	QString m_Manufacture;
	QString m_Model;
	QString m_Location; //for connection, device or hostname+printerName
	QString m_ghostScript;
	QString m_colorDepth;
	QString m_resolution;
  //Aps_Resolution m_resolution;
	QString m_printerName;
	QString m_horizontalMargin;
	QString m_verticalMargin;
	QString m_queue;
	QString m_paperSize;
	QString m_pageFormat;
	QString m_user;
	QString m_passwd;

};


class KPrinterObjects
{
public:
  KPrinterObjects();
  ~KPrinterObjects();
  KPrinterObject* printer_lookup(const  char* name);

  uint getPrintersNumber();
  KPrinterObject * getPrinter(uint);
  void addPrinter(KPrinterObject *ku);
  void delPrinter(KPrinterObject *au);
	KPrinterObject* defaultPrinter();
	void clearObjects();
	KPrinterObjects& operator=(KPrinterObjects& );
private:
	QList <KPrinterObject> printerList;
};
#endif	

	
	
	
	
	
	
