/* Name: corellogo.cpp

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

#include "common.h"
#include "qrect.h"
#include "corellogo.h"
#include "qapplication.h"
#include "progress.h"
#include <unistd.h>

CCorelLogo::CCorelLogo(QWidget *pParent, int w, int h) : QPushButton(pParent)
{
	m_wd = w;
	m_ln = h;
	//setAutoResize(TRUE);
	QByteArray x;
	x.duplicate((LPCSTR)&progress_gif_data[0], sizeof(progress_gif_data)/sizeof(unsigned char));

	m_Movie = QMovie(x);
	qApp->processOneEvent();
	setPixmap(m_Movie.framePixmap());
	m_Movie.pause();

	m_Movie.connectUpdate(this, SLOT(OnMovieUpdate(const QRect&)));
}

void CCorelLogo::OnMovieUpdate(const QRect&)
{
	extern pid_t gMainPID;
	
	if (gMainPID == getpid())
		setPixmap(m_Movie.framePixmap());
}

void CCorelLogo::Pause()
{
	//printf("CorelLogo Pause\n");
	m_Movie.pause();
}

/////////////////////////////////////////////////////////////////////////////

void CCorelLogo::UnPause()
{
	//printf("CorelLogo UnPause\n");
	m_Movie.unpause();
}

CCorelLogo::~CCorelLogo()
{
}

#ifdef OLD_OLD

#include <stdlib.h>
#include <qbitmap.h>
#include "qpalette.h"

#define EXTRA 14

// the points for the Corel logo in counterclockwise order

struct DDDpoint CorelPoints[] =
{
	// Square One
	1600,100,0, 	1900,400,0, 	1600,700,0, 	1300,400,0,
	1600,100,300, 	1900,400,300, 	1600,700,300, 	1300,400,300,
	// Square two
	1300,0,0, 		1300,400,0, 	800,400,0, 		800,0,0,
	1300,0,300, 	1300,400,300, 	800,400,300, 	800,0,300,
	// Square three
	500,100,0,		800,400,0,		500,700,0,		200,400,0,
	500,100,300,	800,400,300,	500,700,300,	200,400,300,
	// Square four
	50,700,0,		500,700,0,		500,1100,0,		50,1100,0,
	50,700,300,		500,700,300,	500,1100,300,	50,1100,300,
	// Square five
	200,1400,0,		500,1100,0,		800,1400,0,		500,1700,0,
	200,1400,300,	500,1100,300,	800,1400,300,	500,1700,300,
	// Square six
	800,1400,0,		1300,1400,0,	1300,1800,0,	800,1800,0,
	800,1400,300,	1300,1400,300,	1300,1800,300,	800,1800,300,
	// Square seven
	1300,1400,0,    1600,1100,0,    1900,1400,0,    1600,1700,0,
	1300,1400,300,  1600,1100,300,  1900,1400,300,  1600,1700,300,
};


////////////////////////////////////////////////////////////////////////////////

// Coordinates for the center of the object

const int OriginX=950;
const int OriginY=900;
const int OriginZ=150;

////////////////////////////////////////////////////////////////////////////////
// CCorelAbout dialog

CCorelLogo::CCorelLogo(QWidget *pParent, int w, int h) : QPushButton(pParent)
{
	//setFrameStyle(Panel|Sunken);
	//setLineWidth(2);
	//setMidLineWidth(2);
	m_wd = w;
	m_ln = h;

	resize(w,h);

	AngleX=AngleY=AngleZ=0;
	ThetaX=ThetaY=ThetaZ=0;
	TDX=TDY=TDZ=0;
	Zoomed=0;

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(GoLogo()));
	
	// Create the off-screen bitmap we render into

	MemDC = new QPixmap(w+EXTRA, h+EXTRA);
}

////////////////////////////////////////////////////////////////////////////////

CCorelLogo::~CCorelLogo()
{
	delete MemDC;
	m_pTimer->stop();

	MemDC = NULL;
}

/////////////////////////////////////////////////////////////////////////////

void CCorelLogo::Pause()
{
	m_pTimer->stop();
}

/////////////////////////////////////////////////////////////////////////////

void CCorelLogo::UnPause()
{
 	m_pTimer->start(50);
}

/////////////////////////////////////////////////////////////////////////////
// CCorelAbout message handlers

void CCorelLogo::GoLogo()
{
	RedrawLogo();

	AngleX+=ThetaX;
	AngleY+=ThetaY;
	AngleZ+=ThetaZ;

	ThetaX+=TDX;
	ThetaY+=TDY;
	ThetaZ+=TDZ;

	TDX+=((rand()%100)-50)/100000.0;
	TDY+=((rand()%100)-50)/100000.0;
	TDZ+=((rand()%100)-50)/100000.0;

	if (ThetaX>0.15)
	{
		ThetaX=0.15;
		TDX=-TDX/2;
	} 
	else
		if (ThetaX<-0.15)
		{
			ThetaX=-0.15;
			TDX=-TDX/2;
		};

	if (ThetaY>0.15)
	{
		ThetaY=0.15;
		TDY=-TDY/2;
	}
	else
		if (ThetaY<-0.15)
		{
			ThetaY=-0.15;
			TDY=-TDY/2;
		};

	if (ThetaZ>0.15)
	{
		ThetaZ=0.15;
		TDZ=-TDZ/2;
	} 
	else
		if (ThetaZ<-0.15)
		{
			ThetaZ=-0.15;
			TDZ=-TDZ/2;
		};
}

// Quicksort callback function

static int shortsort(const void *one, const void *two)
{
	return((*(const short *)two)-(*(const short *)one));
}

// Rotate In into Out by the angles specified

static void RotatePoints(short NumPoints, double AngleX, double AngleY, double AngleZ, struct DDDpoint *In, struct DDDpoint *Out)
{
	short tx, ty;

	while (NumPoints--)
	{
		Out->x=In->x-OriginX;
		Out->y=In->y-OriginY;
		Out->z=In->z-OriginZ;

		// rotate around y
		tx=Out->x;
		Out->x=(short)(Out->x*cos(AngleY)-Out->z*sin(AngleY));
		Out->z=(short)(tx*sin(AngleY)+Out->z*cos(AngleY));

		// rotate around x
		ty=Out->y;
		Out->y=(short)(Out->y*cos(AngleX)-Out->z*sin(AngleX));
		Out->z=(short)(ty*sin(AngleX)+Out->z*cos(AngleX));

		// rotate around z
		tx=Out->x;
		Out->x=(short)(Out->x*cos(AngleZ)-Out->y*sin(AngleZ));
		Out->y=(short)(tx*sin(AngleZ)+Out->y*cos(AngleZ));

		In++;
		Out++;
	};
}

const int Perspective = 0x1fff;
// Add perspective to the points, convert Z into Distance

void CCorelLogo::PerspectPoints(short NumPoints, struct DDDpoint *Pt, short CenterX, short CenterY)
{
	long scale = (CenterX>=CenterY) ? CenterY : CenterX;
	scale = (scale << 15) / Perspective;
	
	int zoff = (int)(((1500l/*1636l*/)<<15)/Perspective);
	
	while (NumPoints--)
	{
		Pt->z+=zoff;

		short NewZ=(short)sqrt((double)((long)Pt->x*Pt->x) + ((long)Pt->y*Pt->y) + ((long)Pt->z*Pt->z));

		if (Pt->z)
		{
			Pt->x=(short)(CenterX+((long)((long)Pt->x*scale)/Pt->z));
			Pt->y=(short)(CenterY+((long)((long)Pt->y*scale)/Pt->z));
		}

		Pt->z=NewZ;
		Pt++;
	}
}

