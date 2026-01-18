/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2022 Nicolas Fella <nicolas.fella@gmx.de>
*/

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <KIO/PreviewJob>

class ThumbnailTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void testThumbnail_data()
    {
        QTest::addColumn<QString>("inputFile");
        QTest::addColumn<QString>("expectedThumbnail");
        QTest::addColumn<qreal>("dpr");
        QTest::addColumn<QSize>("size");

        QTest::addRow("png") << "cherry_tree.png"
                             << "cherry_tree_thumb.png" << 1.0 << QSize(128, 128);

        QTest::addRow("png_dpr2") << "cherry_tree.png"
                                  << "cherry_tree_thumb@2.png" << 2.0 << QSize(128, 128);

        QTest::addRow("jpg") << "boxes.jpg"
                             << "boxes_thumb.png" << 1.0 << QSize(128, 128);

        QTest::addRow("different_size") << "wallpaper.jpg"
                                        << "wallpaper_thumb.png" << 1.0 << QSize(480, 300);

        QTest::addRow("jpg_dpr2") << "boxes.jpg"
                                  << "boxes_thumb@2.png" << 2.0 << QSize(128, 128);

        QTest::addRow("jpg_embedded_thumbnail") << "castle.jpg"
                                                << "castle_thumb.png" << 1.0 << QSize(128, 128);

        // ignoring embedded thumbnail as it is too small
        QTest::addRow("jpg_embedded_thumbnail2") << "castle.jpg"
                                                 << "castle_thumb_256.png" << 2.0 << QSize(128, 128);

        // image using 4 colors table
        QTest::addRow("Screen_color_test_Amiga_4colors") << "Screen_color_test_Amiga_4colors.png"
                                                         << "Screen_color_test_Amiga_4colors_converted.png" << 2.0 << QSize(128, 128);

        QTest::addRow("folder-database.svg") << "folder-database.svg"
                                             << "folder-database-128@2.png" << 2.0 << QSize(128, 128);

        QTest::addRow("Kingdom of Yugoslavia.svg") << "Kingdom of Yugoslavia.svg"
                                                   << "Kingdom of Yugoslavia-128@2.png" << 2.0 << QSize(128, 128);

        QTest::addRow("DZSCG.svg") << "DZSCG.svg"
                                   << "DZSCG-128@2.png" << 2.0 << QSize(128, 128);

        QTest::addRow("exe_ne16") << "ne16_1bpp.exe"
                                  << "ne16_1bpp_thumb.png" << 1.0 << QSize(128, 128);

        QTest::addRow("exe_4bpp") << "ne16_4bpp.exe"
                                  << "ne16_4bpp_thumb.png" << 1.0 << QSize(128, 128);

        QTest::addRow("exe_8bpp") << "ne16_8bpp.exe"
                                  << "ne16_8bpp_thumb.png" << 1.0 << QSize(128, 128);

        QTest::addRow("exe_pe32") << "pe32_32bpp.exe"
                                  << "pe32_32bpp_thumb.png" << 1.0 << QSize(128, 128);

        QTest::addRow("exe_pe32+") << "pe32plus_32bpp.exe"
                                   << "pe32plus_32bpp_thumb.png" << 1.0 << QSize(128, 128);

        QTest::addRow("empty_svg") << "trivial.svg"
                                   << "trivial.png" << 1.0 << QSize(128, 128);
    }

    void testThumbnail()
    {
        QFETCH(QString, inputFile);
        QFETCH(QString, expectedThumbnail);
        QFETCH(qreal, dpr);
        QFETCH(QSize, size);

        QStandardPaths::setTestModeEnabled(true);
        qputenv("KIOWORKER_ENABLE_TESTMODE", "1"); // ensure the worker call QStandardPaths::setTestModeEnabled too

        // wipe thumbnail cache so we always start clean
        QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation));
        cacheDir.removeRecursively();

        QString path = QFINDTESTDATA("data/" + inputFile);

        KFileItemList items;
        items.append(KFileItem(QUrl::fromLocalFile(path)));

        QStringList enabledPlugins{"svgthumbnail", "imagethumbnail", "jpegthumbnail", "windowsexethumbnail"};
        auto *job = KIO::filePreview(items, size, &enabledPlugins);
        job->setDevicePixelRatio(dpr);

        connect(job, &KIO::PreviewJob::gotPreview, this, [path, expectedThumbnail, dpr](const KFileItem &item, const QPixmap &preview) {
            QCOMPARE(item.url(), QUrl::fromLocalFile(path));

            QImage expectedImage;
            expectedImage.load(QFINDTESTDATA("data/" + expectedThumbnail));
            expectedImage.setDevicePixelRatio(dpr);

            if (expectedImage.format() == QImage::Format_ARGB32) {
                // QImage load loads differently from KPreviewJob
                expectedImage = expectedImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            }

            QCOMPARE(preview.devicePixelRatio(), dpr);
            QCOMPARE(preview.toImage(), expectedImage);
        });

        QSignalSpy failedSpy(job, &KIO::PreviewJob::failed);
        QSignalSpy gotPreviewSpy(job, &KIO::PreviewJob::gotPreview);
        QSignalSpy resultSpy(job, &KIO::PreviewJob::result);

        resultSpy.wait();

        QVERIFY2(failedSpy.empty(), qPrintable(job->errorString()));
        QVERIFY(!gotPreviewSpy.empty());
    }
};

QTEST_MAIN(ThumbnailTest)

#include "thumbnailtest.moc"
