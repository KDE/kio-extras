/*
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QTest>
#include <QAbstractItemModelTester>

#include "smburl.h"

class SMBUrlTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testMinimalToSmbcValid()
    {
        // libsmbclient is a bit picky. make sure we convert to minimal applicable form
        {
            SMBUrl url(QUrl("smb:/"));
            QCOMPARE(url.toSmbcUrl(), "smb://");
        }

        // But at the same time it will happily deal with smb:
        {
            SMBUrl url(QUrl("smb:"));
            QCOMPARE(url.toSmbcUrl(), "smb:");
        }
    }

    void testType()
    {
        QCOMPARE(SMBUrl(QUrl("smb://")).getType(), SMBURLTYPE_ENTIRE_NETWORK);
        QCOMPARE(SMBUrl(QUrl("smb://host")).getType(), SMBURLTYPE_WORKGROUP_OR_SERVER);
        QCOMPARE(SMBUrl(QUrl("smb://host/share/file")).getType(), SMBURLTYPE_SHARE_OR_PATH);
        QCOMPARE(SMBUrl(QUrl()).getType(), SMBURLTYPE_UNKNOWN);
    }

    void testPart()
    {
        SMBUrl url(QUrl("smb://host/share/file"));
        QCOMPARE(url.partUrl().toString(), "smb://host/share/file.part");
    }

    void testUp()
    {
        SMBUrl url(QUrl("smb://host/share/file"));
        url.cdUp();
        QCOMPARE(url.toSmbcUrl(), "smb://host/share");
    }

    void testAddPath()
    {
        SMBUrl url(QUrl("smb://host/share"));
        url.addPath("file");
        QCOMPARE(url.toSmbcUrl(), "smb://host/share/file");
    }

    void testCifs()
    {
        // We treat cifs as an alias but need to translate it to smb.
        // https://bugs.kde.org/show_bug.cgi?id=327295
        SMBUrl url(QUrl("cifs://host/share/file"));
        QCOMPARE(url.toSmbcUrl(), "smb://host/share/file");
    }
};

QTEST_GUILESS_MAIN(SMBUrlTest)

#include "smburltest.moc"
