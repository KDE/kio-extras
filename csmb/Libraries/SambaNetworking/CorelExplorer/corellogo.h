/* Name: corellogo.h

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

#ifndef __INC_CORELLOGO_H__
#define __INC_CORELLOGO_H__

#ifdef OLD_OLD

#include <qwidget.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qpointarray.h>
#include <qpixmap.h>
#include <qpaintdevice.h>
#include <qtimer.h>
#include <qsize.h>
#include "qframe.h"
#include "qpushbutton.h"

#define NUM_POINTS	(7*4*2)		// 7 cubes, 8 points per cube

// three dimensional point structure

struct DDDpoint
{
	short x;
	short y;
	short z;
};

#else

#include "qpushbutton.h"
#include "qlabel.h"
#include "qmovie.h"
#endif

class CCorelLogo : public QPushButton
{
	Q_OBJECT
public:
	CCorelLogo(QWidget *pParent, int w, int h);
	~CCorelLogo();
	virtual QSize sizeHint() const { return QSize(m_wd,m_ln); }

public slots:
	void Pause();
	void UnPause();
	void OnMovieUpdate(const QRect &);


#ifdef OLD_OLD
/*	void GoLogo();

protected:
	void RedrawLogo();
	void PerspectPoints(short NumPoints, struct DDDpoint *Pt, short CenterX, short CenterY);
	void DrawCube(QPainter &dc);
	void ConsiderPoly(struct DDDpoint *Pt, struct DDDpoint *Far, short a, short b, short c, short d);
	
	void paintEvent(QPaintEvent *);

private:	
	
	struct DDDpoint PointBuffer[NUM_POINTS];
	double AngleX, AngleY, AngleZ;
	double ThetaX, ThetaY, ThetaZ;
	double TDX, TDY, TDZ;
	
	QPixmap *MemDC;

	// 3d control size

	char Zoomed;
	short OrigWidth, OrigHeight;
	void EndLogo();
	// Constants used while sorting a single polygon.

	struct DDDpoint CubePoints[3][4];
	char Polys;

	QTimer *m_pTimer;
*/
#endif

private:	
	QMovie m_Movie;
	int m_wd;
	int m_ln;
};

#endif /* __INC_CORELLOGO_H__ */
