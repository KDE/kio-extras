/*  This file is part of the KDE libraries
    Copyright (C) 2000 Malte Starostik <malte.starostik@t-online.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

// $Id$

#include <time.h>

#include <qpixmap.h>
#include <qimage.h>
#include <qpainter.h>

#include <kapp.h>
#include <khtml_part.h>

#include "htmlcreator.h"

extern "C"
{
    ThumbCreator *new_creator()
    {
        return new HTMLCreator;
    }
};

HTMLCreator::HTMLCreator()
    : m_html(0)
{
}

HTMLCreator::~HTMLCreator()
{
    delete m_html;
}

bool HTMLCreator::create(const QString &path, int extent, QPixmap &pix)
{
    if (!m_html)
    {
        m_html = new KHTMLPart;
        connect(m_html, SIGNAL(completed()), SLOT(slotCompleted()));
        m_html->enableJScript(false);
        m_html->enableJava(false);
        m_html->enablePlugins(false);
    }
    m_html->openURL(path);
    m_completed = false;
    startTimer(5000);
    while (!m_completed)
        kapp->processOneEvent();
    killTimers();
        
    // render the HTML page on a bigger pixmap and use smoothScale,
    // looks better than directly scaling with the QPainter (malte)
    pix.resize(600, 640);
    // light-grey background, in case loadind the page failed
    pix.fill( QColor( 245, 245, 245 ) );

    float ratio = 15.0 / 16.0; // so we get a page-like size
    int width = (int) (ratio * (float) extent);
    int borderX = pix.width() / width,
        borderY = pix.height() / extent;
    QRect rc(borderX, borderY, pix.width() - borderX * 2, pix.height() - borderY * 2);

    QPainter p;
    p.begin(&pix);
    m_html->paint(&p, rc);
    p.end();
 
    pix.convertFromImage(pix.convertToImage().smoothScale(width, extent));
    return true;
}

void HTMLCreator::timerEvent(QTimerEvent *)
{
    m_html->closeURL();
    m_completed = true;
}

void HTMLCreator::slotCompleted()
{
    m_completed = true;
}

ThumbCreator::Flags HTMLCreator::flags() const
{
    return DrawFrame;
}

#include "htmlcreator.moc"

