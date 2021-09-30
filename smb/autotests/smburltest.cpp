/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020-2021 Harald Sitter <sitter@kde.org>
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

    void testWorkgroupWithSpaces()
    {
        // Workgroups can have spaces but QUrls cannot, so we have a hack
        // that puts the workgroup info into a query.
        // Only applicable to SMB1 pretty much, we do not do workgroup browsing
        // for 2+.
        // https://bugs.kde.org/show_bug.cgi?id=204423

        // wg
        QCOMPARE(SMBUrl(QUrl("smb://?kio-workgroup=hax max")).toSmbcUrl(),
                 "smb://hax max/");
        // wg and query
        QCOMPARE(SMBUrl(QUrl("smb://?kio-workgroup=hax max&q=a")).toSmbcUrl(),
                 "smb://hax max/?q=a");
        // host and wg and query
        QCOMPARE(SMBUrl(QUrl("smb://host/?kio-workgroup=hax max&q=a")).toSmbcUrl(),
                 "smb://hax max/host?q=a");
        // host and wg and query
        QCOMPARE(SMBUrl(QUrl("smb://host/share?kio-workgroup=hax max")).toSmbcUrl(),
                 "smb://hax max/host/share");
        // Non-empty path. libsmbc hates unclean paths
        QCOMPARE(SMBUrl(QUrl("smb:///////?kio-workgroup=hax max")).toSmbcUrl(),
                 "smb://hax max/");
        // % character - run through .url() to simulate behavior of our listDir()
        QCOMPARE(SMBUrl(QUrl(QUrl("smb://?kio-workgroup=HAX%25MAX").url())).toSmbcUrl(),
                 "smb://HAX%25MAX/");
        // !ascii - run through .url() to simulate behavior of our listDir()
        QCOMPARE(SMBUrl(QUrl(QUrl("smb:///?kio-workgroup=DOMÄNE A").url())).toSmbcUrl(),
                 "smb://DOMÄNE A/"); // works as-is with smbc.

        // Also make sure type detection knows about this
        QCOMPARE(SMBUrl(QUrl("smb:/?kio-workgroup=hax max")).getType(),
                 SMBURLTYPE_WORKGROUP_OR_SERVER);
    }

    void testNonSmb()
    {
        // In the kdirnotify integration we load arbitrary urls into smburl,
        // make sure they get reported as unknown.
        QCOMPARE(SMBUrl(QUrl("file:///")).getType(), SMBURLTYPE_UNKNOWN);
        QCOMPARE(SMBUrl(QUrl("file:///home/foo/bar")).getType(), SMBURLTYPE_UNKNOWN);
        QCOMPARE(SMBUrl(QUrl("sftp://me@localhost/foo/bar")).getType(), SMBURLTYPE_UNKNOWN);
    }

    void testFileParts()
    {
        // We use SMBUrl for transfers from and to local files as well, make sure it behaves accordingly.
        SMBUrl url(QUrl("file:///foo"));
        QCOMPARE(QUrl("file:///foo"), url);
        QCOMPARE(QUrl("file:///foo.part"), url.partUrl());
        // Clearly not a file should not work
        QCOMPARE(QUrl(), SMBUrl(QUrl("file:///")).partUrl());
    }
};

QTEST_GUILESS_MAIN(SMBUrlTest)

#include "smburltest.moc"
