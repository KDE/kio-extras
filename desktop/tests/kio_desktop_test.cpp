/* This file is part of the KDE project
   Copyright (C) 2009 David Faure <faure@kde.org>

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

#include <ktemporaryfile.h>
#include <kdebug.h>
#include <QDesktopServices>
#include <QObject>
#include <qtest_kde.h>
#include <kio/job.h>

class TestDesktop : public QObject
{
    Q_OBJECT

public:
    TestDesktop() {}

private Q_SLOTS:
    void initTestCase()
    {
        // copied from kio_desktop.cpp:
        m_desktopPath = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
        if (m_desktopPath.isEmpty())
            m_desktopPath = QDir::homePath() + "/Desktop";
        // Warning, this defaults to $HOME/Desktop, the _real_ desktop dir.
        m_testFileName = "kio_desktop_test_file";
    }
    void cleanupTestCase()
    {
        QFile::remove(m_desktopPath + '/' + m_testFileName);
    }

    void testCopyToDesktop()
    {
        KTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        tempFile.write( "Hello world\n", 12 );
        QString fileName = tempFile.fileName();
        tempFile.close();
        KIO::Job* job = KIO::file_copy(fileName, KUrl("desktop:/" + m_testFileName), KIO::HideProgressInfo);
        QVERIFY(job->exec());
        QVERIFY(QFile::exists(m_desktopPath + '/' + m_testFileName));
    }

    void testMostLocalUrl() // relies on testCopyToDesktop being run before
    {
        const KUrl desktopUrl("desktop:/" + m_testFileName);
        const QString filePath(m_desktopPath + '/' + m_testFileName);
        KIO::StatJob* job = KIO::mostLocalUrl(KUrl(desktopUrl), KIO::HideProgressInfo);
        QVERIFY(job);
        bool ok = job->exec();
        QVERIFY(ok);
        QCOMPARE(job->mostLocalUrl().toLocalFile(), filePath);
    }
private:
    QString m_desktopPath;
    QString m_testFileName;
};

QTEST_KDEMAIN(TestDesktop, NoGUI)

#include "kio_desktop_test.moc"
