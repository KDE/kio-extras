 /* Name: topcombo.cpp

    Description: This file is a part of the Corel File Manager application.

    Author:	Oleg Noskov (olegn@corel.com)
    Modified:	Jasmin Blanchette

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

////////////////////////////////////////////////////////////////////////////

#include "topcombo.h"
#include <qpainter.h>
#include "common.h"
#ifdef QT_20
#include "qlineedit.h"
#define CCorelLineEdit QLineEdit
#include "qclipboard.h"
#else
#include "corelclipboard.h"
#include "corellineedit.h"
#endif

////////////////////////////////////////////////////////////////////////////

#include "qcombobox.h"
#include "qpopupmenu.h"
#include "qlistbox.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qscrollbar.h"   // for qDrawArrow
#include "qkeycode.h"
#include "qstrlist.h"
#include "qpixmap.h"
#include "qtimer.h"
#include "qapplication.h"
#include <limits.h>

////////////////////////////////////////////////////////////////////////////

struct CTopComboData
{
    int		current;
    int		maxCount;
    int		sizeLimit;
    CTopCombo::Policy p;
    bool	usingListBox;
    bool	autoresize;
    bool	poppedUp;
    bool	mouseWasInsidePopup;
    bool	arrowPressed;
    bool	arrowDown;
    bool	discardNextMousePress;
    bool	shortClick;

		union
		{
			QPopupMenu *popup;
			QListBox   *listBox;
    };

		bool	useCompletion;
    bool	completeNow;
    int		completeAt;

    class ComboEdit: public CCorelLineEdit
    {
			public:
				ComboEdit(QWidget * parent) : CCorelLineEdit(parent,"combo edit")
				{
          m_bJustGotFocus = false;
				}

				bool validateAndSet(const char * newText, int newPos,	int newMarkAnchor, int newMarkDrag)
				{
					return CCorelLineEdit::validateAndSet(newText, newPos, newMarkAnchor, newMarkDrag);
				}

        bool m_bJustGotFocus;

        void focusInEvent(QFocusEvent *e)
        {
          CCorelLineEdit::focusInEvent(e);
          m_bJustGotFocus = true;
          qApp->postEvent(this, new QEvent(
#ifdef QT_20
        (QEvent::Type)
#endif
            0x999));
          int nLen = strlen(text());
          setSelection(0, nLen);
          setCursorPosition(nLen);
          repaint(FALSE);
        }

        bool event(QEvent *e)
        {
          if (e->type() == 0x999)
          {
            m_bJustGotFocus = false;
            return false;
          }

          return CCorelLineEdit::event(e);
        }

        void mouseReleaseEvent(QMouseEvent *e)
        {
          QMouseEvent e1(e->type(), e->pos(), RightButton, e->state());
          CCorelLineEdit::mouseReleaseEvent(&e1);
        }

        void mousePressEvent(QMouseEvent *e)
				{
          QFontMetrics fm = fontMetrics();
					QString s(text());

          if (m_bJustGotFocus)
          {
            return;
          }
          else
          {
            if (e->pos().x() >= fm.width(s, s.length()))
  					{
              setSelection(s.length(), 0);
              setCursorPosition(s.length());
              repaint(FALSE);
              return;
  					}
          }

					CCorelLineEdit::mousePressEvent(e);
				}

				void mouseDoubleClickEvent(QMouseEvent *e)
				{
          CCorelLineEdit::mouseDoubleClickEvent(e);
					setCursorPosition(strlen(text()));
				}
		};

    ComboEdit *ed;
};

////////////////////////////////////////////////////////////////////////////

bool CTopCombo::getMetrics(int *dist, int *buttonW, int *buttonH) const
{
	if (d->usingListBox && style() == WindowsStyle)
	{
		QRect r  = arrowRect();
		*buttonW = r.width();
		*buttonH = r.height();
		*dist = 4;
	}
	else
	{
		if (d->usingListBox)
		{
			*dist = 6;
			*buttonW = 16;
			*buttonH = 18;
		}
		else
		{
			*dist     = 8;
			*buttonH  = 7;
			*buttonW  = 11;
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

static inline bool checkInsertIndex(const char *method, const char *name, int count, int *index)
{
	bool range_err = (*index > count);

#ifdef CHECK_RANGE
	if (range_err)
		warning("CTopCombo::%s: (%s) Index %d out of range", method, name ? name : "<no name>", *index);
#endif
	if (*index < 0)				// append
		*index = count;

	return !range_err;
}

////////////////////////////////////////////////////////////////////////////

static inline bool checkIndex(const char *method, const char *name, int count, int index)
{
	bool range_err = (index >= count);

#ifdef CHECK_RANGE
	if (range_err)
		warning("CTopCombo::%s: (%s) Index %i out of range", method, name ? name : "<no name>", index);
#endif

	return !range_err;
}

////////////////////////////////////////////////////////////////////////////
/*!
  Constructs a combo box widget with a parent and a name.

  This constructor creates a popup menu if the program uses Motif look
  and feel; this is compatible with Motif 1.x.
*/

