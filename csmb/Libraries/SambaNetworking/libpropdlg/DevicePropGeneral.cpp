/* Name: DevicePropGeneral.cpp

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

////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "propres.h"
#include "DevicePropGeneral.h"

#define Inherited CDevicePropGeneralData

////////////////////////////////////////////////////////////////////////////

CDevicePropGeneral::CDevicePropGeneral
(
  LPCSTR sName,
  LPCSTR sFileName,
  LPCSTR sModel,
  LPCSTR sDriver,
  LPCSTR sMountedOn,
  QPixmap *pPixmap,
  QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	m_IconLabel->setPixmap(*pPixmap);
	
	m_Name->setText(sName);
	
	m_DeviceLabel->setText(LoadString(knDEVICE_COLON));
	m_DriverLabel->setText(LoadString(knDRIVER_COLON));
	m_ModelLabel->setText(LoadString(knMODEL_COLON));

	m_Device->setText(sFileName);
  
  if (NULL == sModel || *sModel == '\0')
  {
    m_ModelLabel->hide();
    m_Model->hide();
  }
  else
    m_Model->setText(sModel);

  if (NULL == sDriver || *sDriver == '\0')
  {
    m_DriverLabel->hide();
    m_Driver->hide();
  }
  else
    m_Driver->setText(sDriver);

  if (NULL == sMountedOn || *sMountedOn == '\0')
  {
    m_MountedOnLabel->hide();
    m_MountedOn->hide();
    m_NotMountedLabel->setText(LoadString(knDEVICE_NOT_MOUNTED));
  }
  else
  {
    m_MountedOnLabel->show();
    m_MountedOn->setText(sMountedOn);
    m_NotMountedLabel->hide();
  }
}

////////////////////////////////////////////////////////////////////////////

CDevicePropGeneral::~CDevicePropGeneral()
{
}

////////////////////////////////////////////////////////////////////////////

