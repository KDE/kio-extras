/* Name: CorelFileDialog.cpp

   Description: This file is a part of the libmwn library.

   Author:	Oleg Noskov (olegn@corel.com)

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

#include "qglobal.h"

#include "common.h"
#include "CorelFileDialog.h"
#include "qfileinfo.h"
#include "qdir.h"
#include <sys/param.h> // for MAXPATHLEN
#include <unistd.h> // for getcwd
#define Inherited CCorelFileDialogData
#include "listview.h"
#include "header.h"
#include "smbworkgroup.h"
#include "smbserver.h"
#include "smbshare.h"
#include "smbfile.h"
#include "smbutil.h"
#include "msroot.h"
#include "mycomputer.h"
#include "trashentry.h"
#include "localfile.h"
#include "filesystem.h"
#include "qapplication.h"
#include "corellistboxitem.h"
#include "qrect.h"
#include "qregexp.h"

#define kTabSize 100

class COffsetListBoxItem : public QListBoxItem    
{
public:
	COffsetListBoxItem(int nOffset, const char *s, const QPixmap p, void *pUserData)
	     : QListBoxItem(), pm(p)
	{ 
		setText(s);
		m_pUserData = pUserData;
		m_nOffset = nOffset;
	}   

	void* GetUserData()
	{
		return m_pUserData;
	}

	virtual const QPixmap *pixmap() 
	{ 
		return &pm; 
	}    

protected:

	virtual void paint(QPainter *);
	
	virtual int height(const QListBox *) const;
    
	virtual int width(const QListBox *) const;
    
private:
	QPixmap pm;    
	void *m_pUserData;
	int m_nOffset;
};

void COffsetListBoxItem::paint(QPainter *p)
{
	p->drawPixmap(m_nOffset + 3, 0, pm);
	QFontMetrics fm = p->fontMetrics();
    int yPos; // vertical text position
    
	//if (pm.height() < fm.height())
	//	yPos = fm.ascent() + fm.leading()/2;
	//else
	//	yPos = pm.height()/2 - fm.height()/2 + fm.ascent();
    
	if (pm.height() < fm.height())
		yPos = 0;
	else
		yPos = (pm.height() - fm.height())/2;

	p->setTabStops(kTabSize);

#ifdef QT_20
	QSize sz = fm.size(Qt::AlignLeft | Qt::ExpandTabs, text(), -1, kTabSize);
#else
	QSize sz = fm.size(AlignLeft | ExpandTabs, text(), -1, kTabSize);
#endif
	
	
	int x = pm.width() + 5;
	
	if (x < 25)
		x=25;

#ifdef QT_20
	p->drawText(m_nOffset + x, yPos, sz.width(), fm.height(), Qt::AlignLeft | Qt::ExpandTabs | Qt::SingleLine, text());
#else
	p->drawText(m_nOffset + x, yPos, sz.width(), fm.height(), AlignLeft | ExpandTabs | SingleLine, text());
#endif
	
	//p->drawText(pm.width() + 5, yPos, text());
}
    
int COffsetListBoxItem::height(const QListBox *lb) const
{
	return QMAX(pm.height(), lb->fontMetrics().lineSpacing() + 1);
}

int COffsetListBoxItem::width(const QListBox *lb) const
{
#ifdef QT_20
	QSize sz = lb->fontMetrics().size(Qt::AlignLeft | Qt::ExpandTabs, text(), -1, kTabSize);
#else	
	QSize sz = lb->fontMetrics().size(AlignLeft | ExpandTabs, text(), -1, kTabSize);
#endif
	
	int x = pm.width() + 5;
	
	if (x < 25)
		x=25;
	
	return m_nOffset + x + sz.width(); //pm.width() + sz.width() + 6;
}

/* XPM */

static const char* cdtoparent_xpm[] =
{
	"15 14 5 1",
	". c None",
	"# c #000000",
	"a c #9999ff",
	"$ c #ccccff",
	"@ c #ffffff",
	"..#####........",
	".#aaaaa#.......",
	"#aaaaaaa#######",
	"#@@@@@@@@@@@@@#",
	"#@$$$#$$$$$$aa#",
	"#@$$###$$$$aaa#",
	"#@$#####$$aaaa#",
	"#@$$$#$$$$aaaa#",
	"#@$$$#$$$$aaaa#",
	"#@$$$######aaa#",
	"#@$$$$$$aaaaaa#",
	"#@$$$$$$aaaaaa#",
	"###############",
	"..............."
};				 

