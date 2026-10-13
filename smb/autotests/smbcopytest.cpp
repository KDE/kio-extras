/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <KIO/CopyJob>

#include <fcntl.h>
#include <sys/stat.h>

#include "run_smbserver.h"

using namespace Qt::Literals::StringLiterals;

class SmbCopyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY2(m_server.start(), qUtf8Printable(m_server.lastError()));
    }

    void cleanupTestCase()
    {
        m_server.stop();
    }

    void copyToLocalKeepsSourceMTime_data()
    {
        QTest::addColumn<qint64>("size");

        // The size of the last write of a copy depends on the size of the file.
        QTest::newRow("1000 bytes") << qint64(1000);
        QTest::newRow("70000 bytes") << qint64(70000);
        QTest::newRow("4 MiB") << qint64(4 * 1024 * 1024);
        QTest::newRow("64 KiB") << qint64(64 * 1024);
    }

    void copyToLocalKeepsSourceMTime()
    {
        QFETCH(qint64, size);

        const QString name = QString::number(size) + u".bin"_s;
        const QString sourcePath = m_server.serverLocalFilePath(name);
        QFile source(sourcePath);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QVERIFY(source.write(QByteArray(size, 'x')) == size);
        source.close();

        // An old mtime with a fraction of a second, so that a time cut to whole seconds and the
        // time of the copy itself both differ from it.
        const struct timespec sourceTimes[2] = {{0, UTIME_OMIT}, {1577934245, 123456789}};
        QCOMPARE(utimensat(AT_FDCWD, QFile::encodeName(sourcePath).constData(), sourceTimes, 0), 0);

        QTemporaryDir destination;
        QVERIFY(destination.isValid());
        QUrl sourceUrl = m_server.baseUrl();
        sourceUrl.setPath(sourceUrl.path() + u'/' + name);
        auto job = KIO::copy(sourceUrl, QUrl::fromLocalFile(destination.path()), KIO::HideProgressInfo);
        QVERIFY2(job->exec(), qUtf8Printable(job->errorString()));

        struct stat copied;
        QCOMPARE(stat(QFile::encodeName(destination.filePath(name)).constData(), &copied), 0);
        QCOMPARE(copied.st_size, size);
        // SMB keeps times in steps of 100 ns.
        QCOMPARE(copied.st_mtim.tv_sec, sourceTimes[1].tv_sec);
        QCOMPARE(copied.st_mtim.tv_nsec, sourceTimes[1].tv_nsec - sourceTimes[1].tv_nsec % 100);
    }

private:
    SmbServer m_server;
};

QTEST_GUILESS_MAIN(SmbCopyTest)

#include "smbcopytest.moc"
