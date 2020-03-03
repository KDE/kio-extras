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

    void testIPv6Literal()
    {
        // https://bugs.kde.org/show_bug.cgi?id=417682
        // Samba cannot deal with RFC5952 IPv6 notation (e.g. ::1%lo)
        // to work around we convert to windows ipv6 literals.

        // The actual represented URL should not change!
        // i.e. towards the KIO client we do not leak the IPv6
        // literal when returning an URL.
        QCOMPARE(SMBUrl(QUrl("smb://[::1]/share")).toString(),
                 "smb://[::1]/share");

        // The internal smbc representation should be literal though:
        // :: prefix
        QCOMPARE(SMBUrl(QUrl("smb://[::1]/share")).toSmbcUrl(),
                 "smb://0--1.ipv6-literal.net/share");
        // :: suffix
        QCOMPARE(SMBUrl(QUrl("smb://[fe80::]/share")).toSmbcUrl(),
                 "smb://fe80--0.ipv6-literal.net/share");
        // %lo scope
        QCOMPARE(SMBUrl(QUrl("smb://[::1%lo]/share")).toSmbcUrl(),
                 "smb://0--1slo.ipv6-literal.net/share");
        // random valid addr
        QCOMPARE(SMBUrl(QUrl("smb://[fe80::9cd7:32c7:faeb:f23d]/share")).toSmbcUrl(),
                 "smb://fe80--9cd7-32c7-faeb-f23d.ipv6-literal.net/share");
    }
};

QTEST_GUILESS_MAIN(SMBUrlTest)

#include "smburltest.moc"
