/*

$Id$

Requires the Qt widget libraries, available at no cost at
http://www.troll.no

Copyright (C) 1998 Samuel Watts (sgwwatts@undergrad.math.uwaterloo.ca)


This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
//***************************************************************************
//    Modifications to this program were made by Corel Corporation,
//     November, 1999.  All such modifications are copyright (C) 1999
//     Corel Corporation and are licensed under the terms of the GNU
//     General Public License.
//
//
//***************************************************************************

#include <qtabdlg.h>
#include <qstrlist.h>
#include <qlayout.h>
#include <string.h>
//#include <kmsgbox.h>
#include <kapp.h>
#include <stdio.h>

#include "outputdlg.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <klocale.h>
#include <errno.h>
#include "constText.h"
#include "commonfunc.h"

#define TRACE(a) printf("FILE:%s LINE:%d ERR:%d\n", __FILE__, __LINE__, a);

extern char PICS_PATH[256];

////////////////////////////////////////////////////////////////////////////

KPrinterOutputDlg::KPrinterOutputDlg(
	bool bReadOnly,
	QWidget *parent,
	const char *name,
	KPrinterObject* printer) :
		QWidget(parent, name)
{
	m_printer = printer;

	if (NULL == printer)
		return; // Safety check

	setReadOnly(bReadOnly);

	// create the labels & combo boxes
	//pType_cmb = new QComboBox(FALSE, this, "Type");
	m_pType = new QLabel(this, "");
	pType_lbl = new QLabel(m_pType, PRINTER_TYPE_1,this);

	//connect(pType_cmb, SIGNAL(activated(const char *)), this, SLOT(slot_changePrinterSettings(const char *)));

	pColor_cmb = new QComboBox(FALSE, this, "Color");
	pColor_lbl = new QLabel(pColor_cmb, COLOR_DEPTH_1,this);

	pRes_cmb = new QComboBox(FALSE, this, "Resolution");
	pRes_lbl = new QLabel(pRes_cmb, RESOLUTION_1, this);

	pHeader_btn = new QCheckBox(this, "Header");
	pHeader_btn->setText(PRINT_HEADER_1);

  // Now to make the group box at the bottom

	pSetup_gb = new QGroupBox(PAGE_SETUP_1, this, "Setup");
	pSize_cmb = new QComboBox(FALSE, this, "Size");
	pSize_lbl = new QLabel(pSize_cmb, PAPER_SIZE_1, this);
	pFormat_cmb = new QComboBox(FALSE, this, "Format");
	pFormat_lbl = new QLabel(pFormat_cmb, FORMAT_PAGES_1, this);

	pMargin_lbl = new QLabel(MARGINS, this);

	pHorzMar_cmb = new QComboBox(FALSE, this, "HMarg");
	// pHorzMar_cmb->setEnabled(FALSE);
	pHorz_lbl = new QLabel(pHorzMar_cmb, HORIZONTAL_1, this);

	pInch1_lbl = new QLabel(PIXEL, this);

	pVertMar_cmb = new QComboBox(FALSE, this, "VMarg");
	// pVertMar_cmb->setEnabled(FALSE);
	pVert_lbl = new QLabel(pVertMar_cmb, VERTICAL_1, this);
	pInch2_lbl = new QLabel(PIXEL, this);

	pMargin_lbl->setGeometry(pFormat_cmb->x(), pHorzMar_cmb->y(), pMargin_lbl->sizeHint().width(),  pMargin_lbl->sizeHint().height());
	m_printer = printer;

	connect(this, SIGNAL(printerChanged(const char *)),
					this, SLOT(slot_setPrinterConfig(const char *)));

	pColor_cmb->setEnabled(!bReadOnly);
	pRes_cmb->setEnabled(!bReadOnly);
	pHeader_btn->setEnabled(!bReadOnly);
	pSize_cmb->setEnabled(!bReadOnly);
	pFormat_cmb->setEnabled(!bReadOnly);
	pVertMar_cmb->setEnabled(!bReadOnly);
	pHorzMar_cmb->setEnabled(!bReadOnly);
	m_ColorDepthReadOnly = bReadOnly;
	m_ResolutionReadOnly = bReadOnly;
	m_PaperSizeReadOnly = bReadOnly;
	m_PageFormatReadOnly = bReadOnly;
	m_MarginReadOnly = bReadOnly;

	setupPrinters(m_printer);
	setOutPutDlg(m_printer);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::setColorDepthReadOnly(bool ab)
{
	pColor_cmb->setEnabled(!ab);
	m_ColorDepthReadOnly = ab;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::setResolutionReadOnly(bool ab)
{
	pRes_cmb->setEnabled(!ab);
	m_ResolutionReadOnly =ab ;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::setPaperSizeReadOnly(bool ab)
{
	pSize_cmb->setEnabled(!ab);
	m_PaperSizeReadOnly = ab;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::setPageFormatReadOnly(bool ab)
{
	pFormat_cmb->setEnabled(!ab);
	m_PageFormatReadOnly = ab;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::setMarginReadOnly(bool ab)
{
	pVertMar_cmb->setEnabled(!ab);
	pHorzMar_cmb->setEnabled(!ab);
	m_MarginReadOnly = ab;
}

////////////////////////////////////////////////////////////////////////////

int KPrinterOutputDlg::indexOfItem(const char *item, QComboBox *list)
{
	int i;

	// Simple linear search because the list should be small

	for (i = 0; i < list->count(); i++)
	{
		if (!strcmp(item, (const char *)list->text(i)
#ifdef QT_20
  .latin1()
#endif
    ))
			return i;
	}

	return -1;
}

////////////////////////////////////////////////////////////////////////////
// set|get methods for printer type

void KPrinterOutputDlg::setPrinterType(const char *itemKey)
{
	m_pType->setText(itemKey);

/*	for (uint i = 0; i<m_strTypeList.count(); i++)
		if (!strcmp(itemKey, (char*)m_strTypeList.at(i)))
  		pType_cmb->setCurrentItem(i);
*/
	slot_changePrinterSettings(getPrinterTypeItem());
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterOutputDlg::getPrinterTypeItem()
{
	// returns the item displayed
	return (const char *)m_pType->text()
#ifdef QT_20
  .latin1()
#endif
  ; //pType_cmb->currentText();
}

