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

        QTest::addRow("png") << "cherry_tree.png"
                             << "cherry_tree_thumb.png" << 1.0;

        QTest::addRow("png_dpr2") << "cherry_tree.png"
                                  << "cherry_tree_thumb@2.png" << 2.0;

        QTest::addRow("jpg") << "boxes.jpg"
                             << "boxes_thumb.png" << 1.0;

        QTest::addRow("jpg_dpr2") << "boxes.jpg"
                                  << "boxes_thumb@2.png" << 2.0;

        QTest::addRow("jpg_embedded_thumbnail") << "castle.jpg"
                                                << "castle_thumb.png" << 1.0;

        // ignoring embedded thumbnail as it is too small
        QTest::addRow("jpg_embedded_thumbnail2") << "castle.jpg"
                                                 << "castle_thumb_256.png" << 2.0;
    }

    void testThumbnail()
    {
        QFETCH(QString, inputFile);
        QFETCH(QString, expectedThumbnail);
        QFETCH(qreal, dpr);

        QStandardPaths::setTestModeEnabled(true);
        qputenv("KIOWORKER_ENABLE_TESTMODE", "1"); // ensure the worker call QStandardPaths::setTestModeEnabled too

        // wipe thumbnail cache so we always start clean
        QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation));
        cacheDir.removeRecursively();

        QString path = QFINDTESTDATA("data/" + inputFile);

        KFileItemList items;
        items.append(KFileItem(QUrl::fromLocalFile(path)));

        QStringList enabledPlugins{"imagethumbnail", "jpegthumbnail"};
        auto *job = KIO::filePreview(items, QSize(128, 128), &enabledPlugins);
        job->setDevicePixelRatio(dpr);

        QSignalSpy failedSpy(job, &KIO::PreviewJob::failed);
        QSignalSpy gotPreviewSpy(job, &KIO::PreviewJob::gotPreview);
        QSignalSpy resultSpy(job, &KIO::PreviewJob::result);

        connect(job, &KIO::PreviewJob::gotPreview, this, [path, expectedThumbnail, dpr](const KFileItem &item, const QPixmap &preview) {
            QCOMPARE(item.url(), QUrl::fromLocalFile(path));

            QImage expectedImage;
            expectedImage.load(QFINDTESTDATA("data/" + expectedThumbnail));
            expectedImage.setDevicePixelRatio(dpr);

            QCOMPARE(preview.toImage(), expectedImage);
        });

        QVERIFY2(resultSpy.wait(15000), qPrintable(job->errorText()));

        QCOMPARE(job->error(), KJob::NoError);
        QVERIFY(failedSpy.empty());
        QVERIFY(!gotPreviewSpy.isEmpty());
    }
};

QTEST_MAIN(ThumbnailTest)

#include "thumbnailtest.moc"