class CDesktopItem : public CNetworkTreeItem
{
public:
	CDesktopItem(CListView *parent) : CNetworkTreeItem(parent, NULL)
	{
		InitPixmap();
	}

	QTTEXTTYPE text(int column) const
	{
		switch (column)
		{
			case -1:
				return "MyComputer";

			case 0:
				return LoadString(knMY_LINUX);

			default:
				return "";
		}
	}

	QString FullName(BOOL bDoubleSlashes)
	{
		return QString(text(0));
	}

	virtual int CredentialsIndex()
	{
		return 0;
	}

	void Fill()
	{
	}

	QPixmap *Pixmap()
	{
		return LoadPixmap(keDesktopIcon);
	}

	CItemKind Kind()
	{
		return keDesktopItem;
	}

	QTKEYTYPE key(int, bool) const
	{
		return "1";
	}
};

class CShowItem : public CWindowsTreeItem
{
public:
	CShowItem(CListView *pParent, 
						LPCSTR Name, 
						LPCSTR Size, 
						LPCSTR Type, 
						LPCSTR Date, 
						LPCSTR Attributes, 
						LPCSTR NameKey,
						LPCSTR SizeKey = "",
						LPCSTR DateKey = "") :
	
	CWindowsTreeItem(pParent, Name, Size, Type, Date, Attributes),
		m_NameKey(NameKey),
		m_SizeKey(SizeKey),
		m_DateKey(DateKey)
	{
		
	}

	QTKEYTYPE key(int nColumn, bool ascending) const
	{
		static QString s;

		switch (nColumn)
		{
			case 0:	// Name
			return(QTKEYTYPE)m_NameKey;

			case 1:	// Size
			return(QTKEYTYPE)m_SizeKey;

			case 3:	// Date
			return(QTKEYTYPE)m_DateKey;

			default:
			return text(nColumn);
		}
	}

	QString m_NameKey;
	QString m_SizeKey;
	QString m_DateKey;
};

CCorelFileDialog::CCorelFileDialog
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	Init();
}

CCorelFileDialog::CCorelFileDialog(LPCSTR startWith, 
																	 LPCSTR filter,
																	 QWidget *parent, 
																	 LPCSTR name, 
																	 bool modal)
{
	QString StartFileName;

	if (NULL == startWith)
	{
		char buf[MAXPATHLEN+1];
		getcwd(buf, sizeof(buf)-1);
		m_DirName = buf;
	}
	else
	{
		switch (FolderExists(startWith))
		{
			case -3: // exists but is not a folder
			{
				m_DirName	= GetParentURL(startWith);

				StartFileName = startWith + m_DirName.length() + 1;
			}
			break;

			case -2: // permission denied
				StartFileName = startWith;
			break;

			case -1: // doesn't exist
			{
				m_DirName	= GetParentURL(startWith);

				switch (FolderExists(m_DirName))
				{
					case -3: // exists but is not a folder
					case -2: // permission denied
					case -1: // doesn't exist
						StartFileName = startWith;
						m_DirName = "";
					break;

					case 0: // exists and writable
					case 1: // exists and not writable
						StartFileName = startWith + m_DirName.length() + 1;
					break;

					default: // should never happen
					break;
				}
			}
			break;
			
			case 0: // exists and writable
			case 1: // exists and not writable
				m_DirName = startWith;
			break;

			default: // should never happen
			break;
		}
	}
  
	if (!StartFileName.isEmpty())
	{
		m_pFileNameEdit->setText(StartFileName);
	}
	
	SetFilter(filter);
	Init();
}

void CCorelFileDialog::MovieInit()
{
	/* Init movie */

	extern unsigned char search_movie_data[];
	extern int gSearchMovieLen;
	QByteArray x;

	x.duplicate((LPCSTR)&search_movie_data[0], gSearchMovieLen);

	m_pMovie = new QMovie(x);
	m_pMovie->setBackgroundColor(colorGroup().base());

	m_pSearchMovieLabel = new QLabel(m_pListView);

	m_pSearchMovieLabel->resize(knSEARCH_MOVIE_WIDTH, knSEARCH_MOVIE_HEIGHT);
	m_pSearchMovieLabel->setMovie(*m_pMovie);
	m_pSearchMovieLabel->setMargin(0);
	m_pSearchMovieLabel->hide();
	m_pSearchMovieLabel->movie()->pause();
	m_pSearchMovieLabel->setBackgroundMode(NoBackground);

	m_pCancelButton->setText(LoadString(knCANCEL));
	
	m_pLookInLabel->setBuddy(m_pLookInCombo);
	
	m_pFileNameLabel->setText(LoadString(knFILE__NAME_COLON));
	m_pFileNameLabel->setBuddy(m_pFileNameEdit);
	m_pFileTypeLabel->setText(LoadString(knFILES_OF__TYPE_COLON));
	m_pFileTypeLabel->setBuddy(m_pFileTypeCombo);
}