CTopCombo::CTopCombo(QWidget *parent, const char *name)
    : QWidget(parent, name)
{
	d = new CTopComboData;

	if (style() == WindowsStyle)
	{
		d->listBox = new QListBox(0, 0, WType_Popup);
		d->listBox->setAutoScrollBar(FALSE);
		d->listBox->setBottomScrollBar(FALSE);
		d->listBox->setAutoBottomScrollBar(FALSE);
		d->listBox->setFrameStyle(QFrame::Box | QFrame::Plain);
		d->listBox->setLineWidth(1);
		d->listBox->resize(100, 10);

		d->usingListBox = TRUE;
    connect(d->listBox, SIGNAL(clicked (QListBoxItem *)), SLOT(internalActivate(QListBoxItem *)));
		connect(d->listBox, SIGNAL(selected(int)), SLOT(internalActivate(int)));
		connect(d->listBox, SIGNAL(highlighted(int)), SLOT(internalHighlight(int)));
	}
	else
	{
		d->popup = new QPopupMenu;
		d->usingListBox = FALSE;
    connect(d->popup, SIGNAL(clicked (QListBoxItem *)), SLOT(internalActivate(QListBoxItem *)));
		connect(d->popup, SIGNAL(activated(int)), SLOT(internalActivate(int)));
		connect(d->popup, SIGNAL(highlighted(int)), SLOT(internalHighlight(int)));
  }

	d->ed = 0;
	d->current = 0;
	d->maxCount = INT_MAX;
	d->sizeLimit = 10;
	d->p =  AtBottom;
	d->autoresize = FALSE;
	d->poppedUp = FALSE;
	d->arrowDown = FALSE;
	d->discardNextMousePress = FALSE;
	d->shortClick = FALSE;
	d->useCompletion = FALSE;

	setFocusPolicy(TabFocus);
	setPalettePropagation(AllChildren);
	setFontPropagation(AllChildren);
}

////////////////////////////////////////////////////////////////////////////
/*!
  Constructs a combo box with a maximum size and either Motif 2.0 or
  Windows look and feel.

  The input field can be edited if \a rw is TRUE, otherwise the user
  may only choose one of the items in the combo box.
*/

CTopCombo::CTopCombo(bool rw, QWidget *parent, const char *name)
    : QWidget(parent, name)
{
	d = new CTopComboData;
	d->listBox = new QListBox(0, 0, WType_Popup);
	d->listBox->setAutoScrollBar(FALSE);
	d->listBox->setBottomScrollBar(FALSE);
	d->listBox->setAutoBottomScrollBar(TRUE);
	d->listBox->setFrameStyle(QFrame::Box | QFrame::Plain);
	d->listBox->setLineWidth(1);
	d->listBox->resize(100, 10);

	d->usingListBox = TRUE;
  connect(d->listBox, SIGNAL(clicked(QListBoxItem *)), SLOT(internalActivate(QListBoxItem *)));
	connect(d->listBox, SIGNAL(selected(int)), SLOT(internalActivate(int)));
	connect(d->listBox, SIGNAL(highlighted(int)), SLOT(internalHighlight(int)));

	d->current = 0;
	d->maxCount = INT_MAX;
	d->sizeLimit = 10;
	d->p = AtBottom;
	d->autoresize = FALSE;
	d->poppedUp = FALSE;
	d->arrowDown = FALSE;
	d->discardNextMousePress = FALSE;
	d->shortClick = FALSE;
	d->useCompletion = FALSE;

	setFocusPolicy(StrongFocus);

	if (rw)
	{
		d->ed = new CTopComboData::ComboEdit(this);
		d->ed->setFrame(FALSE);

		if (style() == WindowsStyle)
			d->ed->setGeometry(2, 2, width() - 2 - 2 - 16, height() - 2 - 2);
		else
			d->ed->setGeometry(3, 3, width() - 3 - 3 - 21, height() - 3 - 3);

		d->ed->installEventFilter(this);
    connect(d->ed, SIGNAL(textChanged(const QString &)), this, SLOT(OnEditTextChanged(const QString &)));

		setFocusProxy(d->ed);

		setBackgroundMode(NoBackground);

		connect(d->ed, SIGNAL(returnPressed()), SLOT(returnPressed()));
	}
	else
	{
		d->ed = 0;
	}

	setPalettePropagation(AllChildren);
	setFontPropagation(AllChildren);
}

////////////////////////////////////////////////////////////////////////////
/*!
  Destroys the combo box.
*/

CTopCombo::~CTopCombo()
{
	if (!QApplication::closingDown())
	{
		if (d->usingListBox)
			delete d->listBox;
		else
			delete d->popup;
	}
	else
	{
		if (d->usingListBox)
			d->listBox = 0;
		else
			d->popup   = 0;
	}

	delete d;
}

