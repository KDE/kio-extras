/* Name: FilePropGeneral.h

   Description: This file is a part of the Corel File Manager application.

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

#ifndef CFilePropGeneral_included
#define CFilePropGeneral_included

#include "common.h"
#include "FilePropGeneralData.h"
#include <qlist.h>

class CLocalFileItem;
class CNetworkTreeItem;

class CFilePropGeneral : public CFilePropGeneralData
{
  Q_OBJECT

public:

  CFilePropGeneral
  (
    QList<CNetworkTreeItem>& ItemList,
    QWidget* parent = NULL,
    const char* name = NULL
  );

  CFilePropGeneral::CFilePropGeneral
  (
    LPCSTR URL,
    struct stat &st,
  	QWidget* parent,
  	const char* name
  );

  virtual ~CFilePropGeneral();
  
  QPixmap *Pixmap()
  {
    return m_pPixmap;
  }

private:
  //QList<CNetworkTreeItem>& m_ItemList;
  QPixmap *m_pPixmap;
};

#endif // CFilePropGeneral_included