void CCorelFileDialog::MovieRun()
{
	m_pSearchMovieLabel->move((m_pListView->width()-m_pSearchMovieLabel->width())/2, (m_pListView->height() - m_pSearchMovieLabel->height())/2);
	m_pSearchMovieLabel->movie()->unpause();
	m_pSearchMovieLabel->show();
}

void CCorelFileDialog::MovieStop()
{
	m_pSearchMovieLabel->hide();
	m_pSearchMovieLabel->movie()->pause();
}

void CCorelFileDialog::Init()
{
	connect(m_pFileNameEdit, SIGNAL(returnPressed()), this, SLOT(OnFileNameEditReturnPressed()));

	connect(m_pLookInCombo, SIGNAL(activated(int)), this, SLOT(OnSelchangeCombo(int)));
	connect(m_pFileTypeCombo, SIGNAL(activated(int)), this, SLOT(OnSelchangeFileTypeCombo(int)));
	m_pLookInCombo->setSizeLimit(10);
	m_pLookInCombo->setAutoResize(FALSE);
	m_pLookInCombo->setMaxCount(2147483647);
	m_pLookInCombo->setAutoCompletion(FALSE);

	m_pListView->setFocusPolicy( QWidget::StrongFocus );
	m_pListView->setFrameStyle( 51 );
	m_pListView->setLineWidth( 2 );
	m_pListView->setMidLineWidth( 0 );
	m_pListView->QFrame::setMargin( 0 );
	m_pListView->setResizePolicy( QScrollView::Manual );
	m_pListView->setVScrollBarMode( QScrollView::Auto );
	m_pListView->setHScrollBarMode( QScrollView::Auto );
	m_pListView->setTreeStepSize( 20 );
	m_pListView->setMultiSelection( FALSE );
	m_pListView->setAllColumnsShowFocus( FALSE );
	m_pListView->setItemMargin( 1 );
	m_pListView->setRootIsDecorated( FALSE );
	m_pListView->addColumn( "", -1 );
	m_pListView->setColumnWidthMode( 0, CListView::Maximum );
	m_pListView->setColumnAlignment( 0, 1 );

	m_bIgnoreNextDone = false;

	m_pTopButton1->setPixmap(QPixmap(cdtoparent_xpm));
	connect(m_pTopButton1, SIGNAL(clicked()), SLOT(OnGoParentClicked()));

	m_pAcceptButton->setAutoDefault(TRUE);
	m_pAcceptButton->setDefault(TRUE);

	m_pTree = new CListView(this, "tree");
	
	CDesktopItem *pDesktopItem = new CDesktopItem(m_pTree);
	m_pTree->setCurrentItem(pDesktopItem);

	m_pMyComputer = new CMyComputerItem(pDesktopItem);
	
	extern BOOL gbIncludeRegularFilesInTree;
	gbIncludeRegularFilesInTree = TRUE;
		
	if (!gCredentials.count())
		ReadConfiguration();

	m_pNetworkRoot = new CMSWindowsNetworkItem(pDesktopItem);

	m_pTree->hide();
	
	connect(m_pListView, SIGNAL(doubleClicked(CListViewItem *)),
					this, SLOT(OnSelectFile(CListViewItem *)));
	
	connect(m_pListView, SIGNAL(returnPressed(CListViewItem *)),
					this, SLOT(OnListviewReturnPressed(CListViewItem *)));
	
	MovieInit();
}

CCorelFileDialog::~CCorelFileDialog()
{
}

QString CCorelFileDialog::selectedFile() const
{
	return "";
}

QString CCorelFileDialog::getOpenFileURL(const char *startWith,
																					const char *filter,
																					QWidget *parent, const char *name)
{
	QString s = getOpenFileName(startWith, filter, parent, name);

	if (s.isEmpty())
		return s;

	return "file://localhost/" + s;
}