////////////////////////////////////////////////////////////////////////////
/*!  Reimplemented for implementational reasons.

  Note that CTopCombo always turns into a new-style Motif combo box
  when it is changed from Windows to Motif style (even if it was an
  old-style combo box before).
*/

#ifndef QT_20
void CTopCombo::setStyle(GUIStyle s)
{
  if (s != style())
	{
		QWidget::setStyle(s);

		if (!d->usingListBox)
		{
			QPopupMenu * p = d->popup;
	    d->listBox = new QListBox(0, 0, WType_Popup);
	    d->listBox->setAutoScrollBar(FALSE);
	    d->listBox->setBottomScrollBar(FALSE);
	    d->listBox->setAutoBottomScrollBar(FALSE);
	    d->listBox->setFrameStyle(QFrame::Box | QFrame::Plain);
	    d->listBox->setLineWidth(1);
	    d->listBox->resize(100, 10);
	    d->usingListBox      = TRUE;
      connect(d->listBox, SIGNAL(selected(int)), SLOT(internalActivate(int)));
	    connect(d->listBox, SIGNAL(highlighted(int)), SLOT(internalHighlight(int)));

			if (p)
			{
				int n;
				for (n=p->count()-1; n>=0; n--)
				{
					if (p->text(n))
						d->listBox->insertItem(p->text(n), 0);
					else
						if (p->pixmap(n))
							d->listBox->insertItem(*(p->pixmap(n)), 0);
				}
				delete p;
	    }
		}
	}

	if (d->ed)
	{
		d->ed->setStyle(s);
		d->ed->setFrame(s == MotifStyle);
	}

	if (d->listBox)
		d->listBox->setStyle(s);
}
#endif

////////////////////////////////////////////////////////////////////////////
/*!
  Returns the number of items in the combo box.
*/

int CTopCombo::count() const
{
	if (d->usingListBox)
		return d->listBox->count();
	else
		return d->popup->count();
}

/*!
  Inserts the list of strings at the index \e index in the combo box.
*/

void CTopCombo::insertStrList(const QStrList *list, int index)
{
	if (!list)
	{
#if defined(CHECK_NULL)
		ASSERT(list != 0);
#endif
		return;
	}

	QStrListIterator it(*list);

	const char *tmp;

	if (index < 0)
		index = count();

	while ((tmp=it.current()))
	{
		++it;

		if (d->usingListBox)
			d->listBox->insertItem(tmp, index);
		else
			d->popup->insertItem(tmp, index);

		if (index++ == d->current)
		{
			if (d->ed)
				d->ed->setText(text(d->current));
	    else
				repaint();

			currentChanged();
		}
	}

	if (index != count())
		reIndex();
}

////////////////////////////////////////////////////////////////////////////
/*!
  Inserts the array of strings at the index \e index in the combo box.

  The \e numStrings argument is the number of strings.
  If \e numStrings is -1 (default), the \e strs array must be
  terminated with 0.

  Example:
  \code
    static const char *items[] = { "red", "green", "blue", 0 };
    combo->insertStrList(items);
  \endcode
*/

void CTopCombo::insertStrList(const char **strings, int numStrings, int index)
{
	if (!strings)
	{
#if defined(CHECK_NULL)
		ASSERT(strings != 0);
#endif
		return;
	}

	if (index < 0)
		index = count();

	int i = 0;

	while ((numStrings<0 && strings[i]!=0) || i<numStrings)
	{
		if (d->usingListBox)
			d->listBox->insertItem(strings[i], index);
		else
			d->popup->insertItem(strings[i], index);

		i++;

		if (index++ == d->current)
		{
			if (d->ed)
				d->ed->setText(text(d->current));
			else
				repaint();

			currentChanged();
		}
	}

	if (index != count())
		reIndex();
}

////////////////////////////////////////////////////////////////////////////
/*!
  Inserts a text item at position \e index. The item will be appended if
  \e index is negative.
*/

void CTopCombo::insertItem(const char *t, int index)
{
	int cnt = count();

	if (!checkInsertIndex("insertItem", name(), cnt, &index))
		return;

	if (d->usingListBox)
		d->listBox->insertItem(t, index);
	else
		d->popup->insertItem(t, index);

	if (index != cnt)
		reIndex();

	if (index == d->current)
	{
		if (d->ed)
			d->ed->setText(text(d->current));
		else
			repaint();
	}

	if (index == d->current)
		currentChanged();
}

////////////////////////////////////////////////////////////////////////////
/*!
  Inserts a pixmap item at position \e index. The item will be appended if
  \e index is negative.

  If the combo box is writable, the pixmap is not inserted.
*/

void CTopCombo::insertItem(const QPixmap &pixmap, int index)
{
	if (d->ed)
		return;

	int cnt = count();
	bool append = index < 0 || index == cnt;

	if (!checkInsertIndex("insertItem", name(), cnt, &index))
		return;

	if (d->usingListBox)
		d->listBox->insertItem(pixmap, index);
	else
		d->popup->insertItem(pixmap, index);

	if (!append)
		reIndex();

	if (index == d->current)
		currentChanged();
}

