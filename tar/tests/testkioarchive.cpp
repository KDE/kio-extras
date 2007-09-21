/*  This file is part of the KDE project
    Copyright (C) 2007 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "testkioarchive.h"
#include <qtest_kde.h>
#include <kio/copyjob.h>
#include <kio/netaccess.h>
#include <ktar.h>
#include <kstandarddirs.h>

QTEST_KDEMAIN(TestKioArchive, NoGUI)
static const char* s_tarFileName = "karchivetest.tar";

static void writeTestFilesToArchive( KArchive* archive )
{
    bool ok;
    ok = archive->writeFile( "empty", "weis", "users", "", 0 );
    QVERIFY( ok );
    ok = archive->writeFile( "test1", "weis", "users", "Hallo", 5 );
    QVERIFY( ok );
    ok = archive->writeFile( "mydir/subfile", "dfaure", "users", "Bonjour", 7 );
    QVERIFY( ok );
    ok = archive->writeSymLink( "mydir/symlink", "subfile", "dfaure", "users" );
    QVERIFY( ok );
}

void TestKioArchive::initTestCase()
{
    // Make sure we start clean
    cleanupTestCase();

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
    KIO::ListJob* job = KIO::listDir(tarUrl());
    connect( job, SIGNAL( entries( KIO::Job*, const KIO::UDSEntryList& ) ),
             SLOT( slotEntries( KIO::Job*, const KIO::UDSEntryList& ) ) );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    kDebug() << "listDir done - entry count=" << m_listResult.count();
    QVERIFY( m_listResult.count() > 1 );

    kDebug() << m_listResult;
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
    KIO::ListJob* job = KIO::listRecursive(tarUrl());
    connect( job, SIGNAL( entries( KIO::Job*, const KIO::UDSEntryList& ) ),
             SLOT( slotEntries( KIO::Job*, const KIO::UDSEntryList& ) ) );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    kDebug() << "listDir done - entry count=" << m_listResult.count();
    QVERIFY( m_listResult.count() > 1 );

    kDebug() << m_listResult;
    QCOMPARE(m_listResult.count( "." ), 1); // found it, and only once
    QCOMPARE(m_listResult.count("empty"), 1);
    QCOMPARE(m_listResult.count("test1"), 1);
    QCOMPARE(m_listResult.count("mydir"), 1);
    QCOMPARE(m_listResult.count("mydir/subfile"), 1);
    QCOMPARE(m_listResult.count("mydir/symlink"), 1);
}

KUrl TestKioArchive::tarUrl() const
{
    KUrl url;
    url.setProtocol("tar");
    url.setPath(QDir::currentPath());
    url.addPath(s_tarFileName);
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
    // Note that this goes into ~/.kde-unit-test (see qtest_kde.h)
    // Use saveLocation if locateLocal doesn't work
    return KStandardDirs::locateLocal("tmp", "test_kio_archive/");
}

void TestKioArchive::cleanupTestCase()
{
    KIO::NetAccess::del(tmpDir(), 0);
}

void TestKioArchive::copyFromTar(const KUrl& src, const QString& destPath)
{
    KUrl dest(destPath);
    QVERIFY( KIO::NetAccess::exists( src, KIO::NetAccess::SourceSide, (QWidget*)0 ) );
    KIO::Job* job = KIO::copyAs( src, dest );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    QVERIFY( QFile::exists( destPath ) );
}

void TestKioArchive::testExtractFileFromTar()
{
    const QString destPath = tmpDir() + "fileFromTar_copied";
    KUrl u = tarUrl();
    u.addPath("mydir/subfile");
    copyFromTar(u, destPath);
    QVERIFY(QFileInfo(destPath).isFile());
    QVERIFY(QFileInfo(destPath).size() == 7);
}

void TestKioArchive::testExtractSymlinkFromTar()
{
    const QString destPath = tmpDir() + "symlinkFromTar_copied";
    KUrl u = tarUrl();
    u.addPath("mydir/symlink");
    copyFromTar(u, destPath);
    QVERIFY(QFileInfo(destPath).isFile());
    QEXPECT_FAIL("", "See #5601 -- on FTP we want to download the real file, not the symlink...", Continue);
    QVERIFY(QFileInfo(destPath).isSymLink());
}