////////////////////////////////////////////////////////////////////////////
// set|get methods for color depth

void KPrinterOutputDlg::setColorDepth(const char *item)
{
	if (strcmp(item,"0"))
  	pColor_cmb->setCurrentItem(indexOfItem(item, pColor_cmb));
	else
		pColor_cmb->setCurrentItem(0);
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterOutputDlg::getColorDepth()
{
  return (const char *)pColor_cmb->text(pColor_cmb->currentItem())
#ifdef QT_20
  .latin1()
#endif
    ;
}

////////////////////////////////////////////////////////////////////////////
// set|get methods for resolution

void KPrinterOutputDlg::setResolution(const char *item)
{
	if (strcmp(item,"0x0"))
		pRes_cmb->setCurrentItem(indexOfItem(item, pRes_cmb));
	else
		pRes_cmb->setCurrentItem (0);
}

////////////////////////////////////////////////////////////////////////////

const char * KPrinterOutputDlg::getResolution()
{
	return (const char *)pRes_cmb->text(pRes_cmb->currentItem())
#ifdef QT_20
  .latin1()
#endif
  ;
}

////////////////////////////////////////////////////////////////////////////
// set|get methods for print header flag

void KPrinterOutputDlg::setPrintHeader(bool flag)
{
	pHeader_btn->setChecked(flag);
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterOutputDlg::getPrintHeader()
{
	return pHeader_btn->isChecked();
}

////////////////////////////////////////////////////////////////////////////
// set|get methods for paper size

void KPrinterOutputDlg::setPaperSize(const char *item)
{
	pSize_cmb->setCurrentItem(indexOfItem(item, pSize_cmb));
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterOutputDlg::getPaperSize()
{
	return (const char *)pSize_cmb->text(pSize_cmb->currentItem())
#ifdef QT_20
  .latin1()
#endif
  ;
}

////////////////////////////////////////////////////////////////////////////
// set|get methods for page format

void KPrinterOutputDlg::setPageFormat(const char *item)
{
	pFormat_cmb->setCurrentItem(indexOfItem(item, pFormat_cmb));
	slot_enableMargins(pFormat_cmb->currentItem());
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterOutputDlg::getPageFormat()
{
	return (const char *)pFormat_cmb->text(pFormat_cmb->currentItem())
#ifdef QT_20
  .latin1()
#endif
  ;
}

////////////////////////////////////////////////////////////////////////////
// set|get methods for horz margin

void KPrinterOutputDlg::setHorizontalMargin(const char *item)
{
	pHorzMar_cmb->setCurrentItem(indexOfItem(item, pHorzMar_cmb));
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterOutputDlg::getHorizontalMargin()
{
	return (const char *)pHorzMar_cmb->text(pHorzMar_cmb->currentItem())
#ifdef QT_20
  .latin1()
#endif
                                                        ;
}

////////////////////////////////////////////////////////////////////////////
// set|get methods for vert margin

void KPrinterOutputDlg::setVerticalMargin(const char *item)
{
	pVertMar_cmb->setCurrentItem(indexOfItem(item, pVertMar_cmb));
}

////////////////////////////////////////////////////////////////////////////

const char *KPrinterOutputDlg::getVerticalMargin()
{
	return (const char *)pVertMar_cmb->text(pVertMar_cmb->currentItem())
#ifdef QT_20
  .latin1()
#endif
  ;
}

////////////////////////////////////////////////////////////////////////////
// insertion method for printer type

/*
void KPrinterOutputDlg::insertPrinterType(const char *item)//, const char*itemKey)
{
	m_strTypeList.append(item);
	pType_cmb->insertItem(item);
}
*/

////////////////////////////////////////////////////////////////////////////
// clear the printer color depths & res's for new printer

void KPrinterOutputDlg::slot_changePrinterSettings(const char * newPrinter)
{
	pColor_cmb->clear();
	pRes_cmb->clear();
	emit printerChanged(getPrinterTypeItem());
}

////////////////////////////////////////////////////////////////////////////
// insertion method for color depths

void KPrinterOutputDlg::insertColorDepth(const char *item)
{
	pColor_cmb->insertItem(item);
}

////////////////////////////////////////////////////////////////////////////
// insertion method for resolutions

void KPrinterOutputDlg::insertResolution(const char *item)
{
	pRes_cmb->insertItem(item);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::slot_enableMargins(int index)
{
	bool enable = TRUE;

	pHorzMar_cmb->setEnabled((!readOnly()) && enable);
	pVertMar_cmb->setEnabled((!readOnly()) && enable);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::insertPaperSize(const char *item)
{
	pSize_cmb->insertItem(item);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::insertPageFormat(const char *item)
{
	pFormat_cmb->insertItem(item);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::insertHmargin(const char *item)
{
	pHorzMar_cmb->insertItem(item);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::insertVmargin(const char *item)
{
	pVertMar_cmb->insertItem(item);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::resizeEvent(QResizeEvent *e)
{
	QSize label1((pType_lbl->sizeHint()).width()+40, (pType_lbl->sizeHint()).height());
	QSize label2 = pHorz_lbl->sizeHint();
	QSize label3 = pInch1_lbl->sizeHint();
	QSize box = pRes_cmb->sizeHint();
	QSize box2 = pHorzMar_cmb->sizeHint();
	QSize button = pHeader_btn->sizeHint();

	int offset1 = 20;
	int offset2 = 40;
	//   int offset3 = 50 + label1.width() -
	//(pMargin_lbl->sizeHint()).width() - 30;
	int offsetType_cmb = 25 + pType_lbl->sizeHint().width();
	int offset4 = 50 + label1.width();
	int offset5 = offset4 + label2.width() + 10;
	int offset6 = offset5 + box2.width() + 30;
	int offset7 = offset6 + label3.width() + 25;

	// vOffset[1-4] are for the top labels and widgets
	int vOffset1 = 20;
	int vOffset2 = vOffset1 + 15 + label1.height();
	int vOffset3 = vOffset2 + 15 + label1.height();
	int vOffset4 = vOffset3 + 15 + label1.height();
	// vOffset[5-10] are for the group box
	int vOffset5 = vOffset4 + 40;
	int vOffset6 = vOffset5 + 20;
	int vOffset7 = vOffset6 + 15 + label1.height();
	int vOffset8 = vOffset7 + 25 + label1.height();
	int vOffset9 = vOffset8 + 15 + label1.height();
	int vOffset10 = vOffset9 + 20 + label1.height();

	// Buttons and Labels in the top of the dialog
	pType_lbl->setGeometry(offset1, vOffset1, offsetType_cmb-5-offset1, box.height());
	m_pType->setGeometry(/*offsetType_cmb*/offset4, vOffset1, offset7-offsetType_cmb, box.height());

	pColor_lbl->setGeometry(offset1, vOffset2, label1.width(), box.height());
	pColor_cmb->setGeometry(offset4, vOffset2, offset6-offset4 - 5, box.height());

	pRes_lbl->setGeometry(offset1, vOffset3, label1.width(), box.height());
	pRes_cmb->setGeometry(offset4, vOffset3, offset6-offset4 - 5, box.height());

	pHeader_btn->setGeometry(offset1, vOffset4, button.width(), button.height());

	// Group box and its contents

	pSetup_gb->setGeometry(offset1, vOffset5, offset7 - offset1, vOffset10 - vOffset5);

	pSize_lbl->setGeometry(offset2, vOffset6, label1.width(), box.height());
	pSize_cmb->setGeometry(offset4, vOffset6, offset6 - offset4 - 5, box.height());

	pFormat_lbl->setGeometry(offset2, vOffset7, label1.width(), box.height());
	pFormat_cmb->setGeometry(offset4, vOffset7, offset6 - offset4 - 5, box.height());

	//pMargin_lbl->setGeometry(offset3, vOffset8, label1.width(), box.height());

	pMargin_lbl->setGeometry(offset2, vOffset8, label1.width(),box.height());
	pHorz_lbl->setGeometry(offset4-15, vOffset8, label2.width(), box.height());
	pHorzMar_cmb->setGeometry(offset5-12, vOffset8,offset5-offset4, box.height());
	pInch1_lbl->setGeometry(offset6, vOffset8,label3.width(),box.height());
	pVert_lbl->setGeometry(offset4-15, vOffset9,label2.width(), box.height());
	pVertMar_cmb->setGeometry(offset5-12, vOffset9,offset5-offset4, box.height());
	pInch2_lbl->setGeometry(offset6, vOffset9,label3.width(),box.height());

	pHorzMar_cmb->resize(pHorzMar_cmb->sizeHint().width(),pVertMar_cmb->sizeHint().height());
	pVertMar_cmb->resize(pHorzMar_cmb->sizeHint().width(),pVertMar_cmb->sizeHint().height());
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::clearComboBox()
{
	pColor_cmb->clear();
	pRes_cmb->clear();
	pSize_cmb->clear();
	pFormat_cmb->clear();
	pHorzMar_cmb->clear();
	pVertMar_cmb->clear();
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::setAllReadOnly(bool ab)
{
	//m_pType->setEnabled(!ab);
	setColorDepthReadOnly(ab);
	setResolutionReadOnly(ab);
	setPaperSizeReadOnly(ab);
	setPageFormatReadOnly(ab);
	setMarginReadOnly(ab);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::setOutPutDlg(KPrinterObject *fromAprinter)
{
	QString prt(fromAprinter->getManufacture());
	prt += " ";
	prt += fromAprinter->getModel();

	setPrinterType((const char*)prt
#ifdef QT_20
  .latin1()
#endif
  );

	//colordepth

	QString strColorDepth(fromAprinter->getColorDepth());

	if (strColorDepth.isEmpty())
		strColorDepth = MSG_DEFAULT;

	setColorDepthReadOnly(false);
	setColorDepth((const char*)strColorDepth
#ifdef QT_20
  .latin1()
#endif
  );

	//resolution

	QString res = fromAprinter->getResolution();

	if (res.isEmpty())
		res = MSG_DEFAULT;

	setResolutionReadOnly(false);
	setResolution((const char*)res
#ifdef QT_20
  .latin1()
#endif
  );

	//headerpage

	setPrintHeader(fromAprinter->getHeaderPage());

	//pageFormat

	QString pf(fromAprinter->getFormatPages());

	if (pf.isEmpty())
		pf = MSG_DEFAULT;

	setPageFormat((const char*)pf
#ifdef QT_20
  .latin1()
#endif
  );

	//pageSize

	QString pz(fromAprinter->getPaperSize());

	if(pz.length() != 0)
	{
		setPaperSizeReadOnly(false);
		setPaperSize((const char*)pz
#ifdef QT_20
  .latin1()
#endif
    );
	}
	else
		setPaperSizeReadOnly(true);

	//margin

	QString mg(fromAprinter->getHorizontalMargin());

	if (mg.isEmpty())
		mg = MSG_DEFAULT;

	setHorizontalMargin((const char*)mg
#ifdef QT_20
  .latin1()
#endif
  );

	mg = fromAprinter->getVerticalMargin();

	if (mg.isEmpty())
		mg = MSG_DEFAULT;

	setVerticalMargin((const char*)mg
#ifdef QT_20
  .latin1()
#endif
  );

	if (readOnly())
		setAllReadOnly(true);
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterOutputDlg::setPrinterOutputProperty(KPrinterObject *aPrinter)
{
	bool retcode = false;

	if (NULL == aPrinter)
		return false; // Safety check

	// setting the General info
  Aps_PrinterHandle printerHandle;
	Aps_JobAttrHandle jobAttributeHandle;
	QString NewValue;

	if (APS_SUCCESS == Aps_OpenPrinter((const char *)(aPrinter->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
  ), &printerHandle))
	{
		// setting the output info
		const char *printerType = getPrinterTypeItem();

		QString NewManufacturer = printerType;
		NewManufacturer.truncate(NewManufacturer.find(' ',1));

		QString NewModel = printerType + NewManufacturer.length() + 1;
		NewModel.stripWhiteSpace(); // just in case...

		if (NewManufacturer != aPrinter->getManufacture() ||
				NewModel != aPrinter->getModel())
		{
			aPrinter->setManufacture(NewManufacturer);
			aPrinter->setModel(NewModel);

			if (APS_SUCCESS != Aps_PrinterSetModel(printerHandle, (const char *)NewManufacturer
#ifdef QT_20
  .latin1()
#endif
      , (const char *)NewModel
#ifdef QT_20
  .latin1()
#endif
      ))
				goto ReleasePrinter;
		}

		//set headerPage

		if (getPrintHeader() != aPrinter->getHeaderPage())
		{
			aPrinter->setHeaderPage(getPrintHeader());

			if (APS_SUCCESS != Aps_PrinterSetConfigFlags(printerHandle,
																									 aPrinter->getHeaderPage() ? APS_CONFIG_HEADER_PAGE : 0, aPrinter->getHeaderPage() ? 0 : APS_CONFIG_HEADER_PAGE))
				goto ReleasePrinter;
		}

		if (APS_SUCCESS == Aps_PrinterGetDefAttr(printerHandle, &jobAttributeHandle))
		{
			bool bSettingsChanged = false;

			//set Colordepth

			NewValue = getColorDepth();

			if (aPrinter->getColorDepth() != NewValue &&
					(NewValue != MSG_DEFAULT || !aPrinter->getColorDepth().isEmpty()))
			{
				bSettingsChanged = true;
				aPrinter->setColorDepth(NewValue);

				if (APS_SUCCESS != Aps_AttrSetSetting(jobAttributeHandle, "colorrendering", (const char *)NewValue
#ifdef QT_20
  .latin1()
#endif
        ))
					goto ReleaseSettings;
			}

			//set Resolution

			NewValue = getResolution();

			if (aPrinter->getResolution() != NewValue &&
					(NewValue != MSG_DEFAULT || !aPrinter->getResolution().isEmpty()))
			{
				bSettingsChanged = true;
				aPrinter->setResolution(NewValue);

				if (APS_SUCCESS != Aps_AttrSetSetting(jobAttributeHandle, "*Resolution", (const char *)NewValue
#ifdef QT_20
  .latin1()
#endif
        ))
					goto ReleaseSettings;
			}

			//set page format

			NewValue = getPageFormat();

			if (aPrinter->getFormatPages() != NewValue &&
					(NewValue != MSG_DEFAULT || !aPrinter->getFormatPages().isEmpty()))
			{
				bSettingsChanged = true;
				aPrinter->setFormatPages(NewValue);

				if (APS_SUCCESS != Aps_AttrSetSetting(jobAttributeHandle, "n-up", (const char *)NewValue
#ifdef QT_20
  .latin1()
#endif
        ))
					goto ReleaseSettings;
			}

			// set page size

			NewValue = getPaperSize();

			if (aPrinter->getPaperSize() != NewValue)
			{
				bSettingsChanged = true;
				aPrinter->setPaperSize(NewValue);

				if (APS_SUCCESS != Aps_AttrSetSetting(jobAttributeHandle, "*PageSize", (const char *)NewValue
#ifdef QT_20
  .latin1()
#endif
        ))
					goto ReleaseSettings;
			}

			// Vertical margins

			NewValue = getVerticalMargin();

			if (aPrinter->getVerticalMargin() != NewValue &&
					(NewValue != MSG_DEFAULT || !aPrinter->getVerticalMargin().isEmpty()))
			{
				bSettingsChanged = true;
				aPrinter->setVerticalMargin(NewValue);

				if (APS_SUCCESS != Aps_AttrSetSetting(jobAttributeHandle, "TopMargin", (const char *)NewValue
#ifdef QT_20
  .latin1()
#endif
                ) ||
						APS_SUCCESS != Aps_AttrSetSetting(jobAttributeHandle, "BottomMargin", (const char *)NewValue
#ifdef QT_20
  .latin1()
#endif
            ))
					goto ReleaseSettings;
			}

			// Horizontal margins

			NewValue = getHorizontalMargin();

			if (aPrinter->getHorizontalMargin() != NewValue &&
					(NewValue != MSG_DEFAULT || !aPrinter->getHorizontalMargin().isEmpty()))
			{
				bSettingsChanged = true;
				aPrinter->setHorizontalMargin(NewValue);

				if (APS_SUCCESS != Aps_AttrSetSetting(jobAttributeHandle, "LeftMargin", (const char *)NewValue
#ifdef QT_20
  .latin1()
#endif
        ) ||
						APS_SUCCESS != Aps_AttrSetSetting(jobAttributeHandle, "RightMargin", (const char *)NewValue
#ifdef QT_20
  .latin1()
#endif
            ))
					goto ReleaseSettings;
			}

			// Save settings

			if (!bSettingsChanged ||
          APS_SUCCESS == Aps_PrinterSetDefAttr(printerHandle, jobAttributeHandle))
				retcode = true;

ReleaseSettings:;
 			Aps_ReleaseHandle(jobAttributeHandle);
		}
ReleasePrinter:;
		Aps_ReleaseHandle(printerHandle);
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::slot_setPrinterConfig(const char *printerType)
{
  Aps_PrinterHandle printerHandle;
  Aps_JobAttrHandle jobAttributeHandle;
	Aps_Resolution **resolutionArray;
	Aps_AttrOption **options;
	Aps_PageSize **pageSizes;
	int numPageSizes;
	int numOptions;
	double minSetting = 0.0;
	double maxSetting = 0.0;
	Aps_Result result;
	int i;

	QString NewManufacturer = printerType;
	NewManufacturer.truncate(NewManufacturer.find(' ',1));

	QString NewModel = printerType + NewManufacturer.length() + 1;
	NewModel.stripWhiteSpace(); // just in case...

	clearComboBox();
	setAllReadOnly(false);

	// open this printer and set its model to the new one
	// and then get the options or gray out the attitude field

	if ((result = Aps_OpenPrinter((const char*)m_printer->getPrinterName()
#ifdef QT_20
  .latin1()
#endif
    ,
			&printerHandle))== APS_SUCCESS)
 	{
    //if (APS_SUCCESS == Aps_PrinterSetModel(printerHandle, NewManufacturer, NewModel))
		//{
			if (APS_SUCCESS == Aps_PrinterGetDefAttr(printerHandle, &jobAttributeHandle))
			{
        /////////start get options and insert them
				// 1---Obtain a list of avaliable Resolutions

				if (APS_SUCCESS == (result=Aps_AttrQuickGetResOptions(jobAttributeHandle, &resolutionArray, &numOptions)))
 				{
					if (numOptions == 0)
						setResolutionReadOnly(true);
	        else
					{
           	for(i = 0; i < numOptions; ++i)
						{
							double xRes = resolutionArray[i]->horizontalRes;
   						double yRes = resolutionArray[i]->verticalRes;
							QString Resolution;

							Resolution.sprintf("%ldx%lddpi", (long)xRes,(long)yRes);
 							insertResolution((const char *)Resolution
#ifdef QT_20
  .latin1()
#endif
              );
						}
					}

					if (NULL != resolutionArray)
						Aps_ReleaseBuffer(resolutionArray);
				}
				else
				{
					Aps_Resolution ressetting;

					if (APS_SUCCESS == Aps_AttrQuickGetRes(jobAttributeHandle, &ressetting))
					{
						QString Resolution;

						Resolution.sprintf("%ldx%lddpi",(long)ressetting.horizontalRes, (long)ressetting.verticalRes);
						insertResolution((const char *)Resolution
#ifdef QT_20
  .latin1()
#endif
            );
					}
					else
						insertResolution((const char *)MSG_DEFAULT
#ifdef QT_20
  .latin1()
#endif
            );
				}

				if (APS_SUCCESS == Aps_AttrGetOptions(jobAttributeHandle, "colorrendering", &numOptions, &options))
				{
					QString opt;

					if (numOptions == 0)
						setColorDepthReadOnly(true);
					else
					{
						for(i = 0; i < numOptions; ++i)
						{
							opt = QString(options[i]->optionID);
							insertColorDepth((const char *)opt
#ifdef QT_20
  .latin1()
#endif
              );
						}
					}

					if (NULL != options)
							Aps_ReleaseBuffer(options);
				}
				else
					insertColorDepth((const char *)MSG_DEFAULT
#ifdef QT_20
  .latin1()
#endif
          );

				// 3- getPaperSize options

				if (APS_SUCCESS == Aps_AttrQuickGetPageSizeOptions(jobAttributeHandle, &pageSizes, &numPageSizes))
				{
					if (numPageSizes == 0)
						  setPaperSizeReadOnly(true);
					else
					{
           	for(i = 0; i < numPageSizes; ++i)
						{
							QString ps;
							ps = QString(pageSizes[i]->id);
					  	insertPaperSize((const char*)ps
#ifdef QT_20
  .latin1()
#endif
              );
      			}
					}

					if (NULL != pageSizes)
						Aps_ReleaseBuffer(pageSizes);
				}
        else
					setPaperSizeReadOnly(true);

				// 4--- pageFormat

				if (APS_SUCCESS == Aps_AttrGetOptions(jobAttributeHandle, "n-up", &numOptions, &options))
				{
					if (numOptions == 0)
						setPageFormatReadOnly(true);
					else
					{
						for(i = 0; i < numOptions; ++i)
						{
							QString fmt(options[i]->optionID);
							insertPageFormat((const char *)fmt
#ifdef QT_20
  .latin1()
#endif
              );
						}
					}

					if (NULL != options)
						Aps_ReleaseBuffer(options);
				}
				else
					insertPageFormat((const char *)MSG_DEFAULT
#ifdef QT_20
  .latin1()
#endif
          );

				// obtain the margins

				if ((result = Aps_AttrGetRange(jobAttributeHandle, "LeftMargin",
																				&minSetting, &maxSetting))
																				== APS_SUCCESS)
				{
					QString margin;

					if (minSetting == maxSetting && minSetting == 0.0)
           	setMarginReadOnly(true);
					else
					{
						margin.sprintf("%f",minSetting);
						insertHmargin((const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                                );

						// give more option
						margin.sprintf("%f",((minSetting+maxSetting)/4));
						insertHmargin((const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                                );
						margin.sprintf("%f",((minSetting+maxSetting)/2));
						insertHmargin((const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                                );
						margin.sprintf("%f",maxSetting);
						insertHmargin((const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                                );
					}

					minSetting = 0.0;
					maxSetting = 0.0;

					if ((result = Aps_AttrGetRange(jobAttributeHandle, "TopMargin",
																				&minSetting, &maxSetting))
																				== APS_SUCCESS)
					{
						QString margin;
						margin.sprintf("%f",minSetting);
						insertVmargin((const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                              );

						// give more option

						margin.sprintf("%f",((minSetting+maxSetting)/4));
            insertVmargin((const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                              );
						margin.sprintf("%f",((minSetting+maxSetting)/2));
						insertVmargin((const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                              );
						margin.sprintf("%f",maxSetting);
						insertVmargin((const char*)margin
#ifdef QT_20
  .latin1()
#endif
                                              );
					}
				}
        else
				{
					insertHmargin((const char*)MSG_DEFAULT
#ifdef QT_20
  .latin1()
#endif
          );
					insertVmargin((const char*)MSG_DEFAULT
#ifdef QT_20
  .latin1()
#endif
          );
				}

				Aps_ReleaseHandle(jobAttributeHandle);
			}
			else
				setAllReadOnly(true);
		//}

		Aps_ReleaseHandle(printerHandle);
	}
}

////////////////////////////////////////////////////////////////////////////

void KPrinterOutputDlg::setupPrinters(KPrinterObject *apr)
{
/*
	KManufactures *allManufactures1;
  allManufactures1 = new KManufactures();
	int manf = allManufactures1->numberOfManufactures();

	if (manf > 0)
  {
		KManufacture *km;

		for (int k = 0; k < manf; k++)
		{
			km = allManufactures1->getManufacture2(k);

			if (km)
			{
				QStrList arr;
				arr = km->getModels();

				for (int i = 0; i< km->getNumOfModels(); i++)
				{
					QString type(km->getMFName());
					type +=" ";
					type += arr.at(i);
				  insertPrinterType((const char*)type);
				}
			}
		}
  }
	*/

	QString defaultType(apr->getManufacture());
	defaultType += " ";
	defaultType += apr->getModel();
	setPrinterType((const char *)defaultType
#ifdef QT_20
  .latin1()
#endif
  );
}

////////////////////////////////////////////////////////////////////////////