////////////////////////////////////////////////////////////////////////////
/*!
  Removes the item at position \e index.
*/

void CTopCombo::removeItem(int index)
{
	int cnt = count();

	if (!checkIndex("removeItem", name(), cnt, index))
		return;

	if (d->usingListBox)
		d->listBox->removeItem(index);
	else
		d->popup->removeItemAt(index);

	if (index != cnt-1)
		reIndex();

	if (index == d->current)
	{
		if (d->ed)
		{
			QString s = "";

			if (d->current < cnt - 1)
				s = text(d->current);

			d->ed->setText(s);
		}
		else
			repaint();

		currentChanged();
	}
}

////////////////////////////////////////////////////////////////////////////
/*!
  Removes all combo box items.
*/

void CTopCombo::clear()
{
	if (d->usingListBox)
		d->listBox->clear();
	else
		d->popup->clear();

	d->current = 0;

	if (d->ed)
		d->ed->setText("");

	currentChanged();
}

////////////////////////////////////////////////////////////////////////////
/*!
  Returns the text item being edited, or the current text item if the combo
  box is not editable.
  \sa text()
*/

QString CTopCombo::currentText() const
{
  return AttachHiddenPrefix(currentTextUI(), HiddenPrefix());
}

const char *CTopCombo::currentTextUI() const
{
  return d->ed ? (const char *)d->ed->text()
#ifdef QT_20
  .latin1()
#endif
                                            : (const char *)text(currentItem())
                                                                                ;
}

////////////////////////////////////////////////////////////////////////////
/*!
  Returns the text item at a given index, or 0 if the item is not a string.
  \sa currentText()
*/

const char *CTopCombo::text(int index) const
{
	if (!checkIndex("text", name(), count(), index))
		return 0;

	if (d->usingListBox)
		return (LPCSTR)d->listBox->text(index)
#ifdef QT_20
  .latin1()
#endif
                                  ;
	else
		return (LPCSTR)d->popup->text(index)
#ifdef QT_20
  .latin1()
#endif
                                        ;
}

////////////////////////////////////////////////////////////////////////////
/*!
  Returns the pixmap item at a given index, or 0 if the item is not a pixmap.
*/

const QPixmap *CTopCombo::pixmap(int index) const
{
	if (!checkIndex("pixmap", name(), count(), index))
		return 0;

	return d->usingListBox ? d->listBox->pixmap(index) : d->popup->pixmap(index);
}

////////////////////////////////////////////////////////////////////////////
/*!
  Replaces the item at position \e index with a text.
*/

void CTopCombo::changeItem(const char *t, int index)
{
	if (!checkIndex("changeItem", name(), count(), index))
		return;

	if (d->usingListBox)
		d->listBox->changeItem(t, index);
	else
		d->popup->changeItem(t, index);

	if (index == d->current && d->ed)
		d->ed->setText(text(d->current));
}

////////////////////////////////////////////////////////////////////////////
/*!
  Replaces the item at position \e index with a pixmap, unless the
  combo box is writable.

  \sa insertItem()
*/

void CTopCombo::changeItem(const QPixmap &im, int index)
{
	if (d->ed != 0 || !checkIndex("changeItem", name(), count(), index))
		return;

	if (d->usingListBox)
		d->listBox->changeItem(im, index);
	else
		d->popup->changeItem(im, index);
}

////////////////////////////////////////////////////////////////////////////
/*!
  Returns the index of the current combo box item.
  \sa setCurrentItem()
*/

int CTopCombo::currentItem() const
{
	return d->current;
}

////////////////////////////////////////////////////////////////////////////
/*!
  Sets the current combo box item.
  This is the item to be displayed on the combo box button.
  \sa currentItem()
*/

void CTopCombo::setCurrentItem(int index)
{
	if (index == d->current)
		return;

	if (!checkIndex("setCurrentItem", name(), count(), index))
	{
		return;
  }

  d->current = index;

  if (d->ed)
		d->ed->setText(text(index));

	if (d->poppedUp)
	{
		if (d->usingListBox && d->listBox)
			d->listBox->setCurrentItem(index);
		else
			if (d->popup)
				// the popup will soon send an override, but for the
				// moment this is correct
				internalHighlight(index);
	}

	currentChanged();
}

////////////////////////////////////////////////////////////////////////////
/*!
  Returns TRUE if auto-resizing is enabled, or FALSE if auto-resizing is
  disabled.

  Auto-resizing is disabled by default.

  \sa setAutoResize()
*/

bool CTopCombo::autoResize() const
{
	return d->autoresize;
}

////////////////////////////////////////////////////////////////////////////
/*!
  Enables auto-resizing if \e enable is TRUE, or disables it if \e enable is
  FALSE.

  When auto-resizing is enabled, the combo box button will resize itself
  whenever the current combo box item change.

  \sa autoResize(), adjustSize()
*/

