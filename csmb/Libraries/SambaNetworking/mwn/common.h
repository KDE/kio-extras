/* Name: common.h

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

#ifndef __INC_COMMON_H_
#define __INC_COMMON_H_

typedef int BOOL;
#define TRUE (1)
#define FALSE (0)
typedef char * LPSTR;
typedef const char * LPCSTR;
typedef unsigned char BYTE;
#define __cdecl

#include <syscall.h>
#ifdef SYS_lchown
#define LCHOWN lchown
#else
#define LCHOWN chown
#endif

#include "qglobal.h"
#include "qstring.h"

#if (QT_VERSION >= 200)
#define QT_20
#endif

#ifdef QT_20
typedef QString QTTEXTTYPE;
typedef QString QTKEYTYPE;
#define QSTRING_WITH_SIZE(name, size) QString name
#define TOQTTEXTTYPE(x) QString((LPCSTR)(x))
#define QTARROWTYPE(x) (x)
#define QTGUISTYLE(x) (x)
#else
#include "qwindowdefs.h"
#include "qkeycode.h"
#include "qdrawutil.h"
#include "qevent.h"

typedef LPCSTR QTTEXTTYPE;
typedef LPCSTR QTKEYTYPE;
#define QSTRING_WITH_SIZE(name, size) QString name(size)
#define TOQTTEXTTYPE(x) ((LPCSTR)(x))

#define QTARROWTYPE(x) (ArrowType)(int)x
#define QTGUISTYLE(x) (GUIStyle)(int)x

const int __KeyHome = Key_Home;
const int __KeyUp = Key_Up;
const int __KeyDown = Key_Down;
const int __KeyRight = Key_Right;
const int __KeyLeft = Key_Left;
const int __KeyPrior = Key_Prior;
const int __KeyNext = Key_Next;
const int __KeyEnd = Key_End;
const int __KeyEnter = Key_Enter;
const int __KeyReturn = Key_Return;
const int __KeyBackspace = Key_Backspace;
const int __KeyEscape = Key_Escape;
const int __KeyInsert = Key_Insert;
const int __KeyDelete = Key_Delete;
const int __KeyA = Key_A;
const int __KeyB = Key_B;
const int __KeyC = Key_C;
const int __KeyD = Key_D;
const int __KeyE = Key_E;
const int __KeyF = Key_F;
const int __KeyG = Key_G;
const int __KeyH = Key_H;
const int __KeyI = Key_I;
const int __KeyJ = Key_J;
const int __KeyK = Key_K;
const int __KeyL = Key_L;
const int __KeyM = Key_M;
const int __KeyN = Key_N;
const int __KeyO = Key_O;
const int __KeyP = Key_P;
const int __KeyQ = Key_Q;
const int __KeyR = Key_R;
const int __KeyS = Key_S;
const int __KeyT = Key_T;
const int __KeyU = Key_U;
const int __KeyV = Key_V;
const int __KeyW = Key_W;
const int __KeyX = Key_X;
const int __KeyY = Key_Y;
const int __KeyZ = Key_Z;
const int __KeyF1 = Key_F1;
const int __KeyF2 = Key_F2;
const int __KeyF3 = Key_F3;
const int __KeyF4 = Key_F4;
const int __KeyF5 = Key_F5;
const int __KeyF6 = Key_F6;
const int __KeyF7 = Key_F7;
const int __KeyF8 = Key_F8;
const int __KeyF9 = Key_F9;
const int __KeyF10 = Key_F10;
const int __KeyF11 = Key_F11;
const int __KeyF12 = Key_F12;
const int __KeySpace = Key_Space;

#undef Key_Home
#undef Key_Up
#undef Key_Down
#undef Key_Right
#undef Key_Left
#undef Key_Prior
#undef Key_Next
#undef Key_End
#undef Key_Enter
#undef Key_Return
#undef Key_Backspace
#undef Key_Escape
#undef Key_Insert
#undef Key_Delete
#undef Key_A
#undef Key_B
#undef Key_C
#undef Key_D
#undef Key_E
#undef Key_F
#undef Key_G
#undef Key_H
#undef Key_I
#undef Key_J
#undef Key_K
#undef Key_L
#undef Key_M
#undef Key_N
#undef Key_O
#undef Key_P
#undef Key_Q
#undef Key_R
#undef Key_S
#undef Key_T
#undef Key_U
#undef Key_V
#undef Key_W
#undef Key_X
#undef Key_Y
#undef Key_Z
#undef Key_F1
#undef Key_F2
#undef Key_F3
#undef Key_F4
#undef Key_F5
#undef Key_F6
#undef Key_F7
#undef Key_F8
#undef Key_F9
#undef Key_F10
#undef Key_F11
#undef Key_F12
#undef Key_Space

class Qt
{
public:
	enum AlignmentFlags
	{
		AlignLeft = ::AlignLeft,
		ExpandTabs = ::ExpandTabs,
		SingleLine = ::SingleLine,
    AlignVCenter = ::AlignVCenter
	};
    
	enum Keys
	{
		Key_Home = __KeyHome,
		Key_Up = __KeyUp,
		Key_Down = __KeyDown,
		Key_Right = __KeyRight,
		Key_Left = __KeyLeft,
		Key_Prior = __KeyPrior,
		Key_Next = __KeyNext,
		Key_End = __KeyEnd,
		Key_Enter = __KeyEnter,
		Key_Return = __KeyReturn,
		Key_Backspace = __KeyBackspace,
		Key_Escape = __KeyEscape,
		Key_Insert = __KeyInsert,
		Key_Delete = __KeyDelete,
		
		Key_A = __KeyA,
		Key_B = __KeyB,
		Key_C = __KeyC,
		Key_D = __KeyD,
		Key_E = __KeyE,
		Key_F = __KeyF,
		Key_G = __KeyG,
		Key_H = __KeyH,
		Key_I = __KeyI,
		Key_J = __KeyJ,
		Key_K = __KeyK,
		Key_L = __KeyL,
		Key_M = __KeyM,
		Key_N = __KeyN,
		Key_O = __KeyO,
		Key_P = __KeyP,
		Key_Q = __KeyQ,
		Key_R = __KeyR,
		Key_S = __KeyS,
		Key_T = __KeyT,
		Key_U = __KeyU,
		Key_V = __KeyV,
		Key_W = __KeyW,
		Key_X = __KeyX,
		Key_Y = __KeyY,
		Key_Z = __KeyZ,
		Key_F1 = __KeyF1,
		Key_F2 = __KeyF2,
		Key_F3 = __KeyF3,
		Key_F4 = __KeyF4,
		Key_F5 = __KeyF5,
		Key_F6 = __KeyF6,
		Key_F7 = __KeyF7,
		Key_F8 = __KeyF8,
		Key_F9 = __KeyF9,
		Key_F10 = __KeyF10,
		Key_F11 = __KeyF11,
		Key_F12 = __KeyF12,
		Key_Space = __KeySpace,
	};
    
	enum DrawUtil
	{
		DownArrow = ::DownArrow,
		UpArrow = ::UpArrow,
		LeftArrow = ::LeftArrow,
		RightArrow = ::RightArrow
	};
    
	enum GUIStyle
	{
		WindowsStyle = ::WindowsStyle,
		MotifStyle = ::MotifStyle
	};

  enum ButtonTypes
  {
    NoButton = ::NoButton,
    LeftButton = ::LeftButton,
    MidButton = ::MidButton,
    RightButton = ::RightButton
  };

  enum ColorTypes
  {
    NoPen = ::NoPen,
    Dense4Pattern = ::Dense4Pattern
  };
};

#endif

#include <stdio.h>
#include <qstring.h>
#include <qpixmap.h>
#include <sys/types.h>
#include "array.h"
#include "credentials.h"
#include "notifier.h"
#include "resource.h"

typedef enum
{
	keSuccess = 0,
	keFileNotFound = -1,
	keFileReadError = -2,
	keTimeoutDetected = -3,
	keErrorAccessDenied = -4,
	keStoppedByUser = -5,
	keWrongParameters = -6,
	keNetworkError = -7,
	keUnknownHost = -8,
	keDirectoryNotEmpty = -9,
	keUnableToCreate = -10
} CSMBErrorCode;

//error codes used for mapper
#define knNetmapSuccess				0			//Successfully mounted the UNC path
#define knNetunmapSuccess     0     //Successfully unmapped the directory
#define knNoParentPerm      -10     //No permissions in the parent mount directory for the user
#define knNetunmapSMBError  -11     //Error executing the smbumount command
#define knNetunmapError     -12     //Error executing netunmap
#define knNetunmapTagError  -13     //Error removing the tag file
#define knNetunmapDirError  -14     //Error removing the mount directory
#define knNetmapError       -15     //General netmap error, check log
#define knNetmapDirError    -16     //Error creating the mount directory
#define knNetmapTagError    -17     //Error creating the tag file
#define knNetmapCancelled   -18     //The client cancelled the netmap request through the dialog box
#define knNetmapUNCError    -19     //The UNC path specified is invalid
#define knNetunmapUNCError  -20     //The UNC path specified is invalid 
#define knInvalidBufSize		-21			//The buffer size provided to either netmap or netunmap is not large enough
#define knPermissionError		-22			

#include "filejob.h"

extern QString gMasterBrowser;
extern BOOL gbNetworkAvailable;
extern QString gSmbConfLocation;
extern BOOL gbUseSmbMount_2_1_x, gbUseSmbMount_2_2_x;
extern BOOL gbShowHiddenFiles;
extern BOOL gbShowHiddenShares;
extern BOOL gbSmbConfReadonly;
extern QString gDefaultHomeDirectory;
void SearchSmbConfLocation();

extern BOOL gbStopping; // Set by the UI to indicate that aborting of the current operation is requested
extern int gnActiveTaskCount; // Counts existing active tasks (open pipes waiting for input).
extern LPCSTR gTrashID;

typedef CVector<QString, QString&> QStringArray;


BOOL Match(LPCSTR s, LPCSTR p);

QString ExtractWord(LPCSTR& x, LPCSTR SpaceCharList = "\t ");
QString ExtractWordEscaped(LPCSTR& x, LPCSTR SpaceCharList = "\t ");
QString ExtractQuotedWord(LPCSTR& x, LPCSTR SpaceCharList = "\t ");
QString ExtractTail(LPCSTR& x, LPCSTR SpaceCharList = "\t ");

void ExtractFromTail(LPCSTR x, int nExtract, QStringArray& list);
void GetShareAndDirectory(LPCSTR UNCPath, QString& ShareName, QString& Directory);
void InitPixmaps();
void ConvertDlgUnits(QWidget *w);
BOOL ParseUNCPath(LPCSTR UNCPath, QString& Server, QString& Share, QString& Path);

QObject *GetComboEdit(QObject *pCombo);

BOOL	 WaitWithMessageLoop(FILE *f);

extern CTreeExpansionNotifier gTreeExpansionNotifier;

QString SizeBytesFormat(double n);
QString SizeInKilobytes(unsigned long num);
QString NumToCommaString(unsigned long num);

time_t ParseDate(LPCSTR s);

BOOL CanMountAt(LPCSTR Path);

QString MakeSlashesBackwardDouble(LPCSTR s);

QString MakeSlashesForward(LPCSTR s);
QString MakeSlashesBackward(LPCSTR s);
QString EscapeString(LPCSTR s);

BOOL IsUNCPath(LPCSTR s);
void SplitPath(LPCSTR Path, QString& Parent, QString& FileName);

CSMBErrorCode CreateDir(LPCSTR name);
int FolderExists(LPCSTR name);
BOOL IsValidFolder(LPCSTR name);
BOOL EnsureDirectoryExists(LPCSTR name, BOOL bPromptToCreate = TRUE, BOOL bMustBeWritable = TRUE);

BOOL CountFolderContents(
	LPCSTR Path, 
	unsigned long& dwNumFiles, 
	unsigned long& dwNumFolders, 
	double& dwTotalSize, 
	CFileJob *pResult,
	BOOL bRecursive,
	BOOL bContainersGoLast,
	int BaseNameLength = -1);

BOOL AddAutoMountEntry(LPCSTR Entry);
void RemoveAutoMountEntry(LPCSTR MountPath);
BOOL FindAutoMountEntry(LPCSTR UNCPath, LPCSTR MountPath);

QString GetHostName();
QString GetIPAddress();
QString GetHomeDir(LPCSTR UserName);
QString GetGroupName(LPCSTR UserName);

FILE *ServerOpen(LPCSTR ServerCommand);
QString GetServerVariable(LPCSTR Variable);
int GetServerOpenHandle(LPCSTR ServerCommand);
int ServerExecute(LPCSTR Command);
void WriteLog(LPCSTR s);
BOOL LocateSmbMount();
extern LPCSTR gSmbMountLocation;
extern LPCSTR gSmbUmountLocation;

void ParseURL(LPCSTR Url, QString& Hostname, QString& SiteRelativePath, int& nCredentialsIndex);

void MakeURL(LPCSTR Parent, LPCSTR File, QString& Result);

QString GetParentURL(LPCSTR Url);

typedef void (*LPFNStatusCallback)(void *UserData, unsigned long Param);

inline BOOL IsFTPUrl(LPCSTR Url)
{
	return strlen(Url) > 6 && !strnicmp(Url, "ftp://", 6);
}

inline BOOL IsPrinterUrl(LPCSTR Url)
{
	return strlen(Url) > 10 && !strnicmp(Url, "printer://", 10);
}

int FileStat(LPCSTR Url, struct stat *st);
int FileUnlink(LPCSTR Url);

BOOL IsValidFileName(LPCSTR s);
QPixmap *GetFilePixmap(const QString& s, BOOL bIsLink, BOOL bIsExecutable, BOOL bIsBig = FALSE);
QString SplitString(LPCSTR s, int nMaxChar);
void FileModeToString(QString& s, mode_t mode);

void URLEncode(QString& s);
void URLDecode(QString& s);
void URLDecodeSmart(QString &s);
void ExtractCredentialsFromURL(QString &URL, int& nCredentialsIndex);
CSMBErrorCode MakeDir(LPCSTR Url);
QString GetHomeDir();
int ReportCommonFileError(LPCSTR FileName, int nError, BOOL bNeedIgnore, int nCaptionID, int nPrefix = -1, int nIgnoreID = -1);
QPixmap *GetIconFromKDEFile(LPCSTR FileName, BOOL bIsBig);
int SuperUserExecute(LPCSTR OperationDescription, LPCSTR Command, char *ResultString, int nResultSize); // in shell.cpp
BOOL CopyFile(LPCSTR FileIn, LPCSTR FileOut);
void ReReadSambaConfiguration();
void ReadConfiguration();
int IsInSubtree(LPCSTR s1, LPCSTR s2);
BOOL IsTrashFolder(LPCSTR FileName);
BOOL IsMyTrashFolder(LPCSTR FileName);
LPCSTR FormatDateTime(time_t datetime);
BOOL IsExcludedFromRescan(LPCSTR Path); // in ReadConfig.cpp
int NetmapWithMessageLoop(QString &szResult, LPCSTR szUNC, pid_t *pPID = NULL);
BOOL LocalHasSubfolders(LPCSTR Path);

BOOL IsSuperUser();
BOOL HasNFSSharing(); // Checks is NFS server is installed
bool IsShared(LPCSTR pPath);

typedef BOOL (*LPFN_DeletePrinter)(LPCSTR);
extern LPFN_DeletePrinter gpDeletePrinterHandler;

typedef CSMBErrorCode (*LPFN_PrintFileHandler)(LPCSTR pszPrinterName, LPCSTR pszFileName);
extern LPFN_PrintFileHandler gpPrintFileHandler;

///////////////////////////////////////////////////////////////////////////////

#define knPRINTERS_HIDDEN_PREFIX 0
#define knPRINTER_HIDDEN_PREFIX 1
#define knWORKGROUPS_HIDDEN_PREFIX 2
#define knWORKGROUP_HIDDEN_PREFIX 3
#define knNFSSERVERS_HIDDEN_PREFIX 4

///////////////////////////////////////////////////////////////////////////////

LPCSTR GetHiddenPrefix(const unsigned int nIndex);
QString AttachHiddenPrefix(const char *text, int nHiddenPrefix);
LPCSTR DetachHiddenPrefix(LPCSTR text, int& nHiddenPrefix);

///////////////////////////////////////////////////////////////////////////////

QObject *FindChildByName(QWidget *w, LPCSTR Name);
QString GetLocalFileTip(int nType, BOOL bIsLink, BOOL bIsFolder, const QString& ShortName, const QString& FullName, const QString& TargetName);

QPixmap *DefaultFilePixmap(LPCSTR FullName,
                           LPCSTR ShortName,
                           BOOL bIsBig, 
                           LPCSTR FileAttributes, 
                           mode_t TargetMode, 
                           LPCSTR TargetName,
                           BOOL bIsLink,
                           BOOL bIsFolder);

QString SqueezeString(LPCSTR str, unsigned int maxlen);
BOOL IsScreenSaverRunning();
BOOL IsSamePath(LPCSTR Path1, LPCSTR Path2);


#endif /* __INC_COMMON_H_ */
