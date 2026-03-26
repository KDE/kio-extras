/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2026 Kun Ma <mk01022025@outlook.com>
*/

#include "../kiod_module/mtpprotocols_p.h"

#include <QSettings>
#include <QTest>

class MtpProtocolTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void supportsMtpProtocol_data()
    {
        QTest::addColumn<QStringList>("supportedProtocols");
        QTest::addColumn<bool>("expected");

        QTest::addRow("empty") << QStringList{} << false;
        QTest::addRow("usbmux-only") << QStringList{QStringLiteral("usbmux")} << false;
        QTest::addRow("mtp-only") << QStringList{QStringLiteral("mtp")} << true;
        QTest::addRow("mtp-among-others") << QStringList{QStringLiteral("gphoto2"), QStringLiteral("mtp")} << true;
    }

    void supportsMtpProtocol()
    {
        QFETCH(QStringList, supportedProtocols);
        QFETCH(bool, expected);

        QCOMPARE(KMTP::supportsMtpProtocol(supportedProtocols), expected);
    }

    void predicateMatchesDesktopEntry()
    {
        QSettings desktopFile(QStringLiteral(MTP_SOURCE_DIR "/solid_mtp.desktop"), QSettings::IniFormat);
        const QString actionPredicate = desktopFile.value(QStringLiteral("Desktop Entry/X-KDE-Solid-Predicate")).toString();

        QVERIFY(!actionPredicate.isEmpty());
        QCOMPARE(actionPredicate, KMTP::mtpPlayerPredicate());
    }
};

QTEST_MAIN(MtpProtocolTest)

#include "mtpprotocoltest.moc"
