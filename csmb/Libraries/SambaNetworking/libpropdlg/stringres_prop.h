/* Name: stringres_prop.h

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

#ifndef __INC_STRINGRES_PROP_H__
#define __INC_STRINGRES_PROP_H__

#define RESOURCE_ARRAY gPropStringList

LPCSTR gPropStringList[] =
{
	"Comment:", // knCOMMENT_COLON
	"Device:", // knDEVICE_COLON
	"Driver:", // knDRIVER_COLON
	"Model:", // knMODEL_COLON
  "Device is not mounted.", // knDEVICE_NOT_MOUNTED
  "Type:", //knTYPE_LABEL
  "File system type:", //knFILESYSTEM_TYPE_LABEL
  "Mounted from:", //knMOUNTED_FROM_LABEL
  "Network Connection", // knNETWORK_CONNECTION
  "Local Disk", // knLOCAL_DISK
  "CD-ROM Disc", // knCDROM_DISK
  "Floppy Disk", // knFLOPPY_DISK
  ", mounted read-only", //knMOUNTED_READ_ONLY
  "Capacity:", // knCAPACITY
  "(%s bytes)", //knX_BYTES
  "Used space:", // knUSED_SPACE
  "Available:", // knAVAILABLE
  "Status:", //knSTATUS_COLON
	"Full name:", // knFULL_NAME_COLON
	"Size:", // knSIZE_COLON
	"Created:", // knCREATED_COLON
	"Modified:", // knMODIFIED_COLON
	"Accessed:", // knACCESSED_COLON
	"Owner:", // knOWNER_COLON
	"Owner group:", // knOWNER_GROUP_COLON
	"Permissions:", // knPERMISSIONS_COLON
  "%ld files, %ld folders", // knNUM_FILES_FOLDERS
  "Contains:", // knCONTAINS
  "%s (%s bytes)", // knX_Y_BYTES
  "Location:", // knLOCATION_COLON
  "All in %s", // knALL_IN_X
  "Multiple owners", // knMULTIPLE_OWNERS
	"Ownership", // knOWNERSHIP
	"Access permissions", // knACCESS_PERMISSIONS
	"Owner", // knOWNER
	"Group", // knGROUP
	"Others", // knOTHERS
	"Special", // knSPECIAL
	"Set UID", // knSET_UID
	"Set GID", // knSET_GID
	"Sticky", // knSTICKY
  "Read\nentries", //knReadEntries
  "Write\nentries", //knWriteEntries
  "Access\nentries", //knAccessEntries
  "Read", //knREAD
  "Write", //knWrite
  "Exec", //knExec
	"Attributes:", // knATTRIBUTES_COLON
  "&General", // knGENERAL
  "&Permissions", //kn_PERMISSIONS
  "Properties of '%s'", // knPROPERTIES_OF_X
  "%s%s Properties", // knXY_PROPERTIES
  "&Windows Sharing", // knSHARING_2
	"Type:", // knTYPE_COLON
  "Original location:", // knORIGINAL_LOCATION_COLON
	"Deleted:", // knDELETED_COLON
	"Original created:", // knORIGINAL_CREATED_COLON
	"Original modified:", // knORIGINAL_MODIFIED_COLON
	"Domain:", // knDOMAIN_COLON
  "&Advanced", // kn_ADVANCED
  "&Output", // kn_OUTPUT
  "Unable to save printer properties", // knUNABLE_TO_SAVE_PRINTER_PROPERTIES
};

#endif /* STRINGRES_PROP_H__ */
