/* Name: pageTwo.h
            
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

////////////////////////////////////////////////////////////////////////////////
// page two of Wizard

#include "../SambaNetworking/mwn/common.h"
#include <sys/stat.h>
#include <kapp.h>
#include "pageOne.h"
#include "pageTwo.h"
#include "pageThree.h"
#include "../SambaNetworking/mwn/PrinterSelectionDialog.h"
#include "constText.h"
#include "commonfunc.h"
#include "wizard.h"
#include "string.h"

///////////////////////////////////////////////////////////////////////////////

pageTwo::pageTwo(QWidget* parent, const char* name) :
	CWizardPage(parent, name, 0)
{
}

///////////////////////////////////////////////////////////////////////////////

pageTwo::pageTwo(int nPage,
								 bool bFinalPage,
								 int nNextPage,
								 int nPrevPage,
								 const char* pszImage,
								 const char* pszTitle,
								 bool bCheckPage,
                 QWidget *parent,
								 const char *name) :
	CWizardPage(nPage,
							bFinalPage,
							nNextPage,
							nPrevPage,
							pszImage,
							pszTitle,
							bCheckPage,
							parent,
							name)
{
	m_pParent = parent;

	// Description text

	m_pmainMsg = new QLabel(this, "Label_10");
	m_pmainMsg->setGeometry(5, 3, 280, 50);
	m_pmainMsg->setMinimumSize(0, 0);
	m_pmainMsg->setMaximumSize(32767, 32767);
	m_pmainMsg->setFocusPolicy(QWidget::NoFocus);
	m_pmainMsg->setBackgroundMode(QWidget::PaletteBackground);
	m_pmainMsg->setFontPropagation(QWidget::NoChildren);
	m_pmainMsg->setPalettePropagation(QWidget::NoChildren);
	m_pmainMsg->setAlignment(AlignLeft | AlignTop | ExpandTabs | WordBreak); //289
	m_pmainMsg->setMargin(-1);

	// "Network type" group box

	m_pNetworkTypeGroupBox = new QButtonGroup(this, "Network_group");
	m_pNetworkTypeGroupBox->setGeometry(5, 50, 315, 55);
	m_pNetworkTypeGroupBox->setMinimumSize(0, 0);
	m_pNetworkTypeGroupBox->setMaximumSize(32767, 32767);
	m_pNetworkTypeGroupBox->setFocusPolicy(QWidget::NoFocus);
	m_pNetworkTypeGroupBox->setBackgroundMode(QWidget::PaletteBackground);
	m_pNetworkTypeGroupBox->setFontPropagation(QWidget::NoChildren);
	m_pNetworkTypeGroupBox->setPalettePropagation(QWidget::NoChildren);
	m_pNetworkTypeGroupBox->setFrameStyle(49);
	m_pNetworkTypeGroupBox->setLineWidth(1);
	m_pNetworkTypeGroupBox->setMidLineWidth(0);
	m_pNetworkTypeGroupBox->QFrame::setMargin(0);
	m_pNetworkTypeGroupBox->setTitle(NETWORK_TYPE);
	m_pNetworkTypeGroupBox->setAlignment(1);
	m_pNetworkTypeGroupBox->setExclusive(FALSE);

  // Two radio buttons inside group box

	m_pWindows = new QRadioButton(this, "windows");
	m_pWindows->setGeometry(35, 70, 100, 20);
	m_pWindows->setMinimumSize(0, 0);
	m_pWindows->setMaximumSize(32767, 32767);
	m_pWindows->setFocusPolicy(QWidget::TabFocus);
	m_pWindows->setBackgroundMode(QWidget::PaletteBackground);
	m_pWindows->setFontPropagation(QWidget::NoChildren);
	m_pWindows->setPalettePropagation(QWidget::NoChildren);
	m_pWindows->setText(WINDOWS);
	m_pWindows->setAutoRepeat(FALSE);
	m_pWindows->setAutoResize(FALSE);
	m_pWindows->setChecked(FALSE);
	connect(m_pWindows, SIGNAL(clicked()), this, SLOT(WindowsButtonClicked()));
	m_pNetworkTypeGroupBox->insert(m_pWindows);

	m_pUnix = new QRadioButton(this, "unix");
	m_pUnix->setGeometry(165, 70, 60, 20);
	m_pUnix->setMinimumSize(0, 0);
	m_pUnix->setMaximumSize(32767, 32767);
	m_pUnix->setFocusPolicy(QWidget::TabFocus);
	m_pUnix->setBackgroundMode(QWidget::PaletteBackground);
	m_pUnix->setFontPropagation(QWidget::NoChildren);
	m_pUnix->setPalettePropagation(QWidget::NoChildren);
	m_pUnix->setText(UNIX1);
	m_pUnix->setAutoRepeat(FALSE);
	m_pUnix->setAutoResize(FALSE);
	m_pUnix->setChecked(TRUE);
	connect(m_pUnix, SIGNAL(clicked()), this, SLOT(UnixButtonClicked()));
	m_pNetworkTypeGroupBox->insert(m_pUnix);

	// "Nickname" label

	m_pNicknameLabel = new QLabel(this, "Label_11");
	m_pNicknameLabel->setMinimumSize(0, 0);
	m_pNicknameLabel->setMaximumSize(32767, 32767);
	m_pNicknameLabel->setFocusPolicy(QWidget::NoFocus);
	m_pNicknameLabel->setBackgroundMode(QWidget::PaletteBackground);
	m_pNicknameLabel->setFontPropagation(QWidget::NoChildren);
	m_pNicknameLabel->setPalettePropagation(QWidget::NoChildren);
	m_pNicknameLabel->setText(NICK_NAME);
	m_pNicknameLabel->setMargin(-1);

	// "Nickname" edit control

	m_pPrinterName = new QLineEdit(this, "LineEdit_11");
	m_pPrinterName->setMinimumSize(0, 0);
	m_pPrinterName->setMaximumSize(32767, 32767);
	m_pPrinterName->setFocusPolicy(QWidget::StrongFocus);
	m_pPrinterName->setBackgroundMode(QWidget::PaletteBackground);
	m_pPrinterName->setFontPropagation(QWidget::NoChildren);
	m_pPrinterName->setPalettePropagation(QWidget::NoChildren);
	m_pPrinterName->setText("");
	m_pPrinterName->setMaxLength(32767);
	m_pPrinterName->setFrame(QLineEdit::Normal);
	m_pPrinterName->setFrame(TRUE);
	m_pPrinterName->setFocus();
	m_pNicknameLabel->setBuddy(m_pPrinterName);

  // "Device or Port" label

	m_pLabelDeviceName = new QLabel(this, "Label_1");
	m_pLabelDeviceName->setMinimumSize(0, 0);
	m_pLabelDeviceName->setMaximumSize(32767, 32767);
	m_pLabelDeviceName->setFocusPolicy(QWidget::NoFocus);
	m_pLabelDeviceName->setBackgroundMode(QWidget::PaletteBackground);
	m_pLabelDeviceName->setFontPropagation(QWidget::NoChildren);
	m_pLabelDeviceName->setPalettePropagation(QWidget::NoChildren);
	m_pLabelDeviceName->setText(DEVICE_OR_PORT);
	m_pLabelDeviceName->setMargin(-1);

  /////////////////////////////////////////////////////////////////////
	// "Device of Port" combo

	m_pDeviceName_PortList = new QComboBox(this, "ComboBox_1");
	m_pDeviceName_PortList->setMinimumSize(0, 0);
	m_pDeviceName_PortList->setMaximumSize(32767, 32767);
	m_pDeviceName_PortList->setFocusPolicy(QWidget::StrongFocus);
	m_pDeviceName_PortList->setBackgroundMode(QWidget::PaletteBackground);
	m_pDeviceName_PortList->setFontPropagation(QWidget::NoChildren);
	m_pDeviceName_PortList->setPalettePropagation(QWidget::NoChildren);

	//////////////////////////////////////////////////////////////////
	// Initialise the m_pDeviceName_PortList control with Data

	char* DeviceList1[5];
	int count = GetCommandOutput(DeviceList1, 2, false, NULL, "/dev", "lp*");

	for (int i = 0; i < count; i++)
	{
		 m_pDeviceName_PortList->insertItem(DeviceList1[i]);
		 // free(DeviceList[i]);      //disabled by Doreen in setp. 29
	}

	// to add the usb devices, first check the directory existing

	char* DeviceList2[5];
	struct stat fileStat;

	ASSERT("/dev/usb/usblp0");

	if(!stat("/dev/usb/usblp0",&fileStat))
	{
		count = GetCommandOutput(DeviceList2, 3, false, NULL, "/dev/usb/", "usblp*");

		for (int i = 0; i < count; i++)
			m_pDeviceName_PortList->insertItem(DeviceList2[i]);
	}

	// "Hostname" label

	m_pHostnameLabel = new QLabel(this, "Label_3");
	m_pHostnameLabel->setMinimumSize(0, 0);
	m_pHostnameLabel->setMaximumSize(32767, 32767);
	m_pHostnameLabel->setFocusPolicy(QWidget::NoFocus);
	m_pHostnameLabel->setBackgroundMode(QWidget::PaletteBackground);
	m_pHostnameLabel->setFontPropagation(QWidget::NoChildren);
	m_pHostnameLabel->setPalettePropagation(QWidget::NoChildren);
	m_pHostnameLabel->setText(HOSTNAME2);

	// "Hostname" edit control

	m_pHostname = new QLineEdit(this, "LineEdit_2");
	m_pHostname->setMinimumSize(0, 0);
	m_pHostname->setMaximumSize(32767, 32767);
	m_pHostname->setFocusPolicy(QWidget::StrongFocus);
	m_pHostname->setBackgroundMode(QWidget::PaletteBase);
	m_pHostname->setFontPropagation(QWidget::NoChildren);
	m_pHostname->setPalettePropagation(QWidget::NoChildren);
	m_pHostname->setText("");
	m_pHostname->setMaxLength(32767);
	m_pHostname->setFrame(QLineEdit::Normal);
	m_pHostname->setFrame(TRUE);
  m_pHostnameLabel->setBuddy(m_pHostname);

  // "Browse..." button (Windows only)

	m_pBrowseButton = new QPushButton(this, "PushButton_1");
	m_pBrowseButton->setGeometry(320-82, 185, 82, 26);
	m_pBrowseButton->setMinimumSize(0, 0);
	m_pBrowseButton->setMaximumSize(32767, 32767);
	m_pBrowseButton->setFocusPolicy(QWidget::TabFocus);
	m_pBrowseButton->setBackgroundMode(QWidget::PaletteBackground);
	m_pBrowseButton->setFontPropagation(QWidget::NoChildren);
	m_pBrowseButton->setPalettePropagation(QWidget::NoChildren);
	m_pBrowseButton->setText(BROWSE);
	m_pBrowseButton->setAutoRepeat(FALSE);
	m_pBrowseButton->setAutoResize(FALSE);
	m_pBrowseButton->setToggleButton(FALSE);
	m_pBrowseButton->setDefault(FALSE);
	m_pBrowseButton->setAutoDefault(FALSE);
	m_pBrowseButton->setIsMenuButton(FALSE);
	connect(m_pBrowseButton, SIGNAL(clicked()), this, SLOT(BrowseButtonClicked()));

	// "Queue" label (Unix only)

	m_pQueueLabel = new QLabel(this, "Label_4");
	m_pQueueLabel->setMinimumSize(0, 0);
	m_pQueueLabel->setMaximumSize(32767, 32767);
	m_pQueueLabel->setFocusPolicy(QWidget::NoFocus);
	m_pQueueLabel->setBackgroundMode(QWidget::PaletteBackground);
	m_pQueueLabel->setFontPropagation(QWidget::NoChildren);
	m_pQueueLabel->setPalettePropagation(QWidget::NoChildren);
	m_pQueueLabel->setFrameStyle(0);
	m_pQueueLabel->setLineWidth(1);
	m_pQueueLabel->setMidLineWidth(0);
	m_pQueueLabel->QFrame::setMargin(0);
	m_pQueueLabel->setText(QUEUENAME);

	// "Queue" edit control (Unix only)

	m_pQueue = new QLineEdit(this, "LineEdit_3");
	m_pQueue->setMinimumSize(0, 0);
	m_pQueue->setMaximumSize(32767, 32767);
	m_pQueue->setFocusPolicy(QWidget::StrongFocus);
	m_pQueue->setBackgroundMode(QWidget::PaletteBase);
	m_pQueue->setFontPropagation(QWidget::NoChildren);
	m_pQueue->setPalettePropagation(QWidget::NoChildren);
	m_pQueue->setText("");
	m_pQueue->setMaxLength(32767);
	m_pQueue->setFrame(QLineEdit::Normal);
	m_pQueue->setFrame(TRUE);
	m_pQueueLabel->setBuddy(m_pQueue);
}

///////////////////////////////////////////////////////////////////////////////

pageTwo::~pageTwo()
{
	delete m_pmainMsg;
	delete m_pLabelDeviceName;
	delete m_pDeviceName_PortList;
	delete m_pNicknameLabel;
	delete m_pPrinterName;

	delete m_pHostnameLabel;
	delete m_pHostname;
	delete m_pBrowseButton;

	// following is not required at present --enabled by Doreen

	delete m_pQueueLabel;
	delete m_pQueue;

	delete m_pWindows;
	delete m_pUnix;
	delete m_pNetworkTypeGroupBox;
}

///////////////////////////////////////////////////////////////////////////////

int pageTwo::GetPrinterPort_Device(char* name)
{
	strcpy(name, (LPCSTR)m_pDeviceName_PortList->currentText()
#ifdef QT_20
  .latin1()
#endif
  );
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int pageTwo::GetPrinterName(char* name)
{
	if (NULL == name)
		return -1;

	if(NULL == m_pPrinterName)
		return -1;

	strcpy(name, (LPCSTR)m_pPrinterName->text()
#ifdef QT_20
  .latin1()
#endif
  );
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int pageTwo::GetNetworkType() // 0-UNIX  1-WINDOWS
{
	return m_pWindows->isChecked();
}

///////////////////////////////////////////////////////////////////////////////

int pageTwo::GetHostName(char* name)
{
	strcpy(name, (const char *)m_pHostname->text()
#ifdef QT_20
  .latin1()
#endif
  );
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int pageTwo::GetQueueName(char* name) // only for unix printers
{
	if (m_pUnix->isChecked())
	{
			strcpy(name, (const char *)m_pQueue->text()
#ifdef QT_20
  .latin1()
#endif
      );
			return 0;
	}
	return -1; // should be called only if it is unix printer
}

///////////////////////////////////////////////////////////////////////////////

void pageTwo::show()
{
	int nPrnType = ((pageOne*)(((CCorelWizard*)m_pParent)->getPage(0)))->GetTypeOfPrinter();

	if (nPrnType == 0) // local printer handling
	{
		m_pmainMsg->setText(LABEL21); // Give your printer name and select a port for output to be sent through to the printer.

		int nLabelWidth = m_pNicknameLabel->sizeHint().width();

		if (nLabelWidth < m_pLabelDeviceName->sizeHint().width())
			nLabelWidth = m_pLabelDeviceName->sizeHint().width();

		m_pNicknameLabel->setGeometry(5, 50, nLabelWidth, 25);
		m_pPrinterName->setGeometry(15+nLabelWidth, 50, 320-15-nLabelWidth, 25);
		m_pLabelDeviceName->setGeometry(5, 80, nLabelWidth, 25);
		m_pDeviceName_PortList->setGeometry(15+nLabelWidth, 80, 320-15-nLabelWidth, 25);

		m_pLabelDeviceName->show();
    m_pDeviceName_PortList->show();
    m_pHostnameLabel->hide();
		m_pHostname->hide();
		m_pBrowseButton->hide();
		m_pQueueLabel->hide();
		m_pQueue->hide();
		m_pNetworkTypeGroupBox->hide();
		m_pWindows->hide();
		m_pUnix->hide();
	}
	else //network printer
	{
		m_pmainMsg->setText(LABEL2);

		int nLabelWidth = m_pNicknameLabel->sizeHint().width();

		if (nLabelWidth < m_pHostnameLabel->sizeHint().width())
			nLabelWidth = m_pHostnameLabel->sizeHint().width();

		if (nLabelWidth < m_pQueueLabel->sizeHint().width())
			nLabelWidth = m_pQueueLabel->sizeHint().width();

		m_pNicknameLabel->setGeometry(5, 125, nLabelWidth, 25);
		m_pPrinterName->setGeometry(15+nLabelWidth, 125, 320-15-nLabelWidth, 25);
		m_pHostnameLabel->setGeometry(5, 155, nLabelWidth, 25);
		m_pHostname->setGeometry(15+nLabelWidth, 155, 320-15-nLabelWidth, 25);
		m_pQueueLabel->setGeometry(5, 185, nLabelWidth, 25);
		m_pQueue->setGeometry(15+nLabelWidth, 185, 320-15-nLabelWidth, 25);

		m_pLabelDeviceName->hide();
		m_pDeviceName_PortList->hide();
		m_pHostnameLabel->show();
		m_pHostname->show();
		m_pBrowseButton->show();
    m_pNetworkTypeGroupBox->show();
    m_pWindows->show();
    m_pUnix->show();

    if (m_pWindows->isChecked())
      WindowsButtonClicked();
    else
			UnixButtonClicked();
	}

	CWizardPage::show();
	m_pPrinterName->setFocus();
}

///////////////////////////////////////////////////////////////////////////////

void pageTwo::UnixButtonClicked()
{
	m_pQueueLabel->show();
	m_pQueue->show();
	m_pBrowseButton->hide();
}

///////////////////////////////////////////////////////////////////////////////

void pageTwo::WindowsButtonClicked()
{
	m_pQueueLabel->hide();
	m_pQueue->hide();
	m_pBrowseButton->show();
}

///////////////////////////////////////////////////////////////////////////////

void pageTwo::BrowseButtonClicked()
{
	ReadConfiguration();

	CPrinterSelectionDialog dlg(this, "Browsing for Windows printers");
	dlg.setCaption(MSG_BROWSE_FOR_NETWORK_PRINTER);

	if(dlg.exec())
		m_pHostname->setText((const char*)dlg.m_Path
#ifdef QT_20
  .latin1()
#endif
    );
}

///////////////////////////////////////////////////////////////////////////////

void pageTwo::pressedButton(int nButton, int nPage)
{
  if ((nButton == eButtonHelp) && (nPage == m_nPage))
  {
		QString helpfileName;
    char temp[100];
  	GetPrinterPort_Device(temp);
		if (strlen(temp))
		{
			helpfileName = "cs_print_local_attributes.htm";
		}
		else
		{
      helpfileName = "cs_print_network_attributes.htm";
		}

#ifndef QT_20
  	showClientHelp(helpfileName, "");
//commented by alexandrm
#endif

	}
}

///////////////////////////////////////////////////////////////////////////////

bool pageTwo::checkPage(int nButton)
{
	if (nButton == eButtonNext)
	{
		char temp[100], temp1[100], temp2[100], temp3[100];
  	GetPrinterPort_Device(temp);
  	GetPrinterName(temp2);
		static QString vError(i18n("Validation Error"));

		if(!varifyNickName(temp2))
 	  {
        return false;
 	  }
		fprintf(stderr, "\n device = %d",strlen(temp));
		int nPrnType = ((pageOne*)(((CCorelWizard*)m_pParent)->getPage(0)))->GetTypeOfPrinter();
		if (nPrnType) //0--local
		{
			GetHostName(temp1);
      fprintf(stderr, "\n host = %d",strlen(temp1));
   		if (!strlen(temp1))
			{
  			QString tmpError;
#ifndef QT_20
        KMsgBox::message(this, vError.data(),
									WARNINGH, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif
   			return false;
  		}

    	if (!GetNetworkType())
    	{
     		GetQueueName(temp3);
        fprintf(stderr, "\n queue = %d",strlen(temp3));
				if (!strlen(temp3))
				{
#ifndef QT_20
          KMsgBox::message(this, vError.data(),
									WARNING_Q1, KMsgBox::EXCLAMATION, OK_BUTTON);
//commented by alexandrm
#endif

   			return false;
				}
			  if (!validFileName(temp3))
     		{
#ifndef QT_20
       		KMsgBox::message(this, vError.data(), WARNING_Q2,
																		KMsgBox::EXCLAMATION,OK_BUTTON);
//commented by alexandrm
#endif

        	return false;
     		}
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