QString CCorelFileDialog::getOpenFileName(LPCSTR startWith,
																					LPCSTR filter,
																					QWidget *parent, 
																					LPCSTR name)
{
	CCorelFileDialog dlg(startWith, filter, parent, name, TRUE);
	dlg.setCaption(LoadString(knOPEN));
	dlg.m_pLookInLabel->setText(LoadString(knLOOK_IN_COLON));
	dlg.m_pAcceptButton->setText(LoadString(kn_OPEN));

	//dlg->setMode(CCorelFileDialog::ExistingFile);
	
	QString result;
	
	if (dlg.exec() == QDialog::Accepted)
	{
		result = dlg.selectedFile();
	}
	
	return result;
}

QString CCorelFileDialog::getSaveFileURL(const char *startWith,
																					const char *filter,
																					QWidget *parent, const char *name)
{
	QString s = getSaveFileName(startWith, filter, parent, name);
	
	if (s.isEmpty())
		return s;
	
  return QString("file://localhost/") + s;
}

QString CCorelFileDialog::getSaveFileName(const char *startWith,
																					const char *filter,
																					QWidget *parent, const char *name)
{
	char buf[MAXPATHLEN];
	
	if (NULL == startWith)
	{
		getcwd(buf, sizeof(buf)-1);
		startWith = &buf[0];
	}

	CCorelFileDialog *dlg = new CCorelFileDialog(startWith, 
																							 filter, 
																							 parent, 
																							 name, 
																							 TRUE);
	
	dlg->setCaption(LoadString(knSAVE_AS));
	dlg->m_pLookInLabel->setText(LoadString(knSAVE_IN_COLON));
	dlg->m_pAcceptButton->setText(LoadString(kn_SAVE));
	
	QString result;
	
//	if (!initialSelection.isEmpty())
	//	dlg->setSelection(initialSelection);
	
	if (dlg->exec() == QDialog::Accepted)
	{
		result = dlg->selectedFile();
	}
	
	delete dlg;
	return result;
}

void CCorelFileDialog::SetHeaderType(int nType)
{
	switch (nType)
	{
		case 0:  // Name, Comment
		{
			m_pListView->setNumCols(2);
						
			m_pListView->setColumnText(0, LoadString(knNAME));
			m_pListView->setColumnText(1, LoadString(knCOMMENT));
			m_pListView->setColumnAlignment(1, AlignLeft);
		}
		break;

		case 1: // Name, Size, Type, Modified
		{
			m_pListView->setNumCols(4);
			m_pListView->setColumnText(0, LoadString(knNAME));
			m_pListView->setColumnText(1, LoadString(knSIZE));
			m_pListView->setColumnAlignment(1, AlignRight);
			m_pListView->setColumnText(2, LoadString(knTYPE));
			m_pListView->setColumnText(3, LoadString(knMODIFIED));
		}
		break;

		case 2:
		{
			m_pListView->setNumCols(4);
			
			m_pListView->setColumnText(0, LoadString(knMOUNTED_ON));
			m_pListView->setColumnText(1, LoadString(knFILESYSTEM));
			m_pListView->setColumnAlignment(1, AlignLeft);
			m_pListView->setColumnText(2, LoadString(knTOTAL_SIZE));
			m_pListView->setColumnText(3, LoadString(knFREE_SPACE));
		}
		break;

    case 3: // Trash
    {
      m_pListView->setNumCols(5);
      m_pListView->setColumnText(1, LoadString(knORIGINAL_LOCATION));
			m_pListView->setColumnAlignment(1, AlignLeft);
      m_pListView->setColumnText(2, LoadString(knDATE_DELETED));
      
      m_pListView->setColumnText(3, LoadString(knTYPE));
      m_pListView->setColumnText(4, LoadString(knSIZE));
    }
    break;
	}
}