void CTopCombo::setAutoResize(bool enable)
{
	if ((bool)d->autoresize != enable)
	{
		d->autoresize = enable;

		if (enable)
			adjustSize();
	}
}

////////////////////////////////////////////////////////////////////////////
/*!
  Returns a size which fits the contents of the combo box button.
*/

QSize CTopCombo::sizeHint() const
{
	int i, w, h;
	const char *tmp;
	QFontMetrics fm = fontMetrics();

	int extraW = 20;
	int maxW = count() ? 18 : 7 * fm.width('x') + 18;
	int maxH = QMAX(fm.height(), 12);

	for(i = 0; i < count(); i++)
	{
		tmp = text(i);

		if (tmp)
		{
			w = fm.width(tmp);
			h = 0;
		}
		else
		{
			const QPixmap *pix = pixmap(i);

			if (pix)
			{
				w = pix->width();
				h = pix->height();
			}
			else
			{
				w = 0;
				h = height() - 4;
			}
		}

		if (w > maxW)
			maxW = w;

		if (h > maxH)
			maxH = h;
	}

	if (maxH <= 16 && parentWidget() &&
	 (parentWidget()->inherits("QToolBar") ||
	  parentWidget()->inherits("QDialog") && style() == WindowsStyle))
		maxH = 12;

	return QSize(4 + 4 + maxW + extraW, maxH + 5 + 5);
}

////////////////////////////////////////////////////////////////////////////
/*!
  \internal
  Receives activated signals from an internal popup list and emits
  the activated() signal.
*/

void CTopCombo::internalActivate(int index)
{
  if (d->current != index)
	{
		d->current = index;
		currentChanged();
	}

	if (d->usingListBox)
		popDownListBox();
	else
		d->popup->removeEventFilter(this);

	d->poppedUp = FALSE;

	QString t(text(index));
	emit activated(index);

	if (d->ed)
		d->ed->setText(t);

	emit activated(t);
}

void CTopCombo::internalActivate(QListBoxItem * item)
{
  int i = d->listBox->index(item);
  internalActivate(i);
}
////////////////////////////////////////////////////////////////////////////

