/* Name: explres.h

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

#ifndef __INC_EXPLRES_H__
#define __INC_EXPLRES_H__

#include "resource.h"

/* Common data structures */

typedef struct
{
	int m_nItemID;
	int m_nItemLabelID;
	LPCSTR m_pSlot;
} CMenuItemInit;

typedef struct
{
	QPixmap **m_ppIcon;
	QPixmap **m_ppActiveIcon;
	QPixmap **m_ppDisabledIcon;
	int m_nTextID;
	const char *m_pSignal;
} CButtonInfo;

extern int gButtonInfoSize;
extern int gMainWindowButtonInfoSize;
extern int gRightPanelButtonInfoSize;

extern CButtonInfo gButtonInfo[];
extern CButtonInfo gMainWindowButtonInfo[];
extern CButtonInfo gRightPanelButtonInfo[];

extern QString gToolBarActionsNames [];
extern QString gMainWindowToolBarActionsNames [];
extern QString gRightPanelToolBarActionsNames [];

/* Resource IDs */

#define knMENU_SEPARATOR 0

#define knMENU_FILE 100
#define knMENU_FILE_NEW 101
#define knMENU_FILE_DELETE 102
#define knMENU_FILE_RENAME 103
#define knMENU_FILE_PROPERTIES 104
#define knMENU_FILE_EXIT 105
#define knMENU_FILE_RESTORE 106
#define knMENU_FILE_EMPTY_DUMPSTER 107
#define knMENU_FILE_NUKE 108

#define knMENU_EDIT 200
#define knMENU_EDIT_UNDO 201
#define knMENU_EDIT_CUT 202
#define knMENU_EDIT_COPY 203
#define knMENU_EDIT_PASTE 204
#define knMENU_EDIT_PASTE_SHORTCUT 205
#define knMENU_EDIT_SELECT_ALL 206
#define knMENU_EDIT_INVERT_SELECTION 207

#define knMENU_VIEW 300
#define knMENU_VIEW_TOOLBARS 301
#define knMENU_VIEW_STATUS_BAR 302
#define knMENU_VIEW_TREE 303
#define knMENU_VIEW_ADDRESS_BAR 304
#define knMENU_SHOW_HIDDEN_FILES 305
#define knMENU_VIEW_LARGE_ICONS 306
#define knMENU_VIEW_MYCOMPUTER 307
#define knMENU_VIEW_SORT 308
#define knMENU_VIEW_STOP 309
#define knMENU_VIEW_REFRESH 310
#define knMENU_VIEW_MENUBARS 311

#define knMENU_GO 400
#define knMENU_GO_BACK 401
#define knMENU_GO_FORWARD 402
#define knMENU_GO_UPONELEVEL 403
#define knMENU_GO_MYCOMPUTER 404
#define knMENU_GO_NETWORKNEIGHBORHOOD 405

#define knMENU_FAVORITES 500

#define knMENU_TOOLS 600
#define knMENU_TOOLS_FIND_FILES 601
#define knMENU_TOOLS_FIND_COMPUTER 602
#define knMENU_MOUNT_SHARE 603
#define knMENU_DISCONNECT_SHARE 604
#define knMENU_TOOLS_CONSOLE 605

#define knMENU_HELP 700
#define knMENU_HELP_TOPICS 701
#define knMENU_HELP_ABOUT 702

#define CFM_RESOURCE_START 2000

