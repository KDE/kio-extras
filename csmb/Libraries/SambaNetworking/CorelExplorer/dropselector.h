/* Name: dropselector.h

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

#ifndef __INC_DROPSELECTOR_H__
#define __INC_DROPSELECTOR_H__

#include "qobject.h"

typedef enum
{
  keDropNothing,
  keDropCopy,
  keDropMove,
  keDropOpen,
  keDropNickname
} CDropType;

class CDropSelector : public QObject
{
  Q_OBJECT

public:
  CDropSelector()
  {
    m_Type = keDropNothing;
  }

public slots:
  void OnMove();
  void OnCopy();
  void OnOpen();
  void OnNickname();
  void OnCancel();

public:
  void Go(QWidget *pParent, const QPoint &point, bool bMoveOnly = FALSE, bool bCopyOnly = FALSE);
  
  CDropType type()
  {
    return m_Type;
  }

protected:
  CDropType m_Type;
};

#endif /* __INC_DROPMENU_H__ */

