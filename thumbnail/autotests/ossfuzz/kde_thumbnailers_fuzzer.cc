/*
 * SPDX-FileCopyrightText: 2025 Azhar Momin <azhar.momin@kdemail.net>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QByteArray>
#include <QGuiApplication>
#include <QMimeDatabase>
#include <QSize>
#include <QTemporaryFile>
#include <QUrl>
#include <QVariant>
#include <QtEnvironmentVariables>
#include <QtPlugin>

#include "appimagecreator.h"
#include "audiocreator.h"
#include "comiccreator.h"
#include "cursorcreator.h"
#include "djvucreator.h"
#include "ebookcreator.h"
#include "exrcreator.h"
#include "imagecreator.h"
#include "jpegcreator.h"
#include "kritacreator.h"
#include "opendocumentcreator.h"
#include "svgcreator.h"
#include "textcreator.h"
#include "windowsexecreator.h"
#include "windowsimagecreator.h"

Q_IMPORT_PLUGIN(QMinimalIntegrationPlugin)

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    qputenv("QT_QPA_PLATFORM", "minimal");

    int argc = 0;
    QGuiApplication app(argc, nullptr);

    QByteArray b(reinterpret_cast<const char *>(data), static_cast<int>(size));

    QTemporaryFile f;
    if (!f.open()) {
        return 0;
    }
    f.write(b);
    f.close();

    QMimeDatabase mimeDb;
    QString mimetype = mimeDb.mimeTypeForData(b).name();

    QSize targetSize(128, 128);
    qreal dpr = 1.0;
    float sequenceIndex = 0.0f;

    KIO::ThumbnailRequest request(QUrl::fromLocalFile(f.fileName()), targetSize, mimetype, dpr, sequenceIndex);

    CREATOR thumbnailer(nullptr, {});
    KIO::ThumbnailResult result = thumbnailer.create(request);

    return 0;
}
