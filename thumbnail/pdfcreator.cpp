/*
 * Copyright (c) 2020 Philippe Michaud-Boudreault <pitwuu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pdfcreator.h"

#include <QImage>
#include <QString>
#include <QScopedPointer>

#include <poppler-qt5.h>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new PDFCreator;
    }
}

PDFCreator::PDFCreator() = default;
PDFCreator::~PDFCreator() = default;

bool PDFCreator::create(const QString &path, int width, int height, QImage &image)
{
    Poppler::Document* document;
    Poppler::Page* page;

    document = Poppler::Document::load(path);
    if (!document || document->isLocked())
    {
        delete document;
        return false;
    }

    // Document starts at page 0.
    page = document->page(0);
    if (!page)
    {
        delete document;
        return false;
    }

    // Generate a QImage of the rendered page.
    image = page->renderToImage().scaled(width, height);

    delete page;
    delete document;

    return true;
}

