/* Name: corellistboxitem.h

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


#ifndef __INC_LBITEM_H__
#define __INC_LBITEM_H__

#include "ccui_common.h"

#include <qlistbox.h>

class CListBoxItem : public QListBoxItem
{
public:

    CListBoxItem(const char *s, const QPixmap p, void *pUserData
#ifdef QT_2
        , QListBox * listbox = 0) : QListBoxItem( listbox ),
#else
        ) : QListBoxItem(),
#endif
        pm(p), m_pUserData( pUserData )
    {
        setText(s);
    }

    void* GetUserData()
    {
        return m_pUserData;
    }

    virtual const QPixmap *pixmap()
    {
        return &pm;
    }

protected:

    virtual void paint(QPainter *);

    virtual int height(const QListBox *) const;

    virtual int width(const QListBox *) const;

private:
    QPixmap pm;
    void *m_pUserData;
};

#ifndef QT_2
// OBSOLETE for Qt2: no longer needed since QListBox::item() is now public
class CWorkaroundListBox : public QListBox
{
public:
    CWorkaroundListBox( QWidget * parent = 0, const char * name = 0,
        WFlags f = 0) : QListBox( parent, name, f ) { }

    QListBoxItem *item(int nIndex)
    {
        return QListBox::item(nIndex);
    }
};
#endif  // QT_2

#endif /* __INC_LBITEM_H__ */
