/*
 *   SPDX-FileCopyrightText: 2026 Ian Monroe <imonroe@kde.org>
 *   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include <QBuffer>
#include <QCoreApplication>
#include <QImage>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <QTest>

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/ListJob>
#include <KIO/MimetypeJob>
#include <KIO/MkdirJob>
#include <KIO/StatJob>
#include <KIO/StoredTransferJob>

#include "run_sftpserver.h"

using namespace Qt::Literals::StringLiterals;

class TestSftpOperations : public QObject
{
    Q_OBJECT

public:
    QUrl url(const QString &path) const
    {
        Q_ASSERT(path.startsWith(QChar('/')));
        QUrl newUrl = m_server.baseUrl();
        newUrl.setPath(path);
        return newUrl;
    }

    void uploadFile(const QString &path, const QByteArray &data, int permissions = -1)
    {
        auto job = KIO::storedPut(data, this->url(path), permissions, KIO::Overwrite);
        QVERIFY2(job->exec(), qUtf8Printable(job->errorString()));
    }

    KIO::UDSEntry statEntry(const QString &path, KIO::StatDetails details = KIO::StatBasic)
    {
        auto job = KIO::stat(this->url(path), KIO::StatJob::SourceSide, details, KIO::HideProgressInfo);
        const bool ok = job->exec();
        Q_ASSERT_X(ok, "statEntry", qUtf8Printable(job->errorString()));
        return job->statResult();
    }

    void createServerFile(const QString &relativePath, const QByteArray &content)
    {
        QFile file(m_server.serverLocalFilePath(relativePath));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
        file.close();
    }

    void createServerDir(const QString &relativePath)
    {
        QVERIFY(QDir().mkdir(m_server.serverLocalFilePath(relativePath)));
    }

    void verifyServerFile(const QString &relativePath, const QByteArray &expected)
    {
        QFile file(m_server.serverLocalFilePath(relativePath));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), expected);
    }

private:
    SftpServer m_server;

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY2(m_server.start(), qUtf8Printable(m_server.lastError()));
        QStandardPaths::setTestModeEnabled(true);
    }

    void cleanupTestCase()
    {
        m_server.stop();
    }

    void init()
    {
        QVERIFY(m_server.isRunning());
    }

    void testPutSmallFile()
    {
        const QString path("/testPutSmallFile.txt");
        const QByteArray testData = "Hello, SFTP World!";

        uploadFile(path, testData);

        KIO::UDSEntry entry = statEntry(path);
        QCOMPARE((entry.numberValue(KIO::UDSEntry::UDS_SIZE, -1)), testData.size());
    }

    void testPutLargeFile()
    {
        const QString path("/testPutLargeFile.bin");
        constexpr int fileSize = 1024 * 1024;

        QByteArray testData(fileSize, '\0');
        for (int i = 0; i < fileSize; ++i) {
            testData[i] = static_cast<char>(i % 256);
        }

        uploadFile(path, testData);

        auto getJob = KIO::storedGet(this->url(path), KIO::NoReload, KIO::HideProgressInfo);
        QVERIFY2(getJob->exec(), qUtf8Printable(getJob->errorString()));
        QCOMPARE(getJob->data(), testData);
    }

    void testPutWithPermissions()
    {
        const QString path("/testPutWithPermissions.sh");
        const QByteArray testData = "#!/bin/bash\necho 'Hello World'";
        const int permissions = 0755;

        uploadFile(path, testData, permissions);

        KIO::UDSEntry entry = statEntry(path);
        auto mode = entry.numberValue(KIO::UDSEntry::UDS_ACCESS, 0);
        QCOMPARE(mode, permissions);
    }

    void testListDir()
    {
        createServerDir("listdir_test");

        const QStringList files = {"file1.txt", "file2.txt", "file3.bin"};
        for (const QString &filename : files) {
            createServerFile("listdir_test/" + filename, "test content");
        }

        createServerDir("listdir_test/subdir");

        const auto listUrl = this->url("/listdir_test");
        KIO::UDSEntryList entries;
        auto listJob = KIO::listDir(listUrl, KIO::HideProgressInfo);

        connect(listJob, &KIO::ListJob::entries, this, [&entries](KIO::Job *, const KIO::UDSEntryList &list) {
            entries.append(list);
        });
        QVERIFY2(listJob->exec(), qUtf8Printable(listJob->errorString()));

        QStringList names;
        for (const auto &entry : std::as_const(entries)) {
            names.append(entry.stringValue(KIO::UDSEntry::UDS_NAME));
        }

        QVERIFY(names.contains("file1.txt"));
        QVERIFY(names.contains("file2.txt"));
        QVERIFY(names.contains("file3.bin"));
        QVERIFY(names.contains("subdir"));
    }

    void testListDirEmpty()
    {
        createServerDir("empty_dir");

        const auto listUrl = this->url("/empty_dir");
        KIO::UDSEntryList entries;
        auto listJob = KIO::listDir(listUrl, KIO::HideProgressInfo);

        connect(listJob, &KIO::ListJob::entries, this, [&entries](KIO::Job *, const KIO::UDSEntryList &list) {
            entries.append(list);
        });
        QVERIFY2(listJob->exec(), qUtf8Printable(listJob->errorString()));

        QStringList names;
        for (const auto &entry : std::as_const(entries)) {
            names.append(entry.stringValue(KIO::UDSEntry::UDS_NAME));
        }
        QVERIFY(names.size() <= 2);
    }

    void testStatFile()
    {
        const QString path("/testStatFile.txt");
        const QByteArray testData = "Test content for stat";

        uploadFile(path, testData);

        KIO::UDSEntry entry = statEntry(path);
        QCOMPARE(entry.stringValue(KIO::UDSEntry::UDS_NAME), QString("testStatFile.txt"));
        QCOMPARE(entry.numberValue(KIO::UDSEntry::UDS_SIZE, -1), testData.size());
        QVERIFY(!entry.isDir());
    }

    void testStatDirectory()
    {
        const QString path("/testStatDir");
        createServerDir("testStatDir");

        KIO::UDSEntry entry = statEntry(path);
        QCOMPARE(entry.stringValue(KIO::UDSEntry::UDS_NAME), QString("testStatDir"));
        QVERIFY(entry.isDir());
    }

    void testMkdir()
    {
        const QString path("/testMkdir");
        const auto dirUrl = this->url(path);

        auto mkdirJob = KIO::mkdir(dirUrl, -1);
        QVERIFY2(mkdirJob->exec(), qUtf8Printable(mkdirJob->errorString()));

        QVERIFY(QDir(m_server.serverLocalFilePath("testMkdir")).exists());
    }

    void testMkdirRmdir()
    {
        const QString path("/testMkdirRmdir");
        const auto dirUrl = this->url(path);

        auto mkdirJob = KIO::mkdir(dirUrl, -1);
        QVERIFY(mkdirJob->exec());

        auto delJob = KIO::del(dirUrl, KIO::HideProgressInfo);
        QVERIFY2(delJob->exec(), qUtf8Printable(delJob->errorString()));

        QVERIFY(!QDir(m_server.serverLocalFilePath("testMkdirRmdir")).exists());
    }

    void testMkdirWithPermissions()
    {
        const QString path("/testMkdirWithPermissions");
        const auto dirUrl = this->url(path);
        const int permissions = 0750;

        auto mkdirJob = KIO::mkdir(dirUrl, permissions);
        QVERIFY2(mkdirJob->exec(), qUtf8Printable(mkdirJob->errorString()));

        QVERIFY(QDir(m_server.serverLocalFilePath("testMkdirWithPermissions")).exists());
    }

    void testDelFile()
    {
        const QString path("/testDelFile.txt");
        const QByteArray testData = "This file will be deleted";

        uploadFile(path, testData);

        QVERIFY(QFile::exists(m_server.serverLocalFilePath("testDelFile.txt")));

        auto delJob = KIO::del(this->url(path), KIO::HideProgressInfo);
        QVERIFY2(delJob->exec(), qUtf8Printable(delJob->errorString()));

        QVERIFY(!QFile::exists(m_server.serverLocalFilePath("testDelFile.txt")));
    }

    void testDelNonExistent()
    {
        const QString path("/non_existent_file.txt");
        const auto fileUrl = this->url(path);

        auto delJob = KIO::del(fileUrl, KIO::HideProgressInfo);
        QVERIFY(!delJob->exec());
        QVERIFY(delJob->error() == KIO::ERR_DOES_NOT_EXIST || delJob->error() == KIO::ERR_WORKER_DEFINED);
    }

    void testRename()
    {
        const QString srcPath("/testRename_src.txt");
        const QString dstPath("/testRename_dst.txt");
        const QByteArray testData = "This file will be renamed";

        uploadFile(srcPath, testData);

        auto renameJob = KIO::rename(this->url(srcPath), this->url(dstPath), KIO::HideProgressInfo);
        QVERIFY2(renameJob->exec(), qUtf8Printable(renameJob->errorString()));

        QVERIFY(!QFile::exists(m_server.serverLocalFilePath("testRename_src.txt")));
        QVERIFY(QFile::exists(m_server.serverLocalFilePath("testRename_dst.txt")));

        verifyServerFile("testRename_dst.txt", testData);
    }

    void testRenameOverwrite()
    {
        const QString srcPath("/testRenameOverwrite_src.txt");
        const QString dstPath("/testRenameOverwrite_dst.txt");
        const QByteArray srcData = "Source content";
        const QByteArray dstData = "Destination content";

        uploadFile(srcPath, srcData);
        uploadFile(dstPath, dstData);

        const auto renameJob = KIO::rename(this->url(srcPath), this->url(dstPath), KIO::HideProgressInfo | KIO::Overwrite);
        QVERIFY2(renameJob->exec(), qUtf8Printable(renameJob->errorString()));

        QVERIFY(!QFile::exists(m_server.serverLocalFilePath("testRenameOverwrite_src.txt")));
        verifyServerFile("testRenameOverwrite_dst.txt", srcData);
    }

    void testCopy()
    {
        const QString srcPath("/testCopy_src.txt");
        const QString dstPath("/testCopy_dst.txt");
        const QByteArray testData = "Content to be copied";

        uploadFile(srcPath, testData);

        const auto copyJob = KIO::copy(this->url(srcPath), this->url(dstPath), KIO::HideProgressInfo);
        QVERIFY2(copyJob->exec(), qUtf8Printable(copyJob->errorString()));

        QVERIFY(QFile::exists(m_server.serverLocalFilePath("testCopy_src.txt")));
        QVERIFY(QFile::exists(m_server.serverLocalFilePath("testCopy_dst.txt")));

        verifyServerFile("testCopy_dst.txt", testData);
    }

    void testCopyOverwrite()
    {
        const QString srcPath("/testCopyOverwrite_src.txt");
        const QString dstPath("/testCopyOverwrite_dst.txt");
        const QByteArray srcData = "New content";
        const QByteArray dstData = "Old content";

        uploadFile(srcPath, srcData);
        uploadFile(dstPath, dstData);

        const auto copyJob = KIO::copy(this->url(srcPath), this->url(dstPath), KIO::HideProgressInfo | KIO::Overwrite);
        QVERIFY2(copyJob->exec(), qUtf8Printable(copyJob->errorString()));

        verifyServerFile("testCopyOverwrite_dst.txt", srcData);
    }

    void testCopyDirectory()
    {
        const QString dstDir = m_server.serverLocalFilePath("testCopyDir_dst");
        createServerDir("testCopyDir_src");

        createServerFile("testCopyDir_src/file1.txt", "File 1 content");
        createServerFile("testCopyDir_src/file2.txt", "File 2 content");

        createServerDir("testCopyDir_src/subdir");
        createServerFile("testCopyDir_src/subdir/subfile.txt", "Subfile content");

        auto copyJob = KIO::copy(this->url("/testCopyDir_src"), this->url("/testCopyDir_dst"), KIO::HideProgressInfo | KIO::Overwrite);
        QVERIFY2(copyJob->exec(), qUtf8Printable(copyJob->errorString()));

        QVERIFY(QDir(dstDir).exists());
        QVERIFY(QFile::exists(dstDir + "/file1.txt"));
        QVERIFY(QFile::exists(dstDir + "/file2.txt"));
        QVERIFY(QDir(dstDir + "/subdir").exists());
        QVERIFY(QFile::exists(dstDir + "/subdir/subfile.txt"));
    }

    void testMimeTypeText()
    {
        const QString path("/testMimeType.txt");
        const QByteArray testData = "This is a text file\nwith multiple lines.";

        uploadFile(path, testData);

        auto job = KIO::mimetype(this->url(path), KIO::HideProgressInfo);
        QVERIFY2(job->exec(), qUtf8Printable(job->errorString()));

        QCOMPARE(job->mimetype(), u"text/plain"_s);
    }

    void testMimeTypeBinary()
    {
        const QString path("/testMimeTypeNoExtension");

        QImage image(10, 10, QImage::Format_RGB32);
        image.fill(Qt::red);
        QByteArray testData;
        QBuffer buffer(&testData);
        buffer.open(QIODevice::WriteOnly);
        QVERIFY(image.save(&buffer, "PNG"));

        uploadFile(path, testData);

        auto job = KIO::mimetype(this->url(path), KIO::HideProgressInfo);
        QVERIFY2(job->exec(), qUtf8Printable(job->errorString()));
        QCOMPARE(job->mimetype(), u"image/png"_s);
    }
};

QTEST_GUILESS_MAIN(TestSftpOperations)

#include "test_sftp_operations.moc"
