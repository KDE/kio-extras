/* Name: FSPropPropGeneral.cpp

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

#include "FSPropGeneral.h"
#include <stdlib.h>
#include "common.h"
#include "filesystem.h"
#include "propres.h"

#define Inherited CFSPropGeneralData

////////////////////////////////////////////////////////////////////////////

CFSPropGeneral::CFSPropGeneral
(
	CFileSystemInfo *pInfo,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
  m_TypeLabel->setText(LoadString(knTYPE_LABEL));
  m_FileSystemTypeLabel->setText(LoadString(knFILESYSTEM_TYPE_LABEL));
  m_MountedFromLabel->setText(LoadString(knMOUNTED_FROM_LABEL));

	switch (pInfo->m_DriveType)
	{
		case keNetworkDrive:
			m_Type->setText(LoadString(knNETWORK_CONNECTION));
		break;

		case keLocalDrive:
		default:
			m_Type->setText(LoadString(knLOCAL_DISK));
		break;

		case keCdromDrive:
			m_Type->setText(LoadString(knCDROM_DISK));

		break;

		case keFloppyDrive:
			m_Type->setText(LoadString(knFLOPPY_DISK));
		break;
	}

	QString s;

	s = pInfo->m_Type.upper();

	if (pInfo->m_bIsReadOnly)
		s += LoadString(knMOUNTED_READ_ONLY);

	m_FileSystemType->setText((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
                                      );

  /* Capacity */
  m_CapacityLabel->setText(LoadString(knCAPACITY));
  m_Capacity1->setText((LPCSTR)SizeBytesFormat(((double)atol((LPCSTR)pInfo->m_TotalK
#ifdef QT_20
  .latin1()
#endif
                                                                              )) * 1024.)
#ifdef QT_20
  .latin1()
#endif
                                                                                          );
	s.sprintf(LoadString(knX_BYTES), (LPCSTR)NumToCommaString(atol((LPCSTR)pInfo->m_TotalK
#ifdef QT_20
  .latin1()
#endif
                                                                                 ) * 1024)
#ifdef QT_20
  .latin1()
#endif
                                                                                           );
  m_Capacity2->setText(s);

  /* Used space */
  m_UsedSpaceLabel->setText(LoadString(knUSED_SPACE));
  m_UsedSpace1->setText((LPCSTR)SizeBytesFormat(((double)atol((LPCSTR)pInfo->m_UsedK
#ifdef QT_20
  .latin1()
#endif
                                                                              )) * 1024.)
#ifdef QT_20
  .latin1()
#endif
                                                                                        );
  s.sprintf(LoadString(knX_BYTES), (LPCSTR)NumToCommaString(atol((LPCSTR)pInfo->m_UsedK
#ifdef QT_20
  .latin1()
#endif
                                                                                ) * 1024)
#ifdef QT_20
  .latin1()
#endif
                                                                                          );
  m_UsedSpace2->setText(s);

  /* Available */
  m_FreeSpaceLabel->setText(LoadString(knAVAILABLE));
  m_FreeSpace1->setText((LPCSTR)SizeBytesFormat(((double)atol((LPCSTR)pInfo->m_AvailableK
#ifdef QT_20
  .latin1()
#endif
                                                                                  )) * 1024.)
#ifdef QT_20
  .latin1()
#endif
                                                                                              );
  s.sprintf(LoadString(knX_BYTES), (LPCSTR)NumToCommaString(atol((LPCSTR)pInfo->m_AvailableK
#ifdef QT_20
  .latin1()
#endif
                                                                                      ) * 1024)
#ifdef QT_20
  .latin1()
#endif
                                                                                                );
  m_FreeSpace2->setText(s);

	s = pInfo->m_Name;

	m_FileSystemPath->setText((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
                                      );

	m_IconLabel->setPixmap(*pInfo->Pixmap(TRUE));
  m_StatusLabel->setText(LoadString(knSTATUS_COLON));
	//m_DriveDiagram->SetPercent(100-atoi(pInfo->m_PercentUsed));

  QPainter p;
  p.begin(this);
  int textw = p.fontMetrics().width("100%") + 2;
  p.end();
  m_Bar->setGeometry(m_Bar->x(), m_Bar->y(), m_Bar->width() + textw, m_Bar->height());

  m_Bar->setProgress(atoi((LPCSTR)pInfo->m_PercentUsed
#ifdef QT_20
  .latin1()
#endif
  ));
}

////////////////////////////////////////////////////////////////////////////

CFSPropGeneral::~CFSPropGeneral()
{
}

////////////////////////////////////////////////////////////////////////////

