/*
    SPDX-License-Identifier: LGPL-2.1-or-later OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Stefan Brüns <stefan.bruens@rwth-aachen.de>
*/

#include "djvucreator.h"
#include "thumbnail-djvu-logsettings.h"

#include "macros.h"

#include <QProcess>
#include <QString>
#include <QImage>

EXPORT_THUMBNAILER_WITH_JSON(DjVuCreator, "djvuthumbnail.json")

bool DjVuCreator::create(const QString &path, int width, int height, QImage &img)
{
    QProcess ddjvu;

    const QStringList args{
        QStringLiteral("-page=1"),
        QStringLiteral("-size=") + QString::number(width) + QChar('x') + QString::number(height),
        QStringLiteral("-format=ppm"),
        path
    };

    ddjvu.start(QStringLiteral("ddjvu"), args);
    ddjvu.waitForFinished();

    static bool warnOnce = true;
    if (ddjvu.exitCode() != 0) {
        if (warnOnce) {
            qCWarning(KIO_THUMBNAIL_DJVU_LOG) << ddjvu.error()
                                              << ddjvu.readAllStandardError();
            warnOnce = false;
        }
        return false;
    }

    img.load(&ddjvu, "ppm");
    return true;
}

#include "djvucreator.moc"