// Draw the 3 polygons in a cube

void CCorelLogo::DrawCube(QPainter &dc)
{
	QPointArray Array(5);
	short Back[3];

	// Build the distance array in Back with the poly number in the bottom 2 bits

	for (int s=0; s<3; s++)
	{
		short Far=0;

		for (int t=0; t<4; t++)
			if (CubePoints[s][t].z>Far)
				Far=CubePoints[s][t].z;

		Back[s]=(Far<<3)|s;
	};

	// Sort the Back array
	qsort(Back, 3, sizeof(short), shortsort);

	QBrush Green(QColor(0x0, 0x20, 0x90));
	dc.setBrush(Green);

	for (int poly=0; poly<3; poly++)
	{
		int x=Back[poly]&7;

		Array.setPoint(0, CubePoints[x][0].x, CubePoints[x][0].y);
		Array.setPoint(1, CubePoints[x][1].x, CubePoints[x][1].y);
		Array.setPoint(2, CubePoints[x][2].x, CubePoints[x][2].y);
		Array.setPoint(3, CubePoints[x][3].x, CubePoints[x][3].y);

		dc.drawPolygon(Array, FALSE, 0, 4);
	}
}

// Consider this polygon for inclusion in this cube

void CCorelLogo::ConsiderPoly(struct DDDpoint *Pt, struct DDDpoint *Far, short a, short b, short c, short d)
{
	// Toss away polygons which touch the farthest point

	if (Pt+a==Far || Pt+b==Far || Pt+c==Far || Pt+d==Far)
		return;

	CubePoints[Polys][0]=*(Pt+a);
	CubePoints[Polys][1]=*(Pt+b);
	CubePoints[Polys][2]=*(Pt+c);
	CubePoints[Polys++][3]=*(Pt+d);
}

