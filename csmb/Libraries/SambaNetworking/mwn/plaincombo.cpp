#include "plaincombo.h"
#include "qpainter.h"
#include "qbrush.h"
#include "qdrawutil.h"
#ifdef QT_20
#include <qlineedit.h>
#include <q1xcompatibility.h>
#else
#include "corellineedit.h"
#endif

void CPlainCombo::paintEvent(QPaintEvent *event)
{
	QPainter p(this);

	if (event)
		p.setClipRect(event->rect());

	QColorGroup g = colorGroup();
	QColor bg = isEnabled() ? g.base() : g.background();
	
	QBrush fill(bg);
	
	qDrawWinPanel(&p, 0, 0, width(), height(), g, TRUE, NULL);

	//p.fillRect(2, rect().top(), 18, rect().height(), fill);

	QRect arrowR = QRect(width() - 2 - 16, 2, 16, height() - 4);

	qDrawWinPanel(&p, arrowR, g, FALSE);
	
#ifdef QT_2	
	qDrawArrow(&p, (ArrowType)Qt::DownArrow, (GUIStyle)Qt::WindowsStyle, FALSE,
#else
	qDrawArrow(&p, (ArrowType)DownArrow, (GUIStyle)WindowsStyle, FALSE,
#endif
		    arrowR.x() + 2, arrowR.y() + 2,
		    arrowR.width() - 4, arrowR.height() - 4, g
//#ifdef QT_20
		    , TRUE
//#endif
		    );
	
	p.setClipping(FALSE);
}

bool CPlainCombo::eventFilter(QObject *object, QEvent *e)
{
  if (!strcmp(object->name(),"combo edit") &&
      e->type() == Event_FocusIn)
  {
#ifdef QT_20
    QLineEdit *pEdit = (QLineEdit *)object;
#else
    CCorelLineEdit *pEdit = (CCorelLineEdit *)object;
#endif
    int nLen = strlen(pEdit->text());
    pEdit->setSelection(0, nLen);
    pEdit->setCursorPosition(nLen);
    pEdit->repaint(FALSE);
  }
  
  return CCorelComboBox::eventFilter(object, e);
}