void CCorelFileDialog::SetColumnType(int nType)
{
	qApp->processEvents();

	switch (nType)
	{
		case 0:  // Name, Comment
		{
			m_pListView->setColumnWidth(0, m_pListView->viewport()->width() / 2);
			m_pListView->setColumnWidth(1, m_pListView->viewport()->width() / 2);
		}
		break;

		case 1: // Name, Size, Type, Modified
		{
			m_pListView->setColumnWidth(1, 70);
			
			QPainter p;
			p.begin(m_pListView);
			int width2 = p.boundingRect(0,0,200,50, AlignLeft, LoadString(knTYPE)).width() + 10;
			m_pListView->setColumnWidth(2, width2);
			p.end();

			m_pListView->setColumnWidth(3, 120);
			
			int width0 = m_pListView->viewport()->width() - 120 - 70 - width2;
			
			if (width0 < 100)
				width0 = 100;

			if (width0 > 300)
				width0 = 300;
			
			m_pListView->setColumnWidth(0, width0);
		}
		break;

		case 2:
		{
			m_pListView->setColumnWidth(1, 100);
			m_pListView->setColumnWidth(2, 70);
			
			QPainter p;
			p.begin(m_pListView);
			int width3 = p.boundingRect(0,0,200,50, AlignLeft, LoadString(knFREE_SPACE)).width() + 10;
			m_pListView->setColumnWidth(3, width3);
			p.end();
			
			int width0 = m_pListView->viewport()->width() - 100 - 70 - width3;
			
			if (width0 < 100)
				width0 = 100;

			if (width0 > 300)
				width0 = 300;
			
			m_pListView->setColumnWidth(0, width0);
		}
		break;

    case 3: // Trash
    {
			m_pListView->setColumnWidth(0, 70);
			m_pListView->setColumnWidth(1, 150);
			m_pListView->setColumnWidth(2, 120);
      
			QPainter p;
			p.begin(this);
			int width2 = p.boundingRect(0,0,200,50, AlignLeft, LoadString(knTYPE)).width() + 10;
			m_pListView->setColumnWidth(3, width2);
			p.end();
      
      m_pListView->setColumnWidth(4, 70);
    }
    break;
	}
}


