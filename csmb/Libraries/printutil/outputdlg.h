/* Name: outputdlg.h
            
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

#ifndef __OUTPUTDLG_H__
#define __OUTPUTDLG_H__

#include <qwidget.h>
#include <qgroupbox.h>
#include <qcombobox.h>
#include <qpushbt.h>
#include <qlistbox.h>
#include <qlined.h>
#include <qlabel.h>
#include <qchkbox.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qmsgbox.h>
#include <qmlined.h>
#include <qstrlist.h>
#include "printerobject.h"
#include "manufacture.h"

class KPrinterOutputDlg : public QWidget//QDialog
{
	Q_OBJECT
public:
	typedef enum FORMAT_PAGES { FP_1up = 0, FP_2up, FP_4up, FP_8up };
  typedef enum PAPER_SIZE { PS_Letter = 0, PS_Legal, PS_Ledger, PS_A3, PS_A4 };

	KPrinterOutputDlg( bool bReadOnly, QWidget *parent,const char* name,
														KPrinterObject* printer );
	// access methods
	bool readOnly() const		{ return m_bReadOnly; }
	// set|get methods for printer type
	void setPrinterType(const char *item);	// the passed parameter here is the item key not the item
//	const char * getPrinterType();	// returns the key not the item displayed
	const char * getPrinterTypeItem();	// returns the item displayed

	// set|get methods for color depth
	void setColorDepth(const char *item);
	const char * getColorDepth();

	// set|get methods for resolution
	void setResolution(const char *item);
	const char * getResolution();

	// set|get methods for print header flag
	void setPrintHeader(bool flag);
	bool getPrintHeader();

	// set|get methods for paper size
	void setPaperSize(const char *item);
	const char * getPaperSize();

	// set|get methods for page format
	void setPageFormat(const char *item);
	const char * getPageFormat();

	// set|get methods for horz margin
	void setHorizontalMargin(const char *item);
	const char *getHorizontalMargin();

	// set|get methods for vert margin
	void setVerticalMargin(const char *item);
	const char *getVerticalMargin();

	// NOTE:
	// Each time a new printer is displayed in the combo
	// box, the color depth and res. combo boxes must be
	// filled with the appropriate information

	// insertion method for printer type
	void insertPrinterType(const char *item);//, const char* itemKey );

	// insertion method for color depths
	void insertColorDepth(const char *item);

	// insertion method for resolutions
	void insertResolution(const char *item);

	// insertion method for papersize
	void insertPaperSize(const char *item);

	//insertion method for pageformat
	void insertPageFormat(const char *item);

	//insertion method for margin
	void insertHmargin(const char *item);
	void insertVmargin(const char *item);
	

	//used for non PostScript printer
	void setColorDepthReadOnly(bool);
	void setResolutionReadOnly(bool);
	void setPaperSizeReadOnly(bool);
	void setPageFormatReadOnly(bool);
	void setMarginReadOnly(bool);
	bool getColorDepthReadOnly(){return m_ColorDepthReadOnly;};
	bool getResolutionReadOnly(){return m_ResolutionReadOnly;};
	bool getPaperSizeReadOnly(){return m_PaperSizeReadOnly;};
	bool getPageFormatReadOnly(){return m_PageFormatReadOnly;};
	bool getMarginReadOnly(){return m_MarginReadOnly;};
	
	// clear all combobox
	void clearComboBox();
	void setAllReadOnly(bool);

	void setupPrinters(KPrinterObject *apr);
	void setOutPutDlg(KPrinterObject *fromPrinter);
	//void setPrinterOutputProperty(KPrinterObject *toPrinter);
	bool setPrinterOutputProperty(KPrinterObject *toPrinter);
protected slots:
	void slot_enableMargins(int);
	void slot_changePrinterSettings(const char *);
  void slot_setPrinterConfig(const char *);

protected:
	void setReadOnly( bool bReadOnly )	{ m_bReadOnly = bReadOnly; }
	//void show();
	void resizeEvent(QResizeEvent *e);
	int indexOfItem(const char *item, QComboBox *list);

	QLabel *m_pType, *pType_lbl, *pColor_lbl, *pRes_lbl;
	QComboBox *pColor_cmb, *pRes_cmb;
	QCheckBox *pHeader_btn;

	QGroupBox *pSetup_gb;
	QLabel *pSize_lbl, *pFormat_lbl, *pMargin_lbl;
	QLabel *pHorz_lbl, *pVert_lbl, *pInch1_lbl, *pInch2_lbl;
	QComboBox *pSize_cmb, *pFormat_cmb;
	QComboBox *pHorzMar_cmb, *pVertMar_cmb;

signals:
	void printerChanged(const char *);

private:
//attributes
	bool		m_bReadOnly;
	bool 		m_ColorDepthReadOnly;
	bool    m_ResolutionReadOnly;
	bool    m_PaperSizeReadOnly;
	bool    m_PageFormatReadOnly;
	bool 		m_MarginReadOnly;
	QStrList		m_strlTypeKey;
	QStrList 		m_strTypeList;
	KPrinterObject *m_printer;
};

#endif

