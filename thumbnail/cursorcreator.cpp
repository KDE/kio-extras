/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cursorcreator.h"

#include <QFile>
#include <QImage>
#include <QMimeData>
#include <QMimeDatabase>
#include <thumbnail.h>

#include <KPluginFactory>

#include <QApplication>
#include <QCommandLineParser>
#include <QtWidgets/qapplication.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>

K_PLUGIN_CLASS_WITH_JSON(CursorCreator, "cursorthumbnail.json")

CursorCreator::CursorCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult CursorCreator::create(const KIO::ThumbnailRequest &request)
{
    const int width = request.targetSize().width();
    const int height = request.targetSize().height();

    XcursorImage *cursor = XcursorFilenameLoadImage(QFile::encodeName(request.url().toLocalFile()).data(), width > height ? height : width);

    if (cursor) {
        QImage img(reinterpret_cast<uchar *>(cursor->pixels), cursor->width, cursor->height, QImage::Format_ARGB32_Premultiplied);

        // Create a deep copy of the image so the image data is preserved
        img = img.copy();
        XcursorImageDestroy(cursor);
        return KIO::ThumbnailResult::pass(img);
    }

    return KIO::ThumbnailResult::fail();
}

int main(int argc, char **argv)
{
    QApplication application{argc, argv};
    application.setApplicationName(u"cursorcreator"_qs);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Thumbnailer binary file"_qs);
    parser.addHelpOption();
    parser.addOptions({
        {{u"i"_qs, u"input"_qs}, u"The input file"_qs, u"input"_qs},
        {{u"o"_qs, u"output"_qs}, u"The output file"_qs, u"output"_qs},
        {{u"s"_qs, u"size"_qs}, u"The width in pixels"_qs, u"size"_qs},
    });
    parser.process(application);
    int size = parser.value(u"size"_qs).toInt();
    auto filepath = QUrl::fromLocalFile(parser.value(u"input"_qs));
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filepath.toLocalFile());
    auto req = KIO::ThumbnailRequest(filepath, QSize(size, size), type.name(), 1.0, 1.0);
    parser.process(application);
    CursorCreator creator(nullptr, QVariantList());
    auto c = creator.create(req);
    if (c.isValid()) {
        c.image().save(parser.value(u"output"_qs));
        return 0;
    } else {
        return 1;
    }
}

#include "cursorcreator.moc"
#include "moc_cursorcreator.cpp"
