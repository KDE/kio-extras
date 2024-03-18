/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include "../cmdtool.h"
#include "../config.h"

class CmdToolTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testIsAvailable()
    {
        CmdTool nonexist(QStringLiteral(KDE_INSTALL_FULL_DATADIR "/cmdtoolsearch/nonexist"));
        QVERIFY(!nonexist.isAvailable());

        CmdTool find(QStringLiteral(KDE_INSTALL_FULL_DATADIR "/cmdtoolsearch/find"));
        QVERIFY(find.isAvailable());
    }

    void testMetadata()
    {
        CmdTool find(QStringLiteral(KDE_INSTALL_FULL_DATADIR "/cmdtoolsearch/find"));
        QCOMPARE(find.separator(), CmdTool::SEP_NUL);

        CmdTool grep(QStringLiteral(KDE_INSTALL_FULL_DATADIR "/cmdtoolsearch/grep"));
        QCOMPARE(grep.separator(), CmdTool::SEP_NEWLINE);
    }

    void testRun()
    {
        CmdTool find(QStringLiteral(KDE_INSTALL_FULL_DATADIR "/cmdtoolsearch/find"));
        QStringList result;
        connect(&find, &CmdTool::result, this, [&result](const QString &pathStr) {
            result.append(pathStr);
        });
        QVERIFY(find.run(QStringLiteral(KDE_INSTALL_FULL_DATADIR "/cmdtoolsearch/find"), QStringLiteral("metadata"), false));
        result.sort();
        QCOMPARE(result.size(), 1);
        QCOMPARE(result.at(0), QStringLiteral("./metadata.json"));
    }
};

QTEST_GUILESS_MAIN(CmdToolTest)

#include "cmdtooltest.moc"