enum CFM_Strings
{
  knLOCAL_INTRANET_ZONE = CFM_RESOURCE_START,
  knADDRESS,
  kn0_OBJECTS,
  knALL_FOLDERS,
  knAPP_TITLE,
  knNUM_SELECTED_OBJECTS,
  knNUM_OBJECTS,
  knDISCONNECT,
  knMOUNT_NETWORK_SHARE_2,
  knCU_T,
  kn_COPY,
  kn_PASTE,
  knSHARING,
  knP_ROPERTIES,
  knDO_YOU_WANT_TO_MOUNT,
  knABOUT_TITLE,
  knABOUT_BODY,
  knDISCONNECT_NETWORK_SHARE,
  knX_ON_Y,
  knNAVIGATION_CANCELED,
  knUNABLE_TO_DISCONNECT_X_FROM_Y,
  knINTERNET_ZONE,
  knBACK_TO_X,
  knDELETE,
  knRENAME,
  knNEW_FOLDER,
  knPRESS_ENTER_TO_CONTINUE,
  knUNABLE_TO_UMOUNT_X_Y,
  knACCESS_DENIED,
  knSTR_MY_HOME,
  knSHOW_LARGE_ICONS,
  knSHOW_HIDDEN_FILES,
  knOPEN_IN_NEW_WINDOW,
  knOPEN_FRAME_IN_NEW_WINDOW,
  kn_BACK,
  kn_FORWARD,
  kn_STOP,
  knCREATE_NICKNAME,
  knBOOKMARK,
  knSAVE_IMAGE,
  knR_EFRESH,
  knOPEN_WITH,
  knFIND,
  knDO_YOU_WANT_TO_OVERWRITE,
  knMOVE_TO_DUMPSTER,
  knEJECT_CD,
  knRESTORE,
  knEMPTY_DUMPSTER,
  knNEW_FOLDER_MENU,
  knCANNOT_OPEN_FILE_X,
  knCD_ROM,
  knCD_ROM_X,
  knFLOPPY,
  knFLOPPY_X,
  knSORT_MENU,
  knSORT_BY_X,
  knCOPY_HERE,
  knMOVE_HERE,
  knERROR_ACCESSING_URL_X,
  knEMPTY_MESSAGE,
  knNOAMP_OPEN_WITH,
  knUNMOUNT,
  kn_FILE,
  knE_XIT,
  kn_EDIT,
  knCU_T_CTRL_X,
  kn_COPY_CTRL_C,
  kn_PASTE_CTRL_V,
  knSELECT__ALL,
  kn_INVERT_SELECTION,
  kn_VIEW,
  kn_TOOLBAR,
  kn_ADDRESS_BAR,
  kn_STATUS_BAR,
  knT_REE,
  kn_LARGE_ICONS,
  kn_GO,
  knSHOW_THE_S_YSTEM,
  kn_UP_ONE_LEVEL,
  knTHE__SYSTEM,
  knWINDOWS__NETWORK,
  kn_TOOLS,
  knFIND__FILES_OR_FOLDERS,
  knFIND__COMPUTER,
  kn_DISCONNECT_NETWORK_SHARE,
  kn_HELP,
  kn_HELP_TOPICS,
  knCUT,
  knCOPY,
  knPASTE,
  knUP,
  knMOVE_TO_TRASH,
  knSTOP,
  knREFRESH,
  knUSER,
  knFORWARD_TO_X,
  knBACK,
  knFORWARD,
  knPROPERTIES,
  knCONNECTION,
  knLOCATION,
  knMANUFACTURER,
  knMODEL,
  knDOCUMENT,
  knSTATUS,
  knJOB_FORMAT,
  knTIME_SUBMITTED,
  knPURGE,
  knADD_PRINTER_DOTDOTDOT,
  knZIP_DRIVE,
  knZIP_DRIVE_X,
  kn_OPEN_CONSOLE_WINDOW,
  knD_ISCONNECT,
	knEJECT,
	kn_MENUBAR
};

extern CMenuItemInit gMainMenu[];
extern int gMainMenuItemInitSize;

extern CMenuItemInit gMainWindowMenu[];
extern int gMainWindowMenuSize;

extern CMenuItemInit gRightPanelMenu[];
extern int gRightPanelMenuSize;

extern QString gActionsNames[];
extern QString gMainWindowActions[];
extern QString gRightPanelActions[];

extern QPixmap *gpCopyIcon;
extern QPixmap *gpCopyDisabledIcon;
extern QPixmap *gpCopyActiveIcon;

extern QPixmap *gpCutIcon;
extern QPixmap *gpCutDisabledIcon;
extern QPixmap *gpCutActiveIcon;

extern QPixmap *gpPasteIcon;
extern QPixmap *gpPasteDisabledIcon;
extern QPixmap *gpPasteActiveIcon;

extern QPixmap *gpDeleteIcon;
extern QPixmap *gpDeleteDisabledIcon;
extern QPixmap *gpDeleteActiveIcon;

extern QPixmap *gpUndoIcon;
extern QPixmap *gpUndoDisabledIcon;
extern QPixmap *gpUndoActiveIcon;

extern QPixmap *gpPropsIcon;
extern QPixmap *gpPropsDisabledIcon;
extern QPixmap *gpPropsActiveIcon;

extern QPixmap *gpViewsIcon;
extern QPixmap *gpViewsActiveIcon;

extern QPixmap *gpStopIcon;
extern QPixmap *gpStopActiveIcon;

extern QPixmap *gpRefreshIcon;
extern QPixmap *gpRefreshActiveIcon;
extern QPixmap *gpRefreshDisabledIcon;

extern QPixmap *gpSearchIcon;
extern QPixmap *gpSearchActiveIcon;

extern QPixmap *gpGoParentActiveIcon;
extern QPixmap *gpGoParentIcon;
extern QPixmap *gpGoParentDisabledIcon;

extern QPixmap *gpGoBackActiveIcon;
extern QPixmap *gpGoBackIcon;
extern QPixmap *gpGoBackDisabledIcon;

extern QPixmap *gpGoForwardIcon;
extern QPixmap *gpGoForwardActiveIcon;
extern QPixmap *gpGoForwardDisabledIcon;

void ExpInitResources();

#define kHELP_LOCATION "cs_help_cont_fileman.htm"

#endif /* __INC_EXPLRES_H__ */