// Draw one frame

void CCorelLogo::RedrawLogo()
{
	QRect WinSize(0,0,width()-4+EXTRA,height()-4+EXTRA);
	QPainter dc(MemDC);

	short CX=(width()-4+EXTRA)/2;
	short CY=(height()-4+EXTRA)/2;

	// Rotate the points

	RotatePoints(NUM_POINTS, AngleX, AngleY, AngleZ, CorelPoints, PointBuffer);

	// Figure out the order to draw the cubes in: determine the average distance
	// for each one by averaging the Z values of the points.  Do this before
	// perspecting the points.

	short Zavg[7];

	for (int x=0; x<7; x++)
	{
		long Total=0;
		
		for (int y=0; y<8; y++)
			Total+=PointBuffer[(x*8)+y].z;

		// Use the bottom 3 bits to store the square number this represents.
		// This is so we can sort the Zavg array and then use the bottom
		// 3 bits to extract the square number to draw.
		Zavg[x]=(short)(((Total/7)<<3)|x);
	};

	// Do perspective

	PerspectPoints(NUM_POINTS, PointBuffer, CX, CY);

	// Draw the recessed border around the image

	//QBrush Back(colorGroup().midlight());
	QBrush Back(colorGroup().background());
	dc.fillRect(WinSize, Back);

	// Set the pen for the outline
	
	QPen WhitePen(QColor(255,255,255), 1);
	
	dc.setPen(WhitePen);

	qsort(Zavg, 7, sizeof(short), shortsort);

	for (int sq=0; sq<7; sq++)
	{
		int x=Zavg[sq]&7;
		struct DDDpoint *Pt=&PointBuffer[x*8];
		struct DDDpoint *Far=Pt;

		for (int t=1; t<8; t++)
			if (Pt[t].z>Far->z) Far=&Pt[t];

		Polys=0;

		ConsiderPoly(Pt, Far, 0, 1, 2, 3);
		ConsiderPoly(Pt, Far, 4, 5, 6, 7);
		ConsiderPoly(Pt, Far, 0, 1, 5, 4);
		ConsiderPoly(Pt, Far, 1, 2, 6, 5);
		ConsiderPoly(Pt, Far, 2, 3, 7, 6);
		ConsiderPoly(Pt, Far, 0, 3, 7, 4);

		DrawCube(dc);
	};

	::bitBlt(this, 2, 2, MemDC, (isDown() ? 0:2)+EXTRA/2, (isDown() ? 0:2)+EXTRA/2, width()-4, height()-4, CopyROP, TRUE);
}


void CCorelLogo::paintEvent(QPaintEvent *e)
{
	QPushButton::paintEvent(e);
	RedrawLogo();
}

#endif /* OLD_OLD */

