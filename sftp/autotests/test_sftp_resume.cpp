/*
 *   SPDX-FileCopyrightText: 2026 Ian Monroe <imonroe@kde.org>
 *   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTest>

#include <kio/copyjob.h>
#include <kio/storedtransferjob.h>

#include "run_sftpserver.h"

using namespace Qt::Literals::StringLiterals;

class TestSftpResume : public QObject
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

private:
    SftpServer m_server;

    void performResumeTest(const QString &path, qint64 resumeOffset)
    {
        constexpr int fileSize = 128 * 1024; // 128 KiB is enough to test multi-chunk and short reads
        QByteArray testData(fileSize, '\0');
        for (int i = 0; i < fileSize; ++i) {
            testData[i] = static_cast<char>(i % 256);
        }

        const auto destUrl = this->url(path);

        { // upload file
            const auto writeJob = KIO::storedPut(testData, destUrl, -1, KIO::Overwrite);
            writeJob->setUiDelegate(nullptr);
            QVERIFY2(writeJob->exec(), qUtf8Printable(writeJob->errorString()));
        }

        // read back with resume offset
        auto getJob = KIO::get(destUrl, KIO::NoReload, KIO::HideProgressInfo);
        getJob->addMetaData(QStringLiteral("resume"), QString::number(resumeOffset));
        getJob->setUiDelegate(nullptr);

        QByteArray receivedData;
        connect(getJob, &KIO::TransferJob::data, this, [&receivedData](KIO::Job *, const QByteArray &data) {
            receivedData.append(data);
        });

        const bool ok = getJob->exec();

        QVERIFY2(ok, qUtf8Printable(u"KIO::get with resume=%1 failed: %2 (error %3)"_s.arg(resumeOffset).arg(getJob->errorString()).arg(getJob->error())));

        QCOMPARE(receivedData.size(), fileSize - resumeOffset);
        QCOMPARE(receivedData, testData.mid(resumeOffset));
    }

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

    void testResumeOffset()
    {
        performResumeTest(u"/testResumeOffset.bin"_s, 64 * 1024);
    }

    void testResumeAtStart()
    {
        performResumeTest(u"/testResumeAtStart.bin"_s, 0);
    }

    // Edge case: resume from the last byte
    void testResumeAtEof()
    {
        performResumeTest(u"/testResumeAtEof.bin"_s, (128 * 1024) - 1);
    }
};

QTEST_GUILESS_MAIN(TestSftpResume)

#include "test_sftp_resume.moc"