void CCorelFileDialog::Refresh()
{
	CNetworkTreeItem *pItem = (CNetworkTreeItem*)(m_pTree->currentItem());
	
	if (NULL == pItem)
	{
		return;
	}
	
	//d->cdToParent->setEnabled(NULL != pItem->parent());

	m_pListView->clear();

	if (pItem->ExpansionStatus() == keNotExpanded)
	{
		MovieRun();
		pItem->Fill();
		MovieStop();
	}

	int i;
	CListViewItem *c;

	switch (pItem->Kind())
	{
		/* List of filesystems */
	
		case keMyComputerItem:
		{
			/* First prepare the header */
			SetHeaderType(2);
	
			CListViewItem *c = pItem->firstChild();
	
			for (i=pItem->childCount(); i > 0; i--, c = c->nextSibling())
				new CFileSystemItem(m_pListView, (CFileSystemInfo*)(CFileSystemItem *)c, pItem);
	
			SetColumnType(2);
		}
		break;

		/* Microsoft Windows network */

		case keMSRootItem:
		{
			SetHeaderType(0);
			
			for (c = pItem->firstChild(); NULL != c; c = c->nextSibling())
				new CWorkgroupItem(m_pListView, (CSMBWorkgroupInfo*)(CWorkgroupItem *)c);
			
			SetColumnType(0);
		}
		break;
		
		/* Local file */
		
		case keFileSystemItem:
		case keLocalFileItem:
		case keDeviceItem:
		{
			QString Path = pItem->FullName(FALSE);
	
			//if (Path.isEmpty()) // Device not yet mounted
			//{
//				bEnableBackground = FALSE;
				//nMessageID = knDEVICE_NOT_MOUNTED;
				//break;
			//}
	
			//if (access(Path, 0))
			//{
//				emit GoParentRequest();
				//gTreeExpansionNotifier.DoItemDestroyed(item);
				//delete item;
	//			return;
			//}
	
			int nFolderCount = 0;
			//m_TotalSize = 0;
	
			if (IsTrashFolder(Path))
			{
				SetHeaderType(3);

				CTrashEntryArray list;
	
				if (GetTrashEntryList(Path, list))
				{
					int i;
	
					for (i=0; i < list.count(); i++)
					{
						CTrashEntryItem *pI = new CTrashEntryItem(m_pListView, pItem, &list[i]);
	
							if (S_ISDIR(pI->m_OriginalFileMode))
								nFolderCount++;
					}
				}
        
				SetColumnType(3);
			}
			else
			{
				SetHeaderType(1);

				CWaitLogo w;
				CFileArray list;
				GetLocalFileList((LPCSTR)Path, &list, TRUE);
	
				if (pItem->ExpansionStatus() == keNotExpanded)
					((CLocalFileContainer*)pItem)->Fill(list);
	
				for (i=0; i < list.count(); i++)
				{
          if (list[i].IsFolder() || NameMatchesFilter(list[i].m_FileName))
					{
						CLocalFileItem *pI = new CLocalFileItem(m_pListView, pItem, &list[i]);
					
						if (pI->IsFolder())
							nFolderCount++;
					}
				}
	
//				if (list.count() == 0 && access((LPCSTR)Path, X_OK))
	//				bEnableBackground = FALSE;
	
				SetColumnType(1);
			}
		};
		break;

		/*
		case keShareItem:
		case keFileItem:
		{
			SetHeaderType(files, 1);

			CFileArray list;
			CRemoteFileContainer *nitem = (CRemoteFileContainer *)pItem;
			CSMBErrorCode retcode;

			do
			{
				retcode = GetFileList((LPCSTR)nitem->FullName(TRUE), &list, nitem->CredentialsIndex(), TRUE);
			}
			while (retcode == keErrorAccessDenied && PromptForPassword(nitem));

			for (i=0; i < list.count(); i++)
			{
				CSMBFileInfo *pInfo = &list[i];

				QString NameKey, SizeKey, DateKey;

				// Size Key
					
				if (pInfo->IsFolder())
					SizeKey = QString("\1") + pInfo->m_FileName;
				else
					SizeKey.sprintf("%.10ld", atol((LPCSTR)pInfo->m_FileSize));

				// Date Key
				DateKey.sprintf("%.10ld", pInfo->m_FileDate);

				// Name key
				NameKey = pInfo->IsFolder() ? "\1" : "";
				NameKey += pInfo->m_FileName;

				// Date

				struct tm Tm = *localtime(&pInfo->m_FileDate);

				char AMPM[100];
				strftime(AMPM, sizeof(AMPM), "%I:%M %P" , &Tm);
				QString AMPMString(AMPM[0] == '0' ? &AMPM[1] : AMPM);

				char TimeBuf[100];

				sprintf(
							 TimeBuf, 
							 "%d/%d/%d %s", 
							 Tm.tm_mon+1,
							 Tm.tm_mday,
							 Tm.tm_year > 99 ? Tm.tm_year + 1900 : Tm.tm_year,
							 (LPCSTR)AMPMString.upper());

				CShowItem *pNewItem = new CShowItem(files, pInfo->m_FileName, 
																						pInfo->IsFolder() ? "" : (LPCSTR)(SizeInKilobytes((unsigned long)atol(pInfo->m_FileSize)) + "  "),
																						pInfo->IsFolder() ? "Directory" : "File",
																						TimeBuf,
																						pInfo->m_FileAttributes,
																						NameKey,
																						SizeKey,
																						DateKey);

				const QPixmap *pPixmap;

				if (list[i].IsFolder())
					pPixmap = LoadPixmap(keClosedFolderIcon);
				else
					pPixmap	= GetFilePixmap(list[i].m_FileName, FALSE, -1);

				if (pPixmap != NULL)
					pNewItem->setPixmap(0, *pPixmap);
			}
		}
		break;
		*/

			/*
			case keMSRootItem:
			{
				for (c = pItem->firstChild(); NULL != c; c = c->nextSibling())
					new CWorkgroupItem(files, (CSMBWorkgroupInfo*)(CWorkgroupItem *)c);
			}
			break;
	
	//			for (c = pItem->firstChild(); NULL != c; c = c->nextSibling())
		//			new CServerItem(files, (CSMBServerInfo*)(CServerItem *)c);
			//break;
	
			case keServerItem:
				for (c = pItem->firstChild(); NULL != c; c = c->nextSibling())
					new CShareItem(files, pItem, (CSMBShareInfo*)(CShareItem *)c);
			break;
	
			case kePrinterItem:
			break;
	
			case keFileSystemItem:
			case keLocalFileItem:
			case keFTPSiteItem:
			case keFTPFileItem:
			case keWebRootItem:
			case keWebPageItem:
			break;
	*/
		default: // Desktop
		{
			SetHeaderType(0);
			
			for (c=pItem->firstChild(); NULL != c; c = c->nextSibling())
			{
				CWindowsTreeItem *pNewItem = new CWindowsTreeItem(m_pListView, c->text(0), c->text(1), c->text(2)); //, "","",c->key(0, 0));
				
				const QPixmap *pPixmap = c->pixmap(0);

				if (pPixmap != NULL)
					pNewItem->setPixmap(0, *pPixmap);
			}
			
			SetColumnType(0);
		}
	}
	
	QListBox *lb = m_pLookInCombo->listBox();
	
  int nOffset = 0;

	for (CListViewItem *i=pItem->parent(); NULL != i; i=i->parent())
    nOffset += 10;
	
	lb->clear();
	
	int n = 0;

	if (nOffset < 30 && CNetworkTreeItem::IsMyComputerItem(pItem))
	{
		CNetworkTreeItem *pI = (CNetworkTreeItem *)(pItem->listView()->firstChild());

		lb->insertItem(new COffsetListBoxItem(0, pI->text(0), *(pI->pixmap(0)), (void*)pI), n++);
		if (pI == pItem)
		{
			lb->setCurrentItem(n-1);
			m_pLookInCombo->setCurrentItem(n-1);
		}

		for (pI = (CNetworkTreeItem*)(pI->firstChild()); 
				 NULL !=  pI; 
				 pI = (CNetworkTreeItem*)(pI->nextSibling()))
		{
			lb->insertItem(new COffsetListBoxItem(10, pI->text(0), *(pI->pixmap(0)), (void*)pI), n);

			if (pI == pItem)
			{
				lb->setCurrentItem(n);
				m_pLookInCombo->setCurrentItem(n);
			}
			
			if (!strcmp(LoadString(knMY_COMPUTER), pI->text(0)))
			{
				if (pI->ExpansionStatus() == keNotExpanded)
				{
					pI->Fill();
				}
				
				int nn = ++n;

				for (CNetworkTreeItem *pSubItem = (CNetworkTreeItem *)(pI->firstChild());
						 NULL != pSubItem;
						 pSubItem = (CNetworkTreeItem *)(pSubItem->nextSibling()))
				{
					lb->insertItem(new COffsetListBoxItem(20, pSubItem->text(0), *(pSubItem->pixmap(0)), (void*)pSubItem), nn);
					
					if (pSubItem == pItem)
					{
						lb->setCurrentItem(nn);
						m_pLookInCombo->setCurrentItem(nn);
					}
					n++;
				}
			}
		}
	}
	else
	{
		int nTail = 0;
	
		while (NULL != pItem)
		{
			if (nOffset < 30)
			{
				CNetworkTreeItem *pI = (CNetworkTreeItem *)pItem->m_pLogicalParent;
	
				if (NULL != pI)
				{
					int nPos=nTail;
	
					for (pI=(CNetworkTreeItem *)pI->firstChild(); NULL != pI && pItem != pI; pI = (CNetworkTreeItem *)pI->nextSibling())
					{
						lb->insertItem(new COffsetListBoxItem(nOffset, pI->text(0), *(pI->pixmap(0)), (void*)pI), nPos);
						nTail++;
					}
				}
			}
	
			lb->insertItem(new COffsetListBoxItem(nOffset, pItem->text(0), *(pItem->pixmap(0)), (void*)pItem), 0);
			n++;
			nTail++;
	
			// Add more siblings if necessary
			
			if (nOffset < 30)
			{
				for (CNetworkTreeItem *pI = (CNetworkTreeItem *)pItem->nextSibling(); 
						 NULL != pI;
						 pI = (CNetworkTreeItem *)pI->nextSibling())
				{
					lb->insertItem(new COffsetListBoxItem(nOffset, pI->text(0), *(pI->pixmap(0)), (void*)pI), 0);
					n++;
					nTail++;
				}
			}
			
			pItem = (CNetworkTreeItem *)pItem->m_pLogicalParent;
			nOffset -= 10;
		}
		
		lb->setCurrentItem(n-1);
		m_pLookInCombo->setCurrentItem(n-1);
	}

	m_pLookInCombo->repaint();
}

