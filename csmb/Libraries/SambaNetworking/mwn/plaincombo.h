/* Name: plaincombo.cpp

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


#ifndef __INC_PLAINCOMBO_H__
#define __INC_PLAINCOMBO_H__

#include "common.h"

#ifdef QT_20
#include "qcombobox.h"
#define CCorelComboBox QComboBox
#else
#include "corelcombobox.h"
#endif

class CPlainCombo : public CCorelComboBox
{
public:
  CPlainCombo(QWidget *parent=0, const char *name=0) :
    CCorelComboBox(1, parent, name)
  {
  }
  void paintEvent(QPaintEvent *);

  bool eventFilter(QObject *object, QEvent *event);

  void focusInEvent(QFocusEvent *)
  {
    printf("FOCUS IN!\n");
  }
};

#endif /* __INC_PLAINCOMBO_H__ */
