/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testkioarchive.h"
#include <qtest.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <ktar.h>
#include <QDebug>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

QTEST_MAIN(TestKioArchive)
static const char s_tarFileName[] = "karchivetest.tar";

static void writeTestFilesToArchive( KArchive* archive )
{
    bool ok;
    ok = archive->writeFile("empty", QByteArray(), 0100644, "weis", "users");
    QVERIFY( ok );
    ok = archive->writeFile("test1", QByteArrayLiteral("Hallo"), 0100644, "weis", "users");
    QVERIFY( ok );
    ok = archive->writeFile("mydir/subfile", QByteArrayLiteral("Bonjour"), 0100644, "dfaure", "users");
    QVERIFY( ok );
    ok = archive->writeSymLink( "mydir/symlink", "subfile", "dfaure", "users" );
    QVERIFY( ok );
}

void TestKioArchive::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    // avoid KIO triggering klauncher, which can get the CI stuck
    qputenv("KDE_FORK_SLAVES", "yes");
    qputenv("KIO_DISABLE_CACHE_CLEANER", "yes");

    // Make sure we start clean
    cleanupTestCase();

    QVERIFY(QDir().mkpath(tmpDir()));

    // Taken from KArchiveTest::testCreateTar
    KTar tar( s_tarFileName );
    bool ok = tar.open( QIODevice::WriteOnly );
    QVERIFY( ok );
    writeTestFilesToArchive( &tar );
    ok = tar.close();
    QVERIFY( ok );
    QFileInfo fileInfo( QFile::encodeName( s_tarFileName ) );
    QVERIFY( fileInfo.exists() );
}

void TestKioArchive::testListTar()
{
    m_listResult.clear();
    KIO::ListJob* job = KIO::listDir(tarUrl(), KIO::HideProgressInfo);
    connect(job, &KIO::ListJob::entries, this, &TestKioArchive::slotEntries);
    bool ok = job->exec();
    QVERIFY( ok );
    QVERIFY( m_listResult.count() > 1 );

    QCOMPARE(m_listResult.count( "." ), 1); // found it, and only once
    QCOMPARE(m_listResult.count("empty"), 1);
    QCOMPARE(m_listResult.count("test1"), 1);
    QCOMPARE(m_listResult.count("mydir"), 1);
    QCOMPARE(m_listResult.count("mydir/subfile"), 0); // not a recursive listing
    QCOMPARE(m_listResult.count("mydir/symlink"), 0);
}

void TestKioArchive::testListRecursive()
{
    m_listResult.clear();
    KIO::ListJob* job = KIO::listRecursive(tarUrl(), KIO::HideProgressInfo);
    connect(job, &KIO::ListJob::entries, this, &TestKioArchive::slotEntries);
    bool ok = job->exec();
    QVERIFY( ok );
    QVERIFY( m_listResult.count() > 1 );

    QCOMPARE(m_listResult.count( "." ), 1); // found it, and only once
    QCOMPARE(m_listResult.count("empty"), 1);
    QCOMPARE(m_listResult.count("test1"), 1);
    QCOMPARE(m_listResult.count("mydir"), 1);
    QCOMPARE(m_listResult.count("mydir/subfile"), 1);
    QCOMPARE(m_listResult.count("mydir/symlink"), 1);
}

QUrl TestKioArchive::tarUrl() const
{
    QUrl url;
    url.setScheme("tar");
    url.setPath(QDir::currentPath());
    url = url.adjusted(QUrl::StripTrailingSlash);
    url.setPath(url.path() + '/' + s_tarFileName);
    return url;
}

void TestKioArchive::slotEntries( KIO::Job*, const KIO::UDSEntryList& lst )
{
    for( KIO::UDSEntryList::ConstIterator it = lst.begin(); it != lst.end(); ++it ) {
        const KIO::UDSEntry& entry (*it);
        QString displayName = entry.stringValue( KIO::UDSEntry::UDS_NAME );
        m_listResult << displayName;
    }
}

QString TestKioArchive::tmpDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/test_kio_archive/";
}

void TestKioArchive::cleanupTestCase()
{
    QDir(tmpDir()).removeRecursively();
}

void TestKioArchive::copyFromTar(const QUrl &src, const QString& destPath)
{
    QUrl dest = QUrl::fromLocalFile(destPath);
    qDebug() << src << "->" << dest;
    // Check that src exists
    KIO::StatJob* statJob = KIO::statDetails(src, KIO::StatJob::SourceSide, KIO::StatNoDetails, KIO::HideProgressInfo);
    bool ok = statJob->exec();
    QVERIFY( ok );

    KIO::Job* job = KIO::copyAs( src, dest, KIO::HideProgressInfo );
    qDebug() << "copyAs" << src << dest;
    ok = job->exec();
    QVERIFY( ok );
    QVERIFY( QFile::exists( destPath ) );
}

void TestKioArchive::testExtractFileFromTar()
{
    const QString destPath = tmpDir() + "fileFromTar_copied";
    QUrl u = tarUrl();
    u = u.adjusted(QUrl::StripTrailingSlash);
    u.setPath(u.path() + '/' + "mydir/subfile");
    copyFromTar(u, destPath);
    QVERIFY(QFileInfo(destPath).isFile());
    QVERIFY(QFileInfo(destPath).size() == 7);
}

void TestKioArchive::testExtractSymlinkFromTar()
{
    const QString destPath = tmpDir() + "symlinkFromTar_copied";
    QUrl u = tarUrl();
    u = u.adjusted(QUrl::StripTrailingSlash);
    u.setPath(u.path() + '/' + "mydir/symlink");
    copyFromTar(u, destPath);
    QVERIFY(QFileInfo(destPath).isFile());
    QEXPECT_FAIL("", "See #5601 -- on FTP we want to download the real file, not the symlink...", Continue);
    // See comment in 149903
    // Maybe this is something we can do depending on Class=:local and Class=:internet
    // (we already know if a protocol is local or remote).
    // So local->local should copy symlinks, while internet->local and internet->internet should
    // copy the actual file, I guess?
    // -> ### TODO
    QVERIFY(QFileInfo(destPath).isSymLink());
}
