/* Name: common.h

   Description: This file is a part of the ccui library.

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



#ifndef __INC_COMMON_H_
#define __INC_COMMON_H_

#ifndef QT_20
#define QT_20
#endif

typedef int BOOL;
#define TRUE (1)
#define FALSE (0)
typedef char * LPSTR;
typedef const char * LPCSTR;
typedef unsigned char BYTE;
#define __cdecl

#ifdef QT_20
#include "qstring.h"
typedef QString QTTEXTTYPE;
typedef QString QTKEYTYPE;
#define QTARROWTYPE(x) (x)
#define QTGUISTYLE(x) (x)
#include "qwindowsstyle.h"
#include <q1xcompatibility.h>
#include <qmotifstyle.h>
#else
#include "qwindowdefs.h"
#include "qkeycode.h"
#include "qdrawutil.h"
typedef LPCSTR QTTEXTTYPE;
typedef LPCSTR QTKEYTYPE;

#define QTARROWTYPE(x) (ArrowType)(int)x
#define QTGUISTYLE(x) (GUIStyle)(int)x
/*
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
*/
class Qt
{
public:
	enum AlignmentFlags
	{
		AlignLeft = ::AlignLeft,
		AlignCenter = ::AlignCenter,
		ExpandTabs = ::ExpandTabs,
		SingleLine = ::SingleLine
	};
/*    
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
	};
*/    
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
};

#endif//QT_20

#endif /* __INC_COMMON_H_ */