void CCorelFileDialog::show()
{
	Navigate(m_DirName);
	m_pListView->setFocus();
	
	if (NULL != m_pListView->firstChild())
	{
		m_pListView->setCurrentItem(m_pListView->firstChild());
	}
	QDialog::show();
}

void CCorelFileDialog::OnSelectFile(CListViewItem *newItem)
{
	QString name = newItem->text(0);

	CNetworkTreeItem *pItem = (CNetworkTreeItem*)(m_pTree->currentItem());
	CListViewItem *c;

	for (c=pItem->firstChild(); NULL != c; c = c->nextSibling())
	{
		if (!stricmp(name, c->text(0)))
		{
			CNetworkTreeItem *pI = (CNetworkTreeItem *)c;

			if (pI->Kind() == keFileItem && !((CFileItem*)pI)->IsFolder())
			{
				accept();
				return;
			}

			m_pTree->setCurrentItem(c);
			break;
		}
	}

	Refresh();
}

void CCorelFileDialog::OnListviewReturnPressed(CListViewItem *pNewItem)
{
	m_bIgnoreNextDone = true;
	OnSelectFile(pNewItem);
}

void CCorelFileDialog::OnAcceptClicked()
{
	if (m_bIgnoreNextDone)
	{
		m_bIgnoreNextDone = false;
	}
	else
		accept();
}

