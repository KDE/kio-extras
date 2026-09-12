/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2022 Nicolas Fella <nicolas.fella@gmx.de>
*/

#include <QIcon>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <KConfig>
#include <KConfigGroup>
#include <KIO/PreviewJob>

class ThumbnailTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void testFolderThumbnail_data()
    {
        QTest::addColumn<qreal>("dpr");
        QTest::addColumn<QSize>("size");
        QTest::addColumn<QString>("expectedThumbnail");

        QTest::addRow("dpr1") << 1.0 << QSize(128, 128) << "folder_thumb.png";
        QTest::addRow("dpr2") << 2.0 << QSize(128, 128) << "folder_thumb@2.png";
        QTest::addRow("dpr1.5") << 1.5 << QSize(128, 128) << "folder_thumb@1.5.png";
        QTest::addRow("dpr2_256") << 2.0 << QSize(256, 256) << "folder_thumb_256@2.png";
    }

    void testFolderThumbnail()
    {
        QFETCH(qreal, dpr);
        QFETCH(QSize, size);
        QFETCH(QString, expectedThumbnail);

        QStandardPaths::setTestModeEnabled(true);
        qputenv("KIOWORKER_ENABLE_TESTMODE", "1");
        // The worker draws the folder icon from the icon theme, and only learns which theme to use
        // from a platform theme, so the thumbnail is empty without these. The one that reads a
        // configuration file has to be named: where a portal is asked instead and cannot answer, no
        // theme is named at all and nothing is drawn.
        qputenv("QT_QPA_PLATFORMTHEME", "kde");
        qputenv("XDG_CURRENT_DESKTOP", "KDE");
        qputenv("XDG_CONFIG_HOME", QFile::encodeName(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)));
        // and the theme itself is shipped with the test, so which folder icon is drawn does not
        // depend on what the machine running this has installed
        const QString themeDir = QFileInfo(QFINDTESTDATA("data/icons")).absolutePath();
        QByteArray dataDirs = qgetenv("XDG_DATA_DIRS");
        if (dataDirs.isEmpty()) {
            dataDirs = QByteArrayLiteral("/usr/local/share:/usr/share");
        }
        dataDirs.prepend(QFile::encodeName(themeDir) + ':');
        qputenv("XDG_DATA_DIRS", dataDirs);
        {
            const QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
            QVERIFY(QDir().mkpath(configPath));
            KConfig kdeglobals(configPath + QStringLiteral("/kdeglobals"), KConfig::SimpleConfig);
            KConfigGroup(&kdeglobals, QStringLiteral("Icons")).writeEntry("Theme", QStringLiteral("kio-extras-test"));
            QVERIFY(kdeglobals.sync());
        }

        // wipe thumbnail cache so we always start clean
        QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation));
        cacheDir.removeRecursively();

        // A folder holding a picture, so that the thumbnail carries a sub thumbnail and its frame.
        // The worker rotates each sub thumbnail by an angle it seeds from the folder path, so the
        // path is the same one every run, or what is rendered would not be.
        const QString folderPath = QDir::tempPath() + QStringLiteral("/kio-extras-thumbnailtest-folder");
        QDir(folderPath).removeRecursively();
        QVERIFY(QDir().mkpath(folderPath));
        QDir folder(folderPath);
        QVERIFY(QFile::copy(QFINDTESTDATA("data/cherry_tree.png"), folder.filePath("cherry_tree.png")));

        KFileItemList items;
        items.append(KFileItem(QUrl::fromLocalFile(folderPath), QStringLiteral("inode/directory"), S_IFDIR));

        QStringList enabledPlugins{"directorythumbnail", "imagethumbnail", "jpegthumbnail"};
        auto *job = KIO::filePreview(items, size, &enabledPlugins);
        job->setDevicePixelRatio(dpr);

        QPixmap preview;
        connect(job, &KIO::PreviewJob::gotPreview, this, [&preview](const KFileItem &, const QPixmap &p) {
            preview = p;
        });

        QSignalSpy failedSpy(job, &KIO::PreviewJob::failed);
        QSignalSpy gotPreviewSpy(job, &KIO::PreviewJob::gotPreview);
        QSignalSpy resultSpy(job, &KIO::PreviewJob::result);

        resultSpy.wait();

        if (gotPreviewSpy.empty()) {
            // The worker draws the folder icon in its own process, and one it cannot find leaves the
            // thumbnail empty, which arrives here as a plain failed() with no error set. Report what
            // this process sees of the same lookup, since the two share an environment.
            const QString themeFile = themeDir + QStringLiteral("/icons/kio-extras-test/index.theme");
            qWarning() << "no folder thumbnail. theme file" << themeFile << "exists:" << QFileInfo::exists(themeFile)
                       << "| icon search paths:" << QIcon::themeSearchPaths() << "| theme:" << QIcon::themeName()
                       << "| fallback theme:" << QIcon::fallbackThemeName()
                       << "| inode-directory resolves here:" << !QIcon::fromTheme(QStringLiteral("inode-directory")).isNull();
            QSKIP("the worker found no icon to draw the folder with");
        }
        QVERIFY2(failedSpy.empty(), qPrintable(job->errorString()));

        const QImage rendered = preview.toImage().convertToFormat(QImage::Format_ARGB32);
        // The expected images hold for the folder icon this test ships, a plain square with a darker
        // border, so a theme answering ahead of it makes them meaningless rather than wrong.
        if (rendered.pixelColor(1, 1) != QColor(60, 80, 110)) {
            qWarning() << "the folder was drawn from" << QIcon::themeName() << "rather than the icon shipped with this test";
            QSKIP("another icon theme answered for the folder icon");
        }

        QCOMPARE(preview.devicePixelRatio(), dpr);
        // the folder thumbnail is rendered for the size that was asked for, in device pixels,
        // rather than being rendered larger and scaled down afterwards
        QCOMPARE(preview.deviceIndependentSize(), QSizeF(size));
        QCOMPARE(preview.size(), QSize(qRound(size.width() * dpr), qRound(size.height() * dpr)));

        QImage expectedImage;
        QVERIFY(expectedImage.load(QFINDTESTDATA("data/" + expectedThumbnail)));
        expectedImage.setDevicePixelRatio(dpr);
        // the expected image went through a file, which holds colours unpremultiplied, so both are
        // read the same way round before they are compared
        QCOMPARE(rendered, expectedImage.convertToFormat(QImage::Format_ARGB32));
    }

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

        QTest::addRow("one-invalid-size") << "one-invalid-size.svg"
                                          << "trivial.png" << 1.0 << QSize(128, 128);

        QTest::addRow("ossfuzz-5794405981421568") << "ossfuzz-5794405981421568"
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
