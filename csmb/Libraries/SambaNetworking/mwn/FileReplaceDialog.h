/* Name: FileReplaceDialog.h

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

/**********************************************************************

	--- Qt Architect generated file ---

	File: FileReplaceDialog.h
	Last generated: Mon Jun 28 15:28:06 1999

 *********************************************************************/

#ifndef CFileReplaceDialog_included
#define CFileReplaceDialog_included

#include "FileReplaceDialogData.h"

class CFileReplaceDialog : public CFileReplaceDialogData
{
    Q_OBJECT

public:

    CFileReplaceDialog
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
    );

    virtual ~CFileReplaceDialog();
    virtual void OnYes();
    virtual void OnButton3();
    virtual void OnButton2();
};
#endif // CFileReplaceDialog_included