void CCorelFileDialog::OnGoParentClicked()
{
	CNetworkTreeItem *pItem = (CNetworkTreeItem*)(m_pTree->currentItem());
	
	if (NULL != pItem && NULL != pItem->parent())
	{
		m_pTree->setCurrentItem(pItem->parent());
		Refresh();
	}
}

void CCorelFileDialog::OnSelchangeCombo(int nIndex)
{
#ifndef QT_20
	CListBoxItem *lbitem = (CListBoxItem*)((CWorkaroundListBox*)(m_pLookInCombo->listBox()))->item(nIndex);
#else
	CListBoxItem *lbitem = (CListBoxItem*)(m_pLookInCombo->listBox()->item(nIndex));
#endif

	if (lbitem != NULL)
	{
		m_pTree->setCurrentItem((CListViewItem*)(lbitem->GetUserData()));
		Refresh();
	}
}

void CCorelFileDialog::Navigate(LPCSTR Path)
{
	CListViewItem *pTarget = NULL;

	if (IsUNCPath(Path))
	{
		if (gbNetworkAvailable)
			pTarget = m_pNetworkRoot->FindAndExpand(Path);
	}
	else
		if (Path[0] == '/') // Local path
			pTarget = m_pMyComputer->FindAndExpand(Path);

	if (NULL != pTarget)
	{
		m_pTree->setCurrentItem(pTarget);
		Refresh();
	}
}

void CCorelFileDialog::SetFilter(LPCSTR Filter)
{
	m_Filter.clear();

	LPCSTR p = strchr(Filter, '|');

	if (NULL != p)
	{
		while (NULL != p)
		{
      if (p == Filter || '\0' == *p)
				break;
			
			m_Filter.append((LPCSTR)
#ifdef QT_20
                      QCString
#else
                      QString
#endif
                      (Filter, p - Filter + 1));
			
			Filter = p + 1;
			p = strchr(Filter, '|');
		}
	}
	else
	{
		QString f(Filter);
	
		QRegExp r("([a-zA-Z0-9\\.\\*\\?]*)$");
		
		int len;
		int index = r.match(f, 0, &len);
		
		if (index >= 0)
		{
			m_Filter.append(Filter);
			m_Filter.append(f.mid(index+1, len-2));
		}
	}
	
	QStrListIterator it(m_Filter);
	BOOL bOdd = TRUE;

	m_pFileTypeCombo->clear();

	for (it.toFirst(); NULL != it.current(); ++it)
	{
		if (bOdd)
			m_pFileTypeCombo->insertItem(it.current());

		bOdd = !bOdd;
	}
}

BOOL CCorelFileDialog::NameMatchesFilter(LPCSTR Name)
{
	if (!m_TempFilter.isEmpty())
	{
		return Match(Name, m_TempFilter);
	}
	
	if (0 == m_Filter.count())
		return TRUE;
	
	QStrListIterator it(m_Filter);
	BOOL bOdd = TRUE;
	
	for (it.toFirst(); NULL != it.current(); ++it)
	{
		if (bOdd && !strcmp(m_pFileTypeCombo->currentText(), it.current()))
		{
			++it;

			return Match(Name, it.current());
		}
		bOdd = !bOdd;
	}

	return FALSE;
}

void CCorelFileDialog::OnSelchangeFileTypeCombo(int /* nIndex */)
{
	m_pFileNameEdit->setText("");
	m_TempFilter = "";

	Refresh();
}

void CCorelFileDialog::OnFileNameEditReturnPressed()
{
	QString s = m_pFileNameEdit->text();

	if (NULL != strchr(s, '*') ||
			NULL != strchr(s, '?') ||
			(NULL != strchr(s, '[') && NULL != strchr(s, ']')))
	{
		m_bIgnoreNextDone = true;
		m_TempFilter = s;
		Refresh();
	}
	else
		m_TempFilter = "";
}


