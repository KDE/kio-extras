/* Name: advancedlg.cpp
            
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
//
//	Jun 19, 2000 by olegn: 	reformatted according to Corel Coding Standard
//													Completely rewrote setPrinterAdvancedProperty so it doesn't
//													attempt to save unchanged values


//
//***************************************************************************


#include <qstrlist.h>
#include <qfile.h>
#include <qtstream.h>
#include <qtabdlg.h>


#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <kapp.h>
#include <kcharsets.h>
#include "advanceddlg.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <klocale.h>
#include "constText.h"
#include "commonfunc.h"

////////////////////////////////////////////////////////////////////////////

KPrinterAdvancedDlg::KPrinterAdvancedDlg(
	bool bReadOnly,
	QWidget *parent,
	const char *name,
	KPrinterObject *printer) :
		QWidget(parent, name)
{
	m_printer = printer;

	if (NULL == printer) // Safety check
		return;

	setReadOnly(bReadOnly);

	//Set up the checkbox/labels
	pEOF_btn = new QCheckBox(this, "EOF_at_end");
	pEOF_btn->setText(SEND_EOF);

	pFix_btn = new QCheckBox(this, "fix_stair");
	pFix_btn->setText(FIX_STAIR);

	pFast_btn = new QCheckBox(this, "fast_text");
	pFast_btn->setText(FAST_TEXT_PRINT);

	//Set up the ghostscipt label/edit box
	pGhost_leb = new QLineEdit(this, "ghost_option");
	pGhost_lbl = new QLabel(pGhost_leb, GHOSTSCRIPT, this);

	resize(parent->width(),parent->height());
	setMinimumSize (400, 320);

	setAdvancedDlg(m_printer);

	pEOF_btn->setEnabled(!readOnly());
	pFix_btn->setEnabled(!readOnly());
	pFast_btn->setEnabled(!readOnly());
	pGhost_leb->setEnabled(!readOnly());
}

////////////////////////////////////////////////////////////////////////////

KPrinterAdvancedDlg::~KPrinterAdvancedDlg()
{
}

////////////////////////////////////////////////////////////////////////////
// set|get the state of the 'Send EOF at end of print jog' flag

bool KPrinterAdvancedDlg::getSendEOF()
{
	return pEOF_btn->isChecked();
}

////////////////////////////////////////////////////////////////////////////

void KPrinterAdvancedDlg::setSendEOF(bool flag)
{
	pEOF_btn->setChecked(flag);
}

////////////////////////////////////////////////////////////////////////////
// set|get the state of the 'Fix stair-stepping text' flag

bool KPrinterAdvancedDlg::getFixText()
{
	return pFix_btn->isChecked();
}

////////////////////////////////////////////////////////////////////////////

void KPrinterAdvancedDlg::setFixText(bool flag)
{
	pFix_btn->setChecked(flag);
}

////////////////////////////////////////////////////////////////////////////
// set|get the state of the 'Fast text printing' flag

bool KPrinterAdvancedDlg::getFastPrint()
{
	return pFast_btn->isChecked();
}

////////////////////////////////////////////////////////////////////////////

void KPrinterAdvancedDlg::setFastPrint(bool flag)
{
	pFast_btn->setChecked(flag);
}

////////////////////////////////////////////////////////////////////////////
// set|get the contents of the 'Ghostscript Options' edit box

const char* KPrinterAdvancedDlg::getGhostOptions()
{
	return (const char *)pGhost_leb->text()
#ifdef QT_20
  .latin1()
#endif
  ;
}

////////////////////////////////////////////////////////////////////////////

void KPrinterAdvancedDlg::setGhostOptions(const char* options)
{
	pGhost_leb->setText(options);
}

////////////////////////////////////////////////////////////////////////////

void KPrinterAdvancedDlg::resizeEvent(QResizeEvent *e)
{
	QSize button = pEOF_btn->sizeHint();
	QSize box((pGhost_leb->sizeHint()).width()*2, (pGhost_leb->sizeHint()).height());;
	QSize label = pGhost_lbl->sizeHint();

	int offset1 = 20;

	int vOffset1 = 20;
	int vOffset2 = vOffset1 + button.height() + 15;
	int vOffset3 = vOffset2 + button.height() + 15;
	int vOffset4 = vOffset3 + button.height() + 15;
	int vOffset5 = vOffset4 + label.height() + 5;

	pEOF_btn->setGeometry(offset1, vOffset1, button.width(), button.height());
	pFix_btn->setGeometry(offset1, vOffset2, button.width(), button.height());
  pFast_btn->setGeometry(offset1, vOffset3, button.width(), button.height());
  pGhost_lbl->setGeometry(offset1, vOffset4, label.width(),label.height());
	pGhost_leb->setGeometry(offset1, vOffset5, box.width(), box.height());
}

////////////////////////////////////////////////////////////////////////////

void KPrinterAdvancedDlg::setAdvancedDlg(KPrinterObject * fromAprinter)
{
  if (!fromAprinter)
		return;

	// setting the advanced info

	setSendEOF(fromAprinter->getSendEof());
	setFixText(fromAprinter->getFixStair());
	setFastPrint(fromAprinter->getFastTextPrinting());
	setGhostOptions((const char *)fromAprinter->getGhostScript()
#ifdef QT_20
  .latin1()
#endif

  );
}

////////////////////////////////////////////////////////////////////////////

bool KPrinterAdvancedDlg:: setPrinterAdvancedProperty(KPrinterObject *aPrinter)
{
  Aps_PrinterHandle printerHandle;
	Aps_JobAttrHandle jobAttributeHandle;
  Aps_Result result;
	bool retcode = false;	// Assume failure

  if (!aPrinter)
		return false;

	if (APS_SUCCESS == Aps_OpenPrinter((const char*)aPrinter->getPrinterName()
#ifdef QT_20
  .latin1()
#endif

  , &printerHandle))
	{
		long SetFlags = 0;
		long ResetFlags = 0;
		QString NewGhostScriptOptions;

		if (getSendEOF() != aPrinter->getSendEof())
		{
			aPrinter->setSendEof(getSendEOF());

			if (aPrinter->getSendEof())
				SetFlags |= APS_CONFIG_EOF_AT_END;
			else
				ResetFlags |= APS_CONFIG_EOF_AT_END;
		}

		if (getFixText() != aPrinter->getFixStair())
		{
			aPrinter->setFixStair(getFixText());

			if (aPrinter->getFixStair())
				SetFlags |= APS_CONFIG_ADD_CR;
			else
				ResetFlags |= APS_CONFIG_ADD_CR;
		}

		if (getFastPrint() != aPrinter->getFastTextPrinting())
		{
			aPrinter->setFastTextPrinting(getFastPrint());

			if (aPrinter->getFastTextPrinting())
				SetFlags |= APS_CONFIG_TEXT_AS_TEXT;
			else
				ResetFlags |= APS_CONFIG_TEXT_AS_TEXT;
		}

		if ((SetFlags != 0L || ResetFlags != 0L) && // only call APS if something changed
        APS_SUCCESS != (result = Aps_PrinterSetConfigFlags(printerHandle,SetFlags,ResetFlags)) &&
        result != APS_NOT_FOUND &&
				result != APS_NO_CHANGE)
			goto ReleasePrinter;

		// get jobAttributeHandle

		NewGhostScriptOptions = getGhostOptions();

		if (aPrinter->getGhostScript() != NewGhostScriptOptions &&
        APS_SUCCESS == (result = Aps_PrinterGetDefAttr(printerHandle, &jobAttributeHandle)))
		{
			aPrinter->setGhostScript(NewGhostScriptOptions);

			// Save attribute in jobAttribute, then set the default value

			if ((APS_SUCCESS == Aps_AttrSetSetting(jobAttributeHandle, "gsoptions", (const char *)NewGhostScriptOptions
#ifdef QT_20
  .latin1()
#endif

      )) &&
          (APS_SUCCESS == Aps_PrinterSetDefAttr(printerHandle, jobAttributeHandle)))
				retcode = true;

			Aps_ReleaseHandle(jobAttributeHandle);
		}
		else
			retcode = true;

ReleasePrinter:;
		Aps_ReleaseHandle(printerHandle);
	}

	return retcode;
}

////////////////////////////////////////////////////////////////////////////


