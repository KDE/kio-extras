/* Name: SharingPageNFS.cpp

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


/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#include "SharingPageNFS.h"
#include "common.h"

#include "header.h"

#include "ExportsEntryDialog.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h> // for mktemp()
#include <unistd.h> // for getuid(), getgid(), unlink() etc.

#define Inherited CSharingPageNFSData

static struct _MAPPING
{
	LPCSTR m_Option;
	int m_StringID;
} AccessOptions[] =
{
	{ "ro", knSTR_READ },
	{ "rw", knSTR_FULL_CONTROL },
	{	"noaccess", knSTR_NO_ACCESS }
};

///////////////////////////////////////////////////////////////////////////////

CSharingPageNFS::CSharingPageNFS
(
	LPCSTR Path,
  QPixmap *pPixmap,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	m_bChanged = FALSE;

	m_PathLabel->setText(Path);
  m_IconLabel->setPixmap(*pPixmap);
	
	if (NULL != strchr(Path, ' '))
	{
		m_pAddButton->hide();
		m_pDeleteButton->hide();
		m_pEditButton->hide();
		m_pListView->hide();
		m_pIsSharedCheckbox->hide();
		QLabel *l = new QLabel(this, "Warning");
		l->setGeometry(m_pIsSharedCheckbox->x(), m_pIsSharedCheckbox->y(), m_pListView->width(), m_pListView->height());
		l->setFocusPolicy( QWidget::NoFocus );
		l->setBackgroundMode(QWidget::PaletteBackground);
		l->setFrameStyle(0);
		l->setText(LoadString(knNO_SPACE_IN_NFS));
		l->setAlignment(AlignHCenter | AlignTop);

		l->show();
	}
	else
	{
		m_pAddButton->setText(LoadString(kn_ADD_DOTDOTDOT));
		m_pDeleteButton->setText(LoadString(kn_REMOVE));
		m_pEditButton->setText(LoadString(kn_EDIT_DOTDOTDOT));
		m_pIsSharedCheckbox->setText(LoadString(knSHARE_THIS_ITEM_AND_ITS_CONTENTS));
		m_pListView->setFocusPolicy( QWidget::StrongFocus );
		m_pListView->setFrameStyle( 50 );
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
		m_pListView->setAllColumnsShowFocus(true);
		m_pListView->addColumn(LoadString(knHOST), m_pListView->viewport()->width()/3);
		m_pListView->setColumnAlignment( 0, 1 );
		m_pListView->addColumn(LoadString(knPERMISSIONS), m_pListView->viewport()->width()/3);
		m_pListView->addColumn(LoadString(knOPTIONS), m_pListView->viewport()->width()/3);
		PopulateListView();
		
		m_bIsShared = (m_pListView->childCount() > 0);
		m_pIsSharedCheckbox->setChecked(m_bIsShared);
		OnIsShared();
	}
}

///////////////////////////////////////////////////////////////////////////////

CSharingPageNFS::~CSharingPageNFS()
{
}

///////////////////////////////////////////////////////////////////////////////

bool CSharingPageNFS::Apply()
{
	if (!m_pAddButton->isVisible())
	{
		return TRUE;
	}

	if (!m_pIsSharedCheckbox->isChecked())
	{
		if (!m_bIsShared)
			return TRUE; // wasn't shared and isn't now: just exit
	}
	else
	{
		if (m_bIsShared && !m_bChanged)
			return TRUE; // was shared and we didn't change anything: just exit
	}
	
  QString s(m_PathLabel->text());
	bool bEmpty = true;

	if (m_pIsSharedCheckbox->isChecked())
	{
		CListViewItem *pItem;
	
		for (pItem = m_pListView->firstChild(); NULL != pItem; pItem = pItem->nextSibling())
		{
			QString Hostname(pItem->text(0));
			QString AccessType(pItem->text(1));
			QString Options(pItem->text(2));
	
			for (int i = (int)(sizeof(AccessOptions)/sizeof(struct _MAPPING)) - 1; 
					 i >= 0;
					 i--)
			{
				if (!strcmp(LoadString(AccessOptions[i].m_StringID), (LPCSTR)AccessType))
				{
					if (!Options.isEmpty())
						Options += ",";
					
					Options += AccessOptions[i].m_Option;
					
					break;
				}
			}
			
			// s += bEmpty ? " " : ",";
			s += " ";
			s += Hostname;
			s += "(";
			s	+= Options;
			s += ")";
			bEmpty = false;
		}
	}

	LPCSTR FileName = "/etc/exports";

	FILE *fi = fopen(FileName, "r");

	char TmpFileName[] = "/tmp/~~exports~XXXXXX";
	mktemp(TmpFileName);

	FILE *fo = fopen((LPCSTR)TmpFileName, "w");

	if (NULL == fo)
	{
		if (NULL != fi)
				fclose(fi);
		
		return FALSE;
	}
	
	int nPathLen = strlen(m_PathLabel->text());
	bool bDone = false;

	if (NULL != fi)
	{
		fseek(fi, 0L, 2);
		long nSize = ftell(fi);
		fseek(fi, 0L, 0);
		
		char *pBuf = new char[nSize+1];

		while (!feof(fi))
		{
			*pBuf = '\0';
			fgets(pBuf, nSize+1, fi);
			
			if (feof(fi) && !strlen(pBuf))
				break;

			int nLen = strlen(pBuf);

			if (nLen > 0 && pBuf[--nLen] == '\n')
				pBuf[nLen] = '\0';
				 
      while (!feof(fi) && pBuf[strlen(pBuf)-1] == '\\')
			{
				fgets(pBuf+strlen(pBuf)-1, nSize+1, fi);
				
				if (pBuf[strlen(pBuf)-1] == '\n')
					pBuf[strlen(pBuf)-1] = '\0';
			}
			
			if ((int)strlen(pBuf) > nPathLen &&
					!strncmp(pBuf, m_PathLabel->text(), nPathLen))
			{
				if (!bEmpty)
					fprintf(fo, "%s\n", (LPCSTR)s);

				bDone = true;
			}
			else
			{
				fprintf(fo, "%s\n", pBuf);
			}
		}

		delete []pBuf;
		fclose(fi);
	}

	if (!bDone)
	{
		fprintf(fo, "%s\n", (LPCSTR)s);
	}
	
	fclose(fo);

	struct stat st;
	uid_t uid = getuid();
	gid_t gid = getgid();

	if (!stat(FileName, &st))
	{
		uid = st.st_uid;
		gid = st.st_gid;
	}

	QString cmd;

	cmd.sprintf("mv -f %s %s;chown %u.%u %s;/etc/init.d/nfs-server restart;sleep 1", 
							(LPCSTR)TmpFileName, 
							(LPCSTR)FileName, 
							uid, 
							gid, 
							(LPCSTR)FileName);

	if (SuperUserExecute("Save NFS sharing settings", cmd, NULL, 0))
	{
		::unlink(TmpFileName);
		return FALSE; // unable to update file
	}
  
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void CSharingPageNFS::OnIsShared()
{
	BOOL bEnable = m_pIsSharedCheckbox->isChecked();

	m_pAddButton->setEnabled(bEnable);
	m_pDeleteButton->setEnabled(bEnable);
	m_pEditButton->setEnabled(bEnable);
	SetListViewEnabled(bEnable);

	if (bEnable && !m_pListView->childCount())
	{
		new CListViewItem(m_pListView, "", LoadString(knSTR_READ), "");
		m_pListView->setSelected(m_pListView->firstChild(), TRUE);
	}
}

///////////////////////////////////////////////////////////////////////////////

void CSharingPageNFS::SetListViewEnabled(bool bEnable)
{
	m_pListView->setEnabled(bEnable);
	m_pListView->header()->setEnabled(bEnable);
	
	if (NULL != m_pListView->currentItem())
		m_pListView->setSelected(m_pListView->currentItem(), bEnable);
	
	const QColorGroup &CG = colorGroup();
	
	QColor NewBase(bEnable ? CG.base() : CG.midlight());
	QColor NewText(bEnable ? CG.text() : CG.dark());
	QColorGroup cg(CG.foreground(), CG.background(), CG.light(), CG.dark(), CG.midlight(), NewText, NewBase);
	QPalette pal(cg,cg,cg);

	m_pListView->setPalette(pal);
	m_pListView->repaint();
}

///////////////////////////////////////////////////////////////////////////////

static QString GetAccessType(QString &Options)
{
	LPCSTR retval = NULL;

	LPCSTR p = (LPCSTR)Options;
	QString RetOptions;

	while (NULL != p)
	{
		QString s(ExtractWord(p, ", ").stripWhiteSpace());
		
		if (s.isEmpty())
			break;
		
		int i;
		
		for (i = (int)(sizeof(AccessOptions)/sizeof(struct _MAPPING)) - 1; 
				 i >= 0;
         i--)
		{
			if (!strcmp(AccessOptions[i].m_Option, (LPCSTR)s))
			{
				retval = LoadString(AccessOptions[i].m_StringID);
				break;
			}
		}

		if (-1 == i)
		{
			if (!RetOptions.isEmpty())
				RetOptions += ", ";
			
			RetOptions += s;
		}
	}

	if (NULL == retval)
		retval = LoadString(knSTR_FULL_CONTROL);

	Options = RetOptions;
	return retval;
}

///////////////////////////////////////////////////////////////////////////////

void CSharingPageNFS::PopulateListView()
{
	FILE *f = fopen("/etc/exports", "r");

	if (NULL != f)
	{
		fseek(f,0L,2);
		long nSize = ftell(f);
		fseek(f,0L,0);
		char *pBuf = new char[nSize+1];
		
		while (!feof(f))
		{
			fgets(pBuf, nSize+1, f);
			
			if (feof(f))
				break;
	
			LPCSTR p = pBuf;
			
			if (*pBuf == '#' || *pBuf == '\n' || *pBuf == '\0')
				continue;
	
			int nLen = strlen(pBuf);

			if (nLen > 0 && pBuf[--nLen] == '\n')
				pBuf[nLen] = '\0';
				 
      while (!feof(f) && pBuf[strlen(pBuf)-1] == '\\')
			{
				fgets(pBuf+strlen(pBuf)-1, nSize+1, f);
				
				if (pBuf[strlen(pBuf)-1] == '\n')
					pBuf[strlen(pBuf)-1] = '\0';
			}
			
			// Strip any leading whitespace characters
			
			while (*p != '\0' && (*p == (char)9 || *p == (char)10 || *p == (char)11 || *p == (char)12 || *p == (char)13 || *p == (char)32))
				p++;
	   
			if (*p == '\0')
				continue;
			
			QString ShareName(ExtractWord(p));
	
			if (ShareName == m_PathLabel->text())
			{
				while (NULL != p && *p != '\0' && *p != '\n')
				{
					QString Hostname(ExtractWord(p, "(,").stripWhiteSpace());
					QString Options;
	
					if (*p == '(')
					{
						p++;

						Options = ExtractWord(p, ")").lower();
						
						if (*p == ')')
							p++;
					}
	
					QString AccessType(GetAccessType(Options));
					new CListViewItem(m_pListView, 
														(LPCSTR)Hostname, 
														(LPCSTR)AccessType,
														Options);
				}
			}
		}
		
    if (m_pListView->childCount())
			m_pListView->setSelected(m_pListView->firstChild(), TRUE);
		
		delete []pBuf;
		fclose(f);
	}
}

///////////////////////////////////////////////////////////////////////////////

void CSharingPageNFS::OnAdd()
{
	CExportsEntryDialog dlg;
	
	if (QDialog::Accepted == dlg.exec())
	{
		new CListViewItem(m_pListView, 
											(LPCSTR)dlg.m_Hostname, 
											(LPCSTR)dlg.m_AccessType,
											(LPCSTR)dlg.m_Options);
		
		m_bChanged = TRUE;
	}
}

///////////////////////////////////////////////////////////////////////////////

void CSharingPageNFS::OnEdit()
{
	CListViewItem *pItem = m_pListView->currentItem();

	if (NULL != pItem)
	{
		CExportsEntryDialog dlg(pItem->text(0), pItem->text(1), pItem->text(2));
		
		if (QDialog::Accepted == dlg.exec())
		{
			pItem->setText(0, dlg.m_Hostname);
			pItem->setText(1, dlg.m_AccessType);
			pItem->setText(2, dlg.m_Options);
			m_bChanged = TRUE;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void CSharingPageNFS::OnRemove()
{
	CListViewItem *pItem = m_pListView->currentItem();
	
	if (NULL != pItem)
	{
		m_bChanged = TRUE;

		delete pItem;
		
		if (!m_pListView->childCount() &&
				m_pIsSharedCheckbox->isChecked())
		{
			m_pIsSharedCheckbox->setChecked(FALSE);
			OnIsShared();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

