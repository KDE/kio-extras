/* Name: CorelFileDialog.h

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

#ifndef CCorelFileDialog_included
#define CCorelFileDialog_included

#include "common.h"
#include "CorelFileDialogData.h"
#include "qstrlist.h"

class CRemoteFileContainer;
class CMSWindowsNetworkItem;
class CNetworkTreeItem;
class QMovie;
class CListView;
class CMyComputerItem;

class CCorelFileDialog : public CCorelFileDialogData
{
	Q_OBJECT

public:

	CCorelFileDialog(QWidget* parent = NULL, 
									 LPCSTR name = NULL);

	CCorelFileDialog(LPCSTR dirName, 
									 LPCSTR filter,
									 QWidget *parent, 
									 LPCSTR name, 
									 bool modal);

	virtual ~CCorelFileDialog();
	
	QString selectedFile() const;
	
	static QString getOpenFileName( const char *initially = 0,
		const char *filter= 0,
		QWidget *parent = 0, const char *name = 0);
	
	static QString getSaveFileName( const char *initially = 0,
		const char *filter= 0,
		QWidget *parent = 0, const char *name = 0);
	
	static QString getOpenFileURL( const char *initially = 0,
		const char *filter= 0,
		QWidget *parent = 0, const char *name = 0);
	
	static QString getSaveFileURL( const char *initially = 0,
		const char *filter= 0,
		QWidget *parent = 0, const char *name = 0);
	
	static QString getExistingDirectory( const char *dir = 0,
		QWidget *parent = 0,
		const char *name = 0 );
	
	static QStrList getOpenFileNames( const char *filter= 0,
		const char * dir = 0,
		QWidget *parent = 0,
		const char *name = 0);

	virtual void show();

	QString m_DirName;
	QStrList m_Filter;
	QString m_TempFilter;

protected slots:
	void OnSelectFile(CListViewItem *pNewItem);
	void OnListviewReturnPressed(CListViewItem *pNewItem);
	void OnAcceptClicked();
	void OnGoParentClicked();
	void OnFileNameEditReturnPressed();
	void OnSelchangeCombo(int nIndex);
	void OnSelchangeFileTypeCombo(int nIndex);

protected:
	void Refresh();
	void Navigate(LPCSTR Path);
	void SetFilter(LPCSTR Filter);
	BOOL NameMatchesFilter(LPCSTR Name);

protected:
	CListView *m_pTree;
	CMSWindowsNetworkItem *m_pNetworkRoot;
	CMyComputerItem *m_pMyComputer;
	QMovie *m_pMovie;
	QLabel *m_pSearchMovieLabel;

	void Init();
	void MovieInit();
	void MovieRun();
	void MovieStop();

	void SetHeaderType(int nType);
	void SetColumnType(int nType);

	/*
	bool CCorelFileDialog::PromptForPassword(CRemoteFileContainer *pItem);
	void CCorelFileDialog::UpdatePathCombo(const char *NewPath, CNetworkTreeItem *pItem);
	void CCorelFileDialog::GoTo(const char *NewPath);
	void CCorelFileDialog::MovieInit();
	void CCorelFileDialog::MovieRun();
	void CCorelFileDialog::MovieStop();
	void CCorelFileDialog::SetHeaderType(CListView *pView, int nType);
	*/
	bool m_bIgnoreNextDone;
};
#endif // CCorelFileDialog_included
