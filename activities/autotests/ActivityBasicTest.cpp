/*
 *   SPDX-FileCopyrightText: 2022 Alex Kuznetsov <alex@vxpro.io>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDate>
#include <QTest>

#include "../KioActivities.h"
#include "../KioActivitiesApi.h"
#include <utils/d_ptr_implementation.h>
class FixActivitiesProtocol
{
public:
    D_PTRC(ActivitiesProtocolApi); // using this to test a macro in the main class

    FixActivitiesProtocol()
    {
    }

    ActivitiesProtocolApi::PathType pathType(const QUrl &url, QString *activity, QString *filePath) const
    {
        return d->pathType(url, activity, filePath);
    }

    QString mangledPath(const QString &path) const
    {
        return d->mangledPath(path);
    }

    QString demangledPath(const QString &mangled) const
    {
        return d->demangledPath(mangled);
    }

    KIO::UDSEntry activityEntry(const QString &activity)
    {
        return d->activityEntry(activity);
    }

    KIO::UDSEntry filesystemEntry(const QString &path)
    {
        return d->filesystemEntry(path);
    }
};

class ActivityBasicTest : public QObject
{
    Q_OBJECT

private:
    QString filer, filerName;

private Q_SLOTS:
    void initTestCase()
    {
        qDebug("Init activity tests");

        filerName = "filerActivity";

        QDir d = QDir::temp();
        filer = d.absoluteFilePath(filerName);
        QFile f(filer);
        f.open(QIODevice::WriteOnly);
        f.write("hello");
    }

    void ActivityPathItemTest()
    {
        FixActivitiesProtocol fix;
        QString activity;
        QString filepath;
        QString surl = "activities:/current/Zm9v"; // activities:/activityName/base64(filepath)
        QUrl url;
        url.setUrl(surl);

        auto result = fix.pathType(url, &activity, &filepath);

        QVERIFY(result == ActivitiesProtocolApi::PathType::ActivityPathItem);
        QCOMPARE(activity, "current");
        QCOMPARE(filepath, QString("foo"));
    }

    void ActivityRootItemTest()
    {
        FixActivitiesProtocol fix;
        QString activity;
        QString filepath;
        QString surl = "activities:/current";
        QUrl url;
        url.setUrl(surl);

        auto result = fix.pathType(url, &activity, &filepath);

        QVERIFY(result == ActivitiesProtocolApi::PathType::ActivityRootItem);
        QCOMPARE(activity, "current");
    }

    void RootItemTest()
    {
        FixActivitiesProtocol fix;
        QString activity;
        QString filepath;
        QString surl = "activities:/";
        QUrl url;
        url.setUrl(surl);

        auto result = fix.pathType(url, &activity, &filepath);

        QVERIFY(result == ActivitiesProtocolApi::PathType::RootItem);
        QCOMPARE(activity, "");
    }

    void mangleTest()
    {
        FixActivitiesProtocol fix;
        QString surl = "foo";
        auto result = fix.mangledPath(surl);
        QCOMPARE(result, "Zm9v");
    }

    void demangleTest()
    {
        FixActivitiesProtocol fix;
        QString surl = "Zm9v";
        auto result = fix.demangledPath(surl);
        QCOMPARE(result, "foo");
    }

    void activityEntryTest()
    {
        FixActivitiesProtocol fix;
        QString act("current");
        auto result = fix.activityEntry(act);

        QCOMPARE(result.count(), 8);

        QVERIFY(result.contains(KIO::UDSEntry::UDS_NAME));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_DISPLAY_NAME));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_DISPLAY_TYPE));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_ICON_NAME));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_FILE_TYPE));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_MIME_TYPE));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_ACCESS));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_USER));

        QCOMPARE(result.stringValue(KIO::UDSEntry::UDS_NAME), act);
        QCOMPARE(result.numberValue(KIO::UDSEntry::UDS_FILE_TYPE), S_IFDIR);
        QCOMPARE(result.stringValue(KIO::UDSEntry::UDS_MIME_TYPE), QStringLiteral("inode/directory"));
        QCOMPARE(result.numberValue(KIO::UDSEntry::UDS_ACCESS), 0500);
    }

    void fileSystemEntryTest()
    {
        FixActivitiesProtocol fix;
        auto result = fix.filesystemEntry(filer);

        QVERIFY(result.contains(KIO::UDSEntry::UDS_NAME));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_DISPLAY_NAME));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_TARGET_URL));
        QVERIFY(result.contains(KIO::UDSEntry::UDS_LOCAL_PATH));

        auto mfiler = fix.mangledPath(filer);

        QCOMPARE(result.stringValue(KIO::UDSEntry::UDS_NAME), mfiler);
        QCOMPARE(result.stringValue(KIO::UDSEntry::UDS_DISPLAY_NAME), filerName);
        QUrl ufiler = QUrl::fromLocalFile(filer);
        QCOMPARE(result.stringValue(KIO::UDSEntry::UDS_TARGET_URL), ufiler.toString());
        QCOMPARE(result.stringValue(KIO::UDSEntry::UDS_LOCAL_PATH), filer);
    }

    void cleanupTestCase()
    {
        QFile::remove(filer);
        qDebug("Activites test complete.");
    }
};

QTEST_MAIN(ActivityBasicTest)

#include "ActivityBasicTest.moc"
