/*  This file is part of the KDE libraries
    Copyright (C) 2000 Malte Starostik <malte@kde.org>
    Copyright (C) 2006 Roberto Cappuccio <roberto.cappuccio@gmail.com>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <time.h>

#include <qpixmap.h>
#include <qimage.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QTimerEvent>
#include <QEventLoop>

#include <kapplication.h>
#include <khtml_part.h>

#include "htmlcreator.h"

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new HTMLCreator;
    }
}

HTMLCreator::HTMLCreator()
    : m_html(0)
{
}

HTMLCreator::~HTMLCreator()
{
    delete m_html;
}

bool HTMLCreator::create(const QString &path, int width, int height, QImage &img)
{
    if (!m_html)
    {
        m_html = new KHTMLPart;
        connect(m_html, SIGNAL(completed()), SLOT(slotCompleted()));
        m_html->setJScriptEnabled(false);
        m_html->setJavaEnabled(false);
        m_html->setPluginsEnabled(false);
        m_html->setMetaRefreshEnabled(false);
        m_html->setOnlyLocalReferences(true);
    }
    KURL url;
    url.setPath(path);
    m_html->openURL(url);

    int t = startTimer(5000);

    m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);

    killTimer(t);

    // render the HTML page on a bigger pixmap and use smoothScale,
    // looks better than directly scaling with the QPainter (malte)
    QPixmap pix;
    if (width > 400 || height > 600)
    {
        if (height * 3 > width * 4)
            pix.resize(width, width * 4 / 3);
        else
            pix.resize(height * 3 / 4, height);
    }
    else
        pix.resize(400, 600);

    // light-grey background, in case loadind the page failed
    pix.fill( QColor( 245, 245, 245 ) );

    int borderX = pix.width() / width,
        borderY = pix.height() / height;
    QRect rc(borderX, borderY, pix.width() - borderX * 2, pix.height() - borderY * 2);

    QPainter p;
    p.begin(&pix);
    m_html->paint(&p, rc);
    p.end();

    img = pix.convertToImage();

    m_html->closeURL();

    return true;
}

void HTMLCreator::timerEvent(QTimerEvent *)
{
    m_eventLoop.quit();
}

void HTMLCreator::slotCompleted()
{
    m_eventLoop.quit();
}

ThumbCreator::Flags HTMLCreator::flags() const
{
    return DrawFrame;
}

#include "htmlcreator.moc"