void CTopCombo::internalHighlight(int index)
{
	emit highlighted(index);
	//const char *t = text(index);
  QString t = text(index);

  if (!t.isNull())
		emit highlighted(t);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::internalClickTimeout()
{
	d->shortClick = FALSE;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setBackgroundColor(const QColor &color)
{
	QWidget::setBackgroundColor(color);

	if (!d->usingListBox)
		d->popup->setBackgroundColor(color);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setPalette(const QPalette &palette)
{
	QWidget::setPalette(palette);

	if (d->usingListBox)
		d->listBox->setPalette(palette);
	else
		d->popup->setPalette(palette);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setFont(const QFont &font)
{
	QWidget::setFont(font);

	if (d->usingListBox)
		d->listBox->setFont(font);
	else
		d->popup->setFont(font);

	if (d->ed)
		d->ed->setFont(font);

	if (d->autoresize)
		adjustSize();
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::resizeEvent(QResizeEvent *)
{
	if (!d->ed)
		return;
	else
		if (style() == WindowsStyle)
			d->ed->setGeometry(2, 2, width() - 2 - 2 - 16, height() - 2 - 2);
    else
			d->ed->setGeometry(3, 3, width() - 3 - 3 - 21, height() - 3 - 3);
}

////////////////////////////////////////////////////////////////////////////

QRect CTopCombo::arrowRect() const
{
	return QRect(width() - 2 - 16, 2, 16, height() - 4);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::mousePressEvent(QMouseEvent *e)
{
	if (d->discardNextMousePress)
	{
		d->discardNextMousePress = FALSE;
		return;
	}

	d->arrowPressed = FALSE;

	if (style() == WindowsStyle)
	{
		popup();

		if (arrowRect().contains(e->pos()))
		{
			d->arrowPressed = TRUE;
			d->arrowDown    = TRUE;
			repaint(FALSE);
		}
	}
	else
	{
		popup();
		QTimer::singleShot(200, this, SLOT(internalClickTimeout()));
		d->shortClick = TRUE;
	}
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::mouseMoveEvent(QMouseEvent *)
{
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::mouseReleaseEvent(QMouseEvent *)
{

}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::mouseDoubleClickEvent(QMouseEvent *e)
{
	mousePressEvent(e);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::focusInEvent(QFocusEvent *)
{
	repaint(FALSE);
}

////////////////////////////////////////////////////////////////////////////

static int listHeight(QListBox *l, int sl)
{
	int i;
	int sumH = 0;

	for(i = 0 ; i < (int) l->count() && i < sl ; i++)
	{
		sumH += l->itemHeight(i);
	}

	return sumH;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::popup()
{
	if (!count())
		insertItem("", 0);

	if (d->usingListBox)
	{
		// Send all listbox events to eventFilter():
		d->listBox->installEventFilter(this);
		d->mouseWasInsidePopup = FALSE;
		d->listBox->resize(width(), listHeight(d->listBox, d->sizeLimit) + 2);

		if (d->listBox->bottomScrollBar())
			d->listBox->resize(width(),

		listHeight(d->listBox, d->sizeLimit) + 2 + 16); //### hardcoded scrollbar height 16

		QWidget *desktop = QApplication::desktop();
		int sw = desktop->width();			// screen width
		int sh = desktop->height();			// screen height
		QPoint pos = mapToGlobal(QPoint(0,height()));

		// XXX Similar code is in QPopupMenu
		int x = pos.x();
		int y = pos.y();
		int w = d->listBox->width();
		int h = d->listBox->height();

		// the complete widget must be visible
		if (x + w > sw)
			x = sw - w;
		else
			if (x < 0)
				x = 0;

		if (y + h > sh && y - h - height() >= 0)
			y = y - h - height();

		d->listBox->move(x,y);
		d->listBox->raise();
		d->listBox->blockSignals(TRUE);
		d->listBox->setCurrentItem(d->current);
		d->listBox->blockSignals(FALSE);
		d->listBox->setAutoScrollBar(TRUE);
		d->listBox->show();
	}
	else
	{
		d->popup->installEventFilter(this);
		d->popup->popup(mapToGlobal(QPoint(0,0)), d->current);
	}

	d->poppedUp = TRUE;
}

////////////////////////////////////////////////////////////////////////////

QLineEdit *CTopCombo::getLineEdit() const
{
	return d->ed;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::popDownListBox()
{
	ASSERT(d->usingListBox);

	d->listBox->removeEventFilter(this);
	d->listBox->hide();
	d->listBox->setCurrentItem(d->current);

	if (d->arrowDown)
	{
		d->arrowDown = FALSE;
		repaint(FALSE);
	}

	d->poppedUp = FALSE;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::reIndex()
{
	if (!d->usingListBox)
	{
		int cnt = count();

		while (cnt--)
			d->popup->setId(cnt, cnt);
	}
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::currentChanged()
{
	if (d->autoresize)
		adjustSize();

	repaint();
}

////////////////////////////////////////////////////////////////////////////

#ifdef QT_20
#define Event_MouseMove QEvent::MouseMove
#define Event_MouseButtonRelease QEvent::MouseButtonRelease
#define Event_MouseButtonDblClick QEvent::MouseButtonDblClick
#define Event_MouseButtonPress QEvent::MouseButtonPress
#define Event_KeyPress QEvent::KeyPress
#define Event_KeyRelease QEvent::KeyRelease
#define Event_FocusIn QEvent::FocusIn
#define Event_FocusOut QEvent::FocusOut
#endif

bool CTopCombo::eventFilterForListbox(QObject *object, QEvent *event)
{
	QMouseEvent *e = (QMouseEvent*)event;

	switch(event->type())
	{
		case Event_MouseMove:
		{
			if (!d->mouseWasInsidePopup)
			{
				QPoint pos = e->pos();

				if (d->listBox->rect().contains(pos))
					d->mouseWasInsidePopup = TRUE;

				// Check if arrow button should toggle
				// this applies only to windows style

				if (d->arrowPressed)
				{
					QPoint comboPos;
					comboPos = mapFromGlobal(d->listBox->mapToGlobal(pos));

					if (arrowRect().contains(comboPos))
					{
						if (!d->arrowDown )
						{
							d->arrowDown = TRUE;
							repaint(FALSE);
						}
					}
					else
					{
						if (d->arrowDown )
						{
							d->arrowDown = FALSE;
							repaint(FALSE);
						}
					}
				}
			}
		}
		break;

		case Event_MouseButtonRelease:
		{
			if (d->listBox->rect().contains(e->pos()))
			{
				QMouseEvent tmp(Event_MouseButtonDblClick, e->pos(), e->button(), e->state()); // will hide popup
				QApplication::sendEvent(object, &tmp);
				return TRUE;
			}
			else
			{
				if (d->mouseWasInsidePopup)
				{
					popDownListBox();
				}
				else
				{
					d->arrowPressed = FALSE;

					if (d->arrowDown)
					{
						d->arrowDown = FALSE;
						repaint(FALSE);
					}
				}
			}
		}
		break;

		case Event_MouseButtonDblClick:
		case Event_MouseButtonPress:
		{
			if (!d->listBox->rect().contains(e->pos()))
			{
				QPoint globalPos = d->listBox->mapToGlobal(e->pos());

				if (QApplication::widgetAt(globalPos, TRUE) == this)
					d->discardNextMousePress = TRUE;  // avoid popping up again

				popDownListBox();
				return TRUE;
			}
		}
		break;

		case Event_KeyPress:
		{
			switch(((QKeyEvent *)event)->key())
			{
				case Qt::Key_Up:
				case Qt::Key_Down:
					if (!(((QKeyEvent *)event)->state() & AltButton))
						break;

				case Qt::Key_F4:
				case Qt::Key_Escape:
					popDownListBox();
					((QKeyEvent*)event)->accept();
				return TRUE;

				case Qt::Key_Enter:
				case Qt::Key_Return:
					// magic to work around QDialog's enter handling
					((QKeyEvent*)event)->accept();
				return FALSE;

				default:
				break;
			}
		}
		break;

		default:
		break;
	}

	return FALSE;
}

bool CTopCombo::eventFilterForPopup(QObject *object, QEvent *event)
{
	QMouseEvent *e = (QMouseEvent*)event;

	switch (event->type())
	{
		case Event_MouseButtonRelease:
		{
			if (d->shortClick)
			{
				QMouseEvent tmp(Event_MouseMove, e->pos(), e->button(), e->state());// highlight item, but don't pop down:
				QApplication::sendEvent(object, &tmp);
				return TRUE;
			}
		}
		break;

		case Event_MouseButtonDblClick:
		case Event_MouseButtonPress:
		{
			if (!d->popup->rect().contains(e->pos()))
			{
				d->listBox->removeEventFilter(this);
				internalHighlight(d->current);
			}
		}
		break;

		default:
		break;
	}

	return FALSE;
}

void CTopCombo::OnEditTextChanged(const QString & NewText)
{
  if (d->useCompletion && d->completeNow)
  {
    if (NewText &&
        d->ed->cursorPosition() > d->completeAt &&
        d->ed->cursorPosition() == (int)qstrlen(NewText))
    {
      d->completeNow = FALSE;

      if (doAutoCompletion())
        return;

      QString ct(NewText);
      QString it;
      int i =0;
      int foundAt = -1;
      int foundLength = 100000; // lots

      while (i<count())
      {
        it = text(i);

        if (it.length() >= ct.length())
        {
          it.truncate(ct.length());
          int itlen = qstrlen(text(i));

          if (it == ct && itlen < foundLength)
          {
            foundAt = i;
            foundLength = qstrlen(text(i));
          }
        }

        i++;
      }

      if (foundAt > -1)
      {
        it = text(foundAt);

        d->ed->validateAndSet(it, ct.length(),
        ct.length(), it.length());
      }
    }
  }
}

bool CTopCombo::eventFilterForEd(QObject * /*object*/, QEvent *event)
{
	if (event->type() == Event_KeyPress)
	{
    keyPressEvent((QKeyEvent *)event);

		if (((QKeyEvent *)event)->isAccepted())
		{
      d->completeNow = FALSE;
			return TRUE;
		}
		else
		{
			if (((QKeyEvent *)event)->key() != Qt::Key_End)
			{
				d->completeNow = TRUE;
				d->completeAt = d->ed->cursorPosition();
			}
		}
	}
	else
	{
		if (event->type() == Event_KeyRelease)
		{
      d->completeNow = FALSE;
			keyReleaseEvent((QKeyEvent *)event);
			return ((QKeyEvent *)event)->isAccepted();
		}
		else
		{
			if ((event->type() == Event_FocusIn || event->type() == Event_FocusOut))
			{
				d->completeNow = FALSE;
				focusChanged();
				// to get the focus indication right
				update();
			}
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

bool CTopCombo::eventFilter(QObject *object, QEvent *event)
{
	if (!event)
		return TRUE;
	else
	{
		if (object == d->ed)
			eventFilterForEd(object, event);

		if (d->usingListBox && object == d->listBox)
			eventFilterForListbox(object, event);

		if (!d->usingListBox && object == d->popup)
			eventFilterForPopup(object, event);
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

int CTopCombo::sizeLimit() const
{
	return d ? d->sizeLimit : INT_MAX;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setSizeLimit(int lines)
{
	d->sizeLimit = lines;
}

////////////////////////////////////////////////////////////////////////////

int CTopCombo::maxCount() const
{
	return d ? d->maxCount : INT_MAX;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setMaxCount(int count)
{
	int l = this->count();

	while(--l > count)
		removeItem(l);

	d->maxCount = count;
}

////////////////////////////////////////////////////////////////////////////

CTopCombo::Policy CTopCombo::insertionPolicy() const
{
	return d->p;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setInsertionPolicy(Policy policy)
{
	d->p = policy;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::returnPressed()
{
	justBeforeReturnPressed();
	QString s(d->ed->text());

	int c = 0;

	switch (insertionPolicy())
	{
		case AtCurrent:
			if (qstrcmp(s, (const char*)text(currentItem())))
				changeItem((LPCSTR)s, currentItem());
			emit activated(currentItem());
			emit activated(s);
		return;

    case NoInsertion:
			emit activated((const char *)s);
		return;

		case AtTop:
			c = 0;
		break;

		case AtBottom:
			c = count();
		break;

		case BeforeCurrent:
			c = currentItem();
		break;

		case AfterCurrent:
			c = currentItem() + 1;
		break;
	}

	if (count() == d->maxCount)
		removeItem(count() - 1);

	insertItem((const char *)s, c);
	setCurrentItem(c);
	emit activated(c);
	emit activated(s);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setEnabled(bool enable)
{
	if (d && d->ed)
		d->ed->setEnabled(enable);

	QWidget::setEnabled(enable);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setValidator(QValidator * v)
{
	if (d && d->ed)
		d->ed->setValidator(v);
}

////////////////////////////////////////////////////////////////////////////

const QValidator * CTopCombo::validator() const
{
	return d && d->ed ? d->ed->validator() : 0;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::clearValidator()
{
	if (d && d->ed)
		d->ed->setValidator(0);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setListBox(QListBox * newListBox)
{
	clear();

	if (d->usingListBox)
		delete d->listBox;
	else
		delete d->popup;

	newListBox->recreate(0, WType_Popup, QPoint(0,0), FALSE);

	d->listBox = newListBox;
	d->usingListBox = TRUE;

	d->listBox->setAutoScrollBar(FALSE);
	d->listBox->setBottomScrollBar(FALSE);
	d->listBox->setAutoBottomScrollBar(FALSE);
	d->listBox->setFrameStyle(QFrame::Box | QFrame::Plain);
	d->listBox->setLineWidth(1);
	d->listBox->resize(100, 10);

	connect(d->listBox, SIGNAL(selected(int)), SLOT(internalActivate(int)));
	connect(d->listBox, SIGNAL(highlighted(int)), SLOT(internalHighlight(int)));
}

////////////////////////////////////////////////////////////////////////////

QListBox * CTopCombo::listBox() const
{
	return d && d->usingListBox ? d->listBox : 0;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::clearEdit()
{
	if (d && d->ed)
		d->ed->clear();
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setEditText(const char * newText)
{
  if (d && d->ed)
  {
    d->ed->setText(DetachHiddenPrefix(newText, m_nHiddenPrefix));
  }
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::setAutoCompletion(bool enable)
{
	d->useCompletion = enable && (d->ed != 0);
	d->completeNow = FALSE;
}

////////////////////////////////////////////////////////////////////////////

bool CTopCombo::autoCompletion() const
{
	return d->useCompletion;
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::paintEvent(QPaintEvent *event)
{
  QWidget *w = (QWidget*)GetComboEdit(this);

	QRect r = w->geometry();

	if (r.left() == 2)
		w->setGeometry(20, r.top(), r.width()-18, r.height());

	QPainter p(this);

	if (event)
		p.setClipRect(event->rect());

	QColorGroup g = colorGroup();
	QColor bg = isEnabled() ? g.base() : g.background();

	QBrush fill(bg);

	qDrawWinPanel(&p, 0, 0, width(), height(), g, TRUE, NULL);

	p.fillRect(2, r.top(), 18, r.height(), fill);

	QRect arrowR = QRect(width() - 2 - 16, 2, 16, height() - 4);

	qDrawWinPanel(&p, arrowR, g, FALSE);

	qDrawArrow(&p, QTARROWTYPE(Qt::DownArrow), QTGUISTYLE(Qt::WindowsStyle), FALSE,
		    arrowR.x() + 2, arrowR.y() + 2,
		    arrowR.width() - 4, arrowR.height() - 4, g
//#ifdef QT_20
		    , TRUE
//#endif
		    );

	p.drawPixmap(2+(18 - m_Pixmap.width())/2, r.top() + (r.height()-m_Pixmap.height())/2, m_Pixmap);

	p.setClipping(FALSE);
}

////////////////////////////////////////////////////////////////////////////

void CTopCombo::keyPressEvent(QKeyEvent *e)
{
	int c;

	if (e->key() == Qt::Key_F6 && (AltButton != (e->state() & AltButton)))
	{
		e->accept();
		emit TabRequest(ShiftButton != (e->state() & ShiftButton));
		return;
	}

	if (e->key() == Qt::Key_F4 ||
		(e->key() == Qt::Key_Down /*&& (e->state() & AltButton)*/) ||
		(!d->ed && e->key() == Qt::Key_Space))
	{
		e->accept();
		d->popup->setActiveItem(d->current);
		popup();
		return;
	}
	else
	{
		if (d->usingListBox && e->key() == Qt::Key_Up)
		{
			c = currentItem();

			if (c > 0)
				setCurrentItem(c-1);
			else
				setCurrentItem(count()-1);

			e->accept();
    }
		else
		{
			if (d->usingListBox && e->key() == Qt::Key_Down)
			{
				c = currentItem();

				if (++c < count())
					setCurrentItem(c);
				else
					setCurrentItem(0);
				e->accept();
			}
			else
			{
				e->ignore();
				return;
			}
		}
	}

	c = currentItem();
	emit highlighted(c);

	if (text(c))
		emit activated(text(c));

	emit activated(c);
}

////////////////////////////////////////////////////////////////////////////
