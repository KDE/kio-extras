/* Name: FileReplaceDialog.cpp

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

#include "FileReplaceDialog.h"
#include "common.h"

#define Inherited CFileReplaceDialogData

CFileReplaceDialog::CFileReplaceDialog
(
	QWidget* parent,
	const char *TopLine,
	const char *Button2,
	const char* Button3,
	const char *File1Text1,
	const char *File1Text2,
	const char *File2Text1,
	const char *File2Text2,
	QPixmap *FileIcon1,
	QPixmap *FileIcon2
)
	:
	Inherited( parent, NULL)
{
	setCaption(LoadString(knCONFIRM_FILE_REPLACE));
	
	m_pLabel2->setText(LoadString(knFILE_REPLACE_DIALOG_TEXT_LABEL2));
	m_pLabel3->setText(LoadString(knFILE_REPLACE_DIALOG_TEXT_LABEL3));
	m_pYesButton->setText(LoadString(kn_YES));
	m_Button2->setText(LoadString(knYES_TO__ALL));
  m_Button3->setText(LoadString(kn_NO));
	m_Cancel->setText(LoadString(knCANCEL));

	m_FileReplaceIconLabel->setPixmap(*LoadPixmap(keReplaceFileIcon));
	
	if (NULL != TopLine)
		m_TopText->setText(TopLine);

	if (NULL != File1Text1)
		m_File1Text1->setText(File1Text1);

	if (NULL != File1Text2)
		m_File1Text2->setText(File1Text2);
	
	if (NULL != File2Text1)
		m_File2Text1->setText(File2Text1);
	
	if (NULL != File2Text2)
		m_File2Text2->setText(File2Text2);

	if (NULL != Button2)
		m_Button2->setText(Button2);

	if (NULL != Button3)
		m_Button3->setText(Button3);
	
	if (NULL != FileIcon1)
		m_FileIcon1->setPixmap(*FileIcon1);
	
	if (NULL != FileIcon2)
		m_FileIcon2->setPixmap(*FileIcon2);
}


CFileReplaceDialog::~CFileReplaceDialog()
{
}


void CFileReplaceDialog::OnYes()
{
	done(1);
}
   
void CFileReplaceDialog::OnButton2()
{
	done(2);
}
    
void CFileReplaceDialog::OnButton3()
{
	done(3);
}


