/* Name: FSPropGeneral.h

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

/**********************************************************************

	--- Qt Architect generated file ---

	File: FSPropGeneral.h
	Last generated: Thu Nov 12 05:17:14 1998

 *********************************************************************/

#ifndef CFSPropGeneral_included
#define CFSPropGeneral_included

#include "FSPropGeneralData.h"

class CFileSystemInfo;

class CFSPropGeneral : public CFSPropGeneralData
{
    Q_OBJECT

public:

    CFSPropGeneral
    (
		  CFileSystemInfo *pInfo,
      QWidget* parent = NULL,
      const char* name = NULL
    );

    virtual ~CFSPropGeneral();
};
#endif // CFSPropGeneral_included
