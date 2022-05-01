/*
    SPDX-License-Identifier: LGPL-2.1-or-later OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Stefan Brüns <stefan.bruens@rwth-aachen.de>
*/

#include "djvucreator.h"
#include "thumbnail-djvu-logsettings.h"

#include <QProcess>
#include <QString>
#include <QImage>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(DjVuCreator, "djvuthumbnail.json")

DjVuCreator::DjVuCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult DjVuCreator::create(const KIO::ThumbnailRequest &request)
{
    QProcess ddjvu;

    const QStringList args{QStringLiteral("-page=1"),
                           QStringLiteral("-size=") + QString::number(request.targetSize().width()) + QChar('x')
                               + QString::number(request.targetSize().height()),
                           QStringLiteral("-format=ppm"),
                           request.url().toLocalFile()};

    ddjvu.start(QStringLiteral("ddjvu"), args);
    ddjvu.waitForFinished();

    static bool warnOnce = true;
    if (ddjvu.exitCode() != 0) {
        if (warnOnce) {
            qCWarning(KIO_THUMBNAIL_DJVU_LOG) << ddjvu.error()
                                              << ddjvu.readAllStandardError();
            warnOnce = false;
        }
        return KIO::ThumbnailResult::fail();
    }

    QImage img;
    bool okay = img.load(&ddjvu, "ppm");
    return okay ? KIO::ThumbnailResult::pass(img) : KIO::ThumbnailResult::fail();
}

#include "djvucreator.moc"
