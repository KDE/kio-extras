/* Name: topcombo.h

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

#ifndef __INC_CFM_TOPCOMBO_H__
#define __INC_CFM_TOPCOMBO_H__

#include "common.h"
#include "qwidget.h"
#include <qlistbox.h>

struct CTopComboData;
class QStrList;
class QLineEdit;
class QValidator;
class QListBox;

class CTopCombo : public QWidget
{
  Q_OBJECT
public:
	CTopCombo(QWidget *parent=0, const char *name=0);
	CTopCombo(bool rw, QWidget *parent=0, const char *name=0);
	~CTopCombo();

	void SetPixmap(const QPixmap *pPixmap)
	{
		m_Pixmap = *pPixmap;
		update();
	}

	int count() const;

	void insertStrList(const QStrList *, int index=-1);
	void insertStrList(const char **, int numStrings=-1, int index=-1);

	void insertItem(const char *text, int index=-1);
	void insertItem(const QPixmap &pixmap, int index=-1);

	void removeItem(int index);
	void clear();

	const char *currentTextUI() const;
  QString currentText() const;
	const char *text(int index) const;
	const QPixmap *pixmap(int index) const;

	void changeItem(const char *text, int index);
	void changeItem(const QPixmap &pixmap, int index);

	int	currentItem() const;
	void setCurrentItem(int index);

	bool autoResize()	const;
	void setAutoResize(bool);
	QSize	sizeHint() const;
	void setBackgroundColor(const QColor &);
	void setPalette(const QPalette &);
	void setFont(const QFont &);
	void setEnabled(bool);

	void setSizeLimit(int);
	int	sizeLimit() const;

	void setMaxCount(int);
	int	maxCount() const;

	enum Policy { NoInsertion, AtTop, AtCurrent, AtBottom,
		AfterCurrent, BeforeCurrent };

	void setInsertionPolicy(Policy policy);
	Policy insertionPolicy() const;

#ifndef QT_20
  void setStyle(GUIStyle);
#endif

	void setValidator(QValidator *);
	const QValidator *validator() const;

	void setListBox(QListBox *);
	QListBox *listBox() const;

	void setAutoCompletion(bool);
	bool autoCompletion() const;

	bool eventFilter(QObject *object, QEvent *event);
	bool eventFilterForListbox(QObject *object, QEvent *event);
	bool eventFilterForPopup(QObject *object, QEvent *event);
	bool eventFilterForEd(QObject *object, QEvent *event);

  int HiddenPrefix() const
  {
    return m_nHiddenPrefix;
  }

public slots:
	void clearValidator();
	void clearEdit();
	void setEditText(const char *);
  void OnEditTextChanged(const QString &text);

signals:
	void TabRequest(BOOL bIsBacktab);
	void activated(int index);
	void highlighted(int index);
	void activated(const char *);
	void highlighted(const char *);

private slots:
	void internalActivate(int);
  void internalActivate(QListBoxItem * item);
	void internalHighlight(int);
	void internalClickTimeout();
	void returnPressed();

protected:
	void paintEvent(QPaintEvent *);
	void resizeEvent(QResizeEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void mouseDoubleClickEvent(QMouseEvent *);
	void keyPressEvent(QKeyEvent *e);
	void focusInEvent(QFocusEvent *e);

	void popup();
	QLineEdit *getLineEdit() const;
	virtual void focusChanged() { }
	virtual bool doAutoCompletion() { return FALSE; }
	virtual void justBeforeReturnPressed() { }

private:
	void popDownListBox();
	void reIndex();
	void currentChanged();
	QRect	arrowRect() const;
	bool getMetrics(int *dist, int *buttonW, int *buttonH) const;

  CTopComboData *d;
	QPixmap m_Pixmap;
  int m_nHiddenPrefix;

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
	CTopCombo(const CTopCombo &);
	CTopCombo &operator=(const CTopCombo &);
#endif
};

QString AttachHiddenPrefix(const char *text, int nHiddenPrefix);

#endif /* __INC_CFM_TOPCOMBO_H__ */
