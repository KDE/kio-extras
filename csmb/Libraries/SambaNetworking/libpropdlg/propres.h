/* Name: DevicePropGeneral.h

   Description: This file is a part of the Corel File Manager application.

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

#ifndef __INC_PROPRES_H__
#define __INC_PROPRES_H__

#define PROP_RESOURCE_START 3000

enum PROP_Strings
{
  knCOMMENT_COLON = PROP_RESOURCE_START,
  knDEVICE_COLON,
  knDRIVER_COLON,
  knMODEL_COLON,
  knDEVICE_NOT_MOUNTED,
  knTYPE_LABEL,
  knFILESYSTEM_TYPE_LABEL,
  knMOUNTED_FROM_LABEL,
  knNETWORK_CONNECTION,
  knLOCAL_DISK,
  knCDROM_DISK,
  knFLOPPY_DISK,
  knMOUNTED_READ_ONLY,
  knCAPACITY,
  knX_BYTES,
  knUSED_SPACE,
  knAVAILABLE,
  knSTATUS_COLON,
  knFULL_NAME_COLON,
  knSIZE_COLON,
  knCREATED_COLON,
  knMODIFIED_COLON,
  knACCESSED_COLON,
  knOWNER_COLON,
  knOWNER_GROUP_COLON,
  knPERMISSIONS_COLON,
  knNUM_FILES_FOLDERS,
  knCONTAINS,
  knX_Y_BYTES,
  knLOCATION_COLON,
  knALL_IN_X,
  knMULTIPLE_OWNERS,
  knOWNERSHIP,
  knACCESS_PERMISSIONS,
  knOWNER,
  knGROUP,
  knOTHERS,
  knSPECIAL,
  knSET_UID,
  knSET_GID,
  knSTICKY,
  knREAD_ENTRIES,
  knWRITE_ENTRIES,
  knACCESS_ENTRIES,
  knREAD,
  knWRITE,
  knEXEC,
  knATTRIBUTES_COLON,
  knGENERAL,
  kn_PERMISSIONS,
  knPROPERTIES_OF_X,
  knXY_PROPERTIES,
  knSHARING_2,
  knTYPE_COLON,
  knORIGINAL_LOCATION_COLON,
  knDELETED_COLON,
  knORIGINAL_CREATED_COLON,
  knORIGINAL_MODIFIED_COLON,
  knDOMAIN_COLON,
  kn_ADVANCED,
  kn_OUTPUT,
  knUNABLE_TO_SAVE_PRINTER_PROPERTIES
};

void PropInitResources();

#endif /* __INC_PROPRES_H__ */

