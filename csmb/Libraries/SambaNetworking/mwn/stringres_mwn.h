/* Name: string_res.cpp

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

#ifndef __INC_STRINGRES_MWN_H__
#define __INC_STRINGRES_MWN_H__

LPCSTR gStringList[] =
{
	/* 0 */"New Share",	   // knSTR_NEW_SHARE
	/* 1 */"The share name %s already exists for this resource.\nPlease  choose another share name.", // knSTR_SHARE_DUP
	/* 2 */"You are already sharing %s using the name %s.\nDo you want to  share %s using the name %s instead?", // knSTR_SHARE_DUP2
	/* 3 */"Access Through Share Permissions", // knSTR_ACCESS_THROUGH_SHARE
	/* 4 */"Full Control", // knSTR_FULL_CONTROL
	/* 5 */"No Access", // knSTR_NO_ACCESS
	/* 6 */"Read", // knSTR_READ
	/* 7 */"Everyone", // knSTR_EVERYONE
	/* 8 */"Mount Network Share", // knMOUNT_NETWORK_SHARE
	/* 9 */"Browse For Folder", // knBROWSE_FOR_FOLDER
	/*10 */"Unable to mount at %s.\nThe folder does not exist or you have\nnot enough privileges to mount here.", // knUNABLE_TO_MOUNT
	/*11 */"Mount Point", // knMOUNT_POINT
	/*12 */"Share Name", // knUNC_PATH
	/*13 */"Please enter a value for UNC path.", // knENTER_UNC_PATH
	/*14 */"UNC path you have entered is invalid.\nPlease enter the correct value.", // knBAD_UNC_PATH
	/*15 */"Yes", // knYES
	/*16 */"No", // knNO
	/*17 */"Incorrect password or unknown username for:", // knBAD_PASSWORD
	/*18 */"Connect As:", // knCONNECT_AS
	/*19 */"&Password:", // knPASSWORD
	/*20 */"OK", // knOK
	/*21 */"Cancel", // knCANCEL
	/*22 */"Enter Network Password", // knENTER_NETWORK_PASSWORD
	/*23 */"Add Users and Groups", // knADD_USERS_AND_GROUPS
	/*24 */"Please enter a value for user or group name.", // knENTER_USER_GROUP_NAME
	/*25 */"The share name \"%s\" is too long.\nPlease enter the name no longer than 13 characters", //knSTR_SHARE_TOOLONG
	/*26 */"Please enter a value for mount point.", // knENTER_MOUNT_POINT
	/*27 */"Please enter a value for share name.", // knENTER_SHARE_NAME
	/*28 */"Preparing To Copy...", // knSTR_PREPARING_TO_COPY
	/*29 */"Preparing To Move...", // knSTR_PREPARING_TO_MOVE
	/*30 */"Copying...", // knSTR_COPYING
	/*31 */"Moving...", // knSTR_MOVING
	/*32 */"File Move", // knSTR_FILE_MOVE
	/*33 */"File Copy", // knSTR_FILE_COPY
	/*34 */"Unable to open file %s for reading", // knSTR_UNABLE_TO_OPEN_FOR_READING
	/*35 */"Unable to open file %s for writing", // knSTR_UNABLE_TO_OPEN_FOR_WRITING
	/*36 */"Error writing file %s", // knSTR_ERROR_WRITING_FILE
	/*37 */"Error reading file %s", // knSTR_ERROR_READING_FILE
	/*38 */"Unable to remove file %s", // knSTR_UNABLE_TO_REMOVE_FILE
	/*39 */"You have no permissions to use %s", // knSTR_NO_PERMISSIONS
	/*40 */"Directory %s doesn't exist.\nDo you want to create it?", // knSTR_CONFIRM_CREATE_DIRECTORY
	/*41 */"Directory", // knSTR_DIRECTORY
	/*42 */"Unable to create %s.", // knSTR_UNABLE_TO_CREATE
	/*43 */"Folder \'%s\'\nalready contains file named \'%s\'.", // knSTR_DESTINATION_FILE_EXISTS
	/*44 */"Yes All", // knSTR_YES_ALL
	/*45 */"No All", // knSTR_NO_ALL
	/*46 */"Retry", // knSTR_RETRY
	/*47 */"Done", // knSTR_DONE
	/*48 */"Remaining", // knSTR_REMAINING
	/*49 */"Seconds Remaining", // knSTR_SECONDS_REMAINING
	/*50 */"Unable to establish FTP session to %s:\n%s", // knSTR_UNABLE_TO_ESTABLISH_FTP_SESSION
	/*51 */"A connection with the server cannot be established", // knSTR_SERVER_NOT_FOUND
	/*52 */"A filename cannot contain any of the following characters:\n\\ / : * ? \" < > |", // knFILENAME_CANNOT_CONTAIN
	/*53 */"Rename", // knSTR_RENAME
	/*54 */"You must type a filename.", // knSTR_YOU_MUST_TYPE_A_FILENAME
	/*55 */"Unable to rename", // knUNABLE_TO_RENAME
	/*56 */"Unable to rename %s:\nfile no longer exists in this location.", // knUNABLE_TO_RENAME_X_NOFILE
	/*57 */"%s\n\nThe network path was not found.", // knX_NETWORK_PATH_NOT_FOUND
	/*58 */"Building file list: %ld files, %lu folders, %s", //knBUILDING_FILE_LIST
	/*59 */"Confirm File Replace", // knCONFIRM_FILE_REPLACE
	/*60 */"modified on %s", //knMODIFIED_ON_XXX
	/*61 */"Deleting...", //knSTR_DELETING
	/*62 */"Preparing To Delete...",	//knSTR_PREPARING_TO_DELETE
	/*63 */"Confirm Multiple File Delete", //knCONFIRM_MULTIPLE_FILE_DELETE
	/*64 */"Confirm Folder Delete", //knCONFIRM_FOLDER_DELETE
	/*65 */"Confirm Folder Replace", //knCONFIRM_FOLDER_REPLACE
	/*66 */"Folder \'%s\'\nalready contains a folder named \'%s\'.\n\nIf the files in the existing folder have the same name as files in the folder you\nare moving, they will be replaced. Do you still want to move the folder?", //knSTR_DESTINATION_FOLDER_EXISTS
	/*67 */"Windows Network", //knSTR_WINDOWS_NETWORK
	/*68 */"Are you sure you want to delete \'%s\'?", //knCONFIRM_FILE_DELETE_QUESTION
	/*69 */"Are you sure you want to delete these %d items?", //knCONFIRM_MULTIPLE_FILE_DELETE_QUESTION
	/*70 */"Confirm File Delete", //knCONFIRM_FILE_DELETE
	/*71 */"Are you sure you want to remove the folder \'%s\' and all its contents?", //knCONFIRM_FOLDER_DELETE_QUESTION
	/*72 */"Unable to delete", //knUNABLE_TO_DELETE
	/*73 */"Unable to delete %s:\nfile no longer exists in this location.", //knUNABLE_TO_DELETE_X_NOFILE
	/*74 */"Delete", //knSTR_DELETE
	/*75 */"Path \'%s\'\npoints outside your accessible address space.", //knEFAULT
	/*76 */"\'%s\': permission denied.", //knEACCES
	/*77 */"\'%s\':\nfile name is too long.", //knENAMETOOLONG
	/*78 */"\'%s\':\nfile no longer exists.", // knENOENT
	/*79 */"\'%s\':\ninvalid path.", //knENOTDIR
	/*80 */"File \'%s\' is a directory.",// knEISDIR
	/*81 */"\'%s\':\nInsufficient kernel memory was available to complete the request.", //knENOMEM
	/*82 */"File \'%s\"\nis on a read-only filesystem.", //knEROFS
	/*83 */"Too many symbolic links were encountered in translating\n\'%s\'.", //knELOOP
	/*84 */"An I/O error occured while accessing\n\'%s\'.", //knEIO
	/*85 */"NFS Network", //knSTR_NFS_NETWORK
	/*86 */"Cannot rename %s.\nA file named %s already exists.\nPlease specify a different filename.", // knCANNOT_RENAME_X_Y
	/*87 */"Error Renaming File", // knERROR_RENAMING_FILE
	/*88 */"Unable to rename file %s.", // knUNABLE_TO_RENAME_FILE_X
	/*89 */"This operation requires super-user access rights.", //knSUPER_USER_RIGHTS
	/*90 */"The password you have entered is not valid.\nPlease enter a super-user password or press 'Cancel'.", //knPASSWORD_INVALID
	/*91 */"Unable to mount %s.\n\n%s", //knUNABLE_TO_MOUNT_X_Y
	/*92 */"Unable to access %s.\nMount failed.", //knMOUNT_X_FAILED
	/*93 */"Are you sure you want to move the folder\n\'%s\' and all its contents to Trash?", //knCONFIRM_FOLDER_TO_TRASH_QUESTION
	/*94 */"Are you sure you want to move \'%s\' to Trash?", //knCONFIRM_FILE_TO_TRASH_QUESTION
	/*95 */"Cannot move %s: The destination folder is the same as the source folder.", // knDESTINATION_FOLDER_SAME
	/*96 */"Cannot copy/move %s: The destination folder is a subfolder of the source folder.", // knDESTINATION_FOLDER_SUBTREE
	/*97 */"Directory \'%s\' is not empty.", // keNOTEMPTY
  /*98 */"Unable to access \'%s\': file is in use by another process.", // keTXTBSY
  /*99 */"Unable to create new folder.", // knUNABLE_TO_CREATE_FOLDER
  /*100*/"Create New Folder", //knCREATE_NEW_FOLDER
  /*101*/"Remote server refused connection.", //knECONNREFUSED        
  /*102*/"\n\nReason given by server:\n", //knREASON_GIVEN_BY_SERVER
	/*103*/"Are you sure you want to move these %d items to Trash?", //knCONFIRM_MULTIPLE_FILE_TO_DUMPSTER_QUESTION
  /*104*/"Not Enough Space Available", //knSIZE_WARNING
  /*105*/"The destination filesystem has\napproximately %s of free space.\n\nFiles being copied will take at least %s.", // knSIZE_WARNING_TEXT
  /*106*/"Ignore", //knIGNORE
  /*107*/"Abort", //knABORT
  /*108*/"Are you sure you want to abort?", //knESCAPE_WARNING
  /*109*/"Continue", //knCONTINUE
	/*110*/"Printing...", // knPRINTING
	/*111*/"Would you like to replace the existing file", // knFILE_REPLACE_DIALOG_TEXT_LABEL2
	/*112*/"with this one?", // knFILE_REPLACE_DIALOG_TEXT_LABEL3
	/*113*/"&Yes", // kn_YES
	/*114*/"Yes to &All", // knYES_TO__ALL
	/*115*/"&No", // kn_NO
	/*116*/"Printer:", // knPRINTER_COLON
	/*117*/"&Connect as:", // knCONNECT_AS_COLON
	/*118*/"S&hare this item and its contents", // knSHARE_THIS_ITEM_AND_ITS_CONTENTS
	/*119*/"&Share name:", // knSHARE_NAME_COLON
	/*120*/"&Comment:", // knSTR_COMMENT_COLON
	/*121*/"Share &enabled", // knSHARE_ENABLED
	/*122*/"&New Share...", // knNEW_SHARE_DOTDOTDOT
	/*123*/"&Remove Share", // knREMOVE_SHARE
	/*124*/"User limit", // knUSER_LIMIT
	/*125*/"Allow &all users", // knALLOW_ALL_USERS
	/*126*/"Allow &maximum", // knALLOW_MAXIMUM
	/*127*/"users", // knUSERS
	/*128*/"&Permissions...", // kn_PERMISSIONS_DOTDOTDOT
	/*129*/"Access through share:", // knACCESS_THROUGH_SHARE
	/*130*/"Name:", // knNAME_COLON
	/*131*/"Access type:", // knACCESS_TYPE_COLON
	/*132*/"&Add...", // kn_ADD_DOTDOTDOT
	/*133*/"&Remove", // kn_REMOVE
	/*134*/"&Share to mount:", // knSHARE_TO_MOUNT_COLON
	/*135*/"Mount &point:", // knMOUNT_POINT_COLON
	/*136*/"&Browse...", // kn_BROWSE_DOTDOTDOT
	/*137*/"Disconnect at logout and &reconnect at logon", // kn_RECONNECT_AT_LOGON
	/*138*/"Shared &directories:", // knSHARED_DIRECTORIES_COLON
	/*139*/"Files:", // knFILES_COLON
	/*140*/"Bytes:", // knBYTES_COLON
	/*141*/"Users and groups", // knUSERS_AND_GROUPS
	/*142*/"&User:", // knUSER_COLON
	/*143*/"Group:", // knGROUP_COLON
	/*144*/"Type of access:", // knTYPE_OF_ACCESS_COLON
	/*145*/"Unable to create symbolic link %s:\ndestination file system does not support symbolic links.", // knUNABLE_TO_CREATE_SYMBOLIC_LINK_X
	/*146*/"Instead of creating this link, you can choose to create a copy of the file this link refers to.\nDo you want to do this?", // knCOPY_INSTEAD_QUESTION
	/*147*/"Ignore All", // knIGNORE_ALL
	/*148*/"Unable to create special file '%s':\ntarget device has no room for the new node.", // knENOSPC
	/*149*/"'%s': file exists.", // knEEXIST
	/*150*/"&Save", // kn_SAVE
	/*151*/"&Open", // kn_OPEN
	/*152*/"Save As", // knSAVE_AS
  /*154*/"The System", // knMY_COMPUTER
  /*155*/"Name", // knNAME
  /*156*/"Size", // knSIZE
  /*157*/"Attributes", // knATTRIBUTES
  /*158*/"Modified", // knMODIFIED
  /*159*/"Mounted On", // knMOUNTED_ON
  /*160*/"Filesystem", // knFILESYSTEM
  /*161*/"Total Size", // knTOTAL_SIZE
  /*162*/"Free Space", // knFREE_SPACE
  /*163*/"Comment", // knCOMMENT
  /*164*/"Original Location", // knORIGINAL_LOCATION
  /*165*/"Date Deleted", // knDATE_DELETED
	/*166*/"Open", // knOPEN
	/*167*/"My Linux", // knMY_LINUX
	/*168*/"Look &in:", // knLOOK_IN_COLON
	/*169*/"Save &in:", // knSAVE_IN_COLON
	/*170*/"File &name:", // knFILE__NAME_COLON
	/*171*/"Files of &type:", // knFILES_OF__TYPE_COLON
	/*172*/"Passwords typed are not identical.\nPlease try again.", // knPASSWORDS_NOT_IDENTICAL
	/*173*/"Error", // knERROR
	/*174*/"&Anonymous access", // knANONYMOUS_ACCESS
	/*175*/"Allow access using this &password:", // knALLOW_ACCESS_USING_THIS_PASSWORD_COLON
	/*176*/"&Confirm password:", // kn_CONFIRM_PASSWORD_COLON
	/*177*/"Please select how this share can be accessed:", // knSHARE_ACCESS_DIALOG_TOP
	/*178*/"Access to this share is &read-only", // knACCESS_TO_THIS_SHARE_IS_READONLY
	/*179*/"&NFS Sharing", // knNFS_SHARING
	/*180*/"&Edit...", // kn_EDIT_DOTDOTDOT
	/*181*/"Host", // knHOST
	/*182*/"Permissions", // knPERMISSIONS
	/*183*/"Options", // knOPTIONS
	/*184*/"Please enter a value for the host name.", // knPLEASE_ENTER_HOSTNAME
	/*185*/"Add Host Permissions", // knADD_HOST_PERMISSIONS
	/*186*/"Edit Host Permissions", // knEDIT_HOST_PERMISSIONS
	/*187*/"&Host:", // kn_HOST_COLON
	/*188*/"&Permissions:", // kn_PERMISSIONS_COLON
	/*189*/"&Options:", // kn_OPTIONS_COLON
	/*190*/"NFS server does not allow sharing of directories\nwhich have space characters in the file names.", // knNO_SPACE_IN_NFS
  /*191*/"Type", // knTYPE
  /*192*/"Unable to mount %s\nat %s:\nno privileges.\n\nDo you want to remove that share from your list\nof automatically reconnected shares?", // knUNABLE_TO_MOUNT_NO_PRIVILEGES
  /*193*/"Unable to reconnect network share %s\nat %s:\nmount point doesn't exist.\n\nDo you want to remove that share from your list\nof automatically reconnected shares?", // knUNABLE_TO_MOUNT_NOMOUNTPOINT
  /*194*/"Mount point file name cannot contain spaces.\nPlease enter another value for the mount point.", // knMOUNT_POINT_SPACES_NOT_ALLOWED
  /*195*/"Unable to mount shares with names containing spaces.\nPlease rename this share on the remote computer or select another share to mount.", // knSHARE_NAME_SPACES_NOT_ALLOWED
  /*196*/"Local", // knLOCAL
  /*197*/"Network lpd", // knNETWORK_LPD
  /*198*/"Unknown", // knUNKNOWN
  /*199*/"Printers", //knPRINTERS
  /*200*/"Trash contains one item.\nAre you sure you want to delete it?", //knCONFIRM_ONE_ITEM_DELETE_QUESTION
  /*201*/"Skip", // knSKIP
  /*202*/"Unable to read file", // knUNABLE_TO_READ_FILE
  /*203*/"&Domain:", //kn_DOMAIN_COLON
  /*204*/"Copy of ", // knCOPY_OF_SPACE
  /*205*/"%sCopy (%d) of %s", // knXCOPY_N_OF_Y
  /*206*/"Unable to move file \"%s\"\nbecause it is located on a read-only filesystem.", // knUNABLE_TO_MOVE_FROM_READONLY_FS
  /*207*/"Print", // knSTR_PRINT
	/*208*/"Are you sure you want to delete printer \'%s\'?", //knCONFIRM_PRINTER_DELETE_QUESTION
	/*209*/"Confirm Printer Delete", //knCONFIRM_PRINTER_DELETE
	/*210*/"File Printing", // knSTR_FILE_PRINT
	/*211*/"Restoring Network Connections...", // knRESTORING_NETWORK_CONNECTIONS_DOTDOTDOT
	/*212*/"Closing Network Connections...", // knCLOSING_NETWORK_CONNECTIONS_DOTDOTDOT
	/*213*/"Creating print job...", // knSTR_CREATING_PRINT_JOB,
	/*214*/"Preparing to create a print job...", // knSTR_PREPARING_TO_CREATE_PRINT_JOB
};

#endif /* __INC_STRINGRES_MWN_H__ */

