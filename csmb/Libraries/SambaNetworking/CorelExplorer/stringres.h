/* Name: stringres.h

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

#ifndef __INC_STRINGRES_H__
#define __INC_STRINGRES_H__

#define APPLICATION_NAME "Corel\xae File Manager"
#define RESOURCE_ARRAY gExpStringList

LPCSTR gExpStringList[] =
{
  "Local intranet zone", // knLOCAL_INTRANET_ZONE
  "Address", // knADDRESS
  "0 object(s)", // kn0_OBJECTS
  "All Folders", // knALL_FOLDERS
  APPLICATION_NAME, // knAPP_TITLE
  "%d object(s) selected", // knNUM_SELECTED_OBJECTS
  "%d object(s)", // knNUM_OBJECTS
  "Disconnect", // knDISCONNECT
  "Mount Networ&k Share...", // knMOUNT_NETWORK_SHARE_2
  "Cu&t", // knCU_T
  "&Copy", // kn_COPY
  "&Paste", // kn_PASTE
  "&Windows Sharing...", // knSHARING
  "P&roperties...", // knP_ROPERTIES
  "Do you want to mount %s next time you log on?", // knDO_YOU_WANT_TO_MOUNT
  "About " APPLICATION_NAME, // knABOUT_TITLE
  APPLICATION_NAME " 1.2\n\nAuthor: Oleg Noskov (olegn@corel.com)\n(C) 1999-2000 Corel Corporation", // knABOUT_BODY
  "Disconnect Network Share", // knDISCONNECT_NETWORK_SHARE
  "%s on %s", // knX_ON_Y
  "Navigation canceled.", // knNAVIGATION_CANCELED
	"Unable to disconnect %s from %s.", // knUNABLE_TO_DISCONNECT_X_FROM_Y
  "Internet zone", // knINTERNET_ZONE
  "Back to %s", // knBACK_TO_X
  "&Delete", // knDELETE
  "Rena&me", // knRENAME
  "New Folder", //knNEW_FOLDER
  "Press <ENTER> to continue...", // knPRESS_ENTER_TO_CONTINUE
  "Unable to unmount %s.\n\n%s", //knUNABLE_TO_UMOUNT_X_Y
  "Access denied.", //knACCESS_DENIED
  "My Home", // knSTR_MY_HOME
  "Show Lar&ge Icons", // knSHOW_LARGE_ICONS
  "Show &Hidden Files", // knSHOW_HIDDEN_FILES
	"O&pen in New Window", // knOPEN_IN_NEW_WINDOW
  "O&pen Frame in New Window", // knOPEN_FRAME_IN_NEW_WINDOW
  "&Back", // kn_BACK
  "&Forward", // kn_FORWARD
  "&Stop", // kn_STOP
  "Create &Nickname", // knCREATE_NICKNAME
  "B&ookmark", // knBOOKMARK
  "S&ave Image...", // knSAVE_IMAGE
  "R&efresh", // knR_EFRESH
	"Open &With ...",  // knOPEN_WITH
  "&Find ...",       // knFIND
  "Do you want to overwrite existing file\n%s?", // knDO_YOU_WANT_TO_OVERWRITE
  "Move to &Trash",  //knMOVE_TO_DUMPSTER
  "E&ject CD",          // knEJECT_CD
  "R&estore", // knRESTORE
  "E&mpty Trash", //knEMPTY_DUMPSTER
  "&New Folder", //knNEW_FOLDER_MENU
  "Cannot open file %s\n(%s.)", // knCANNOT_OPEN_FILE_X
  "CD-ROM", // knCD_ROM
  "CD-ROM %d", // knCD_ROM_X
  "Floppy", // knFLOPPY
  "Floppy %d", //knFLOPPY_X
  "S&ort", //knSORT_MENU
  "by %s", //knSORT_BY_X
  "Copy Here", // knCOPY_HERE
  "Move Here", // knMOVE_HERE
  "Error accessing URL\n%s.", // knERROR_ACCESSING_URL_X
  "", // knEMPTY_MESSAGE
  "Open with:",  // knNOAMP_OPEN_WITH
	"&Unmount", // knUNMOUNT
	"&File", // kn_FILE
	"E&xit", // knE_XIT
  "&Edit", // kn_EDIT
	"Cu&t\tCtrl+X", // knCU_T_CTRL_X
	"&Copy\tCtrl+C", // kn_COPY_CTRL_C
	"&Paste\tCtrl+V", // kn_PASTE_CTRL_V
	"Select &All\tCtrl+A", // knSELECT__ALL
	"&Invert Selection", // kn_INVERT_SELECTION
	"&View", // kn_VIEW
	"&Toolbar", // kn_TOOLBAR
	"&Address Bar", // kn_ADDRESS_BAR
	"&Status Bar", // kn_STATUS_BAR
	"T&ree", // knT_REE
	"&Large Icons", // kn_LARGE_ICONS
	"&Go", // kn_GO
  "Show The S&ystem", // knSHOW_THE_S_YSTEM
	"&Up One Level", // kn_UP_ONE_LEVEL
	"The &System", // knTHE__SYSTEM
  "Windows &Network", // knWINDOWS__NETWORK
  "&Tools", // kn_TOOLS
	"Find &Files or Folders...", // knFIND__FILES_OR_FOLDERS
	"Find &Computer...", // knFIND__COMPUTER
  "&Disconnect Network Share...", // kn_DISCONNECT_NETWORK_SHARE
  "&Help", // kn_HELP
	"&Help Topics", // kn_HELP_TOPICS
	"Cut", // knCUT
	"Copy", // knCOPY
	"Paste", // knPASTE
	"Up", // knUP
	"Move To Trash", //knMOVE_TO_TRASH
	"Stop", // knSTOP
	"Refresh", // knREFRESH
	"User", // knUSER
  "Forward to %s", // knFORWARD_TO_X
  "Back", // knBACK
  "Forward", // knFORWARD
  "Properties", // knPROPERTIES
  "Connection", // knCONNECTION
  "Location", // knLOCATION
  "Manufacturer", // knMANUFACTURER
  "Model", // knMODEL
  "Document", // knDOCUMENT
  "Status", // knSTATUS
  "Job Format" , // knJOB_FORMAT
  "Time Submitted", // knTIME_SUBMITTED
  "Purge", // knPURGE
  "Add Printer...", // knADD_PRINTER_DOTDOTDOT
  "Zip Drive", // knZIP_DRIVE
  "Zip Drive %d", // knZIP_DRIVE_X
  "Open Conso&le Window\tCtrl+t", // kn_OPEN_CONSOLE_WINDOW
  "D&isconnect", // knD_ISCONNECT
  "E&ject",      // knEJECT
  "&Menubar", // kn_MENUBAR
};

#endif /* __INC_STRINGRES_H__ */

